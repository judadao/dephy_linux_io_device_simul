#include <linux_io_device_simul/io_device_simul.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    linux_io_sim_sample_t sample;
    int initialized;
} sim_slot_t;

static sim_slot_t g_slots[LINUX_IO_SIM_MAX_CHANNELS];
static size_t g_count;
static linux_io_sim_options_t g_options;

static int is_input(linux_io_sim_type_t type)
{
    return type == LINUX_IO_SIM_DI || type == LINUX_IO_SIM_AI;
}

static int is_output(linux_io_sim_type_t type)
{
    return type == LINUX_IO_SIM_DO || type == LINUX_IO_SIM_AO;
}

static int find_channel(const char *name)
{
    size_t i;

    if (!name) {
        return -1;
    }

    for (i = 0; i < g_count; ++i) {
        if (g_slots[i].sample.name && strcmp(g_slots[i].sample.name, name) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static void emit_event(linux_io_sim_event_type_t event_type,
                       const linux_io_sim_sample_t *sample)
{
    linux_io_sim_event_t event;

    if (!g_options.event_cb) {
        return;
    }

    event.event = event_type;
    event.sample = *sample;
    g_options.event_cb(&event, g_options.user);
}

int linux_io_sim_init(const linux_io_sim_channel_t *channels,
                      size_t channel_count,
                      const linux_io_sim_options_t *options)
{
    size_t i;

    if (!channels || channel_count > LINUX_IO_SIM_MAX_CHANNELS) {
        return -1;
    }

    linux_io_sim_reset();
    if (options) {
        g_options = *options;
    }

    for (i = 0; i < channel_count; ++i) {
        if (!channels[i].name) {
            linux_io_sim_reset();
            return -1;
        }

        g_slots[i].sample.name = channels[i].name;
        g_slots[i].sample.type = channels[i].type;
        g_slots[i].sample.value = channels[i].initial_value;
        g_slots[i].sample.fault = 0;
        g_slots[i].sample.changed_at_ms = 0;
        g_slots[i].initialized = 1;
    }

    g_count = channel_count;
    return 0;
}

void linux_io_sim_reset(void)
{
    memset(g_slots, 0, sizeof(g_slots));
    memset(&g_options, 0, sizeof(g_options));
    g_count = 0;
}

size_t linux_io_sim_channel_count(void)
{
    return g_count;
}

int linux_io_sim_read(const char *name, linux_io_sim_sample_t *sample)
{
    int idx = find_channel(name);

    if (idx < 0 || !sample) {
        return -1;
    }

    *sample = g_slots[idx].sample;
    return 0;
}

int linux_io_sim_set_input(const char *name, int32_t value, uint32_t now_ms)
{
    int idx = find_channel(name);
    linux_io_sim_event_type_t event_type = LINUX_IO_SIM_EVENT_CHANGED;
    linux_io_sim_sample_t *sample;

    if (idx < 0) {
        return -1;
    }

    sample = &g_slots[idx].sample;
    if (!is_input(sample->type)) {
        return -1;
    }

    if (sample->value == value && sample->fault == 0) {
        return 0;
    }

    if (sample->type == LINUX_IO_SIM_DI) {
        if (sample->value == 0 && value != 0) {
            event_type = LINUX_IO_SIM_EVENT_RISING;
        } else if (sample->value != 0 && value == 0) {
            event_type = LINUX_IO_SIM_EVENT_FALLING;
        }
    }

    sample->value = value;
    sample->fault = 0;
    sample->changed_at_ms = now_ms;
    emit_event(event_type, sample);
    return 0;
}

int linux_io_sim_write_output(const char *name, int32_t value, uint32_t now_ms)
{
    int idx = find_channel(name);
    linux_io_sim_sample_t *sample;

    if (idx < 0) {
        return -1;
    }

    sample = &g_slots[idx].sample;
    if (!is_output(sample->type)) {
        return -1;
    }

    sample->value = value;
    sample->fault = 0;
    sample->changed_at_ms = now_ms;
    emit_event(LINUX_IO_SIM_EVENT_WRITE, sample);
    return 0;
}

int linux_io_sim_poll(uint32_t now_ms)
{
    (void)now_ms;
    return 0;
}

const char *linux_io_sim_type_name(linux_io_sim_type_t type)
{
    switch (type) {
    case LINUX_IO_SIM_DI:
        return "di";
    case LINUX_IO_SIM_DO:
        return "do";
    case LINUX_IO_SIM_AI:
        return "ai";
    case LINUX_IO_SIM_AO:
        return "ao";
    default:
        return "unknown";
    }
}

const char *linux_io_sim_event_name(linux_io_sim_event_type_t event)
{
    switch (event) {
    case LINUX_IO_SIM_EVENT_CHANGED:
        return "changed";
    case LINUX_IO_SIM_EVENT_RISING:
        return "rising";
    case LINUX_IO_SIM_EVENT_FALLING:
        return "falling";
    case LINUX_IO_SIM_EVENT_WRITE:
        return "write";
    case LINUX_IO_SIM_EVENT_FAULT:
        return "fault";
    default:
        return "unknown";
    }
}

int linux_io_sim_format_topic(char *out,
                              size_t out_size,
                              const char *site,
                              const char *node,
                              const char *channel,
                              const char *suffix)
{
    int n;

    if (!out || out_size == 0 || !site || !node || !channel || !suffix) {
        return -1;
    }

    n = snprintf(out,
                 out_size,
                 "site/%s/node/%s/io/%s/%s",
                 site,
                 node,
                 channel,
                 suffix);
    if (n < 0 || (size_t)n >= out_size) {
        return -1;
    }

    return n;
}

int linux_io_sim_format_event_json(char *out,
                                   size_t out_size,
                                   const linux_io_sim_event_t *event)
{
    int n;

    if (!out || out_size == 0 || !event || !event->sample.name) {
        return -1;
    }

    n = snprintf(out,
                 out_size,
                 "{\"event\":\"%s\",\"channel\":\"%s\",\"type\":\"%s\","
                 "\"value\":%d,\"fault\":%u,\"t_ms\":%u}",
                 linux_io_sim_event_name(event->event),
                 event->sample.name,
                 linux_io_sim_type_name(event->sample.type),
                 event->sample.value,
                 event->sample.fault,
                 event->sample.changed_at_ms);
    if (n < 0 || (size_t)n >= out_size) {
        return -1;
    }

    return n;
}

int linux_io_sim_apply_script_line(const char *line, uint32_t *now_ms)
{
    char op[16];
    char name[64];
    int value = 0;
    unsigned delta = 0;

    if (!line || !now_ms) {
        return -1;
    }

    if (line[0] == '\0' || line[0] == '\n' || line[0] == '#') {
        return 0;
    }

    if (sscanf(line, "%15s", op) != 1) {
        return -1;
    }

    if (strcmp(op, "sleep") == 0) {
        if (sscanf(line, "%15s %u", op, &delta) != 2) {
            return -1;
        }
        *now_ms += delta;
        return 0;
    }

    if (strcmp(op, "poll") == 0) {
        return linux_io_sim_poll(*now_ms);
    }

    if (strcmp(op, "set") == 0) {
        if (sscanf(line, "%15s %63s %d", op, name, &value) != 3) {
            return -1;
        }
        return linux_io_sim_set_input(name, value, *now_ms);
    }

    if (strcmp(op, "write") == 0) {
        if (sscanf(line, "%15s %63s %d", op, name, &value) != 3) {
            return -1;
        }
        return linux_io_sim_write_output(name, value, *now_ms);
    }

    return -1;
}

