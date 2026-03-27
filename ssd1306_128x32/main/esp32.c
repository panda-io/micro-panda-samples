#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct GfxDriver GfxDriver;
typedef struct DisplayConn DisplayConn;
typedef struct DisplayInfo DisplayInfo;

typedef void (*__Fn_void_int32_t_int32_t_int32_t)(int32_t, int32_t, int32_t);
typedef void (*__Fn_void_int32_t_int32_t_int32_t_int32_t_int32_t)(int32_t, int32_t, int32_t, int32_t, int32_t);
typedef void (*__Fn_void)(void);
typedef void (*__Fn_void_void)(void);

typedef struct { uint8_t* ptr; size_t size; } __Slice_uint8_t;

#define PWM_MAX_CHANNELS 8
#define I2C_MAX_BUSES 2
#define I2C_MAX_DEVICES 8
#define I2C_TIMEOUT_MS 1000
#define SPI_MAX_BUSES 2
#define SPI_MAX_DEVICES 6
#define SPI_DMA 1
#define I2C_BUS 0
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 400000
#define OLED_ADDR 60
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t __mp_i2c_buses[I2C_MAX_BUSES];
static i2c_master_dev_handle_t __mp_i2c_devs[I2C_MAX_DEVICES];
static uint32_t                __mp_i2c_freq[I2C_MAX_BUSES];

// Initialize I2C bus. Returns bus index or -1 on error.
static inline int32_t __mp_i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
    if (bus < 0 || bus >= I2C_MAX_BUSES) return -1;
    if (__mp_i2c_buses[bus] != NULL) return bus; // already initialized
    i2c_master_bus_config_t cfg = {
        .i2c_port    = (i2c_port_num_t)bus,
        .sda_io_num  = (gpio_num_t)sda,
        .scl_io_num  = (gpio_num_t)scl,
        .clk_source  = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &__mp_i2c_buses[bus]);
    if (err != ESP_OK) return -1;
    __mp_i2c_freq[bus] = (uint32_t)freq_hz;
    return bus;
}

// Open a device on the bus by 7-bit address. Returns device slot or -1 on error.
static inline int32_t __mp_i2c_open(int32_t bus, int32_t addr) {
    if (bus < 0 || bus >= I2C_MAX_BUSES || __mp_i2c_buses[bus] == NULL) return -1;
    for (int i = 0; i < I2C_MAX_DEVICES; i++) {
        if (__mp_i2c_devs[i] == NULL) {
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address  = (uint16_t)addr,
                .scl_speed_hz    = __mp_i2c_freq[bus],
            };
            esp_err_t err = i2c_master_bus_add_device(__mp_i2c_buses[bus], &dev_cfg, &__mp_i2c_devs[i]);
            return err == ESP_OK ? i : -1;
        }
    }
    return -1; // no free slot
}

static inline void __mp_i2c_close(int32_t dev) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return;
    i2c_master_bus_rm_device(__mp_i2c_devs[dev]);
    __mp_i2c_devs[dev] = NULL;
}

