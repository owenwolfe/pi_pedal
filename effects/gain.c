#include <stdint.h>
#include <limits.h>
#include "gain.h"

int16_t apply_gain(int16_t side, float gain) {
    float amplified = side * gain;

    if (amplified > INT16_MAX) {
        amplified = INT16_MAX;
    }

    if (amplified < INT16_MIN) {
        amplified = INT16_MIN;
    }

    return (int16_t)amplified;
}
