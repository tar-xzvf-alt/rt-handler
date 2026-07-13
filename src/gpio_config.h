#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <gpiod.h>
#include <stdbool.h>

typedef struct {
    char *chip_path;
    unsigned int offset_in;
    unsigned int offset_out;
    int edge;
    bool mode_toggle;
    char *consumer;
    bool has_chip_path;
    bool has_offset_in;
    bool has_offset_out;
    bool has_edge;
    bool has_consumer;
} GpioConfig;

typedef struct {
    const char *name;
    const char *chip_path;
    unsigned int offset_in;
    unsigned int offset_out;
    int edge;
    bool mode_toggle;
    const char *consumer;
} BoardPreset;

extern const BoardPreset board_presets[];
extern const int board_presets_count;

void config_init(GpioConfig *cfg);
void config_free(GpioConfig *cfg);
int config_from_preset(GpioConfig *cfg, const char *board_name);
int config_from_file(GpioConfig *cfg, const char *filepath);
int config_validate(const GpioConfig *cfg);

#endif
