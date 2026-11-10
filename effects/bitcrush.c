#include <stdint.h>
#include "bitcrush.h"

int16_t bitcrush(int16_t sample, int step){

    return (sample / step) * step;
}
