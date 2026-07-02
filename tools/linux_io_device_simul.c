#include <linux_io_device_simul/io_device_simul.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int parse_u32(const char *text, uint32_t *out)
{
    char *end = 0;
    unsigned long value;

    if (!text || !out) {
        return -1;
    }

    value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value > 1000000UL) {
        return -1;
    }

    *out = (uint32_t)value;
    return 0;
}

static void print_event(const linux_io_sim_event_t *event, void *user)
{
    char topic[128];
    char payload[256];
    const linux_io_sim_options_t *options = (const linux_io_sim_options_t *)user;

    if (linux_io_sim_format_topic(topic,
                                  sizeof(topic),
                                  options->site,
                                  options->node,
                                  event->sample.name,
                                  "event") < 0) {
        return;
    }
    if (linux_io_sim_format_event_json(payload, sizeof(payload), event) < 0) {
        return;
    }

    printf("%s %s\n", topic, payload);
}

static int read_script(FILE *input, char lines[][160], size_t max_lines, size_t *line_count)
{
    size_t count = 0;

    if (!input || !lines || !line_count) {
        return -1;
    }

    while (count < max_lines && fgets(lines[count], 160, input)) {
        ++count;
    }
    *line_count = count;
    return 0;
}

static int normalize_slot_line(const char *line,
                               uint8_t *slot,
                               char *type,
                               size_t type_size,
                               uint16_t *channel,
                               int *value)
{
    char copy[160];
    char *parts[4];
    char *cursor;
    size_t count = 0;
    unsigned parsed_slot;
    unsigned parsed_channel;

    if (!line || !slot || !type || !channel || !value) {
        return -1;
    }
    if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
        return 1;
    }

    snprintf(copy, sizeof(copy), "%s", line);
    for (cursor = copy; *cursor; ++cursor) {
        if (*cursor == '/') {
            *cursor = ' ';
        }
    }

    cursor = strtok(copy, " \t\r\n");
    while (cursor && count < sizeof(parts) / sizeof(parts[0])) {
        parts[count++] = cursor;
        cursor = strtok(0, " \t\r\n");
    }

    if (count == 2 && strcmp(parts[0], "sleep") == 0) {
        return 2;
    }
    if (count != 4 || strncmp(parts[0], "slot", 4) != 0) {
        return -1;
    }
    if (sscanf(parts[0], "slot%u", &parsed_slot) != 1 ||
        parsed_slot == 0 || parsed_slot > 20) {
        return -1;
    }
    if (sscanf(parts[2], "%u", &parsed_channel) != 1 || parsed_channel > 65535) {
        return -1;
    }

    *slot = (uint8_t)parsed_slot;
    snprintf(type, type_size, "%s", parts[1]);
    for (cursor = type; *cursor; ++cursor) {
        if (*cursor >= 'A' && *cursor <= 'Z') {
            *cursor = (char)(*cursor - 'A' + 'a');
        }
    }
    *channel = (uint16_t)parsed_channel;
    *value = atoi(parts[3]);
    return 0;
}

static int normalize_hand_line(const char *line,
                               char *frame_id,
                               size_t frame_id_size,
                               float *x,
                               float *y,
                               float *z,
                               float *yaw,
                               float *pitch,
                               float *roll,
                               float *grip,
                               unsigned *hold_ms,
                               float *tolerance,
                               unsigned *safety_hold)
{
    char op[16];
    char id[64];

    if (!line || !frame_id || !x || !y || !z || !yaw || !pitch || !roll ||
        !grip || !hold_ms || !tolerance || !safety_hold) {
        return -1;
    }
    if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
        return 1;
    }

    if (sscanf(line,
               "%15s %63s %f %f %f %f %f %f %f %u %f %u",
               op,
               id,
               x,
               y,
               z,
               yaw,
               pitch,
               roll,
               grip,
               hold_ms,
               tolerance,
               safety_hold) != 12) {
        return -1;
    }
    if (strcmp(op, "hand") != 0) {
        return -1;
    }
    snprintf(frame_id, frame_id_size, "%s", id);
    return 0;
}

