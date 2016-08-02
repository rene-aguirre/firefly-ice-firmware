#include "fdi_relay_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_relay.h"

#include "fd_binary.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t control;
} fdi_relay_instrument_t;

#define fdi_relay_instrument_count 5

static const uint64_t apiTypeSet = 1;

fdi_relay_instrument_t fdi_relay_instruments[fdi_relay_instrument_count];

fdi_relay_instrument_t *fdi_relay_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_relay_instrument_count; ++i) {
        fdi_relay_instrument_t *instrument = &fdi_relay_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_relay_instrument_set(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_relay_instrument_t *instrument = fdi_relay_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool on = fd_binary_get_uint8(binary) != 0;
    fdi_relay_set(instrument->control, on);
}

void fdi_relay_instrument_initialize(void) {
    uint32_t controls[] = {
        FDI_RELAY_ATE_BUTTON_EN,
        FDI_RELAY_ATE_USB_5V_EN,
        FDI_RELAY_ATE_USB_D_EN,
        FDI_RELAY_ATE_MCU_VCC_SENSE,
        FDI_RELAY_ATE_BATTERY_SENSE,
    };

    for (int i = 0; i < fdi_relay_instrument_count; ++i) {
        fdi_relay_instrument_t *instrument = &fdi_relay_instruments[i];
        instrument->super.category = "Relay";
        fdi_instrument_register(&instrument->super);
        instrument->control = controls[i];
        fdi_api_register(instrument->super.identifier, apiTypeSet, fdi_relay_instrument_set);
    }
}