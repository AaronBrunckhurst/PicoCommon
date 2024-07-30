#ifndef PICO_SYSTEM_TEMPERTURE_H
#define PICO_SYSTEM_TEMPERTURE_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

bool temperture_init(void);
bool temperture_deinit(void);

float read_onboard_temperature_f();
float read_onboard_temperature_c();

#ifdef __cplusplus
};
#endif

#endif // PICO_SYSTEM_TEMPERTURE_H
