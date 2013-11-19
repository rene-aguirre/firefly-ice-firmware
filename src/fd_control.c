#include "fd_aes.h"
#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_boot.h"
#include "fd_control.h"
#include "fd_control_codes.h"
#include "fd_event.h"
#include "fd_indicator.h"
#include "fd_lock.h"
#include "fd_main.h"
#include "fd_map.h"
#include "fd_power.h"
#include "fd_processor.h"
#include "fd_reset.h"
#include "fd_rtc.h"
#include "fd_sha.h"
#include "fd_storage.h"
#include "fd_sync.h"
#include "fd_system.h"
#include "fd_update.h"

#include <em_int.h>
#include <em_msc.h>

#include <string.h>

typedef void (*fd_control_command_t)(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#define COMMAND_BUFFER_SIZE 300

uint8_t fd_control_command_buffer[COMMAND_BUFFER_SIZE];

#define INPUT_BUFFER_SIZE 600

uint8_t fd_control_input_buffer[INPUT_BUFFER_SIZE];
uint32_t fd_control_input_buffer_count;

typedef struct {
    fd_detour_source_collection_t *detour_source_collection;
    uint8_t *data;
    uint32_t length;
} fd_control_input_t;

#define INPUTS_SIZE 16

fd_control_input_t fd_control_inputs[INPUTS_SIZE];
uint32_t fd_control_inputs_count;

#define DETOUR_BUFFER_SIZE 300

fd_detour_source_t fd_control_detour_source;
uint8_t fd_control_detour_buffer[DETOUR_BUFFER_SIZE];
fd_binary_t fd_control_detour_binary;

void fd_control_command(void);

void fd_control_initialize(void) {
    fd_detour_source_initialize(&fd_control_detour_source);

    fd_control_input_buffer_count = 0;
    fd_control_inputs_count = 0;

    fd_event_add_callback(FD_EVENT_COMMAND, fd_control_command);
}

void fd_control_process(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    if (fd_control_inputs_count >= INPUTS_SIZE) {
        return;
    }
    if ((fd_control_input_buffer_count + length) >= INPUT_BUFFER_SIZE) {
        return;
    }

    fd_control_input_t *input = &fd_control_inputs[fd_control_inputs_count];
    input->detour_source_collection = detour_source_collection;
    input->data = &fd_control_input_buffer[fd_control_input_buffer_count];
    input->length = length;

    memcpy(&fd_control_input_buffer[fd_control_input_buffer_count], data, length);
    fd_control_input_buffer_count += length;

    ++fd_control_inputs_count;

    fd_event_set(FD_EVENT_COMMAND);
}

void fd_control_detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    memcpy(data, &fd_control_detour_buffer[offset], length);
}

fd_binary_t *fd_control_send_start(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t type) {
    fd_binary_initialize(&fd_control_detour_binary, fd_control_detour_buffer, DETOUR_BUFFER_SIZE);
    fd_binary_put_uint8(&fd_control_detour_binary, type);
    return &fd_control_detour_binary;
}

void fd_control_send_complete(fd_detour_source_collection_t *detour_source_collection) {
    fd_detour_source_set(&fd_control_detour_source, fd_control_detour_supplier, fd_control_detour_binary.put_index);
    fd_detour_source_collection_push(detour_source_collection, &fd_control_detour_source);
}

void fd_control_ping(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint16_t ping_length = fd_binary_get_uint16(&binary);
    uint8_t *ping_data = &binary.buffer[binary.get_index];

    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_PING);
    fd_binary_put_uint16(binary_out, ping_length);
    fd_binary_put_bytes(binary_out, ping_data, ping_length);
    fd_control_send_complete(detour_source_collection);
}

#define USER_DATA_ADDRESS 0x0fe00000
// user data is 2kB

#define LOCK_BITS_ADDRESS 0x0FE04000
#define DEBUG_LOCK_WORD_ADDRESS (LOCK_BITS_ADDRESS + 127)

bool fd_get_debug_lock(void) {
    uint32_t *address = (uint32_t*)DEBUG_LOCK_WORD_ADDRESS;
    uint32_t debug_lock = *address;
    return (debug_lock & 0xf) != 0xf;
}

