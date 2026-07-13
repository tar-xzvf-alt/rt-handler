#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>
#include "gpio_config.h"

typedef struct {
    volatile int running;
    volatile int pulse_counter;

    pthread_mutex_t stats_mutex;

    pthread_t gpio_thread;

    GpioConfig config;
} SharedData;

#endif
