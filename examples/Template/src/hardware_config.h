/*
 * hardware_config.h
 *
 * Revision: V2.0
 * Modified: 07-09-2026
 */ 

#ifndef HARDWARE_CONFIG_H_
#define HARDWARE_CONFIG_H_
#ifdef INCLUDE_NODEMCU32_BOARD_SUPPORT

#include "dio_lib.h"
#include "spi_lib.h"
#include "qc_7366_lib.h"
#include "dac_4922_lib.h"
#include "adc_3208_lib.h"
#include "button_lib.h"


typedef struct {
    dio_device dio;
    spi_device spi_bus;
    qc7366 qc;
    dac4922 dac;
    adc3208 adc;
    button buttons;
} HardwareConfig;

#else
typedef struct {
} HardwareConfig;
#endif // INCLUDE_NODEMCU32_BOARD_SUPPORT

HardwareConfig *ConfigureHardware(void);

#endif  // HARDWARE_CONFIG_H_   
