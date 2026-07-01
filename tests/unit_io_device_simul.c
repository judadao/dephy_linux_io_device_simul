#include <linux_io_device_simul/io_device_simul.h>

#include <assert.h>
#include <string.h>

typedef struct {
    linux_io_sim_event_t events[8];
    size_t count;
} recorder_t;

static void record_event(const linux_io_sim_event_t *event, void *user)
{
    recorder_t *rec = (recorder_t *)user;

    assert(rec->count < 8);
    rec->events[rec->count++] = *event;
}

static void test_script_drives_inputs_and_outputs(void)
{
    static const linux_io_sim_channel_t channels[] = {
        { "door", LINUX_IO_SIM_DI, 0 },
        { "relay", LINUX_IO_SIM_DO, 0 },
        { "pressure", LINUX_IO_SIM_AI, 0 },
    };
    recorder_t rec = { 0 };
    linux_io_sim_options_t options = {
        .site = "site-a",
        .node = "node-a",
        .event_cb = record_event,
        .user = &rec,
    };
    uint32_t now_ms = 0;
    linux_io_sim_sample_t sample;
    char payload[256];

    assert(linux_io_sim_init(channels, sizeof(channels) / sizeof(channels[0]), &options) == 0);
    assert(linux_io_sim_apply_script_line("sleep 10", &now_ms) == 0);
    assert(now_ms == 10);
    assert(linux_io_sim_apply_script_line("set door 1", &now_ms) == 0);
    assert(linux_io_sim_apply_script_line("set pressure 1234", &now_ms) == 0);
    assert(linux_io_sim_apply_script_line("write relay 1", &now_ms) == 0);

    assert(rec.count == 3);
    assert(rec.events[0].event == LINUX_IO_SIM_EVENT_RISING);
    assert(rec.events[1].event == LINUX_IO_SIM_EVENT_CHANGED);
    assert(rec.events[2].event == LINUX_IO_SIM_EVENT_WRITE);

    assert(linux_io_sim_read("pressure", &sample) == 0);
    assert(sample.value == 1234);
    assert(linux_io_sim_format_event_json(payload, sizeof(payload), &rec.events[0]) > 0);
    assert(strstr(payload, "\"channel\":\"door\"") != 0);
}

static void test_rejects_wrong_direction(void)
{
    static const linux_io_sim_channel_t channels[] = {
        { "door", LINUX_IO_SIM_DI, 0 },
        { "relay", LINUX_IO_SIM_DO, 0 },
    };

    assert(linux_io_sim_init(channels, sizeof(channels) / sizeof(channels[0]), 0) == 0);
    assert(linux_io_sim_write_output("door", 1, 0) != 0);
    assert(linux_io_sim_set_input("relay", 1, 0) != 0);
}

int main(void)
{
    test_script_drives_inputs_and_outputs();
    test_rejects_wrong_direction();
    return 0;
}

