#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "shared_data.h"
#include "gpio_handler.h"

static SharedData *shared = NULL;

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -b <board> [-c <config>]\n", prog);
    fprintf(stderr, "  -b <board>   board preset (");
    for (int i = 0; i < board_presets_count; i++) {
        fprintf(stderr, "%s%s", i > 0 ? "|" : "", board_presets[i].name);
    }
    fprintf(stderr, ")\n");
    fprintf(stderr, "  -c <config>  config file (overrides preset)\n");
    fprintf(stderr, "  -h           this help\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "For systemd service use: rt-handler-set-board <board>\n");
}

void signal_handler(int sig)
{
    printf("\nReceived signal %d (%s), cleaning up...\n",
           sig, strsignal(sig));

    if (shared) {
        __atomic_store_n(&shared->running, 0, __ATOMIC_SEQ_CST);
        usleep(100000);
        pthread_cancel(shared->gpio_thread);
    }
}

int main(int argc, char *argv[])
{
    const char *board_name = NULL;
    const char *config_path = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "b:c:h")) != -1) {
        switch (opt) {
        case 'b':
            board_name = optarg;
            break;
        case 'c':
            config_path = optarg;
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            return opt == 'h' ? 0 : 1;
        }
    }

    if (!board_name && !config_path) {
        fprintf(stderr, "Error: specify -b <board> and/or -c <config>\n");
        print_usage(argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    shared = calloc(1, sizeof(SharedData));
    if (!shared) {
        perror("calloc");
        return 1;
    }

    pthread_mutex_init(&shared->stats_mutex, NULL);

    config_init(&shared->config);

    if (board_name) {
        if (config_from_preset(&shared->config, board_name) != 0) {
            fprintf(stderr, "Unknown board: %s\n", board_name);
            print_usage(argv[0]);
            config_free(&shared->config);
            pthread_mutex_destroy(&shared->stats_mutex);
            free(shared);
            return 1;
        }
    }

    if (config_path) {
        if (config_from_file(&shared->config, config_path) != 0) {
            fprintf(stderr, "Failed to load config: %s\n", config_path);
            config_free(&shared->config);
            pthread_mutex_destroy(&shared->stats_mutex);
            free(shared);
            return 1;
        }
    }

    __atomic_store_n(&shared->running, 1, __ATOMIC_SEQ_CST);
    shared->pulse_counter = 0;

    printf("========================================\n");
    printf("RT Handler started\n");
    printf("Board:  %s\n", board_name ? board_name : "custom");
    printf("Chip:   %s\n", shared->config.chip_path);
    printf("IN:     offset=%u\n", shared->config.offset_in);
    printf("OUT:    offset=%u\n", shared->config.offset_out);
    printf("Edge:   %d\n", shared->config.edge);
    printf("Toggle: %s\n", shared->config.mode_toggle ? "yes" : "no");
    printf("Consumer: %s\n", shared->config.consumer);
    printf("Press Ctrl+C to stop\n");
    printf("========================================\n");

    if (pthread_create(&shared->gpio_thread, NULL, gpio_handler_thread, shared) != 0) {
        perror("pthread_create gpio_thread");
        config_free(&shared->config);
        pthread_mutex_destroy(&shared->stats_mutex);
        free(shared);
        return 1;
    }

    pthread_join(shared->gpio_thread, NULL);

    config_free(&shared->config);
    pthread_mutex_destroy(&shared->stats_mutex);
    free(shared);

    printf("RT Handler stopped normally\n");
    return 0;
}
