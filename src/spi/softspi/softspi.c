#include "softspi_platform.h"

char softspi_init(struct softspi *__FAR bus, uint8_t flags) {
    uint8_t i;
    const struct softspi_platform *__FAR plat = bus->platform;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    bus->flags = flags;

    for (i = 0; i < plat->cs_num; ++i) {
        gpio_init(gpio_ops, plat->cs_pin[i], GPIO_MODE_OUTPUT_PUSHPULL);
        gpio_set_output(gpio_ops, plat->cs_pin[i], 1);
    }

    gpio_init(gpio_ops, plat->clk, GPIO_MODE_OUTPUT_PUSHPULL);
    gpio_set_output(gpio_ops, plat->clk, GPIO_MODE_OUTPUT_PUSHPULL);
    gpio_init(gpio_ops, plat->mosi, GPIO_MODE_OUTPUT_PUSHPULL);
    gpio_set_output(gpio_ops, plat->mosi, GPIO_MODE_OUTPUT_PUSHPULL);
    gpio_init(gpio_ops, plat->miso, GPIO_MODE_INPUT);

    gpio_set_output(gpio_ops, plat->clk, flags & SPI_FLAG_CLK_IDLE_HIGH);

    return 1;
}

char softspi_config_clk_idle(struct softspi *__FAR bus, uint8_t is_high) {
    const struct softspi_platform *__FAR plat = bus->platform;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    if (is_high) {
        bus->flags |= SPI_FLAG_CLK_IDLE_HIGH;
        gpio_set_output(gpio_ops, plat->clk, 1);
    } else {
        bus->flags &= ~SPI_FLAG_CLK_IDLE_HIGH;
        gpio_set_output(gpio_ops, plat->clk, 0);
    }

    return 1;
}

char softspi_is_clk_idle_high(struct softspi *__FAR bus) {
    return (bus->flags & SPI_FLAG_CLK_IDLE_HIGH) != 0;
}

char softspi_config_clk_edge(struct softspi *__FAR bus, uint8_t is_first) {
    if (is_first) {
        bus->flags |= SPI_FLAG_CLK_FIRST_EDGE;
    } else {
        bus->flags &= ~SPI_FLAG_CLK_FIRST_EDGE;
    }
    return 1;
}

char softspi_is_clk_edge_first(struct softspi *__FAR bus) {
    return (bus->flags & SPI_FLAG_CLK_FIRST_EDGE) != 0;
}

char softspi_config_first_bit(struct softspi *__FAR bus, uint8_t is_lsb_first) {
    if (is_lsb_first) {
        bus->flags |= SPI_FLAG_LSB_FIRST;
    } else {
        bus->flags &= ~SPI_FLAG_LSB_FIRST;
    }

    return 1;

}

char softspi_is_lsb_first(struct softspi *__FAR bus) {
    return (bus->flags & SPI_FLAG_LSB_FIRST ) != 0;
}


char softspi_select(struct softspi *__FAR bus, uint8_t which, char is_select) {
    const struct softspi_platform *__FAR plat = bus->platform;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    if (which >= plat->cs_num) {
        return 0;
    }

    gpio_set_output(gpio_ops, plat->cs_pin[which], is_select == 0);
    return 1;
}

inline static char __transmit_1st_edge_msb_first(struct softspi_platform const *__FAR plat, uint8_t *__FAR dat, uint8_t clk_idle_high) {
    uint8_t s, r, b;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    s = *dat;
    r = 0;
    for (b = 0x80; b != 0; b = b >> 1) {
        gpio_set_output(gpio_ops, plat->mosi, 0 != (b & s));
        gpio_set_output(gpio_ops, plat->clk, 0 == clk_idle_high);
        if (gpio_input_is_high(gpio_ops, plat->mosi)) {
            r |= b;
        }
        gpio_set_output(gpio_ops, plat->clk, 0 != clk_idle_high);
    }

    *dat = r;
    return 1;
}

inline static char __transmit_2st_edge_msb_first(struct softspi_platform const *__FAR plat, uint8_t *__FAR dat, uint8_t clk_idle_high) {
    uint8_t s, r, b;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    s = *dat;
    r = 0;
    for (b = 0x80; b != 0; b = b >> 1) {
        gpio_set_output(gpio_ops, plat->clk, 0 == clk_idle_high);
        gpio_set_output(gpio_ops, plat->mosi, 0 != (b & s));
        gpio_set_output(gpio_ops, plat->clk, 0 != clk_idle_high);
        if (gpio_input_is_high(gpio_ops, plat->mosi)) {
            r |= b;
        }
    }

    *dat = r;
    return 1;
}

inline static char __transmit_1st_edge_lsb_first(struct softspi_platform const *__FAR plat, uint8_t *__FAR dat, uint8_t clk_idle_high) {
    uint8_t s, r, b;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    s = *dat;
    r = 0;
    for (b = 0x01; b != 0; b = b << 1) {
        gpio_set_output(gpio_ops, plat->mosi, 0 != (b & s));
        gpio_set_output(gpio_ops, plat->clk, 0 == clk_idle_high);
        if (gpio_input_is_high(gpio_ops, plat->mosi)) {
            r |= b;
        }
        gpio_set_output(gpio_ops, plat->clk, 0 != clk_idle_high);
    }

    *dat = r;
    return 1;
}

inline static char __transmit_2st_edge_lsb_first(struct softspi_platform const *__FAR plat, uint8_t *__FAR dat, uint8_t clk_idle_high) {
    uint8_t s, r, b;
    gpio_ops_t gpio_ops = plat->gpio_ops;

    s = *dat;
    r = 0;
    for (b = 0x01; b != 0; b = b << 1) {
        gpio_set_output(gpio_ops, plat->clk, 0 == clk_idle_high);
        gpio_set_output(gpio_ops, plat->mosi, 0 != (b & s));
        gpio_set_output(gpio_ops, plat->clk, 0 != clk_idle_high);
        if (gpio_input_is_high(gpio_ops, plat->mosi)) {
            r |= b;
        }
    }

    *dat = r;
    return 1;
}

char softspi_transmit(struct softspi *__FAR bus, uint8_t *__FAR dat) {
    if (bus->flags & SPI_FLAG_LSB_FIRST) {
        if (bus->flags & SPI_FLAG_CLK_FIRST_EDGE) {
            return __transmit_1st_edge_lsb_first(bus->platform, dat, bus->flags & SPI_FLAG_CLK_IDLE_HIGH);
        } else {
            return __transmit_2st_edge_lsb_first(bus->platform, dat, bus->flags & SPI_FLAG_CLK_IDLE_HIGH);
        }
    } else {
        if (bus->flags & SPI_FLAG_CLK_FIRST_EDGE) {
            return __transmit_1st_edge_msb_first(bus->platform, dat, bus->flags & SPI_FLAG_CLK_IDLE_HIGH);
        } else {
            return __transmit_2st_edge_msb_first(bus->platform, dat, bus->flags & SPI_FLAG_CLK_IDLE_HIGH);
        }
    }
}

const struct spi_operations softspi_ops = {
    (spi_init_func)softspi_init,
    (spi_config_clk_idle_func)softspi_config_clk_idle,
    (spi_is_clk_idle_high_fucn)softspi_is_clk_idle_high,
    (spi_config_clk_edge_func)softspi_config_clk_edge,
    (spi_is_clk_first_edge_func)softspi_is_clk_edge_first,
    (spi_config_first_bit_func)softspi_config_first_bit,
    (spi_is_lsb_first_func)softspi_is_lsb_first,
    (spi_select_func)softspi_select,
    (spi_transmit_func)softspi_transmit,
};