static const char *json_value_pos(const char *json, const char *key)
{
    char pattern[64];
    const char *pos;

    if (!json || !key) {
        return 0;
    }
    if (snprintf(pattern, sizeof(pattern), "\"%s\"", key) >= (int)sizeof(pattern)) {
        return 0;
    }
    pos = strstr(json, pattern);
    if (!pos) {
        return 0;
    }
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t') {
        ++pos;
    }
    if (*pos != ':') {
        return 0;
    }
    ++pos;
    while (*pos == ' ' || *pos == '\t') {
        ++pos;
    }
    return pos;
}

static int json_find_string(const char *json, const char *key, char *out, size_t out_size)
{
    const char *pos;
    const char *end;
    size_t len;

    if (!out || out_size == 0) {
        return -1;
    }
    pos = json_value_pos(json, key);
    if (!pos || *pos != '"') {
        return -1;
    }
    ++pos;
    end = strchr(pos, '"');
    if (!end) {
        return -1;
    }
    len = (size_t)(end - pos);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, pos, len);
    out[len] = '\0';
    return 0;
}

static int json_find_float(const char *json, const char *key, float *out)
{
    const char *pos;
    char *end = 0;
    float value;

    if (!out) {
        return -1;
    }
    pos = json_value_pos(json, key);
    if (!pos) {
        return -1;
    }
    value = strtof(pos, &end);
    if (end == pos) {
        return -1;
    }
    *out = value;
    return 0;
}

static int json_find_u32(const char *json, const char *key, uint32_t *out)
{
    const char *pos;
    char *end = 0;
    unsigned long value;

    if (!out) {
        return -1;
    }
    pos = json_value_pos(json, key);
    if (!pos) {
        return -1;
    }
    value = strtoul(pos, &end, 10);
    if (end == pos || value > 1000000UL) {
        return -1;
    }
    *out = (uint32_t)value;
    return 0;
}

static int json_find_i32(const char *json, const char *key, int *out)
{
    const char *pos;
    char *end = 0;
    long value;

    if (!out) {
        return -1;
    }
    pos = json_value_pos(json, key);
    if (!pos) {
        return -1;
    }
    value = strtol(pos, &end, 10);
    if (end == pos || value < -1000000L || value > 1000000L) {
        return -1;
    }
    *out = (int)value;
    return 0;
}

