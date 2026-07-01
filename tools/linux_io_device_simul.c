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
    int i;

    options.user = &options;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--slot-stream") == 0) {
            slot_stream = 1;
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
