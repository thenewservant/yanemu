#pragma once

#include <stdint.h>
#include <stdlib.h>

#define REF 4000
#define DUTY_CYCLE 0b11000000
#define TIMER_HIGH 0b00000111
//$4015-- - D NT21 	Control : DMC enable, length counter enables : noise, triangle, pulse 2, pulse 1 (write)
#define DMC_ENABLE 0b00010000
#define LEN_COUNT_EN_N 0b00001000
#define LEN_COUNT_EN_T 0b00000100
#define LEN_COUNT_EN_P2 0b00000010
#define LEN_COUNT_EN_P1 0b00000001
#define SWEEP_ENABLE_1 0b10000000
#define SWEEP_PERIOD 0b01110000
#define SHIFT_COUNT 0b00000111

void readChannels();
