#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
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

int main(int argc, char *argv[])
{
    const char *board_name = NULL;
    const char *config_path = NULL;
    sigset_t signal_set;
    int opt;

    setvbuf(stdout, NULL, _IOLBF, 0);

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

    signal(SIGPIPE, SIG_IGN);

    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGTERM);
    sigaddset(&signal_set, SIGQUIT);
    if (pthread_sigmask(SIG_BLOCK, &signal_set, NULL) != 0) {
        perror("pthread_sigmask");
        return 1;
    }

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

    if (config_validate(&shared->config) != 0) {
        config_free(&shared->config);
        pthread_mutex_destroy(&shared->stats_mutex);
        free(shared);
        return 1;
    }

    __atomic_store_n(&shared->running, 1, __ATOMIC_SEQ_CST);
    shared->pulse_counter = 0;
    shared->worker_failed = 0;

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

    for (;;) {
        int sig;
        siginfo_t si;
        struct timespec timeout = { .tv_sec = 0, .tv_nsec = 100000000 };

        if (pthread_tryjoin_np(shared->gpio_thread, NULL) == 0) {
            break;
        }

        sig = sigtimedwait(&signal_set, &si, &timeout);
        if (sig > 0) {
            printf("\nReceived signal %d (%s), cleaning up...\n",
                   sig, strsignal(sig));
            __atomic_store_n(&shared->running, 0, __ATOMIC_SEQ_CST);
            usleep(100000);
            pthread_cancel(shared->gpio_thread);
            pthread_join(shared->gpio_thread, NULL);
            break;
        }

        if (sig < 0 && errno != EAGAIN && errno != EINTR) {
            perror("sigtimedwait");
            __atomic_store_n(&shared->running, 0, __ATOMIC_SEQ_CST);
            pthread_cancel(shared->gpio_thread);
            pthread_join(shared->gpio_thread, NULL);
            break;
        }
    }

    int exit_code = __atomic_load_n(&shared->worker_failed, __ATOMIC_SEQ_CST) ? 1 : 0;

    config_free(&shared->config);
    pthread_mutex_destroy(&shared->stats_mutex);
    free(shared);

    printf("RT Handler %s\n", exit_code == 0 ? "stopped normally" : "stopped with errors");
    return exit_code;
}