void fd_set_debug_lock(void) {
    if (!fd_get_debug_lock()) {
        uint32_t word = 0xfffffff0;
        fd_interrupts_disable();
        MSC_Init();
        MSC_WriteWord((uint32_t*)DEBUG_LOCK_WORD_ADDRESS, &word, 4);
        MSC_Deinit();
        fd_interrupts_enable();
    }
}

// provisioning format:
// - uint16_t version
// - uint16_t flags;
// - uint8_t[16] AES key
// - map

#define PROVISION_OPTION_DEBUG_LOCK 0x00000001
#define PROVISION_OPTION_RESET 0x00000002

typedef struct {
    uint16_t version;
    uint16_t flags;
    uint8_t key[FD_AES_KEY_SIZE];
    // map follows...
} fd_provision_t;

#define PROVISION_MAP_ADDRESS (USER_DATA_ADDRESS + 20)

void fd_control_provision(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint32_t options = fd_binary_get_uint32(&binary);
    uint32_t provision_data_length = fd_binary_get_uint16(&binary);
    uint8_t *provision_data = &binary.buffer[binary.get_index];

    uint32_t *address = (uint32_t*)USER_DATA_ADDRESS;
    uint32_t n = (provision_data_length + 3) & ~0x3; // round up to multiple of 4 bytes
    fd_interrupts_disable();
    MSC_Init();
    MSC_ErasePage(address);
    MSC_WriteWord(address, provision_data, n);
    MSC_Deinit();
    fd_interrupts_enable();

    if (options & PROVISION_OPTION_DEBUG_LOCK) {
        fd_set_debug_lock();
    }
    if (options & PROVISION_OPTION_RESET) {
        NVIC_SystemReset();
    }
}

void fd_control_reset(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint8_t type = fd_binary_get_uint8(&binary);
    fd_reset_by(type);
}

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 16
#define VERSION_CAPABILITIES (FD_CONTROL_CAPABILITY_LOCK | FD_CONTROL_CAPABILITY_BOOT_VERSION)

// !!! should come from gcc command line define
#define GIT_COMMIT 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19

static const uint8_t git_commit[20] = {GIT_COMMIT};

void fd_control_get_property_version(fd_binary_t *binary) {
    fd_binary_put_uint16(binary, VERSION_MAJOR);
    fd_binary_put_uint16(binary, VERSION_MINOR);
    fd_binary_put_uint16(binary, VERSION_PATCH);
    fd_binary_put_uint32(binary, VERSION_CAPABILITIES);
    fd_binary_put_bytes(binary, (uint8_t *)git_commit, sizeof(git_commit));
}

void fd_control_get_property_hardware_id(fd_binary_t *binary) {
    fd_get_hardware_id(binary);
}

void fd_control_get_property_site(fd_binary_t *binary) {
    uint8_t *address = (uint8_t*)USER_DATA_ADDRESS;
    uint8_t *site;
    uint16_t site_length;
    fd_map_get(address, "site", FD_MAP_TYPE_UTF8, &site, &site_length);

    fd_binary_put_uint16(binary, site_length);
    fd_binary_put_bytes(binary, site, site_length);
}

void fd_control_get_property_reset(fd_binary_t *binary) {
    fd_binary_put_uint32(binary, fd_reset_last_cause);
    fd_binary_put_time64(binary, fd_reset_last_time);
}

void fd_control_get_property_storage(fd_binary_t *binary) {
    fd_binary_put_uint32(binary, fd_storage_used_page_count());
}

void fd_control_get_property_debug_lock(fd_binary_t *binary) {
    fd_binary_put_uint8(binary, fd_get_debug_lock());
}

void fd_control_set_property_debug_lock(fd_binary_t *binary __attribute__((unused))) {
    fd_set_debug_lock();
}

void fd_control_get_property_rtc(fd_binary_t *binary) {
    fd_time_t time = fd_rtc_get_time();
    fd_binary_put_time64(binary, time);
}

void fd_control_set_property_rtc(fd_binary_t *binary) {
    fd_time_t time = fd_binary_get_time64(binary);
    fd_rtc_set_time(time);
}

void fd_control_get_property_power(fd_binary_t *binary) {
    fd_power_t power;
    fd_power_get(&power);
    fd_binary_put_float32(binary, power.battery_level);
    fd_binary_put_float32(binary, power.battery_voltage);
    fd_binary_put_uint8(binary, power.is_usb_powered);
    fd_binary_put_uint8(binary, power.is_charging);
    fd_binary_put_float32(binary, power.charge_current);
    fd_binary_put_float32(binary, power.temperature);
}

