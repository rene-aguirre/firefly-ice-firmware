#include "fd_processor.h"

#include <em_cmu.h>
#include <em_i2c.h>
#include <em_gpio.h>
#include <em_usart.h>

#include <stddef.h>

/*
void *memset(void *s, int c, size_t n) {
    uint8_t* p = s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}
*/

volatile int32_t fd_interrupts_disable_level;

void fd_interrupts_disable() {
    __disable_irq();
    ++fd_interrupts_disable_level;
}

void fd_interrupts_enable() {
    --fd_interrupts_disable_level;

    // paranoia: check for unmatched enable call and try to recover
    if (fd_interrupts_disable_level < 0) {
        fd_interrupts_disable_level = 0;
    }

    if (fd_interrupts_disable_level == 0) {
        __enable_irq();
    }
}

void CMU_IRQHandler(void) {
    uint32_t interrupts = CMU_IntGet();
    CMU_IntClear(CMU_IF_HFXORDY | CMU_IF_HFRCORDY);

    if (interrupts & CMU_IF_HFXORDY) {
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
//        CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
    } else
    if (interrupts & CMU_IF_HFRCORDY) {
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
//        CMU_OscillatorEnable(cmuOsc_HFXO, false, false);
    }
}

void fd_processor_initialize(void) {
    fd_interrupts_disable_level = 0;

    CMU_ClockDivSet(cmuClock_HFPER, cmuClkDiv_1);
    CMU_IntEnable(CMU_IF_HFXORDY | CMU_IF_HFRCORDY);
    NVIC_EnableIRQ(CMU_IRQn);

    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

    CMU_ClockEnable(cmuClock_HFPER, true);

    CMU_ClockEnable(cmuClock_GPIO, true);

    GPIO_PinModeSet(gpioPortA, 0, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 1, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 2, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 3, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 4, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 5, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 6, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortA, 15, gpioModeDisabled, 0); // unused port pin
    //
    GPIO_PinModeSet(gpioPortC, 4, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortC, 5, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortC, 8, gpioModeDisabled, 0); // unused port pin
    //
    GPIO_PinModeSet(gpioPortE, 8, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortE, 10, gpioModeDisabled, 0); // unused port pin
    GPIO_PinModeSet(gpioPortE, 11, gpioModeDisabled, 0); // unused port pin

    GPIO_PinModeSet(BAT_VDIV2EN_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(BAT_VDIV2_PORT_PIN, gpioModeDisabled, 0); // analog input

    GPIO_PinModeSet(AUX_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(MAG_INT_PORT_PIN, gpioModeInputPull, 0);
    GPIO_IntConfig(MAG_INT_PORT_PIN, true /* rising */, false /* falling */, true);
    GPIO_PinModeSet(I2C1_SDA_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C1_SCL_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(ACC_CSN_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(US0_CLK_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_MISO_PORT_PIN, gpioModeInputPull, 0);
    GPIO_PinModeSet(US0_MOSI_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(ACC_INT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(ACC_INT_PORT_PIN, true /* rising */, true /* falling */, true);

    GPIO_PinModeSet(LED_EN_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(LED1_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED2_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED3_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED4_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(LED5_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED6_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED7_PORT_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(LED8_PORT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(SWD_CLK_PORT_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(SWD_IO_PORT_PIN, gpioModeDisabled, 0);

    GPIO_PinModeSet(CHG_STAT_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(CHG_STAT_PORT_PIN, true /* rising */, true /* falling */, true);
    GPIO_PinModeSet(CHG_RATE_PORT_PIN, gpioModeDisabled, 0); // analog input

//    GPIO_PinModeSet(PWR_MODE_PORT_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(PWR_HIGH_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(USB_DM_PORT_PIN, gpioModeInput, 0);
    GPIO_PinModeSet(USB_DP_PORT_PIN, gpioModeInput, 0);

    GPIO_PinModeSet(NRF_PWR_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US1_CLK_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US1_MISO_PORT_PIN, gpioModeInputPull, 0);
    GPIO_PinModeSet(US1_MOSI_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_RESETN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_REQN_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(NRF_RDYN_PORT_PIN, gpioModeInput, 0);
    GPIO_IntConfig(NRF_RDYN_PORT_PIN, true /* rising */, true /* falling */, true);

//    GPIO_PinModeSet(LFXTAL_P_PORT_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(LFXTAL_N_PORT_PIN, gpioModePushPull, 0);

//    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
//    GPIO_PinModeSet(HFXTAL_P_PORT_PIN, gpioModePushPull, 0);
}

void __attribute((naked)) fd_delay_3x_cycles(uint32_t cycles __attribute__((unused))) {
    __asm(
        "    subs r0, #1\n"
        "    bne fd_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_SECOND 48000000

void fd_delay_ms(uint32_t ms) {
    while (ms--) {
        fd_delay_3x_cycles(CYCLES_PER_SECOND / 3000);
    }
}