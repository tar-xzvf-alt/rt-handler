#include "gpio_config.h"
#include <errno.h>
#include <limits.h>
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
            cfg->has_chip_path = true;
            cfg->has_offset_in = true;
            cfg->has_offset_out = true;
            cfg->has_edge = true;
            cfg->has_consumer = true;
            if (!cfg->chip_path || !cfg->consumer) {
                perror("strdup board preset");
                config_free(cfg);
                return -1;
            }
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

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;

    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) {
        *--end = '\0';
    }

    return s;
}

static int parse_uint_value(const char *s, unsigned int *out)
{
    char *end = NULL;

    errno = 0;
    unsigned long value = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || value > UINT_MAX) {
        return -1;
    }

    *out = (unsigned int)value;
    return 0;
}

static int parse_bool_value(const char *s, bool *out)
{
    if (strcmp(s, "1") == 0 || strcmp(s, "true") == 0 || strcmp(s, "yes") == 0) {
        *out = true;
        return 0;
    }

    if (strcmp(s, "0") == 0 || strcmp(s, "false") == 0 || strcmp(s, "no") == 0) {
        *out = false;
        return 0;
    }

    return -1;
}

static int set_string(char **dst, const char *value)
{
    char *copy = strdup(value);
    if (!copy) {
        perror("strdup config value");
        return -1;
    }

    free(*dst);
    *dst = copy;
    return 0;
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
        char *key = trim(p);
        char *val = trim(eq + 1);

        if (strcmp(key, "GPIO_CHIP") == 0) {
            if (*val == '\0' || set_string(&cfg->chip_path, val) != 0) goto error;
            cfg->has_chip_path = true;
        } else if (strcmp(key, "GPIO_OFFSET_IN") == 0) {
            if (parse_uint_value(val, &cfg->offset_in) != 0) {
                fprintf(stderr, "Invalid GPIO_OFFSET_IN: %s\n", val);
                goto error;
            }
            cfg->has_offset_in = true;
        } else if (strcmp(key, "GPIO_OFFSET_OUT") == 0) {
            if (parse_uint_value(val, &cfg->offset_out) != 0) {
                fprintf(stderr, "Invalid GPIO_OFFSET_OUT: %s\n", val);
                goto error;
            }
            cfg->has_offset_out = true;
        } else if (strcmp(key, "GPIO_EDGE") == 0) {
            int e = parse_edge(val);
            if (e < 0) {
                fprintf(stderr, "Invalid GPIO_EDGE: %s\n", val);
                goto error;
            }
            cfg->edge = e;
            cfg->has_edge = true;
        } else if (strcmp(key, "GPIO_MODE_TOGGLE") == 0) {
            if (parse_bool_value(val, &cfg->mode_toggle) != 0) {
                fprintf(stderr, "Invalid GPIO_MODE_TOGGLE: %s\n", val);
                goto error;
            }
        } else if (strcmp(key, "GPIO_CONSUMER") == 0) {
            if (*val == '\0' || set_string(&cfg->consumer, val) != 0) goto error;
            cfg->has_consumer = true;
        }
    }

    fclose(f);
    return 0;

error:
    fclose(f);
    return -1;
}

int config_validate(const GpioConfig *cfg)
{
    if (!cfg->has_chip_path || !cfg->chip_path || cfg->chip_path[0] == '\0') {
        fprintf(stderr, "Missing GPIO_CHIP\n");
        return -1;
    }
    if (!cfg->has_offset_in) {
        fprintf(stderr, "Missing GPIO_OFFSET_IN\n");
        return -1;
    }
    if (!cfg->has_offset_out) {
        fprintf(stderr, "Missing GPIO_OFFSET_OUT\n");
        return -1;
    }
    if (!cfg->has_edge) {
        fprintf(stderr, "Missing GPIO_EDGE\n");
        return -1;
    }
    if (!cfg->has_consumer || !cfg->consumer || cfg->consumer[0] == '\0') {
        fprintf(stderr, "Missing GPIO_CONSUMER\n");
        return -1;
    }

    return 0;
}