void fd_control_set_property_power(fd_binary_t *binary) {
    float battery_level = fd_binary_get_float32(binary);
    fd_power_set(battery_level);
}

void fd_control_get_property_mode(fd_binary_t *binary) {
    fd_binary_put_uint8(binary, fd_main_get_mode());
}

void fd_control_set_property_mode(fd_binary_t *binary) {
    uint8_t mode = fd_binary_get_uint8(binary);
    fd_main_set_mode(mode);
}

void fd_control_get_property_tx_power(fd_binary_t *binary) {
    fd_binary_put_uint8(binary, fd_bluetooth_get_tx_power());
}

void fd_control_set_property_tx_power(fd_binary_t *binary) {
    uint8_t level = fd_binary_get_uint8(binary);
    fd_bluetooth_set_tx_power(level);
}

void fd_control_get_property_boot_version(fd_binary_t *binary) {
    fd_boot_data_t boot_data = *FD_BOOT_DATA;
    if (boot_data.magic != FD_BOOT_MAGIC) {
        memset(&boot_data, 0, sizeof(fd_boot_data_t));
        boot_data.minor = 1;
    }

    fd_binary_put_uint16(binary, boot_data.major);
    fd_binary_put_uint16(binary, boot_data.minor);
    fd_binary_put_uint16(binary, boot_data.patch);
    fd_binary_put_uint32(binary, boot_data.capabilities);
    fd_binary_put_bytes(binary, boot_data.git_commit, sizeof(boot_data.git_commit));
}

#define GET_PROPERTY_MASK\
 (FD_CONTROL_PROPERTY_VERSION |\
 FD_CONTROL_PROPERTY_HARDWARE_ID |\
 FD_CONTROL_PROPERTY_DEBUG_LOCK |\
 FD_CONTROL_PROPERTY_RTC |\
 FD_CONTROL_PROPERTY_POWER |\
 FD_CONTROL_PROPERTY_SITE |\
 FD_CONTROL_PROPERTY_RESET |\
 FD_CONTROL_PROPERTY_STORAGE |\
 FD_CONTROL_PROPERTY_MODE |\
 FD_CONTROL_PROPERTY_TX_POWER |\
 FD_CONTROL_PROPERTY_BOOT_VERSION)