static inline int32_t __mp_i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit(__mp_i2c_devs[dev], data.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_receive(__mp_i2c_devs[dev], buf.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_write_read(int32_t dev,
                                           __Slice_uint8_t tx, int32_t tx_len,
                                           __Slice_uint8_t rx, int32_t rx_len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit_receive(
        __mp_i2c_devs[dev],
        tx.ptr, (size_t)tx_len,
        rx.ptr, (size_t)rx_len,
        I2C_TIMEOUT_MS);
}
#include <stdint.h>
#include <stddef.h>

// CP437 8x8 font — row-major, bit7 = leftmost pixel, 256 glyphs × 8 bytes.
static const uint8_t __mp_gfx_font[256][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 00 NUL
    {0x7E,0x81,0xA5,0x81,0xBD,0x99,0x81,0x7E}, // 01 ☺
    {0x7E,0xFF,0xDB,0xFF,0xC3,0xE7,0xFF,0x7E}, // 02 ☻
    {0x6C,0xFE,0xFE,0xFE,0x7C,0x38,0x10,0x00}, // 03 ♥
    {0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10,0x00}, // 04 ♦
    {0x38,0x7C,0x38,0xFE,0xFE,0x10,0x28,0x00}, // 05 ♣
    {0x10,0x10,0x38,0x7C,0xFE,0x7C,0x28,0xEE}, // 06 ♠
    {0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x00}, // 07 •
    {0xFF,0xFF,0xE7,0xC3,0xC3,0xE7,0xFF,0xFF}, // 08 ◘
    {0x00,0x3C,0x66,0x42,0x42,0x66,0x3C,0x00}, // 09 ○
    {0xFF,0xC3,0x99,0xBD,0xBD,0x99,0xC3,0xFF}, // 0A ◙
    {0x0F,0x07,0x0F,0x7D,0xCC,0xCC,0xCC,0x78}, // 0B ♂
    {0x3C,0x66,0x66,0x66,0x3C,0x18,0x7E,0x18}, // 0C ♀
    {0x3F,0x33,0x3F,0x30,0x30,0x70,0xF0,0xE0}, // 0D ♪
    {0x7F,0x63,0x7F,0x63,0x63,0x67,0xE6,0xC0}, // 0E ♫
    {0xFF,0x5A,0x3C,0xE7,0xE7,0x3C,0x5A,0xFF}, // 0F ☼
    {0x80,0xC0,0xE0,0xF0,0xE0,0xC0,0x80,0x00}, // 10 ►
    {0x02,0x06,0x0E,0x1E,0x0E,0x06,0x02,0x00}, // 11 ◄
    {0x18,0x3C,0x7E,0x18,0x18,0x7E,0x3C,0x18}, // 12 ↕
    {0x66,0x66,0x66,0x66,0x66,0x00,0x66,0x00}, // 13 ‼
    {0x7F,0xDB,0xDB,0x7B,0x1B,0x1B,0x1B,0x00}, // 14 ¶
    {0x3E,0x63,0x38,0x6E,0x63,0x1C,0x6C,0x3E}, // 15 §
    {0x00,0x00,0x00,0x7E,0x7E,0x00,0x00,0x00}, // 16 ▬
    {0x18,0x3C,0x7E,0x18,0x7E,0x3C,0x18,0xFF}, // 17 ↨
    {0x10,0x38,0x7C,0xFE,0x38,0x38,0x38,0x00}, // 18 ↑
    {0x38,0x38,0x38,0xFE,0x7C,0x38,0x10,0x00}, // 19 ↓
    {0x00,0x18,0x0C,0xFE,0x0C,0x18,0x00,0x00}, // 1A →
    {0x00,0x30,0x60,0xFE,0x60,0x30,0x00,0x00}, // 1B ←
    {0x00,0x00,0x60,0x60,0x60,0x7E,0x00,0x00}, // 1C └ (ctrl)
    {0x00,0x18,0x3C,0x7E,0x18,0x18,0x18,0x00}, // 1D ↔
    {0x3C,0x60,0xC0,0xFE,0xC0,0x60,0x3C,0x00}, // 1E ▲
    {0x3C,0x06,0x03,0x7F,0x03,0x06,0x3C,0x00}, // 1F ▼
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 20 ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 21 !
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // 22 "
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // 23 #
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 24 $
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, // 25 %
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // 26 &
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 27 '
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 28 (
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 29 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 2A *
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 2B +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 2C ,
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 2D -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 2E .
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // 2F /
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 30 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 31 1
    {0x7C,0xC6,0x06,0x1C,0x38,0x60,0xFE,0x00}, // 32 2
    {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, // 33 3
    {0x0E,0x1E,0x36,0x66,0xFE,0x06,0x06,0x00}, // 34 4
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, // 35 5
    {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, // 36 6
    {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, // 37 7
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, // 38 8
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, // 39 9
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 3A :
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 3B ;
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // 3C <
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // 3D =
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, // 3E >
    {0x3C,0x66,0x06,0x1C,0x18,0x00,0x18,0x00}, // 3F ?
    {0x7C,0xC6,0xDE,0xD6,0xDE,0xC0,0x7C,0x00}, // 40 @
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 41 A
    {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, // 42 B
    {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, // 43 C
    {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, // 44 D
    {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, // 45 E
    {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, // 46 F
    {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00}, // 47 G
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // 48 H
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 49 I
    {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, // 4A J
    {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00}, // 4B K
    {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, // 4C L
    {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00}, // 4D M
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, // 4E N
    {0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 4F O
    {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, // 50 P
    {0x78,0xCC,0xCC,0xCC,0xDC,0x78,0x1C,0x00}, // 51 Q
    {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, // 52 R
    {0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00}, // 53 S
    {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00}, // 54 T
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // 55 U
    {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 56 V
    {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00}, // 57 W
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, // 58 X
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00}, // 59 Y
    {0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00}, // 5A Z
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // 5B [
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // 5C '\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // 5D ]
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // 5E ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 5F _
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 60 `
    {0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // 61 a
    {0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00}, // 62 b
    {0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00}, // 63 c
    {0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00}, // 64 d
    {0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00}, // 65 e
    {0x38,0x6C,0x64,0xF0,0x60,0x60,0xF0,0x00}, // 66 f
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8}, // 67 g
    {0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00}, // 68 h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 69 i
    {0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C}, // 6A j
    {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00}, // 6B k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 6C l
    {0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00}, // 6D m
    {0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00}, // 6E n
    {0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00}, // 6F o
    {0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0}, // 70 p
    {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E}, // 71 q
    {0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00}, // 72 r
    {0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00}, // 73 s
    {0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00}, // 74 t
    {0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, // 75 u
    {0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // 76 v
    {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00}, // 77 w
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, // 78 x
    {0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8}, // 79 y
    {0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00}, // 7A z
    {0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00}, // 7B {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 7C |
    {0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00}, // 7D }
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, // 7E ~
    {0x10,0x38,0x6C,0xC6,0xC6,0xFE,0x00,0x00}, // 7F DEL
    // 0x80..0x9F  extended Latin
    {0xF8,0xCC,0xCC,0xF8,0xC4,0xCC,0xCE,0x00}, // 80 Ç
    {0x00,0xCC,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8}, // 81 ü
    {0x06,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00}, // 82 é
    {0x7C,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // 83 â
    {0x00,0xCC,0x00,0x78,0x0C,0x7C,0xCC,0x76}, // 84 ä
    {0xC0,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // 85 à
    {0x38,0x6C,0x00,0x78,0x0C,0x7C,0xCC,0x76}, // 86 å
    {0x00,0x00,0x7C,0xC0,0xC0,0x7C,0x0C,0x38}, // 87 ç
    {0x7C,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00}, // 88 ê
    {0x00,0xCC,0x00,0x78,0xCC,0xFC,0xC0,0x78}, // 89 ë
    {0xC0,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00}, // 8A è
    {0x00,0xCC,0x00,0x38,0x18,0x18,0x3C,0x00}, // 8B ï
    {0x7C,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 8C î
    {0xC0,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 8D ì
    {0x00,0xC6,0x38,0x6C,0xC6,0xFE,0xC6,0x00}, // 8E Ä
    {0x38,0x6C,0x38,0x6C,0xC6,0xFE,0xC6,0x00}, // 8F Å
    {0x1E,0x00,0xFE,0x62,0x78,0x62,0xFE,0x00}, // 90 É
    {0x00,0x00,0x6E,0x3B,0x1F,0x3B,0x6E,0x00}, // 91 æ
    {0x00,0x00,0x7E,0xD8,0xFE,0xD8,0x7E,0x00}, // 92 Æ
    {0x7C,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00}, // 93 ô
    {0x00,0xCC,0x00,0x78,0xCC,0xCC,0x78,0x00}, // 94 ö
    {0xC0,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00}, // 95 ò
    {0x7C,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, // 96 û
    {0xC0,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, // 97 ù
    {0x00,0xCC,0x00,0xCC,0xCC,0x7C,0x0C,0xF8}, // 98 ÿ
    {0x00,0xC6,0x00,0x7C,0xC6,0xC6,0x7C,0x00}, // 99 Ö
    {0x00,0xC6,0x00,0xC6,0xC6,0xC6,0x7E,0x00}, // 9A Ü
    {0x18,0x3C,0x6E,0x68,0x6E,0x3C,0x18,0x00}, // 9B ¢
    {0x38,0x6C,0x64,0xF0,0x60,0xE6,0xFC,0x00}, // 9C £
    {0xC6,0xC6,0x7C,0x38,0x7C,0x38,0x10,0xFE}, // 9D ¥
    {0x00,0x00,0xDC,0x76,0x76,0xDC,0x00,0x00}, // 9E ₧
    {0x3C,0x66,0x3C,0x00,0x7E,0x00,0x00,0x00}, // 9F ƒ
    // 0xA0..0xAF
    {0xC0,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // A0 á
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // A1 í
    {0x00,0x06,0x00,0x78,0xCC,0xCC,0x78,0x00}, // A2 ó
    {0x00,0x06,0x00,0xCC,0xCC,0xCC,0x76,0x00}, // A3 ú
    {0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x00}, // A4 ñ (tilde only, approx)
    {0x00,0xC6,0x00,0xC6,0xE6,0xF6,0xDE,0xC6}, // A5 Ñ
    {0x00,0x00,0x00,0xFC,0x60,0x60,0x00,0x00}, // A6 ª
    {0x00,0x00,0x00,0xFC,0x0C,0x0C,0x00,0x00}, // A7 º
    {0x3C,0x66,0x3C,0x00,0x00,0x00,0x00,0x00}, // A8 ¿ (approx)
    {0x10,0x38,0x6C,0xC6,0xFE,0x00,0x00,0x00}, // A9 ⌐
    {0x7C,0xC6,0xFE,0xC0,0xC0,0xC6,0x7C,0x00}, // AA ¬
    {0x00,0x00,0x18,0x00,0x7E,0x00,0x18,0x00}, // AB ½
    {0x00,0x00,0x00,0x7E,0x06,0x0C,0x18,0x00}, // AC ¼
    {0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x00}, // AD ¡
    {0x00,0x06,0x0C,0x18,0x30,0x60,0x00,0x00}, // AE «
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x00,0x00}, // AF »
    // 0xB0..0xBF  shading + box
    {0xAA,0x00,0x55,0x00,0xAA,0x00,0x55,0x00}, // B0 ░
    {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55}, // B1 ▒
    {0xFF,0xAA,0xFF,0x55,0xFF,0xAA,0xFF,0x55}, // B2 ▓
    {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08}, // B3 │
    {0x08,0x08,0x08,0xF8,0x08,0x08,0x08,0x08}, // B4 ┤
    {0x08,0x08,0xF8,0x08,0xF8,0x08,0x08,0x08}, // B5 ╡
    {0x14,0x14,0x14,0xF4,0x14,0x14,0x14,0x14}, // B6 ╢
    {0x00,0x00,0x00,0xFC,0x14,0x14,0x14,0x14}, // B7 ╖
    {0x00,0x00,0xF8,0x08,0xF8,0x14,0x14,0x14}, // B8 ╕
    {0x14,0x14,0xF4,0x04,0xF4,0x14,0x14,0x14}, // B9 ╣
    {0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14}, // BA ║
    {0x00,0x00,0xFC,0x04,0xF4,0x14,0x14,0x14}, // BB ╗
    {0x14,0x14,0xF4,0x04,0xFC,0x00,0x00,0x00}, // BC ╝
    {0x14,0x14,0x14,0xFC,0x00,0x00,0x00,0x00}, // BD ╜
    {0x00,0x00,0xFC,0x14,0xFC,0x00,0x00,0x00}, // BE ╛
    {0x00,0x00,0x00,0xF8,0x08,0x08,0x08,0x08}, // BF ┐
    // 0xC0..0xCF  box continued
    {0x08,0x08,0x08,0x0F,0x00,0x00,0x00,0x00}, // C0 └
    {0x08,0x08,0x08,0xFF,0x00,0x00,0x00,0x00}, // C1 ┴
    {0x00,0x00,0x00,0xFF,0x08,0x08,0x08,0x08}, // C2 ┬
    {0x08,0x08,0x08,0x0F,0x08,0x08,0x08,0x08}, // C3 ├
    {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00}, // C4 ─
    {0x08,0x08,0x08,0xFF,0x08,0x08,0x08,0x08}, // C5 ┼
    {0x08,0x08,0x0F,0x08,0x0F,0x14,0x14,0x14}, // C6 ╞
    {0x14,0x14,0x14,0x17,0x14,0x14,0x14,0x14}, // C7 ╟
    {0x14,0x14,0xF7,0x00,0xFF,0x00,0x00,0x00}, // C8 ╚
    {0x00,0x00,0xFF,0x00,0xF7,0x14,0x14,0x14}, // C9 ╔
    {0x14,0x14,0x17,0x10,0x17,0x14,0x14,0x14}, // CA ╩
    {0x00,0x00,0xF7,0x10,0xF7,0x14,0x14,0x14}, // CB ╦
    {0x14,0x14,0x14,0xF4,0x14,0x14,0x14,0x14}, // CC ╠
    {0x00,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00}, // CD ═
    {0x14,0x14,0xF4,0x04,0xF4,0x14,0x14,0x14}, // CE ╬
    {0x00,0x00,0xFF,0x00,0xFF,0x08,0x08,0x08}, // CF ╧
    // 0xD0..0xDF
    {0x08,0x08,0xFF,0x00,0xFF,0x00,0x00,0x00}, // D0 ╨
    {0x00,0x00,0xFF,0x14,0xFF,0x00,0x00,0x00}, // D1 ╤
    {0x00,0x00,0xFF,0x14,0x14,0x14,0x14,0x14}, // D2 ╥
    {0x14,0x14,0x14,0xFE,0x00,0x00,0x00,0x00}, // D3 ╙
    {0x14,0x14,0xF7,0x00,0xF7,0x00,0x00,0x00}, // D4 ╘
    {0x00,0x00,0xF7,0x14,0xF7,0x00,0x00,0x00}, // D5 ╒
    {0x00,0x00,0x14,0xFE,0x14,0x00,0x00,0x00}, // D6 ╓
    {0x08,0x08,0xFF,0x08,0xFF,0x14,0x14,0x14}, // D7 ╫
    {0x14,0x14,0x14,0xFF,0x14,0x14,0x14,0x14}, // D8 ╪
    {0x08,0x08,0x08,0xF8,0x00,0x00,0x00,0x00}, // D9 ┘
    {0x00,0x00,0x00,0x0F,0x08,0x08,0x08,0x08}, // DA ┌
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, // DB █
    {0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF}, // DC ▄
    {0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0}, // DD ▌
    {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F}, // DE ▐
    {0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00}, // DF ▀
    // 0xE0..0xFF  Greek / math
    {0x00,0x00,0x6E,0x91,0x91,0x91,0x6E,0x00}, // E0 α
    {0xFE,0x66,0x66,0x7E,0x66,0x66,0xFE,0x30}, // E1 ß
    {0x00,0x00,0xFF,0xCC,0xCC,0xCC,0x33,0x00}, // E2 Γ
    {0x00,0x00,0xFF,0xCC,0xCC,0xCC,0xCC,0x00}, // E3 π
    {0x7F,0x04,0x7F,0x04,0x04,0x04,0x04,0x00}, // E4 Σ
    {0x00,0x00,0x7F,0xDB,0xDB,0x7F,0x00,0x00}, // E5 σ
    {0x00,0x00,0xDB,0xDB,0xDB,0x7F,0x00,0x00}, // E6 µ
    {0x00,0x00,0x10,0x7C,0x10,0x7C,0x10,0x00}, // E7 τ
    {0x00,0x00,0x7C,0x42,0x7C,0x40,0x40,0x00}, // E8 Φ
    {0x00,0x66,0x3C,0x18,0x3C,0x66,0x00,0x00}, // E9 Θ
    {0x00,0x00,0xC6,0xAA,0x92,0xAA,0xC6,0x00}, // EA Ω
    {0x00,0x00,0x66,0x66,0x3C,0x18,0x18,0x00}, // EB δ
    {0x00,0x78,0x18,0x18,0x3C,0x18,0x18,0x78}, // EC ∞
    {0x00,0x00,0x7C,0xC6,0xC6,0x7C,0x44,0x6C}, // ED φ
    {0x00,0x66,0x3C,0x18,0x18,0x18,0x18,0x00}, // EE ε
    {0x00,0x00,0xFF,0x08,0x04,0x08,0xFF,0x00}, // EF ∩
    {0x00,0x00,0xFF,0x80,0x80,0x80,0xFF,0x00}, // F0 ≡
    {0x00,0x10,0xFE,0x11,0xFF,0x10,0xFE,0x00}, // F1 ±
    {0xFE,0xC0,0xC0,0xC0,0xFE,0x00,0x00,0x00}, // F2 ≥
    {0xFE,0x06,0x06,0x06,0xFE,0x00,0x00,0x00}, // F3 ≤
    {0x18,0x18,0x18,0x18,0xF8,0x18,0x18,0x00}, // F4 ⌠
    {0x18,0x18,0x18,0x18,0x1F,0x18,0x18,0x00}, // F5 ⌡
    {0x00,0x18,0x00,0xFF,0x00,0x18,0x00,0x00}, // F6 ÷
    {0x00,0x00,0x76,0xDC,0x00,0x76,0xDC,0x00}, // F7 ≈
    {0x38,0x6C,0x6C,0x38,0x00,0x00,0x00,0x00}, // F8 °
    {0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00}, // F9 ∙
    {0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00}, // FA ·
    {0x18,0x38,0x18,0x18,0x3C,0x00,0x00,0x00}, // FB √
    {0x3C,0x66,0x0C,0x18,0x00,0x7E,0x00,0x00}, // FC ⁿ
    {0x3C,0x66,0x0C,0x18,0x30,0x66,0x3C,0x00}, // FD ²
    {0x00,0xFF,0xDB,0xDB,0xDB,0xFF,0x00,0x00}, // FE ■
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // FF NBSP
};

static inline uint8_t __mp_gfx_font_row(uint8_t c, int row) {
    return __mp_gfx_font[c][row];
}

static inline int32_t __mp_gfx_abs(int32_t v) { return v < 0 ? -v : v; }
static inline int32_t __mp_gfx_sign(int32_t v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

static inline int32_t __mp_gfx_isqrt(int32_t n) {
    if (n <= 0) return 0;
    int32_t x = n, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static inline void    __mp_delay_ms(int32_t ms)     { vTaskDelay(pdMS_TO_TICKS(ms)); }
static inline int32_t __mp_time_ms(void)             { return (int32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS); }
static inline int32_t __mp_task_core_id(void)            { return (int32_t)xPortGetCoreID(); }

static inline void*   __mp_task_create(void (*fn)(void), int32_t stack, int32_t priority) {
    TaskHandle_t h = NULL;
    xTaskCreate((TaskFunction_t)(void*)fn, "", (uint32_t)stack, NULL, (UBaseType_t)priority, &h);
    return (void*)h;
}
static inline void*   __mp_task_create_pinned(void (*fn)(void), int32_t stack, int32_t priority, int32_t core) {
    TaskHandle_t h = NULL;
    xTaskCreatePinnedToCore((TaskFunction_t)(void*)fn, "", (uint32_t)stack, NULL, (UBaseType_t)priority, &h, (BaseType_t)core);
    return (void*)h;
}
static inline void    __mp_task_exit(void)           { vTaskDelete(NULL); }
static inline void    __mp_task_notify(void* handle) { xTaskNotify((TaskHandle_t)handle, 0, eNoAction); }
static inline int32_t __mp_task_wait(void)           { return (int32_t)ulTaskNotifyTake(pdTRUE, portMAX_DELAY); }
#include <string.h>

// 128×32 mono framebuffer: byte[0] = 0x40 (I2C data control), byte[1..512] = pixels
// Page p, column c -> buf[p*128 + c + 1]; bit (y & 7) within byte, LSB = top row.
static uint8_t __mp_ssd1306_32_fb[513];

static inline void __mp_ssd1306_32_clear(void) {
    memset(__mp_ssd1306_32_fb + 1, 0, 512);
}

static inline void __mp_ssd1306_32_set_pixel(int32_t x, int32_t y, int32_t on) {
    if ((uint32_t)x >= 128u || (uint32_t)y >= 32u) return;
    uint8_t *p = __mp_ssd1306_32_fb + 1 + (y >> 3) * 128 + x;
    uint8_t  m = (uint8_t)(1u << (y & 7));
    if (on) *p |= m; else *p &= ~m;
}

static inline void __mp_ssd1306_32_fill_rect(int32_t x, int32_t y,
                                              int32_t w, int32_t h, int32_t on) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            __mp_ssd1306_32_set_pixel(col, row, on);
}

static inline __Slice_uint8_t __mp_ssd1306_32_fb_slice(void) {
    return (__Slice_uint8_t){ __mp_ssd1306_32_fb, 513 };
}
#include "driver/gpio.h"

typedef enum {
  GfxRotation_R0 = 0,
  GfxRotation_R90 = 1,
  GfxRotation_R180 = 2,
  GfxRotation_R270 = 3,
} GfxRotation;

typedef enum {
  DisplayInterface_I2C = 0,
  DisplayInterface_SPI = 1,
} DisplayInterface;

typedef enum {
  DisplayColor_MONO = 0,
  DisplayColor_RGB565 = 1,
} DisplayColor;

typedef enum {
  GpioMode_INPUT = 1,
  GpioMode_OUTPUT = 3,
  GpioMode_INPUT_PULLUP = 5,
  GpioMode_INPUT_PULLDOWN = 6,
} GpioMode;

typedef enum {
  GpioLevel_LOW = 0,
  GpioLevel_HIGH = 1,
} GpioLevel;

struct GfxDriver {
  int32_t width;
  int32_t height;
  __Fn_void_int32_t_int32_t_int32_t set_pixel;
  __Fn_void_int32_t_int32_t_int32_t_int32_t_int32_t fill_rect;
  __Fn_void flush;
};

struct DisplayConn {
  DisplayInterface interface;
  int32_t device;
  int32_t dc_pin;
  int32_t reset_pin;
  int32_t back_light_pin;
};

struct DisplayInfo {
  int32_t width;
  int32_t height;
  DisplayColor color;
};

void main__main(void);
int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz);
int32_t i2c__i2c_open(int32_t bus, int32_t addr);
void i2c__i2c_close(int32_t dev);
int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len);
int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len);
int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len);
static void gfx___flush_noop(void);
GfxDriver* gfx__gfx_driver(void);
static void gfx___pixel_r0(int32_t x, int32_t y, int32_t color);
static void gfx___pixel_r90(int32_t x, int32_t y, int32_t color);
static void gfx___pixel_r180(int32_t x, int32_t y, int32_t color);
static void gfx___pixel_r270(int32_t x, int32_t y, int32_t color);
static void gfx___fill_r0(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
static void gfx___fill_r90(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
static void gfx___fill_r180(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
static void gfx___fill_r270(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void gfx__gfx_set_rotation(GfxRotation r);
void gfx__gfx_init(void);
int32_t gfx__gfx_width(void);
int32_t gfx__gfx_height(void);
void gfx__gfx_pixel(int32_t x, int32_t y, int32_t color);
void gfx__gfx_hline(int32_t x, int32_t y, int32_t w, int32_t color);
void gfx__gfx_vline(int32_t x, int32_t y, int32_t h, int32_t color);
void gfx__gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color);
void gfx__gfx_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void gfx__gfx_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void gfx__gfx_clear(int32_t color);
static void gfx___circle_8(int32_t cx, int32_t cy, int32_t x, int32_t y, int32_t color);
void gfx__gfx_circle(int32_t cx, int32_t cy, int32_t r, int32_t color);
void gfx__gfx_fill_circle(int32_t cx, int32_t cy, int32_t r, int32_t color);
static void gfx___line_raw(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color);
void gfx__gfx_triangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void gfx__gfx_fill_triangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);
void gfx__gfx_round_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color);
void gfx__gfx_fill_round_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color);
static void gfx___char_raw(int32_t x, int32_t y, uint8_t c, int32_t color, int32_t bg, int32_t scale);
void gfx__gfx_char(int32_t x, int32_t y, uint8_t c, int32_t color, int32_t bg, int32_t scale);
int32_t gfx__gfx_str(int32_t x, int32_t y, __Slice_uint8_t s, int32_t color, int32_t bg, int32_t scale);
static inline int32_t math__floor_fixed(int32_t v);
static inline int32_t math__ceil_fixed(int32_t v);
static inline int32_t math__round_fixed(int32_t v);
DisplayConn* display__display_conn(void);
DisplayInfo* display__display_info(void);
static int32_t drivers__ssd1306_128x32___cmd1(int32_t dev, int32_t c);
static int32_t drivers__ssd1306_128x32___cmd2(int32_t dev, int32_t c, int32_t v);
static void drivers__ssd1306_128x32___hw_init(int32_t dev);
static void drivers__ssd1306_128x32___flush(int32_t dev);
static void drivers__ssd1306_128x32___gfx_set_pixel(int32_t x, int32_t y, int32_t color);
static void drivers__ssd1306_128x32___gfx_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
static void drivers__ssd1306_128x32___gfx_flush(void);
void drivers__ssd1306_128x32__display_attach_gfx(GfxDriver* driver);
void gpio__gpio_mode(int32_t pin, GpioMode mode);
static inline void gpio__gpio_write(int32_t pin, GpioLevel value);
static inline GpioLevel gpio__gpio_read(int32_t pin);

const __Slice_uint8_t main__MESSAGE = (__Slice_uint8_t){(uint8_t*)"Hello, world!", sizeof("Hello, world!") - 1};
const int32_t main__TOTAL_CHARS = 13;
static GfxDriver gfx___driver;
static int32_t gfx___view_w;
static int32_t gfx___view_h;
static __Fn_void_int32_t_int32_t_int32_t gfx___pixel_fn;
static __Fn_void_int32_t_int32_t_int32_t_int32_t_int32_t gfx___fill_fn;
static DisplayConn display___conn;
static DisplayInfo display___info;

void main__main(void) {
  int32_t bus = i2c__i2c_init(I2C_BUS, I2C_SDA, I2C_SCL, I2C_FREQ);
  int32_t dev = i2c__i2c_open(bus, OLED_ADDR);
  DisplayConn* conn = display__display_conn();
  (conn->interface = DisplayInterface_I2C);
  (conn->device = dev);
  (conn->reset_pin = (-1));
  (conn->back_light_pin = (-1));
  drivers__ssd1306_128x32__display_attach_gfx(gfx__gfx_driver());
  gfx__gfx_init();
  gfx__gfx_clear(0);
  gfx__gfx_rect(0, 0, 128, 32, 1);
  int32_t location = 0;
  int32_t first_char = 0;
  int32_t char_len = 0;
  int32_t x = 0;
  while (true) {
    gfx__gfx_fill_rect(1, 1, 126, 30, 0);
    if ((location < 0)) {
      (first_char = (-location));
      (char_len = (main__TOTAL_CHARS + location));
      (x = 4);
    } else {
      (first_char = 0);
      (char_len = (main__TOTAL_CHARS - location));
      (x = ((location * 9) + 4));
    }
    gfx__gfx_str(x, 12, (__Slice_uint8_t){(&main__MESSAGE.ptr[first_char]), char_len}, 1, 0, 1);
    (location -= 1);
    if ((location < ((-main__TOTAL_CHARS) + 1))) {
      (location = (main__TOTAL_CHARS - 1));
    }
    __mp_delay_ms(200);
  }
}

int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
  return __mp_i2c_init(bus, sda, scl, freq_hz);
}

int32_t i2c__i2c_open(int32_t bus, int32_t addr) {
  return __mp_i2c_open(bus, addr);
}

void i2c__i2c_close(int32_t dev) {
  __mp_i2c_close(dev);
}

int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
  return __mp_i2c_write(dev, data, len);
}

int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
  return __mp_i2c_read(dev, buf, len);
}

int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len) {
  return __mp_i2c_write_read(dev, tx, tx_len, rx, rx_len);
}

static void gfx___flush_noop(void) {
  return;
}

GfxDriver* gfx__gfx_driver(void) {
  return (&gfx___driver);
}

static void gfx___pixel_r0(int32_t x, int32_t y, int32_t color) {
  (&gfx___driver)->set_pixel(x, y, color);
}

static void gfx___pixel_r90(int32_t x, int32_t y, int32_t color) {
  (&gfx___driver)->set_pixel(((gfx___driver.width - 1) - y), x, color);
}

static void gfx___pixel_r180(int32_t x, int32_t y, int32_t color) {
  (&gfx___driver)->set_pixel(((gfx___driver.width - 1) - x), ((gfx___driver.height - 1) - y), color);
}

static void gfx___pixel_r270(int32_t x, int32_t y, int32_t color) {
  (&gfx___driver)->set_pixel(y, ((gfx___driver.height - 1) - x), color);
}

static void gfx___fill_r0(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  (&gfx___driver)->fill_rect(x, y, w, h, color);
}

static void gfx___fill_r90(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  (&gfx___driver)->fill_rect(((gfx___driver.width - y) - h), x, h, w, color);
}

static void gfx___fill_r180(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  (&gfx___driver)->fill_rect(((gfx___driver.width - x) - w), ((gfx___driver.height - y) - h), w, h, color);
}

static void gfx___fill_r270(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  (&gfx___driver)->fill_rect(y, ((gfx___driver.height - x) - w), h, w, color);
}

void gfx__gfx_set_rotation(GfxRotation r) {
  if ((r == GfxRotation_R0)) {
    (gfx___pixel_fn = gfx___pixel_r0);
    (gfx___fill_fn = gfx___fill_r0);
    (gfx___view_w = gfx___driver.width);
    (gfx___view_h = gfx___driver.height);
  }
  if ((r == GfxRotation_R90)) {
    (gfx___pixel_fn = gfx___pixel_r90);
    (gfx___fill_fn = gfx___fill_r90);
    (gfx___view_w = gfx___driver.height);
    (gfx___view_h = gfx___driver.width);
  }
  if ((r == GfxRotation_R180)) {
    (gfx___pixel_fn = gfx___pixel_r180);
    (gfx___fill_fn = gfx___fill_r180);
    (gfx___view_w = gfx___driver.width);
    (gfx___view_h = gfx___driver.height);
  }
  if ((r == GfxRotation_R270)) {
    (gfx___pixel_fn = gfx___pixel_r270);
    (gfx___fill_fn = gfx___fill_r270);
    (gfx___view_w = gfx___driver.height);
    (gfx___view_h = gfx___driver.width);
  }
}

void gfx__gfx_init(void) {
  if ((gfx___driver.flush == NULL)) {
    (gfx___driver.flush = gfx___flush_noop);
  }
  gfx__gfx_set_rotation(GfxRotation_R0);
}

int32_t gfx__gfx_width(void) {
  return gfx___view_w;
}

int32_t gfx__gfx_height(void) {
  return gfx___view_h;
}

void gfx__gfx_pixel(int32_t x, int32_t y, int32_t color) {
  if (((((x < 0) || (x >= gfx___view_w)) || (y < 0)) || (y >= gfx___view_h))) {
    return;
  }
  gfx___pixel_fn(x, y, color);
  (&gfx___driver)->flush();
}

void gfx__gfx_hline(int32_t x, int32_t y, int32_t w, int32_t color) {
  if ((((w <= 0) || (y < 0)) || (y >= gfx___view_h))) {
    return;
  }
  if ((x < 0)) {
    (w += x);
    (x = 0);
  }
  if (((x + w) > gfx___view_w)) {
    (w = (gfx___view_w - x));
  }
  if ((w <= 0)) {
    return;
  }
  gfx___fill_fn(x, y, w, 1, color);
}

void gfx__gfx_vline(int32_t x, int32_t y, int32_t h, int32_t color) {
  if ((((h <= 0) || (x < 0)) || (x >= gfx___view_w))) {
    return;
  }
  if ((y < 0)) {
    (h += y);
    (y = 0);
  }
  if (((y + h) > gfx___view_h)) {
    (h = (gfx___view_h - y));
  }
  if ((h <= 0)) {
    return;
  }
  gfx___fill_fn(x, y, 1, h, color);
}

void gfx__gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  int32_t dx = __mp_gfx_abs((x1 - x0));
  int32_t dy = __mp_gfx_abs((y1 - y0));
  int32_t sx = __mp_gfx_sign((x1 - x0));
  int32_t sy = __mp_gfx_sign((y1 - y0));
  int32_t err = (dx - dy);
  while (true) {
    if (((((x0 >= 0) && (x0 < gfx___view_w)) && (y0 >= 0)) && (y0 < gfx___view_h))) {
      gfx___pixel_fn(x0, y0, color);
    }
    if (((x0 == x1) && (y0 == y1))) {
      (&gfx___driver)->flush();
      return;
    }
    int32_t e2 = (err * 2);
    if ((e2 > (-dy))) {
      (err -= dy);
      (x0 += sx);
    }
    if ((e2 < dx)) {
      (err += dx);
      (y0 += sy);
    }
  }
}

void gfx__gfx_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  gfx___fill_fn(x, y, w, 1, color);
  gfx___fill_fn(x, ((y + h) - 1), w, 1, color);
  gfx___fill_fn(x, (y + 1), 1, (h - 2), color);
  gfx___fill_fn(((x + w) - 1), (y + 1), 1, (h - 2), color);
  (&gfx___driver)->flush();
}

void gfx__gfx_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  gfx___fill_fn(x, y, w, h, color);
  (&gfx___driver)->flush();
}

void gfx__gfx_clear(int32_t color) {
  (&gfx___driver)->fill_rect(0, 0, gfx___driver.width, gfx___driver.height, color);
  (&gfx___driver)->flush();
}

static void gfx___circle_8(int32_t cx, int32_t cy, int32_t x, int32_t y, int32_t color) {
  gfx__gfx_pixel((cx + x), (cy + y), color);
  gfx__gfx_pixel((cx - x), (cy + y), color);
  gfx__gfx_pixel((cx + x), (cy - y), color);
  gfx__gfx_pixel((cx - x), (cy - y), color);
  gfx__gfx_pixel((cx + y), (cy + x), color);
  gfx__gfx_pixel((cx - y), (cy + x), color);
  gfx__gfx_pixel((cx + y), (cy - x), color);
  gfx__gfx_pixel((cx - y), (cy - x), color);
}

void gfx__gfx_circle(int32_t cx, int32_t cy, int32_t r, int32_t color) {
  int32_t x = 0;
  int32_t y = r;
  int32_t d = (1 - r);
  gfx___circle_8(cx, cy, x, y, color);
  while ((x < y)) {
    if ((d < 0)) {
      (d += ((2 * x) + 3));
    } else {
      (d += ((2 * (x - y)) + 5));
      (y -= 1);
    }
    (x += 1);
    gfx___circle_8(cx, cy, x, y, color);
  }
  (&gfx___driver)->flush();
}

void gfx__gfx_fill_circle(int32_t cx, int32_t cy, int32_t r, int32_t color) {
  int32_t dy = (-r);
  while ((dy <= r)) {
    int32_t dx = __mp_gfx_isqrt(((r * r) - (dy * dy)));
    gfx___fill_fn((cx - dx), (cy + dy), ((dx * 2) + 1), 1, color);
    (dy += 1);
  }
  (&gfx___driver)->flush();
}

static void gfx___line_raw(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  int32_t dx = __mp_gfx_abs((x1 - x0));
  int32_t dy = __mp_gfx_abs((y1 - y0));
  int32_t sx = __mp_gfx_sign((x1 - x0));
  int32_t sy = __mp_gfx_sign((y1 - y0));
  int32_t err = (dx - dy);
  while (true) {
    if (((((x0 >= 0) && (x0 < gfx___view_w)) && (y0 >= 0)) && (y0 < gfx___view_h))) {
      gfx___pixel_fn(x0, y0, color);
    }
    if (((x0 == x1) && (y0 == y1))) {
      return;
    }
    int32_t e2 = (err * 2);
    if ((e2 > (-dy))) {
      (err -= dy);
      (x0 += sx);
    }
    if ((e2 < dx)) {
      (err += dx);
      (y0 += sy);
    }
  }
}

void gfx__gfx_triangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
  gfx___line_raw(x0, y0, x1, y1, color);
  gfx___line_raw(x1, y1, x2, y2, color);
  gfx___line_raw(x2, y2, x0, y0, color);
  (&gfx___driver)->flush();
}

void gfx__gfx_fill_triangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
  int32_t tx = 0;
  int32_t ty = 0;
  if ((y1 < y0)) {
    (tx = x0);
    (ty = y0);
    (x0 = x1);
    (y0 = y1);
    (x1 = tx);
    (y1 = ty);
  }
  if ((y2 < y0)) {
    (tx = x0);
    (ty = y0);
    (x0 = x2);
    (y0 = y2);
    (x2 = tx);
    (y2 = ty);
  }
  if ((y2 < y1)) {
    (tx = x1);
    (ty = y1);
    (x1 = x2);
    (y1 = y2);
    (x2 = tx);
    (y2 = ty);
  }
  int32_t y = y0;
  while ((y <= y2)) {
    int32_t xa = 0;
    int32_t xb = 0;
    if (((y <= y1) && (y1 != y0))) {
      (xa = (x0 + (((y - y0) * (x1 - x0)) / (y1 - y0))));
    } else {
      (xa = x1);
    }
    if ((y2 != y0)) {
      (xb = (x0 + (((y - y0) * (x2 - x0)) / (y2 - y0))));
    } else {
      (xb = x0);
    }
    if ((xa > xb)) {
      (tx = xa);
      (xa = xb);
      (xb = tx);
    }
    gfx___fill_fn(xa, y, ((xb - xa) + 1), 1, color);
    (y += 1);
  }
  (&gfx___driver)->flush();
}

void gfx__gfx_round_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color) {
  gfx___fill_fn((x + r), y, (w - (2 * r)), 1, color);
  gfx___fill_fn((x + r), ((y + h) - 1), (w - (2 * r)), 1, color);
  gfx___fill_fn(x, (y + r), 1, (h - (2 * r)), color);
  gfx___fill_fn(((x + w) - 1), (y + r), 1, (h - (2 * r)), color);
  int32_t px = 0;
  int32_t py = r;
  int32_t d = (1 - r);
  while ((px <= py)) {
    gfx___pixel_fn(((((x + w) - 1) - r) + py), ((y + r) - px), color);
    gfx___pixel_fn(((x + r) - py), ((y + r) - px), color);
    gfx___pixel_fn(((((x + w) - 1) - r) + py), ((((y + h) - 1) - r) + px), color);
    gfx___pixel_fn(((x + r) - py), ((((y + h) - 1) - r) + px), color);
    gfx___pixel_fn(((((x + w) - 1) - r) + px), ((y + r) - py), color);
    gfx___pixel_fn(((x + r) - px), ((y + r) - py), color);
    gfx___pixel_fn(((((x + w) - 1) - r) + px), ((((y + h) - 1) - r) + py), color);
    gfx___pixel_fn(((x + r) - px), ((((y + h) - 1) - r) + py), color);
    if ((d < 0)) {
      (d += ((2 * px) + 3));
    } else {
      (d += ((2 * (px - py)) + 5));
      (py -= 1);
    }
    (px += 1);
  }
  (&gfx___driver)->flush();
}

void gfx__gfx_fill_round_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, int32_t color) {
  gfx___fill_fn((x + r), y, (w - (2 * r)), h, color);
  int32_t px = 0;
  int32_t py = r;
  int32_t d = (1 - r);
  while ((px <= py)) {
    gfx___fill_fn((((((x + w) - 1) - r) - py) + 1), ((y + r) - px), ((py * 2) - 1), 1, color);
    gfx___fill_fn((((((x + w) - 1) - r) - py) + 1), ((((y + h) - 1) - r) + px), ((py * 2) - 1), 1, color);
    gfx___fill_fn((((((x + w) - 1) - r) - px) + 1), ((y + r) - py), ((px * 2) - 1), 1, color);
    gfx___fill_fn((((((x + w) - 1) - r) - px) + 1), ((((y + h) - 1) - r) + py), ((px * 2) - 1), 1, color);
    if ((d < 0)) {
      (d += ((2 * px) + 3));
    } else {
      (d += ((2 * (px - py)) + 5));
      (py -= 1);
    }
    (px += 1);
  }
  (&gfx___driver)->flush();
}

static void gfx___char_raw(int32_t x, int32_t y, uint8_t c, int32_t color, int32_t bg, int32_t scale) {
  int32_t row = 0;
  while ((row < 8)) {
    uint8_t bits = __mp_gfx_font_row(c, row);
    int32_t col = 0;
    while ((col < 8)) {
      if (((bits & ((uint8_t)((0x80 >> col)))) != 0)) {
        gfx___fill_fn((x + (col * scale)), (y + (row * scale)), scale, scale, color);
      } else {
        if ((bg >= 0)) {
          gfx___fill_fn((x + (col * scale)), (y + (row * scale)), scale, scale, bg);
        }
      }
      (col += 1);
    }
    (row += 1);
  }
}

void gfx__gfx_char(int32_t x, int32_t y, uint8_t c, int32_t color, int32_t bg, int32_t scale) {
  gfx___char_raw(x, y, c, color, bg, scale);
  (&gfx___driver)->flush();
}

int32_t gfx__gfx_str(int32_t x, int32_t y, __Slice_uint8_t s, int32_t color, int32_t bg, int32_t scale) {
  int32_t i = 0;
  int32_t cx = x;
  while ((i < ((int32_t)(s.size)))) {
    gfx___char_raw(cx, y, s.ptr[i], color, bg, scale);
    (cx += (9 * scale));
    (i += 1);
  }
  (&gfx___driver)->flush();
  return cx;
}

static inline int32_t math__floor_fixed(int32_t v) {
  return (v & (-65536));
}

static inline int32_t math__ceil_fixed(int32_t v) {
  int32_t f = (v & (-65536));
  if ((v != f)) {
    return (f + 65536);
  }
  return f;
}

static inline int32_t math__round_fixed(int32_t v) {
  return ((v + 32768) & (-65536));
}

DisplayConn* display__display_conn(void) {
  return (&display___conn);
}

DisplayInfo* display__display_info(void) {
  return (&display___info);
}

static int32_t drivers__ssd1306_128x32___cmd1(int32_t dev, int32_t c) {
  uint8_t buf[2];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){buf, 2}, 2);
}

static int32_t drivers__ssd1306_128x32___cmd2(int32_t dev, int32_t c, int32_t v) {
  uint8_t buf[3];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  (buf[2] = ((uint8_t)(v)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){buf, 3}, 3);
}

static void drivers__ssd1306_128x32___hw_init(int32_t dev) {
  drivers__ssd1306_128x32___cmd1(dev, 0xAE);
  drivers__ssd1306_128x32___cmd2(dev, 0xD5, 0x80);
  drivers__ssd1306_128x32___cmd2(dev, 0xA8, 0x1F);
  drivers__ssd1306_128x32___cmd2(dev, 0xD3, 0x00);
  drivers__ssd1306_128x32___cmd1(dev, 0x40);
  drivers__ssd1306_128x32___cmd2(dev, 0x8D, 0x14);
  drivers__ssd1306_128x32___cmd2(dev, 0x20, 0x00);
  drivers__ssd1306_128x32___cmd1(dev, 0xA1);
  drivers__ssd1306_128x32___cmd1(dev, 0xC8);
  drivers__ssd1306_128x32___cmd2(dev, 0xDA, 0x02);
  drivers__ssd1306_128x32___cmd2(dev, 0x81, 0xCF);
  drivers__ssd1306_128x32___cmd2(dev, 0xD9, 0xF1);
  drivers__ssd1306_128x32___cmd2(dev, 0xDB, 0x40);
  drivers__ssd1306_128x32___cmd1(dev, 0xA4);
  drivers__ssd1306_128x32___cmd1(dev, 0xA6);
  drivers__ssd1306_128x32___cmd1(dev, 0xAF);
}

static void drivers__ssd1306_128x32___flush(int32_t dev) {
  uint8_t col[4];
  (col[0] = 0x00);
  (col[1] = 0x21);
  (col[2] = 0x00);
  (col[3] = 0x7F);
  i2c__i2c_write(dev, (__Slice_uint8_t){col, 4}, 4);
  uint8_t page[4];
  (page[0] = 0x00);
  (page[1] = 0x22);
  (page[2] = 0x00);
  (page[3] = 0x03);
  i2c__i2c_write(dev, (__Slice_uint8_t){page, 4}, 4);
  __Slice_uint8_t fb = __mp_ssd1306_32_fb_slice();
  i2c__i2c_write(dev, fb, 513);
}

static void drivers__ssd1306_128x32___gfx_set_pixel(int32_t x, int32_t y, int32_t color) {
  __mp_ssd1306_32_set_pixel(x, y, color);
}

static void drivers__ssd1306_128x32___gfx_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color) {
  __mp_ssd1306_32_fill_rect(x, y, w, h, color);
}

static void drivers__ssd1306_128x32___gfx_flush(void) {
  drivers__ssd1306_128x32___flush(display___conn.device);
}

void drivers__ssd1306_128x32__display_attach_gfx(GfxDriver* driver) {
  DisplayInfo* info = display__display_info();
  (info->width = 128);
  (info->height = 32);
  (info->color = DisplayColor_MONO);
  if ((display___conn.reset_pin >= 0)) {
    gpio__gpio_mode(display___conn.reset_pin, GpioMode_OUTPUT);
    gpio__gpio_write(display___conn.reset_pin, GpioLevel_LOW);
    gpio__gpio_write(display___conn.reset_pin, GpioLevel_HIGH);
  }
  drivers__ssd1306_128x32___hw_init(display___conn.device);
  __Slice_uint8_t fb = __mp_ssd1306_32_fb_slice();
  (fb.ptr[0] = 0x40);
  __mp_ssd1306_32_clear();
  (driver->width = 128);
  (driver->height = 32);
  (driver->set_pixel = drivers__ssd1306_128x32___gfx_set_pixel);
  (driver->fill_rect = drivers__ssd1306_128x32___gfx_fill_rect);
  (driver->flush = drivers__ssd1306_128x32___gfx_flush);
}

void gpio__gpio_mode(int32_t pin, GpioMode mode) {
  gpio_reset_pin(pin);
  if ((mode == GpioMode_INPUT_PULLUP)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 0);
  } else if ((mode == GpioMode_INPUT_PULLDOWN)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 1);
  } else {
    gpio_set_direction(pin, ((int32_t)(mode)));
  }
}

static inline void gpio__gpio_write(int32_t pin, GpioLevel value) {
  gpio_set_level(pin, ((int32_t)(value)));
}

static inline GpioLevel gpio__gpio_read(int32_t pin) {
  return ((GpioLevel)(gpio_get_level(pin)));
}

void app_main(void) { main__main(); }

