#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

bool temperture_init(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    return true;
}
bool temperture_deinit(void)
{
    adc_set_temp_sensor_enabled(false);
    return true;
}

float read_onboard_temperature_c() 
{    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return tempC;
}

float read_onboard_temperature_f()
{
    return read_onboard_temperature_c() * 9.0 / 5.0 + 32;
}
