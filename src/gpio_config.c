#include "gpio_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const BoardPreset board_presets[] = {
    {
        .name       = "bcvm",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 15,
        .offset_out = 9,
        .edge       = GPIOD_LINE_EDGE_FALLING,
        .mode_toggle = true,
        .consumer   = "bcvm-monitor",
    },
    {
        .name       = "bvc",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 0,
        .offset_out = 4,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "bvcarm-mo",
    },
    {
        .name       = "bvc_arm",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 0,
        .offset_out = 4,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "bvcarm-mo",
    },
    {
        .name       = "lichee",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 140,
        .offset_out = 144,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "lichee-monitor",
    },
    {
        .name       = "radxa",
        .chip_path  = "/dev/gpiochip3",
        .offset_in  = 10,
        .offset_out = 11,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "radxa-monitor",
    },
    {
        .name       = "starfive",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 60,
        .offset_out = 61,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "starfive-monitor",
    },
    {
        .name       = "mangopi",
        .chip_path  = "/dev/gpiochip0",
        .offset_in  = 35,
        .offset_out = 36,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "mangopi-monitor",
    },
    {
        .name       = "rockpi4",
        .chip_path  = "/dev/gpiochip4",
        .offset_in  = 6,
        .offset_out = 7,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "rockpi4-monitor",
    },
    {
        .name       = "repkapi4",
        .chip_path  = "/dev/gpiochip1",
        .offset_in  = 205,
        .offset_out = 204,
        .edge       = GPIOD_LINE_EDGE_BOTH,
        .mode_toggle = false,
        .consumer   = "repkapi4-monitor",
    },
};

const int board_presets_count = sizeof(board_presets) / sizeof(board_presets[0]);

void config_init(GpioConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
}

void config_free(GpioConfig *cfg)
{
    free(cfg->chip_path);
    free(cfg->consumer);
    config_init(cfg);
}

int config_from_preset(GpioConfig *cfg, const char *board_name)
{
    for (int i = 0; i < board_presets_count; i++) {
        if (strcmp(board_presets[i].name, board_name) == 0) {
            const BoardPreset *p = &board_presets[i];
            cfg->chip_path  = strdup(p->chip_path);
            cfg->offset_in  = p->offset_in;
            cfg->offset_out = p->offset_out;
            cfg->edge       = p->edge;
            cfg->mode_toggle = p->mode_toggle;
            cfg->consumer   = strdup(p->consumer);
            return 0;
        }
    }
    return -1;
}

static int parse_edge(const char *s)
{
    if (strcmp(s, "both") == 0)    return GPIOD_LINE_EDGE_BOTH;
    if (strcmp(s, "rising") == 0)  return GPIOD_LINE_EDGE_RISING;
    if (strcmp(s, "falling") == 0) return GPIOD_LINE_EDGE_FALLING;
    if (strcmp(s, "none") == 0)    return GPIOD_LINE_EDGE_NONE;
    return -1;
}

int config_from_file(GpioConfig *cfg, const char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("fopen config");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '#' || *p == '\n' || *p == '\0') continue;

        size_t len = strlen(p);
        while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r'))
            p[--len] = '\0';

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = p;
        char *val = eq + 1;

        while (*key == ' ' || *key == '\t') key++;
        char *kend = key + strlen(key) - 1;
        while (kend > key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';

        while (*val == ' ' || *val == '\t') val++;
        char *vend = val + strlen(val) - 1;
        while (vend > val && (*vend == ' ' || *vend == '\t')) *vend-- = '\0';

        if (strcmp(key, "GPIO_CHIP") == 0) {
            free(cfg->chip_path);
            cfg->chip_path = strdup(val);
        } else if (strcmp(key, "GPIO_OFFSET_IN") == 0) {
            cfg->offset_in = (unsigned int)atoi(val);
        } else if (strcmp(key, "GPIO_OFFSET_OUT") == 0) {
            cfg->offset_out = (unsigned int)atoi(val);
        } else if (strcmp(key, "GPIO_EDGE") == 0) {
            int e = parse_edge(val);
            if (e >= 0) cfg->edge = e;
        } else if (strcmp(key, "GPIO_MODE_TOGGLE") == 0) {
            cfg->mode_toggle = (atoi(val) != 0);
        } else if (strcmp(key, "GPIO_CONSUMER") == 0) {
            free(cfg->consumer);
            cfg->consumer = strdup(val);
        }
    }

    fclose(f);
    return 0;
}