void fd_control_get_properties(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);

    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_GET_PROPERTIES);
    fd_binary_put_uint32(binary_out, properties & GET_PROPERTY_MASK);
    for (uint32_t property = 1; property != 0; property <<= 1) {
        if (property & properties) {
            switch (property) {
                case FD_CONTROL_PROPERTY_VERSION: {
                    fd_control_get_property_version(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_HARDWARE_ID: {
                    fd_control_get_property_hardware_id(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_DEBUG_LOCK: {
                    fd_control_get_property_debug_lock(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_RTC: {
                    fd_control_get_property_rtc(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_POWER: {
                    fd_control_get_property_power(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_SITE: {
                    fd_control_get_property_site(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_RESET: {
                    fd_control_get_property_reset(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_STORAGE: {
                    fd_control_get_property_storage(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_MODE: {
                    fd_control_get_property_mode(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_TX_POWER: {
                    fd_control_get_property_tx_power(binary_out);
                } break;
                case FD_CONTROL_PROPERTY_BOOT_VERSION: {
                    fd_control_get_property_boot_version(binary_out);
                } break;
            }
        }
    }
    fd_control_send_complete(detour_source_collection);
}

void fd_control_set_properties(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);
    for (uint32_t property = 1; property != 0; property <<= 1) {
        if (property & properties) {
            switch (property) {
                case FD_CONTROL_PROPERTY_DEBUG_LOCK: {
                    fd_control_set_property_debug_lock(&binary);
                } break;
                case FD_CONTROL_PROPERTY_RTC: {
                    fd_control_set_property_rtc(&binary);
                } break;
                case FD_CONTROL_PROPERTY_POWER: {
                    fd_control_set_property_power(&binary);
                } break;
                case FD_CONTROL_PROPERTY_MODE: {
                    fd_control_set_property_mode(&binary);
                } break;
                case FD_CONTROL_PROPERTY_TX_POWER: {
                    fd_control_set_property_tx_power(&binary);
                } break;
            }
        }
    }
}

void fd_control_update_get_external_hash(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t external_address = fd_binary_get_uint32(&binary);
    uint32_t external_length = fd_binary_get_uint32(&binary);
    uint8_t hash[FD_SHA_HASH_SIZE];
    fd_update_get_external_hash(external_address, external_length, hash);
    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_UPDATE_GET_EXTERNAL_HASH);
    fd_binary_put_bytes(binary_out, hash, FD_SHA_HASH_SIZE);
    fd_control_send_complete(detour_source_collection);
}

void fd_control_update_read_page(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t page = fd_binary_get_uint32(&binary);
    uint8_t page_data[256];
    fd_update_read_page(page, page_data);
    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_UPDATE_READ_PAGE);
    fd_binary_put_bytes(binary_out, page_data, 256);
    fd_control_send_complete(detour_source_collection);
}

void fd_control_update_get_sector_hashes(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t sector_count = fd_binary_get_uint8(&binary);

    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_UPDATE_GET_SECTOR_HASHES);
    fd_binary_put_uint8(binary_out, sector_count);
    for (uint32_t i = 0; i < sector_count; ++i) {
        uint8_t hash[FD_SHA_HASH_SIZE];
        uint32_t sector = fd_binary_get_uint16(&binary);
        fd_update_get_sector_hash(sector, hash);
        fd_binary_put_uint16(binary_out, sector);
        fd_binary_put_bytes(binary_out, hash, FD_SHA_HASH_SIZE);
    }
    fd_control_send_complete(detour_source_collection);
}

void fd_control_update_erase_sectors(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t sector_count = fd_binary_get_uint8(&binary);
    for (uint32_t i = 0; i < sector_count; ++i) {
        uint32_t sector = fd_binary_get_uint16(&binary);
        fd_update_erase_sector(sector);
    }
}

void fd_control_update_write_page(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t page = fd_binary_get_uint16(&binary);
    uint8_t *page_data = &binary.buffer[binary.get_index];
    fd_update_write_page(page, page_data);
}

void fd_control_update_commit(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_update_metadata_t metadata;
    metadata.flags = fd_binary_get_uint32(&binary);
    metadata.length = fd_binary_get_uint32(&binary);
    fd_binary_get_bytes(&binary, metadata.hash, FD_SHA_HASH_SIZE);
    fd_binary_get_bytes(&binary, metadata.crypt_hash, FD_SHA_HASH_SIZE);
    fd_binary_get_bytes(&binary, metadata.crypt_iv, 16);

    uint8_t result = fd_update_commit(&metadata);

    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_UPDATE_COMMIT);
    fd_binary_put_uint8(binary_out, result);
    fd_control_send_complete(detour_source_collection);
}

void fd_control_radio_direct_test_mode_enter(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint16_t request = fd_binary_get_uint16(&binary);
    fd_time_t duration = fd_binary_get_time64(&binary);
    fd_bluetooth_direct_test_mode_enter(request, duration);
}

void fd_control_radio_direct_test_mode_exit(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
    fd_bluetooth_direct_test_mode_exit();
}

void fd_control_radio_direct_test_mode_report(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_RADIO_DIRECT_TEST_MODE_REPORT);
    fd_binary_put_uint16(binary_out, fd_bluetooth_direct_test_mode_report());
    fd_control_send_complete(detour_source_collection);
}

void fd_control_disconnect(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t flags __attribute__((unused)) = fd_binary_get_uint8(&binary);
    // fd_bluetooth_?
    // fd_usb_?
}

static
void get_rgb(fd_binary_t *binary, fd_indicator_rgb_t *rgb) {
    rgb->r = fd_binary_get_uint8(binary);
    rgb->g = fd_binary_get_uint8(binary);
    rgb->b = fd_binary_get_uint8(binary);
}

void fd_control_indicator_override(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_indicator_state_t state;
    state.usb.o = fd_binary_get_uint8(&binary);
    state.usb.g = fd_binary_get_uint8(&binary);
    state.d0.r = fd_binary_get_uint8(&binary);
    get_rgb(&binary, &state.d1);
    get_rgb(&binary, &state.d2);
    get_rgb(&binary, &state.d3);
    state.d4.r = fd_binary_get_uint8(&binary);
    fd_time_t duration = fd_binary_get_time64(&binary);

    fd_indicator_override(&state, duration);
}

void fd_control_lock(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_lock_identifier_t identifier = fd_binary_get_uint8(&binary);
    fd_lock_operation_t operation = fd_binary_get_uint8(&binary);
    fd_lock_owner_t owner = detour_source_collection->owner;
    fd_lock_owner_t lock_owner = fd_lock(identifier, operation, owner);

    fd_binary_t *binary_out = fd_control_send_start(detour_source_collection, FD_CONTROL_LOCK);
    fd_binary_put_uint8(binary_out, identifier);
    fd_binary_put_uint8(binary_out, operation);
    fd_binary_put_uint32(binary_out, lock_owner);
    fd_control_send_complete(detour_source_collection);
}

// !!! should we encrypt/decrypt everything? or just syncs? or just things that modify? -denis

void fd_control_process_command(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    if (length < 1) {
        return;
    }
    uint8_t code = data[0];
    fd_control_command_t command = 0;
    switch (code) {
        case FD_CONTROL_PING:
            command = fd_control_ping;
            break;

        case FD_CONTROL_GET_PROPERTIES:
            command = fd_control_get_properties;
            break;
        case FD_CONTROL_SET_PROPERTIES:
            command = fd_control_set_properties;
            break;

        case FD_CONTROL_PROVISION:
            command = fd_control_provision;
            break;
        case FD_CONTROL_RESET:
            command = fd_control_reset;
            break;

        case FD_CONTROL_UPDATE_GET_EXTERNAL_HASH:
            command = fd_control_update_get_external_hash;
            break;
        case FD_CONTROL_UPDATE_READ_PAGE:
            command = fd_control_update_read_page;
            break;

        case FD_CONTROL_UPDATE_GET_SECTOR_HASHES:
            command = fd_control_update_get_sector_hashes;
            break;
        case FD_CONTROL_UPDATE_ERASE_SECTORS:
            command = fd_control_update_erase_sectors;
            break;
        case FD_CONTROL_UPDATE_WRITE_PAGE:
            command = fd_control_update_write_page;
            break;
        case FD_CONTROL_UPDATE_COMMIT:
            command = fd_control_update_commit;
            break;

        case FD_CONTROL_RADIO_DIRECT_TEST_MODE_ENTER:
            command = fd_control_radio_direct_test_mode_enter;
            break;
        case FD_CONTROL_RADIO_DIRECT_TEST_MODE_EXIT:
            command = fd_control_radio_direct_test_mode_exit;
            break;
        case FD_CONTROL_RADIO_DIRECT_TEST_MODE_REPORT:
            command = fd_control_radio_direct_test_mode_report;
            break;

        case FD_CONTROL_DISCONNECT:
            command = fd_control_disconnect;
            break;

        case FD_CONTROL_INDICATOR_OVERRIDE:
            command = fd_control_indicator_override;
            break;

        case FD_CONTROL_SYNC_START:
            command = fd_sync_start;
            break;
        case FD_CONTROL_SYNC_ACK:
            command = fd_sync_ack;
            break;

        case FD_CONTROL_LOCK:
            command = fd_control_lock;
            break;
    }
    if (command) {
        (*command)(detour_source_collection, &data[1], length - 1);
    }
}

void fd_control_command(void) {
    if (fd_control_inputs_count == 0) {
        return;
    }

    fd_interrupts_disable();

    // get the command info
    fd_control_input_t *input = &fd_control_inputs[0];
    fd_detour_source_collection_t *detour_source_collection = input->detour_source_collection;
    memcpy(fd_control_command_buffer, input->data, input->length);
    uint32_t length = input->length;

    // remove it from the inputs
    --fd_control_inputs_count;
    memcpy(fd_control_inputs, &fd_control_inputs[1], sizeof(fd_control_input_t) * fd_control_inputs_count);
    fd_control_input_buffer_count -= length;
    memcpy(fd_control_input_buffer, &fd_control_input_buffer[length], fd_control_input_buffer_count);

    fd_interrupts_enable();

    // process it
    fd_control_process_command(detour_source_collection, fd_control_command_buffer, length);

    if (fd_control_inputs_count > 0) {
        fd_event_set_exclusive(FD_EVENT_COMMAND);
    }
}