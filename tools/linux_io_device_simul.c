#include <linux_io_device_simul/io_device_simul.h>

#include <stdio.h>

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

    options.user = &options;

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror(argv[1]);
            return 1;
        }
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

