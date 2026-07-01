#ifndef LINUX_IO_DEVICE_SIMUL_H
#define LINUX_IO_DEVICE_SIMUL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LINUX_IO_SIM_MAX_CHANNELS
#define LINUX_IO_SIM_MAX_CHANNELS 64
#endif

typedef enum {
    LINUX_IO_SIM_DI = 0,
    LINUX_IO_SIM_DO,
    LINUX_IO_SIM_AI,
    LINUX_IO_SIM_AO,
} linux_io_sim_type_t;

typedef enum {
    LINUX_IO_SIM_EVENT_CHANGED = 0,
    LINUX_IO_SIM_EVENT_RISING,
    LINUX_IO_SIM_EVENT_FALLING,
    LINUX_IO_SIM_EVENT_WRITE,
    LINUX_IO_SIM_EVENT_FAULT,
} linux_io_sim_event_type_t;

typedef struct {
    const char *name;
    linux_io_sim_type_t type;
    int32_t initial_value;
} linux_io_sim_channel_t;

typedef struct {
    const char *name;
    linux_io_sim_type_t type;
    int32_t value;
    uint8_t fault;
    uint32_t changed_at_ms;
} linux_io_sim_sample_t;

typedef struct {
    linux_io_sim_event_type_t event;
    linux_io_sim_sample_t sample;
} linux_io_sim_event_t;

typedef void (*linux_io_sim_event_cb_t)(const linux_io_sim_event_t *event, void *user);

typedef struct {
    const char *site;
    const char *node;
    linux_io_sim_event_cb_t event_cb;
    void *user;
} linux_io_sim_options_t;

int linux_io_sim_init(const linux_io_sim_channel_t *channels,
                      size_t channel_count,
                      const linux_io_sim_options_t *options);
void linux_io_sim_reset(void);
size_t linux_io_sim_channel_count(void);
int linux_io_sim_read(const char *name, linux_io_sim_sample_t *sample);
int linux_io_sim_set_input(const char *name, int32_t value, uint32_t now_ms);
int linux_io_sim_write_output(const char *name, int32_t value, uint32_t now_ms);
int linux_io_sim_poll(uint32_t now_ms);

const char *linux_io_sim_type_name(linux_io_sim_type_t type);
const char *linux_io_sim_event_name(linux_io_sim_event_type_t event);

int linux_io_sim_format_topic(char *out,
                              size_t out_size,
                              const char *site,
                              const char *node,
                              const char *channel,
                              const char *suffix);
int linux_io_sim_format_event_json(char *out,
                                   size_t out_size,
                                   const linux_io_sim_event_t *event);
int linux_io_sim_apply_script_line(const char *line, uint32_t *now_ms);

#ifdef __cplusplus
}
#endif

#endif

