#pragma once

typedef enum
{
    OFF = 0b000,
    RED = 0b100,
    GREEN = 0b010,
    BLUE = 0b001,
    YELLOW = 0b110,
    MAGENTA = 0b101,
    CYAN = 0b011,
    WHITE = 0b111,
} LedColor;

void ledInit();

void ledSetColor(LedColor color, uint8_t idx);