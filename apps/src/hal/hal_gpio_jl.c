/**
 * @file hal_gpio_jl.c
 * @brief 杰理 BD19 GPIO 实现（hal_gpio_*）。
 */
#include "hal_gpio.h"
#include "typedef.h"
#include "asm/gpio.h"

int hal_pin_valid(hal_pin_t pin)
{
    return pin >= 0;
}

void hal_gpio_write(hal_pin_t pin, int level_high)
{
    if (!hal_pin_valid(pin)) {
        return;
    }
    gpio_set_output_value((u32)pin, level_high ? 1 : 0);
}

void hal_gpio_i2c_bb_init(hal_pin_t scl, hal_pin_t sda)
{
    if (!hal_pin_valid(scl) || !hal_pin_valid(sda)) {
        return;
    }
    gpio_set_die((u32)scl, 1);
    gpio_set_die((u32)sda, 1);
    gpio_set_direction((u32)scl, 0);
    gpio_set_pull_up((u32)scl, 1);
    gpio_set_pull_down((u32)scl, 0);
    gpio_set_direction((u32)sda, 0);
    gpio_set_pull_up((u32)sda, 1);
    gpio_set_pull_down((u32)sda, 0);
}

void hal_gpio_input_floating(hal_pin_t pin)
{
    if (!hal_pin_valid(pin)) {
        return;
    }
    gpio_set_direction((u32)pin, 1);
    gpio_set_pull_up((u32)pin, 0);
    gpio_set_pull_down((u32)pin, 0);
}

u8 hal_gpio_read(hal_pin_t pin)
{
    if (!hal_pin_valid(pin)) {
        return 1;
    }
    return gpio_read((u32)pin) ? (u8)1 : (u8)0;
}

void hal_gpio_direction_output(hal_pin_t pin, int level_high)
{
    if (!hal_pin_valid(pin)) {
        return;
    }
    gpio_set_direction((u32)pin, 0);
    gpio_set_output_value((u32)pin, level_high ? 1 : 0);
}
