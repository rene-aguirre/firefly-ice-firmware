#include "fd_adc.h"
#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_pins.h"

#include <em_adc.h>
#include <em_cmu.h>
#include <em_gpio.h>

#include <stdint.h>

static volatile uint32_t adc_temperature_value;
static volatile uint32_t adc_battery_voltage_value;
static volatile uint32_t adc_charge_current_value;

static volatile bool adc_complete;
static volatile fd_adc_channel_t adc_channel;
static volatile uint32_t adc_event;

static float fd_adc_vdd;

void fd_adc_initialize(void) {
    fd_adc_vdd = 2.5f;

    fd_adc_start(fd_adc_channel_temperature, false);
    fd_adc_start(fd_adc_channel_battery_voltage, false);
    fd_adc_start(fd_adc_channel_charge_current, false);
}

void fd_adc_set_vdd(float vdd) {
    fd_adc_vdd = vdd;
}

float fd_adc_get_vdd(void) {
    return fd_adc_vdd;
}

#define CAL_TEMP_0 ((uint8_t *)0x0FE081B2)
#define ADC0_TEMP_0_READ_1V25 ((uint16_t *)0x0FE081BE)

#define TGRAD_ADCTH -6.3f

float fd_adc_get_temperature(void) {
    uint8_t cal_temp_0 = *CAL_TEMP_0;
    int32_t temp_0_read = (*ADC0_TEMP_0_READ_1V25) >> 4;
    return cal_temp_0 - (temp_0_read - (int32_t)adc_temperature_value) * (1.0f / TGRAD_ADCTH);
}

#define ADCMAX 4095.0f
#define REF_1V25 1.25f

// correction for worst case when resistance value on top resistor is off by -1%, and bottom by +1%
#define WORST_CASE_CORRECTION 1.02f

float fd_adc_get_battery_voltage(void) {
    float v = adc_battery_voltage_value * fd_adc_vdd * ((2.0f / ADCMAX) * WORST_CASE_CORRECTION);
    if (v > 4.2f) {
        v = 4.2f;
    }
    return v;
}

#define CHARGE_AMPS 0.080f
#define CHARGE_VREF 1.22f

float fd_adc_get_charge_current(void) {
    return adc_charge_current_value * (REF_1V25 / ADCMAX) * (CHARGE_AMPS / CHARGE_VREF);
}

void fd_adc_sleep(void) {
}

void fd_adc_wake(void) {
}

bool fd_adc_in_progress(void) {
    return !adc_complete;
}

void fd_adc_ready(void) {
    uint32_t value = ADC_DataSingleGet(ADC0);
    switch (adc_channel) {
        case fd_adc_channel_temperature:
            adc_temperature_value = value;
        break;
        case fd_adc_channel_battery_voltage:
            GPIO_PinOutClear(BAT_VDIV2EN_PORT_PIN);
            adc_battery_voltage_value = value;
        break;
        case fd_adc_channel_charge_current:
            adc_charge_current_value = value;
        break;
    }
    GPIO_PinModeSet(BAT_VDIV2_PORT_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(CHG_RATE_PORT_PIN, gpioModeDisabled, 0);

    adc_complete = true;
    ADC_IntClear(ADC0, ADC_IF_SINGLE);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
    CMU_ClockEnable(cmuClock_ADC0, false);
}

void ADC0_IRQHandler(void) {
    fd_adc_ready();
    fd_event_set(adc_event);
}

void fd_adc_start(fd_adc_channel_t channel, bool asynchronous) {
    CMU_ClockEnable(cmuClock_ADC0, true);

    ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(7000000, 0);

    ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;
    singleInit.acqTime = adcAcqTime256;

    switch (channel) {
        case fd_adc_channel_temperature:
            singleInit.reference = adcRef1V25;
            singleInit.input = adcSingleInpTemp;
            adc_event = FD_EVENT_ADC_TEMPERATURE;
        break;
        case fd_adc_channel_battery_voltage:
            GPIO_PinModeSet(BAT_VDIV2_PORT_PIN, gpioModeInput, 0);
            GPIO_PinOutSet(BAT_VDIV2EN_PORT_PIN);
            singleInit.reference = adcRefVDD;
            singleInit.input = adcSingleInpCh6;
            adc_event = FD_EVENT_ADC_BATTERY_VOLTAGE;
        break;
        case fd_adc_channel_charge_current:
            GPIO_PinModeSet(CHG_RATE_PORT_PIN, gpioModeInput, 0);
            singleInit.reference = adcRef1V25;
            singleInit.input = adcSingleInpCh7;
            adc_event = FD_EVENT_ADC_CHARGE_CURRENT;
        break;
    }

    ADC_Init(ADC0, &init);
    ADC_InitSingle(ADC0, &singleInit);

    ADC_IntEnable(ADC0, ADC_IF_SINGLE);
    if (asynchronous) {
        NVIC_EnableIRQ(ADC0_IRQn);
    }

    adc_channel = channel;
    adc_complete = false;
    ADC_Start(ADC0, adcStartSingle);

    if (!asynchronous) {
        while ((ADC_IntGet(ADC0) & ADC_IF_SINGLE) == 0);
        fd_adc_ready();
    }
}