static float clamp_f32(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static int parse_hand_event_json(const char *line,
                                 char *frame_id,
                                 size_t frame_id_size,
                                 uint32_t *t_ms,
                                 float *x,
                                 float *y,
                                 float *z,
                                 float *yaw,
                                 float *pitch,
                                 float *roll,
                                 float *grip,
                                 uint32_t *hold_ms,
                                 float *tolerance,
                                 uint32_t *safety_hold)
{
    if (!line || !strstr(line, "hand/keyframe/event") || !strstr(line, "\"event\":\"keyframe\"")) {
        return 1;
    }
    if (json_find_string(line, "frame_id", frame_id, frame_id_size) != 0 ||
        json_find_u32(line, "t_ms", t_ms) != 0 ||
        json_find_float(line, "x", x) != 0 ||
        json_find_float(line, "y", y) != 0 ||
        json_find_float(line, "z", z) != 0 ||
        json_find_float(line, "yaw", yaw) != 0 ||
        json_find_float(line, "pitch", pitch) != 0 ||
        json_find_float(line, "roll", roll) != 0 ||
        json_find_float(line, "grip", grip) != 0 ||
        json_find_u32(line, "hold_ms", hold_ms) != 0 ||
        json_find_float(line, "tolerance", tolerance) != 0 ||
        json_find_u32(line, "safety_hold", safety_hold) != 0) {
        return -1;
    }
    return 0;
}

static int parse_slot_event_json(const char *line,
                                 uint8_t *slot,
                                 char *type,
                                 size_t type_size,
                                 uint16_t *channel,
                                 int *value,
                                 uint32_t *t_ms)
{
    uint32_t parsed_slot;
    uint32_t parsed_channel;

    if (!line || !strstr(line, "/slot/") || !strstr(line, "\"event\":\"changed\"")) {
        return 1;
    }
    if (json_find_u32(line, "slot", &parsed_slot) != 0 ||
        json_find_string(line, "type", type, type_size) != 0 ||
        json_find_u32(line, "channel", &parsed_channel) != 0 ||
        json_find_i32(line, "value", value) != 0 ||
        json_find_u32(line, "t_ms", t_ms) != 0 ||
        parsed_slot == 0 || parsed_slot > 20 || parsed_channel > 65535) {
        return -1;
    }

    *slot = (uint8_t)parsed_slot;
    *channel = (uint16_t)parsed_channel;
    return 0;
}

static int run_slot_stream(FILE *input, uint32_t loops, uint32_t sample_ms)
{
    char lines[512][160];
    size_t line_count = 0;
    uint32_t loop;
    uint32_t now_ms = 0;

    if (read_script(input, lines, sizeof(lines) / sizeof(lines[0]), &line_count) != 0) {
        return 1;
    }

    for (loop = 0; loop < loops; ++loop) {
        size_t i;

        for (i = 0; i < line_count; ++i) {
            uint8_t slot = 0;
            char type[16];
            uint16_t channel = 0;
            int value = 0;
            int parsed = normalize_slot_line(lines[i], &slot, type, sizeof(type), &channel, &value);

            if (parsed == 1) {
                continue;
            }
            if (parsed == 2) {
                unsigned sleep_ms = 0;
                if (sscanf(lines[i], "%*s %u", &sleep_ms) == 1) {
                    now_ms += sleep_ms;
                }
                continue;
            }
            if (parsed != 0) {
                fprintf(stderr, "invalid slot trigger command: %s", lines[i]);
                return 1;
            }

            printf("site/demo/node/linux-sim-001/slot/%u/io/%s/%u/event "
                   "{\"event\":\"changed\",\"slot\":%u,\"type\":\"%s\","
                   "\"channel\":%u,\"value\":%d,\"t_ms\":%u,\"loop\":%u}\n",
                   slot,
                   type,
                   channel,
                   slot,
                   type,
                   channel,
                   value,
                   now_ms,
                   loop + 1);
            now_ms += sample_ms;
        }
    }

    return 0;
}

static int run_hand_stream(FILE *input, uint32_t loops, uint32_t sample_ms)
{
    char lines[512][160];
    size_t line_count = 0;
    uint32_t loop;
    uint32_t now_ms = 0;

    if (read_script(input, lines, sizeof(lines) / sizeof(lines[0]), &line_count) != 0) {
        return 1;
    }

    for (loop = 0; loop < loops; ++loop) {
        size_t i;

        for (i = 0; i < line_count; ++i) {
            char frame_id[64];
            float x;
            float y;
            float z;
            float yaw;
            float pitch;
            float roll;
            float grip;
            float tolerance;
            unsigned hold_ms;
            unsigned safety_hold;
            int parsed = normalize_hand_line(lines[i],
                                             frame_id,
                                             sizeof(frame_id),
                                             &x,
                                             &y,
                                             &z,
                                             &yaw,
                                             &pitch,
                                             &roll,
                                             &grip,
                                             &hold_ms,
                                             &tolerance,
                                             &safety_hold);

            if (parsed == 1) {
                continue;
            }
            if (parsed != 0) {
                fprintf(stderr, "invalid hand keyframe command: %s", lines[i]);
                return 1;
            }

            printf("site/demo/node/linux-sim-001/hand/keyframe/event "
                   "{\"event\":\"keyframe\",\"frame_id\":\"%s\",\"t_ms\":%u,"
                   "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,"
                   "\"yaw\":%.5f,\"pitch\":%.5f,\"roll\":%.5f,"
                   "\"grip\":%.5f,\"hold_ms\":%u,\"tolerance\":%.5f,"
                   "\"safety_hold\":%u,\"loop\":%u}\n",
                   frame_id,
                   now_ms,
                   x,
                   y,
                   z,
                   yaw,
                   pitch,
                   roll,
                   grip,
                   hold_ms,
                   tolerance,
                   safety_hold ? 1U : 0U,
                   loop + 1);
            now_ms += sample_ms;
        }
    }

    return 0;
}

static int adapt_io_to_hand(FILE *input, const char *frame_prefix)
{
    char line[1024];
    size_t count = 0;
    uint32_t first_t_ms = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float grip = 0.0f;
    uint32_t safety_hold = 0;
    uint32_t seen_mask = 0;

    if (!input) {
        return 1;
    }
    if (!frame_prefix) {
        frame_prefix = "observed";
    }

    while (fgets(line, sizeof(line), input)) {
        uint8_t slot = 0;
        char type[16];
        uint16_t channel = 0;
        int value = 0;
        uint32_t t_ms = 0;
        int parsed = parse_slot_event_json(line, &slot, type, sizeof(type), &channel, &value, &t_ms);

        (void)slot;
        if (parsed == 1) {
            continue;
        }
        if (parsed != 0) {
            fprintf(stderr, "invalid slot event: %s", line);
            return 1;
        }

        if (strcmp(type, "ai") == 0 || strcmp(type, "ao") == 0) {
            float normalized = (float)value / 1000.0f;

            if (channel == 1 && seen_mask == 0x7fU) {
                ++count;
                printf("site/demo/node/linux-sim-001/hand/keyframe/event "
                       "{\"event\":\"keyframe\",\"frame_id\":\"%s_%04zu\",\"t_ms\":%u,"
                       "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,"
                       "\"yaw\":%.5f,\"pitch\":%.5f,\"roll\":%.5f,"
                       "\"grip\":%.5f,\"hold_ms\":0,\"tolerance\":0.01500,"
                       "\"safety_hold\":%u,\"source\":\"io\"}\n",
                       frame_prefix,
                       count,
                       first_t_ms,
                       x,
                       y,
                       z,
                       yaw,
                       pitch,
                       roll,
                       grip,
                       safety_hold);
                seen_mask = 0;
            }
            if (seen_mask == 0) {
                first_t_ms = t_ms;
            }

            switch (channel) {
            case 1:
                x = clamp_f32(normalized, -1.0f, 1.0f);
                seen_mask |= 1U << 0;
                break;
            case 2:
                y = clamp_f32(normalized, -1.0f, 1.0f);
                seen_mask |= 1U << 1;
                break;
            case 3:
                z = clamp_f32(normalized, -1.0f, 1.0f);
                seen_mask |= 1U << 2;
                break;
            case 4:
                yaw = clamp_f32(normalized, -3.14159f, 3.14159f);
                seen_mask |= 1U << 3;
                break;
            case 5:
                pitch = clamp_f32(normalized, -3.14159f, 3.14159f);
                seen_mask |= 1U << 4;
                break;
            case 6:
                roll = clamp_f32(normalized, -3.14159f, 3.14159f);
                seen_mask |= 1U << 5;
                break;
            case 7:
                grip = clamp_f32(normalized, 0.0f, 1.0f);
                seen_mask |= 1U << 6;
                break;
            default:
                continue;
            }
        } else if ((strcmp(type, "di") == 0 || strcmp(type, "do") == 0 ||
                    strcmp(type, "relay") == 0) && channel == 1) {
            safety_hold = value ? 1U : 0U;
        } else {
            continue;
        }
    }

    if (seen_mask == 0x7fU) {
        ++count;
        printf("site/demo/node/linux-sim-001/hand/keyframe/event "
               "{\"event\":\"keyframe\",\"frame_id\":\"%s_%04zu\",\"t_ms\":%u,"
               "\"x\":%.5f,\"y\":%.5f,\"z\":%.5f,"
               "\"yaw\":%.5f,\"pitch\":%.5f,\"roll\":%.5f,"
               "\"grip\":%.5f,\"hold_ms\":0,\"tolerance\":0.01500,"
               "\"safety_hold\":%u,\"source\":\"io\"}\n",
               frame_prefix,
               count,
               first_t_ms,
               x,
               y,
               z,
               yaw,
               pitch,
               roll,
               grip,
               safety_hold);
    }

    return count > 0 ? 0 : 1;
}

static int record_hand_keyframes(FILE *input)
{
    char line[1024];
    size_t count = 0;

    if (!input) {
        return 1;
    }

    printf("frame_id,t_ms,x,y,z,yaw,pitch,roll,grip,hold_ms,tolerance,safety_hold\n");
    while (fgets(line, sizeof(line), input)) {
        char frame_id[64];
        uint32_t t_ms = 0;
        uint32_t hold_ms = 0;
        uint32_t safety_hold = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float grip = 0.0f;
        float tolerance = 0.0f;
        int parsed = parse_hand_event_json(line,
                                           frame_id,
                                           sizeof(frame_id),
                                           &t_ms,
                                           &x,
                                           &y,
                                           &z,
                                           &yaw,
                                           &pitch,
                                           &roll,
                                           &grip,
                                           &hold_ms,
                                           &tolerance,
                                           &safety_hold);

        if (parsed == 1) {
            continue;
        }
        if (parsed != 0) {
            fprintf(stderr, "invalid hand keyframe event: %s", line);
            return 1;
        }

        printf("%s,%u,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%u,%.5f,%u\n",
               frame_id,
               t_ms,
               x,
               y,
               z,
               yaw,
               pitch,
               roll,
               grip,
               hold_ms,
               tolerance,
               safety_hold ? 1U : 0U);
        ++count;
    }

    return count > 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    static const linux_io_sim_channel_t channels[] = {
        { "door", LINUX_IO_SIM_DI, 0 },
        { "relay", LINUX_IO_SIM_DO, 0 },
        { "pressure", LINUX_IO_SIM_AI, 0 },
        { "valve", LINUX_IO_SIM_AO, 0 },
    };
    linux_io_sim_options_t options = {
        .site = "demo",
        .node = "linux-sim-001",
        .event_cb = print_event,
    };
    FILE *input = stdin;
    char line[160];
    uint32_t now_ms = 0;
    uint32_t slot_loops = 1;
    uint32_t sample_ms = 300;
    int slot_stream = 0;
    int hand_stream = 0;
    int hand_record = 0;
    int io_hand_adapter = 0;
    const char *frame_prefix = "observed";
    int i;

    options.user = &options;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--slot-stream") == 0) {
            slot_stream = 1;
        } else if (strcmp(argv[i], "--hand-stream") == 0) {
            hand_stream = 1;
        } else if (strcmp(argv[i], "--record-hand-keyframes") == 0) {
            hand_record = 1;
        } else if (strcmp(argv[i], "--io-hand-adapter") == 0) {
            io_hand_adapter = 1;
        } else if (strcmp(argv[i], "--frame-prefix") == 0 && i + 1 < argc) {
            frame_prefix = argv[++i];
        } else if (strcmp(argv[i], "--loop") == 0 && i + 1 < argc) {
            if (parse_u32(argv[++i], &slot_loops) != 0 || slot_loops == 0) {
                return 2;
            }
        } else if (strcmp(argv[i], "--sample-ms") == 0 && i + 1 < argc) {
            if (parse_u32(argv[++i], &sample_ms) != 0 || sample_ms == 0) {
                return 2;
            }
        } else {
            input = fopen(argv[i], "r");
            if (!input) {
                perror(argv[i]);
                return 1;
            }
        }
    }

    if (slot_stream) {
        int result = run_slot_stream(input, slot_loops, sample_ms);
        if (input != stdin) {
            fclose(input);
        }
        return result;
    }

    if (hand_stream) {
        int result = run_hand_stream(input, slot_loops, sample_ms);
        if (input != stdin) {
            fclose(input);
        }
        return result;
    }

    if (hand_record) {
        int result = record_hand_keyframes(input);
        if (input != stdin) {
            fclose(input);
        }
        return result;
    }

    if (io_hand_adapter) {
        int result = adapt_io_to_hand(input, frame_prefix);
        if (input != stdin) {
            fclose(input);
        }
        return result;
    }

    if (linux_io_sim_init(channels, sizeof(channels) / sizeof(channels[0]), &options) != 0) {
        if (input != stdin) {
            fclose(input);
        }
        return 1;
    }

    while (fgets(line, sizeof(line), input)) {
        if (linux_io_sim_apply_script_line(line, &now_ms) != 0) {
            fprintf(stderr, "invalid simulator command: %s", line);
            if (input != stdin) {
                fclose(input);
            }
            return 1;
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
