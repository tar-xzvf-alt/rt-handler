#include "gpio_handler.h"
#include "shared_data.h"
#include <gpiod.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

void* gpio_handler_thread(void* arg)
{
    SharedData *shared = (SharedData*)arg;
    GpioConfig *cfg = &shared->config;
    int status = 0;

    printf("GPIO handler thread started\n");

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    struct gpiod_chip *chip = NULL;
    struct gpiod_line_request *req = NULL;
    struct gpiod_line_settings *in_cfg = NULL, *out_cfg = NULL;
    struct gpiod_line_config *line_cfg = NULL;
    struct gpiod_request_config *req_cfg = NULL;
    struct gpiod_edge_event_buffer *evbuf = NULL;
    unsigned int offsets[2];

    offsets[0] = cfg->offset_in;
    offsets[1] = cfg->offset_out;

    chip = gpiod_chip_open(cfg->chip_path);
    if (!chip) {
        perror("gpiod_chip_open");
        status = -1;
        goto cleanup;
    }

    in_cfg = gpiod_line_settings_new();
    if (!in_cfg) {
        perror("gpiod_line_settings_new input");
        status = -1;
        goto cleanup;
    }
    gpiod_line_settings_set_direction(in_cfg, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(in_cfg, cfg->edge);

    out_cfg = gpiod_line_settings_new();
    if (!out_cfg) {
        perror("gpiod_line_settings_new output");
        status = -1;
        goto cleanup;
    }
    gpiod_line_settings_set_direction(out_cfg, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(out_cfg, 0);

    line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        perror("gpiod_line_config_new");
        status = -1;
        goto cleanup;
    }
    if (gpiod_line_config_add_line_settings(line_cfg, &offsets[0], 1, in_cfg) != 0) {
        perror("gpiod_line_config_add_line_settings input");
        status = -1;
        goto cleanup;
    }
    if (gpiod_line_config_add_line_settings(line_cfg, &offsets[1], 1, out_cfg) != 0) {
        perror("gpiod_line_config_add_line_settings output");
        status = -1;
        goto cleanup;
    }

    req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        perror("gpiod_request_config_new");
        status = -1;
        goto cleanup;
    }
    gpiod_request_config_set_consumer(req_cfg, cfg->consumer);

    req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!req) {
        perror("gpiod_chip_request_lines");
        status = -1;
        goto cleanup;
    }

    evbuf = gpiod_edge_event_buffer_new(1);
    if (!evbuf) {
        perror("gpiod_edge_event_buffer_new");
        status = -1;
        goto cleanup;
    }

    int wait_status = 0;
    struct gpiod_edge_event *ev;

    printf("GPIO handler waiting for interrupts...\n");

    bool toggle = false;

    while (__atomic_load_n(&shared->running, __ATOMIC_SEQ_CST)) {
        wait_status = gpiod_line_request_read_edge_events(req, evbuf, 1);

        if (wait_status < 0) {
            if (errno == EINTR) {
                break;
            }
            perror("gpiod_line_request_read_edge_events");
            usleep(10000);
            continue;
        }

        if (wait_status == 0) {
            continue;
        }

        ev = gpiod_edge_event_buffer_get_event(evbuf, 0);
        if (!ev) {
            continue;
        }

        if (cfg->mode_toggle) {
            if (toggle) {
                gpiod_line_request_set_value(req, offsets[1], 1);
            } else {
                gpiod_line_request_set_value(req, offsets[1], 0);
            }
            toggle = !toggle;

            pthread_mutex_lock(&shared->stats_mutex);
            shared->pulse_counter++;
            pthread_mutex_unlock(&shared->stats_mutex);
        } else {
            if (gpiod_edge_event_get_event_type(ev) == GPIOD_EDGE_EVENT_RISING_EDGE) {
                gpiod_line_request_set_value(req, offsets[1], 1);
            } else {
                gpiod_line_request_set_value(req, offsets[1], 0);
            }

            pthread_mutex_lock(&shared->stats_mutex);
            shared->pulse_counter++;
            pthread_mutex_unlock(&shared->stats_mutex);
        }

        pthread_testcancel();
    }

cleanup:
    if (status != 0) {
        __atomic_store_n(&shared->worker_failed, 1, __ATOMIC_SEQ_CST);
    }

    printf("\nGPIO handler stopping...\n");

    if (evbuf) gpiod_edge_event_buffer_free(evbuf);
    if (req) gpiod_line_request_release(req);
    if (chip) gpiod_chip_close(chip);

    if (in_cfg) gpiod_line_settings_free(in_cfg);
    if (out_cfg) gpiod_line_settings_free(out_cfg);
    if (line_cfg) gpiod_line_config_free(line_cfg);
    if (req_cfg) gpiod_request_config_free(req_cfg);

    pthread_testcancel();
    printf("GPIO handler thread stopped\n");
    return NULL;
}
