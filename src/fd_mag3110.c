#include "fd_i2c1.h"
#include "fd_log.h"
#include "fd_mag3110.h"

#define ADDRESS 0x1c

#define DR_STATUS 0x00
#define OUT_X_MSB 0x01
#define OUT_X_LSB 0x02
#define OUT_Y_MSB 0x03
#define OUT_Y_LSB 0x04
#define OUT_Z_MSB 0x05
#define OUT_Z_LSB 0x06
#define WHO_AM_I 0x07
#define SYSMOD 0x08
#define DIE_TEMP 0x0f
#define CTRL_REG1 0x10
#define CTRL_REG2 0x11

#define CTRL_REG1_DR_OS_80_16 0x00
#define CTRL_REG1_DR_OS_40_32 0x08
#define CTRL_REG1_DR_OS_20_64 0x10
#define CTRL_REG1_DR_OS_10_128 0x18
#define CTRL_REG1_FR 0x04
#define CTRL_REG1_TM 0x02
#define CTRL_REG1_AC 0x01

#define CTRL_REG2_AUTO_MRST_EN 0x80
#define CTRL_REG2_RAW 0x20
#define CTRL_REG2_MAG_RST 0x10

void fd_mag3110_initialize(void) {
    uint8_t who_am_i;
    bool result = fd_i2c1_register_read(ADDRESS, WHO_AM_I, &who_am_i);
    if (!result) {
        return;
    }
    if (who_am_i != 0xc4) {
        fd_log_assert_fail("");
        return;
    }
    result = fd_i2c1_register_write(ADDRESS, CTRL_REG2, CTRL_REG2_AUTO_MRST_EN);
    if (!result) {
         fd_log_assert_fail("");
         return;
    }
    result = fd_i2c1_register_write(ADDRESS, CTRL_REG1, CTRL_REG1_DR_OS_80_16 | CTRL_REG1_AC);
    if (!result) {
         fd_log_assert_fail("");
         return;
    }
    uint8_t ctrl_reg1 = 0xff;
    result = fd_i2c1_register_read(ADDRESS, CTRL_REG1, &ctrl_reg1);
}

#define SCALE (0.001 / 30000.0f)

void fd_mag3110_read(float *x, float *y, float *z) {
    uint8_t bytes[18];
    bool result = fd_i2c1_register_read_bytes(ADDRESS, DR_STATUS, bytes, sizeof(bytes));
    if (!result) {
        return;
    }

    uint16_t out_x = (bytes[1] << 8) | (bytes[2]);
    uint16_t out_y = (bytes[3] << 8) | (bytes[4]);
    uint16_t out_z = (bytes[5] << 8) | (bytes[6]);
    *x = out_x * SCALE;
    *y = out_y * SCALE;
    *z = out_z * SCALE;
}