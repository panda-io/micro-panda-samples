#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct GraphicsDriver GraphicsDriver;
typedef struct Graphics Graphics;
typedef struct Point Point;
typedef struct Rect Rect;
typedef struct DisplayConn DisplayConn;
typedef struct Ssd1306 Ssd1306;
typedef struct Canvas Canvas;
typedef struct Allocator Allocator;
typedef struct Node Node;
typedef struct RenderContext RenderContext;

typedef enum {
  PixelFormat_Mono = 0,
  PixelFormat_RGB565 = 1,
} PixelFormat;

typedef enum {
  IndexFormat_Index1 = 0,
  IndexFormat_Index2 = 1,
  IndexFormat_Index4 = 2,
  IndexFormat_Index8 = 3,
} IndexFormat;

typedef enum {
  Rotation_R0 = 0,
  Rotation_R90 = 1,
  Rotation_R180 = 2,
  Rotation_R270 = 3,
} Rotation;

typedef enum {
  DisplayInterface_I2C = 0,
  DisplayInterface_SPI = 1,
} DisplayInterface;

typedef enum {
  DisplayColor_MONO = 0,
  DisplayColor_RGB565 = 1,
} DisplayColor;

typedef enum {
  MonoColor_Off = 0,
  MonoColor_On = 1,
} MonoColor;

typedef enum {
  Gray4Color_Black = 0,
  Gray4Color_DarkGray = 1,
  Gray4Color_LightGray = 2,
  Gray4Color_White = 3,
} Gray4Color;

typedef enum {
  Gray16Color_Black = 0,
  Gray16Color_White = 15,
} Gray16Color;

typedef enum {
  UIColor_Black = 0,
  UIColor_White = 1,
  UIColor_Red = 2,
  UIColor_Green = 3,
  UIColor_Blue = 4,
  UIColor_Yellow = 5,
  UIColor_Cyan = 6,
  UIColor_Magenta = 7,
  UIColor_DarkGray = 8,
  UIColor_LightGray = 9,
  UIColor_DarkRed = 10,
  UIColor_DarkGreen = 11,
  UIColor_DarkBlue = 12,
  UIColor_Orange = 13,
  UIColor_Purple = 14,
  UIColor_Teal = 15,
} UIColor;

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

typedef struct { uint8_t* ptr; int32_t size; } __Slice_uint8_t;
typedef struct { uint16_t* ptr; int32_t size; } __Slice_uint16_t;

typedef void (*__Fn_void_void_p_Rotation)(void*, Rotation);
typedef void (*__Fn_void_void_p)(void*);
typedef void (*__Fn_void_void_p_Rect_p_Slice_uint8_t)(void*, Rect*, __Slice_uint8_t);
typedef void (*__Fn_void_void)(void);
typedef void (*__Fn_void_RenderContext_p_Point_p_void_p)(RenderContext*, Point*, void*);

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
#include <math.h>
#include <string.h>
#include "driver/gpio.h"

struct GraphicsDriver {
  int32_t width;
  int32_t height;
  void* handle;
  __Fn_void_void_p_Rotation set_rotation;
  __Fn_void_void_p wait;
  __Fn_void_void_p_Rect_p_Slice_uint8_t flush;
};

struct Point {
  int32_t x;
  int32_t y;
};

struct Rect {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

struct DisplayConn {
  DisplayInterface interface;
  int32_t device;
  int32_t dc_pin;
  int32_t reset_pin;
  int32_t back_light_pin;
};

struct Allocator {
  __Slice_uint8_t _memory;
  int32_t _cursor;
};

struct Node {
  void* handle;
  __Fn_void_RenderContext_p_Point_p_void_p renderer;
};

struct RenderContext {
  __Slice_uint8_t buffer;
  PixelFormat format;
  Rect viewpoint;
};

struct Canvas {
  Node _node;
  __Slice_uint8_t _buffer;
  Rect _viewpoint;
  uint16_t _background;
  IndexFormat _index_format;
  __Slice_uint16_t _palette;
};

struct Graphics {
  GraphicsDriver* _driver;
  Node* _root;
  uint16_t _background;
  bool _dirty_render;
  Rect _render_window;
  __Slice_uint8_t _strip0;
  __Slice_uint8_t _strip1;
  bool _single_buffer;
  int32_t _front_buffer;
  RenderContext _context;
};

struct Ssd1306 {
  uint8_t strip[512];
  GraphicsDriver driver;
  Graphics gfx;
  int32_t _dev;
  int32_t _width;
  int32_t _height;
  uint8_t _page_buf[528];
};

void main__main(void);
int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz);
int32_t i2c__i2c_open(int32_t bus, int32_t addr);
void i2c__i2c_close(int32_t dev);
int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len);
int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len);
int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len);
void Graphics_set_root(Graphics* this, Node* node);
void Graphics_init(Graphics* this, GraphicsDriver* driver, __Slice_uint8_t buffer0, __Slice_uint8_t buffer1, PixelFormat pixel_format, uint16_t background, Rotation rotation, bool dirty_render);
void Graphics_mark_dirty(Graphics* this, Rect* rect);
void Graphics_render(Graphics* this);
static __Slice_uint8_t Graphics__front_strip(Graphics* this);
static __Slice_uint8_t Graphics__back_strip(Graphics* this);
static void Graphics__clear_strip(Graphics* this, __Slice_uint8_t buffer);
static inline void Point_copy(Point* this, Point* point);
static inline void Rect_copy(Rect* this, Rect* rect);
static inline bool Rect_intersect(Rect* this, Rect* rect);
static inline bool Rect_contains(Rect* this, Point* point);
static inline void Rect_merge(Rect* this, Rect* rect);
static void drivers__ssd1306___ssd1306_set_rotation(void* handle, Rotation r);
static void drivers__ssd1306___ssd1306_wait(void* handle);
static void drivers__ssd1306___ssd1306_flush(void* handle, Rect* rect, __Slice_uint8_t buf);
static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c);
static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v);
static void drivers__ssd1306___hw_init(int32_t dev, int32_t rows);
static void drivers__ssd1306___attach(Ssd1306* display, DisplayConn* conn, int32_t width, int32_t height);
void drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* display, DisplayConn* conn);
void drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* display, DisplayConn* conn);
static void Ssd1306__set_rotation(Ssd1306* this, Rotation r);
static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src, int32_t page_start, int32_t page_count);
static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf);
static void Ssd1306__send_pages(Ssd1306* this, int32_t page_start, int32_t page_count);
Canvas* node__canvas__create_canvas(Allocator* allocator, Rect* rect, uint16_t background, IndexFormat index_format, __Slice_uint16_t palette);
void node__canvas__render_canvas(RenderContext* context, Point* offset, void* handle);
Node* Canvas_get_node(Canvas* this);
static inline void Canvas_draw_pixel(Canvas* this, int32_t x, int32_t y, uint16_t color);
static inline void Canvas_draw_hline(Canvas* this, int32_t x, int32_t y, int32_t width, uint16_t color);
static inline void Canvas_draw_vline(Canvas* this, int32_t x, int32_t y, int32_t height, uint16_t color);
void Canvas_draw_line(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
void Canvas_draw_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color);
static inline void Canvas_fill_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color);
void Canvas_clear(Canvas* this);
void Canvas_draw_circle(Canvas* this, int32_t cx, int32_t cy, int32_t r, uint16_t color);
void Canvas_fill_circle(Canvas* this, int32_t cx, int32_t cy, int32_t r, uint16_t color);
void Canvas_draw_triangle(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);
void Canvas_fill_triangle(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);
void Canvas_draw_round_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, int32_t r, uint16_t color);
void Canvas_fill_round_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, int32_t r, uint16_t color);
static void Canvas__init(Canvas* this, Allocator* allocator, Rect* rect, uint16_t background, IndexFormat index_format, __Slice_uint16_t palette);
static inline void Canvas__render(Canvas* this, RenderContext* context, Rect* rect, Point* offset);
static void Canvas__fill_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color);
static void Canvas__circle_8(Canvas* this, int32_t cx, int32_t cy, int32_t x, int32_t y, uint16_t color);
static void Canvas__set_pixel_index1(Canvas* this, int32_t x, int32_t y, uint16_t color);
static void Canvas__set_pixel_index2(Canvas* this, int32_t x, int32_t y, uint16_t color);
static void Canvas__set_pixel_index4(Canvas* this, int32_t x, int32_t y, uint16_t color);
static void Canvas__set_pixel_index8(Canvas* this, int32_t x, int32_t y, uint16_t color);
static void Canvas__render_index1(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset);
static void Canvas__render_index2(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset);
static void Canvas__render_index4(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset);
static void Canvas__render_index8(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset);
static inline int32_t Canvas__abs(Canvas* this, int32_t v);
static inline int32_t Canvas__sign(Canvas* this, int32_t v);
static inline int32_t Canvas__sqrt(Canvas* this, int32_t n);
void Allocator_init(Allocator* this, __Slice_uint8_t mem);
static inline void* Allocator_allocate(Allocator* this, size_t __sizeof_T);
static inline int32_t Allocator_available(Allocator* this);
static inline void Allocator_reset(Allocator* this);
static inline int32_t math__floor_q16(int32_t value);
static inline int32_t math__ceil_q16(int32_t value);
static inline int32_t math__round_q16(int32_t value);
static inline int32_t math__floor_fixed(int32_t value);
static inline int32_t math__ceil_fixed(int32_t value);
static inline int32_t math__round_fixed(int32_t value);
static inline bool RenderContext_intersect(RenderContext* this, Rect* rect);
void RenderContext_set_pixel(RenderContext* this, Point* point, uint16_t color);
void RenderContext_fill_rect(RenderContext* this, Rect* rect, uint16_t color);
static inline void RenderContext__set_pixel_mono(RenderContext* this, int32_t x, int32_t y, uint16_t color);
static inline void RenderContext__set_pixel_rgb565(RenderContext* this, int32_t x, int32_t y, uint16_t color);
static inline void memory__memory_set(__Slice_uint8_t dst, uint8_t value);
static inline void memory__memory_copy(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size);
static inline void memory__memory_move(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size);
static inline void memory__memory_zero(__Slice_uint8_t dst);
void gpio__gpio_mode(int32_t pin, GpioMode mode);
static inline void gpio__gpio_write(int32_t pin, GpioLevel value);
static inline GpioLevel gpio__gpio_read(int32_t pin);
static inline int32_t math__min_int32_t(int32_t a, int32_t b);
static inline int32_t math__max_int32_t(int32_t a, int32_t b);
static inline Canvas* Allocator_allocate_Canvas(Allocator* this);
static inline __Slice_uint8_t Allocator_allocate_array_uint8_t(Allocator* this, int32_t length);

const int32_t main__HEAP_SIZE = 1024;
static uint8_t main___heap[1024];
static uint8_t main___strip[512];
static Ssd1306 main___driver;
static Graphics main___gfx;
int32_t drivers__ssd1306__BUF_SIZE = 528;
const uint16_t palette__TRANSPARENT = 0x0020;
uint16_t palette__PALETTE_MONO[2] = {0x0000, 0xFFFF};
uint16_t palette__PALETTE_GRAY4[4] = {0x0000, 0x52AA, 0xAD55, 0xFFFF};
uint16_t palette__PALETTE_GRAY16[16] = {0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x528A, 0x630C, 0x738E, 0x8410, 0x9492, 0xA514, 0xB596, 0xC618, 0xD69A, 0xE71C, 0xFFFF};
uint16_t palette__PALETTE_UI16[16] = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0x39E7, 0xC618, 0x8000, 0x0400, 0x000F, 0xFD20, 0x8010, 0x07A0};
const float math__PI = 3.14159265358979323846f;
const float math__TAU = 6.28318530717958647692f;
const float math__E = 2.71828182845904523536f;

void main__main(void) {
  int32_t bus = i2c__i2c_init(I2C_BUS, I2C_SDA, I2C_SCL, I2C_FREQ);
  int32_t dev = i2c__i2c_open(bus, OLED_ADDR);
  DisplayConn conn = {0};
  (conn.interface = DisplayInterface_I2C);
  (conn.device = dev);
  (conn.dc_pin = (-1));
  (conn.reset_pin = (-1));
  (conn.back_light_pin = (-1));
  Allocator alloc = {0};
  Allocator_init((&alloc), (__Slice_uint8_t){(&main___heap[0]), main__HEAP_SIZE});
  drivers__ssd1306__attach_ssd1306_128x32((&main___driver), (&conn));
  Rect rect = (Rect){0, 0, 128, 32};
  Canvas* canvas = node__canvas__create_canvas((&alloc), (&rect), 0, IndexFormat_Index1, (__Slice_uint16_t){palette__PALETTE_MONO, 2});
  Canvas_clear(canvas);
  Canvas_draw_rect(canvas, 0, 0, 128, 32, 1);
  Graphics_init((&main___gfx), (&main___driver.driver), (__Slice_uint8_t){(&main___strip[0]), 512}, (__Slice_uint8_t){NULL, 0}, PixelFormat_Mono, 0, Rotation_R0, false);
  Graphics_set_root((&main___gfx), Canvas_get_node(canvas));
  Graphics_render((&main___gfx));
  while (true) {
    __mp_delay_ms(1000);
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

void Graphics_set_root(Graphics* this, Node* node) {
  (this->_root = node);
}

void Graphics_init(Graphics* this, GraphicsDriver* driver, __Slice_uint8_t buffer0, __Slice_uint8_t buffer1, PixelFormat pixel_format, uint16_t background, Rotation rotation, bool dirty_render) {
  (this->_driver = driver);
  (this->_strip0 = buffer0);
  (this->_strip1 = buffer1);
  (this->_context.format = pixel_format);
  (this->_background = background);
  (this->_dirty_render = dirty_render);
  if ((buffer1.size == 0)) {
    (this->_single_buffer = true);
  }
  (this->_render_window.x = 0);
  (this->_render_window.y = 0);
  if (this->_dirty_render) {
    (this->_render_window.width = 0);
    (this->_render_window.height = 0);
  }
  this->_driver->set_rotation(this->_driver->handle, rotation);
  if (((rotation == Rotation_R0) || (rotation == Rotation_R180))) {
    if ((!this->_dirty_render)) {
      (this->_render_window.width = this->_driver->width);
      (this->_render_window.height = this->_driver->height);
    }
  } else {
    if ((!this->_dirty_render)) {
      (this->_render_window.width = this->_driver->height);
      (this->_render_window.height = this->_driver->width);
    }
  }
  (this->_front_buffer = 0);
}

void Graphics_mark_dirty(Graphics* this, Rect* rect) {
  int32_t aligned_x = rect->x;
  int32_t aligned_width = rect->width;
  if ((this->_context.format == PixelFormat_Mono)) {
    int32_t right = (((rect->x + rect->width) + 7) & (~7));
    (aligned_x = (rect->x & (~7)));
    (aligned_width = (right - aligned_x));
  }
  Rect aligned = (Rect){aligned_x, rect->y, aligned_width, rect->height};
  if (((this->_render_window.width == 0) && (this->_render_window.height == 0))) {
    Rect_copy((&this->_render_window), (&aligned));
  } else {
    Rect_merge((&this->_render_window), (&aligned));
  }
}

void Graphics_render(Graphics* this) {
  if (((this->_render_window.width == 0) || (this->_render_window.height == 0))) {
    return;
  }
  (this->_context.buffer = Graphics__back_strip(this));
  (this->_context.viewpoint.x = this->_render_window.x);
  (this->_context.viewpoint.width = this->_render_window.width);
  int32_t row_bytes = 0;
  if ((this->_context.format == PixelFormat_Mono)) {
    (row_bytes = ((this->_context.viewpoint.width + 7) / 8));
  } else {
    (row_bytes = (this->_context.viewpoint.width * 2));
  }
  int32_t strip_rows = (((this->_context.buffer.size + row_bytes) - 1) / row_bytes);
  int32_t strip_count = (((this->_render_window.height + strip_rows) - 1) / strip_rows);
  Point offset = (Point){0, 0};
  for (int32_t strip_index = 0; strip_index < strip_count; strip_index++) {
    (this->_context.viewpoint.y = (this->_render_window.y + (strip_index * strip_rows)));
    (this->_context.viewpoint.height = math__min_int32_t(strip_rows, ((this->_render_window.y + this->_render_window.height) - this->_context.viewpoint.y)));
    Graphics__clear_strip(this, this->_context.buffer);
    this->_root->renderer((&this->_context), (&offset), this->_root->handle);
    this->_driver->wait(this->_driver->handle);
    (this->_front_buffer = (1 - this->_front_buffer));
    this->_driver->flush(this->_driver->handle, (&this->_context.viewpoint), Graphics__front_strip(this));
  }
  if (this->_dirty_render) {
    (this->_render_window.x = 0);
    (this->_render_window.y = 0);
    (this->_render_window.width = 0);
    (this->_render_window.height = 0);
  }
}

static __Slice_uint8_t Graphics__front_strip(Graphics* this) {
  if ((this->_single_buffer || (this->_front_buffer == 0))) {
    return this->_strip0;
  }
  return this->_strip1;
}

static __Slice_uint8_t Graphics__back_strip(Graphics* this) {
  if ((this->_single_buffer || (this->_front_buffer == 1))) {
    return this->_strip0;
  }
  return this->_strip1;
}

static void Graphics__clear_strip(Graphics* this, __Slice_uint8_t buffer) {
  if ((this->_context.format == PixelFormat_Mono)) {
    if ((this->_background == 0)) {
      memory__memory_zero(buffer);
    } else {
      memory__memory_set(buffer, 0xFF);
    }
  } else {
    if ((this->_background == 0)) {
      memory__memory_zero(buffer);
    } else {
      int32_t i = 0;
      int32_t size = buffer.size;
      while ((i < size)) {
        (buffer.ptr[i] = ((uint8_t)((this->_background >> 8))));
        (buffer.ptr[(i + 1)] = ((uint8_t)(this->_background)));
        (i += 2);
      }
    }
  }
}

static inline void Point_copy(Point* this, Point* point) {
  (this->x = point->x);
  (this->y = point->y);
}

static inline void Rect_copy(Rect* this, Rect* rect) {
  (this->x = rect->x);
  (this->y = rect->y);
  (this->width = rect->width);
  (this->height = rect->height);
}

static inline bool Rect_intersect(Rect* this, Rect* rect) {
  return ((((rect->x < (this->x + this->width)) && ((rect->x + rect->width) > this->x)) && (rect->y < (this->y + this->height))) && ((rect->y + rect->height) > this->y));
}

static inline bool Rect_contains(Rect* this, Point* point) {
  return ((((point->x >= this->x) && (point->x < (this->x + this->width))) && (point->y >= this->y)) && (point->y < (this->y + this->height)));
}

static inline void Rect_merge(Rect* this, Rect* rect) {
  int32_t right = math__max_int32_t((this->x + this->width), (rect->x + rect->width));
  int32_t bottom = math__max_int32_t((this->y + this->height), (rect->y + rect->height));
  (this->x = math__min_int32_t(this->x, rect->x));
  (this->y = math__min_int32_t(this->y, rect->y));
  (this->width = (right - this->x));
  (this->height = (bottom - this->y));
}

static void drivers__ssd1306___ssd1306_set_rotation(void* handle, Rotation r) {
  Ssd1306__set_rotation(((Ssd1306*)(handle)), r);
}

static void drivers__ssd1306___ssd1306_wait(void* handle) {
}

static void drivers__ssd1306___ssd1306_flush(void* handle, Rect* rect, __Slice_uint8_t buf) {
  Ssd1306__flush(((Ssd1306*)(handle)), rect, buf);
}

static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c) {
  uint8_t buf[2];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){(&buf[0]), 2}, 2);
}

static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v) {
  uint8_t buf[3];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  (buf[2] = ((uint8_t)(v)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){(&buf[0]), 3}, 3);
}

static void drivers__ssd1306___hw_init(int32_t dev, int32_t rows) {
  drivers__ssd1306___cmd1(dev, 0xAE);
  drivers__ssd1306___cmd2(dev, 0xD5, 0x80);
  drivers__ssd1306___cmd2(dev, 0xA8, (rows - 1));
  drivers__ssd1306___cmd2(dev, 0xD3, 0x00);
  drivers__ssd1306___cmd1(dev, 0x40);
  drivers__ssd1306___cmd2(dev, 0x8D, 0x14);
  drivers__ssd1306___cmd2(dev, 0x20, 0x00);
  drivers__ssd1306___cmd1(dev, 0xA1);
  drivers__ssd1306___cmd1(dev, 0xC8);
  drivers__ssd1306___cmd2(dev, 0xDA, 0x02);
  drivers__ssd1306___cmd2(dev, 0x81, 0xCF);
  drivers__ssd1306___cmd2(dev, 0xD9, 0xF1);
  drivers__ssd1306___cmd2(dev, 0xDB, 0x40);
  drivers__ssd1306___cmd1(dev, 0xA4);
  drivers__ssd1306___cmd1(dev, 0xA6);
  drivers__ssd1306___cmd1(dev, 0xAF);
}

static void drivers__ssd1306___attach(Ssd1306* display, DisplayConn* conn, int32_t width, int32_t height) {
  (display->_dev = conn->device);
  (display->_width = width);
  (display->_height = height);
  (display->_page_buf[0] = 0x40);
  if ((conn->reset_pin >= 0)) {
    gpio__gpio_mode(conn->reset_pin, GpioMode_OUTPUT);
    gpio__gpio_write(conn->reset_pin, GpioLevel_LOW);
    gpio__gpio_write(conn->reset_pin, GpioLevel_HIGH);
  }
  (display->driver.width = width);
  (display->driver.height = height);
  (display->driver.handle = ((void*)(display)));
  (display->driver.set_rotation = drivers__ssd1306___ssd1306_set_rotation);
  (display->driver.wait = drivers__ssd1306___ssd1306_wait);
  (display->driver.flush = drivers__ssd1306___ssd1306_flush);
}

void drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* display, DisplayConn* conn) {
  drivers__ssd1306___attach(display, conn, 128, 32);
  drivers__ssd1306___hw_init(conn->device, 32);
}

void drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* display, DisplayConn* conn) {
  drivers__ssd1306___attach(display, conn, 128, 64);
  drivers__ssd1306___hw_init(conn->device, 64);
}

static void Ssd1306__set_rotation(Ssd1306* this, Rotation r) {
}

static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src, int32_t page_start, int32_t page_count) {
  int32_t stride = (this->_width >> 3);
  for (int32_t ry = 0; ry < rect->height; ry++) {
    int32_t abs_y = (rect->y + ry);
    if (((abs_y >= 0) && (abs_y < this->_height))) {
      int32_t page = (abs_y >> 3);
      if (((page >= page_start) && (page < (page_start + page_count)))) {
        int32_t bit = (abs_y & 7);
        int32_t row = (abs_y * stride);
        int32_t local_page = (page - page_start);
        for (int32_t rx = 0; rx < rect->width; rx++) {
          int32_t abs_x = (rect->x + rx);
          if (((abs_x >= 0) && (abs_x < this->_width))) {
            int32_t pixel = ((((int32_t)(src.ptr[(row + (abs_x >> 3))])) >> (7 - (abs_x & 7))) & 1);
            int32_t p = ((1 + (local_page * this->_width)) + abs_x);
            if ((pixel != 0)) {
              (this->_page_buf[p] = (this->_page_buf[p] | ((uint8_t)((1 << bit)))));
            } else {
              (this->_page_buf[p] = (this->_page_buf[p] & ((uint8_t)((~(1 << bit))))));
            }
          }
        }
      }
    }
  }
}

static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf) {
  int32_t total_pages = (this->_height >> 3);
  int32_t half = 4;
  int32_t passes = (total_pages / half);
  for (int32_t pass = 0; pass < passes; pass++) {
    int32_t ps = (pass * half);
    Rect hr = {0};
    (hr.x = 0);
    (hr.y = (ps << 3));
    (hr.width = this->_width);
    (hr.height = 32);
    for (int32_t i = 1; i < (1 + (half * this->_width)); i++) {
      (this->_page_buf[i] = 0x00);
    }
    Ssd1306__blit(this, (&hr), buf, ps, half);
    Ssd1306__send_pages(this, ps, half);
  }
}

static void Ssd1306__send_pages(Ssd1306* this, int32_t page_start, int32_t page_count) {
  uint8_t col[4];
  (col[0] = 0x00);
  (col[1] = 0x21);
  (col[2] = 0x00);
  (col[3] = ((uint8_t)((this->_width - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){col, 4}, 4);
  uint8_t page[4];
  (page[0] = 0x00);
  (page[1] = 0x22);
  (page[2] = ((uint8_t)(page_start)));
  (page[3] = ((uint8_t)(((page_start + page_count) - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){page, 4}, 4);
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){this->_page_buf, 528}, (1 + (page_count * this->_width)));
}

Canvas* node__canvas__create_canvas(Allocator* allocator, Rect* rect, uint16_t background, IndexFormat index_format, __Slice_uint16_t palette) {
  Canvas* canvas = Allocator_allocate_Canvas(allocator);
  Canvas__init(canvas, allocator, rect, background, index_format, palette);
  return canvas;
}

void node__canvas__render_canvas(RenderContext* context, Point* offset, void* handle) {
  Canvas* canvas = ((Canvas*)(handle));
  Rect viewpoint = {0};
  (viewpoint.x = (canvas->_viewpoint.x + offset->x));
  (viewpoint.y = (canvas->_viewpoint.y + offset->y));
  (viewpoint.width = canvas->_viewpoint.width);
  (viewpoint.height = canvas->_viewpoint.height);
  if ((!RenderContext_intersect(context, (&viewpoint)))) {
    return;
  }
  (viewpoint.x = math__max_int32_t(viewpoint.x, context->viewpoint.x));
  (viewpoint.y = math__max_int32_t(viewpoint.y, context->viewpoint.y));
  (viewpoint.width = (math__min_int32_t((viewpoint.x + viewpoint.width), (context->viewpoint.x + context->viewpoint.width)) - viewpoint.x));
  (viewpoint.height = (math__min_int32_t((viewpoint.y + viewpoint.height), (context->viewpoint.y + context->viewpoint.height)) - viewpoint.y));
  Canvas__render(canvas, context, (&viewpoint), offset);
}

Node* Canvas_get_node(Canvas* this) {
  return (&this->_node);
}

static inline void Canvas_draw_pixel(Canvas* this, int32_t x, int32_t y, uint16_t color) {
  switch (this->_index_format) {
    case IndexFormat_Index1: {
      Canvas__set_pixel_index1(this, x, y, color);
      break;
    }
    case IndexFormat_Index2: {
      Canvas__set_pixel_index2(this, x, y, color);
      break;
    }
    case IndexFormat_Index4: {
      Canvas__set_pixel_index4(this, x, y, color);
      break;
    }
    case IndexFormat_Index8: {
      Canvas__set_pixel_index8(this, x, y, color);
      break;
    }
  }
}

static inline void Canvas_draw_hline(Canvas* this, int32_t x, int32_t y, int32_t width, uint16_t color) {
  Canvas__fill_rect(this, x, y, width, 1, color);
}

static inline void Canvas_draw_vline(Canvas* this, int32_t x, int32_t y, int32_t height, uint16_t color) {
  Canvas__fill_rect(this, x, y, 1, height, color);
}

void Canvas_draw_line(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  int32_t dx = Canvas__abs(this, (x1 - x0));
  int32_t dy = Canvas__abs(this, (y1 - y0));
  int32_t sx = Canvas__sign(this, (x1 - x0));
  int32_t sy = Canvas__sign(this, (y1 - y0));
  int32_t err = (dx - dy);
  int32_t cx = x0;
  int32_t cy = y0;
  while (true) {
    Canvas_draw_pixel(this, cx, cy, color);
    if (((cx == x1) && (cy == y1))) {
      return;
    }
    int32_t e2 = (err * 2);
    if ((e2 > (-dy))) {
      (err -= dy);
      (cx += sx);
    }
    if ((e2 < dx)) {
      (err += dx);
      (cy += sy);
    }
  }
}

void Canvas_draw_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color) {
  Canvas__fill_rect(this, x, y, width, 1, color);
  Canvas__fill_rect(this, x, ((y + height) - 1), width, 1, color);
  Canvas__fill_rect(this, x, (y + 1), 1, (height - 2), color);
  Canvas__fill_rect(this, ((x + width) - 1), (y + 1), 1, (height - 2), color);
}

static inline void Canvas_fill_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color) {
  Canvas__fill_rect(this, x, y, width, height, color);
}

void Canvas_clear(Canvas* this) {
  Canvas__fill_rect(this, this->_viewpoint.x, this->_viewpoint.y, this->_viewpoint.width, this->_viewpoint.height, this->_background);
}

void Canvas_draw_circle(Canvas* this, int32_t cx, int32_t cy, int32_t r, uint16_t color) {
  int32_t x = 0;
  int32_t y = r;
  int32_t d = (1 - r);
  Canvas__circle_8(this, cx, cy, x, y, color);
  while ((x < y)) {
    if ((d < 0)) {
      (d += ((2 * x) + 3));
    } else {
      (d += ((2 * (x - y)) + 5));
      (y -= 1);
    }
    (x += 1);
    Canvas__circle_8(this, cx, cy, x, y, color);
  }
}

void Canvas_fill_circle(Canvas* this, int32_t cx, int32_t cy, int32_t r, uint16_t color) {
  for (int32_t dy = (-r); dy < (r + 1); dy++) {
    int32_t dx = Canvas__sqrt(this, ((r * r) - (dy * dy)));
    Canvas__fill_rect(this, (cx - dx), (cy + dy), ((dx * 2) + 1), 1, color);
  }
}

void Canvas_draw_triangle(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
  Canvas_draw_line(this, x0, y0, x1, y1, color);
  Canvas_draw_line(this, x1, y1, x2, y2, color);
  Canvas_draw_line(this, x2, y2, x0, y0, color);
}

void Canvas_fill_triangle(Canvas* this, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
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
  for (int32_t y = y0; y < (y2 + 1); y++) {
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
    Canvas__fill_rect(this, xa, y, ((xb - xa) + 1), 1, color);
  }
}

void Canvas_draw_round_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, int32_t r, uint16_t color) {
  Canvas__fill_rect(this, (x + r), y, (width - (2 * r)), 1, color);
  Canvas__fill_rect(this, (x + r), ((y + height) - 1), (width - (2 * r)), 1, color);
  Canvas__fill_rect(this, x, (y + r), 1, (height - (2 * r)), color);
  Canvas__fill_rect(this, ((x + width) - 1), (y + r), 1, (height - (2 * r)), color);
  int32_t px = 0;
  int32_t py = r;
  int32_t d = (1 - r);
  while ((px <= py)) {
    Canvas_draw_pixel(this, ((((x + width) - 1) - r) + py), ((y + r) - px), color);
    Canvas_draw_pixel(this, ((x + r) - py), ((y + r) - px), color);
    Canvas_draw_pixel(this, ((((x + width) - 1) - r) + py), ((((y + height) - 1) - r) + px), color);
    Canvas_draw_pixel(this, ((x + r) - py), ((((y + height) - 1) - r) + px), color);
    Canvas_draw_pixel(this, ((((x + width) - 1) - r) + px), ((y + r) - py), color);
    Canvas_draw_pixel(this, ((x + r) - px), ((y + r) - py), color);
    Canvas_draw_pixel(this, ((((x + width) - 1) - r) + px), ((((y + height) - 1) - r) + py), color);
    Canvas_draw_pixel(this, ((x + r) - px), ((((y + height) - 1) - r) + py), color);
    if ((d < 0)) {
      (d += ((2 * px) + 3));
    } else {
      (d += ((2 * (px - py)) + 5));
      (py -= 1);
    }
    (px += 1);
  }
}

void Canvas_fill_round_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, int32_t r, uint16_t color) {
  Canvas__fill_rect(this, (x + r), y, (width - (2 * r)), height, color);
  int32_t px = 0;
  int32_t py = r;
  int32_t d = (1 - r);
  while ((px <= py)) {
    Canvas__fill_rect(this, (((((x + width) - 1) - r) - py) + 1), ((y + r) - px), ((py * 2) - 1), 1, color);
    Canvas__fill_rect(this, (((((x + width) - 1) - r) - py) + 1), ((((y + height) - 1) - r) + px), ((py * 2) - 1), 1, color);
    Canvas__fill_rect(this, (((((x + width) - 1) - r) - px) + 1), ((y + r) - py), ((px * 2) - 1), 1, color);
    Canvas__fill_rect(this, (((((x + width) - 1) - r) - px) + 1), ((((y + height) - 1) - r) + py), ((px * 2) - 1), 1, color);
    if ((d < 0)) {
      (d += ((2 * px) + 3));
    } else {
      (d += ((2 * (px - py)) + 5));
      (py -= 1);
    }
    (px += 1);
  }
}

static void Canvas__init(Canvas* this, Allocator* allocator, Rect* rect, uint16_t background, IndexFormat index_format, __Slice_uint16_t palette) {
  Rect_copy((&this->_viewpoint), rect);
  (this->_background = background);
  (this->_index_format = index_format);
  (this->_palette = palette);
  int32_t buffer_size = 0;
  if ((index_format == IndexFormat_Index1)) {
    (buffer_size = (((rect->width + 7) / 8) * rect->height));
  }
  if ((index_format == IndexFormat_Index2)) {
    (buffer_size = (((rect->width + 3) / 4) * rect->height));
  }
  if ((index_format == IndexFormat_Index4)) {
    (buffer_size = (((rect->width + 1) / 2) * rect->height));
  }
  if ((index_format == IndexFormat_Index8)) {
    (buffer_size = (rect->width * rect->height));
  }
  if ((buffer_size > 0)) {
    (this->_buffer = Allocator_allocate_array_uint8_t(allocator, buffer_size));
  }
  (this->_node.handle = ((void*)(this)));
  (this->_node.renderer = node__canvas__render_canvas);
}

static inline void Canvas__render(Canvas* this, RenderContext* context, Rect* rect, Point* offset) {
  switch (this->_index_format) {
    case IndexFormat_Index1: {
      Canvas__render_index1(this, context, rect, offset);
      break;
    }
    case IndexFormat_Index2: {
      Canvas__render_index2(this, context, rect, offset);
      break;
    }
    case IndexFormat_Index4: {
      Canvas__render_index4(this, context, rect, offset);
      break;
    }
    case IndexFormat_Index8: {
      Canvas__render_index8(this, context, rect, offset);
      break;
    }
  }
}

static void Canvas__fill_rect(Canvas* this, int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color) {
  if (((width <= 0) || (height <= 0))) {
    return;
  }
  bool intersect = ((((x < (this->_viewpoint.x + this->_viewpoint.width)) && ((x + width) > this->_viewpoint.x)) && (y < (this->_viewpoint.y + this->_viewpoint.height))) && ((y + height) > this->_viewpoint.y));
  if ((!intersect)) {
    return;
  }
  int32_t start_x = math__max_int32_t(x, this->_viewpoint.x);
  int32_t start_y = math__max_int32_t(y, this->_viewpoint.y);
  int32_t end_x = math__min_int32_t((x + width), (this->_viewpoint.x + this->_viewpoint.width));
  int32_t end_y = math__min_int32_t((y + height), (this->_viewpoint.y + this->_viewpoint.height));
  for (int32_t py = start_y; py < end_y; py++) {
    for (int32_t px = start_x; px < end_x; px++) {
      Canvas_draw_pixel(this, px, py, color);
    }
  }
}

static void Canvas__circle_8(Canvas* this, int32_t cx, int32_t cy, int32_t x, int32_t y, uint16_t color) {
  Canvas_draw_pixel(this, (cx + x), (cy + y), color);
  Canvas_draw_pixel(this, (cx - x), (cy + y), color);
  Canvas_draw_pixel(this, (cx + x), (cy - y), color);
  Canvas_draw_pixel(this, (cx - x), (cy - y), color);
  Canvas_draw_pixel(this, (cx + y), (cy + x), color);
  Canvas_draw_pixel(this, (cx - y), (cy + x), color);
  Canvas_draw_pixel(this, (cx + y), (cy - x), color);
  Canvas_draw_pixel(this, (cx - y), (cy - x), color);
}

static void Canvas__set_pixel_index1(Canvas* this, int32_t x, int32_t y, uint16_t color) {
  if (((((x < this->_viewpoint.x) || (x >= (this->_viewpoint.x + this->_viewpoint.width))) || (y < this->_viewpoint.y)) || (y >= (this->_viewpoint.y + this->_viewpoint.height)))) {
    return;
  }
  int32_t lx = (x - this->_viewpoint.x);
  int32_t ly = (y - this->_viewpoint.y);
  int32_t index = ((ly * ((this->_viewpoint.width + 7) / 8)) + (lx / 8));
  if (((color & 1) != 0)) {
    (this->_buffer.ptr[index] = (this->_buffer.ptr[index] | ((uint8_t)((0x80 >> (lx & 7))))));
  } else {
    (this->_buffer.ptr[index] = (this->_buffer.ptr[index] & ((uint8_t)((~(0x80 >> (lx & 7)))))));
  }
}

static void Canvas__set_pixel_index2(Canvas* this, int32_t x, int32_t y, uint16_t color) {
  if (((((x < this->_viewpoint.x) || (x >= (this->_viewpoint.x + this->_viewpoint.width))) || (y < this->_viewpoint.y)) || (y >= (this->_viewpoint.y + this->_viewpoint.height)))) {
    return;
  }
  int32_t lx = (x - this->_viewpoint.x);
  int32_t ly = (y - this->_viewpoint.y);
  int32_t index = ((ly * ((this->_viewpoint.width + 3) / 4)) + (lx / 4));
  uint16_t shift = ((3 - (((uint16_t)(lx)) & 3)) * 2);
  (this->_buffer.ptr[index] = ((this->_buffer.ptr[index] & ((uint8_t)((~(0x03 << shift))))) | ((uint8_t)(((color & 0x03) << shift)))));
}

static void Canvas__set_pixel_index4(Canvas* this, int32_t x, int32_t y, uint16_t color) {
  if (((((x < this->_viewpoint.x) || (x >= (this->_viewpoint.x + this->_viewpoint.width))) || (y < this->_viewpoint.y)) || (y >= (this->_viewpoint.y + this->_viewpoint.height)))) {
    return;
  }
  int32_t lx = (x - this->_viewpoint.x);
  int32_t ly = (y - this->_viewpoint.y);
  int32_t index = ((ly * ((this->_viewpoint.width + 1) / 2)) + (lx / 2));
  if (((lx & 1) == 0)) {
    (this->_buffer.ptr[index] = ((this->_buffer.ptr[index] & ((uint8_t)(0x0F))) | ((uint8_t)((color << 4)))));
  } else {
    (this->_buffer.ptr[index] = ((this->_buffer.ptr[index] & ((uint8_t)(0xF0))) | ((uint8_t)((color & 0x0F)))));
  }
}

static void Canvas__set_pixel_index8(Canvas* this, int32_t x, int32_t y, uint16_t color) {
  if (((((x < this->_viewpoint.x) || (x >= (this->_viewpoint.x + this->_viewpoint.width))) || (y < this->_viewpoint.y)) || (y >= (this->_viewpoint.y + this->_viewpoint.height)))) {
    return;
  }
  int32_t lx = (x - this->_viewpoint.x);
  int32_t ly = (y - this->_viewpoint.y);
  (this->_buffer.ptr[((ly * this->_viewpoint.width) + lx)] = ((uint8_t)(color)));
}

static void Canvas__render_index1(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset) {
  for (int32_t y = viewpoint->y; y < (viewpoint->y + viewpoint->height); y++) {
    int32_t ly = ((y - this->_viewpoint.y) - offset->y);
    for (int32_t x = viewpoint->x; x < (viewpoint->x + viewpoint->width); x++) {
      int32_t lx = ((x - this->_viewpoint.x) - offset->x);
      int32_t index = ((ly * ((this->_viewpoint.width + 7) / 8)) + (lx / 8));
      uint16_t color = this->_palette.ptr[0];
      if (((this->_buffer.ptr[index] & ((uint8_t)((0x80 >> (lx & 7))))) != 0)) {
        (color = this->_palette.ptr[1]);
      }
      if ((color != palette__TRANSPARENT)) {
        Point point = (Point){x, y};
        RenderContext_set_pixel(context, (&point), color);
      }
    }
  }
}

static void Canvas__render_index2(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset) {
  for (int32_t y = viewpoint->y; y < (viewpoint->y + viewpoint->height); y++) {
    int32_t ly = ((y - this->_viewpoint.y) - offset->y);
    for (int32_t x = viewpoint->x; x < (viewpoint->x + viewpoint->width); x++) {
      int32_t lx = ((x - this->_viewpoint.x) - offset->x);
      int32_t index = ((ly * ((this->_viewpoint.width + 3) / 4)) + (lx / 4));
      int32_t shift = ((3 - (lx & 3)) * 2);
      uint16_t color = this->_palette.ptr[((((int32_t)(this->_buffer.ptr[index])) >> shift) & 0x03)];
      if ((color != palette__TRANSPARENT)) {
        Point point = (Point){x, y};
        RenderContext_set_pixel(context, (&point), color);
      }
    }
  }
}

static void Canvas__render_index4(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset) {
  for (int32_t y = viewpoint->y; y < (viewpoint->y + viewpoint->height); y++) {
    int32_t ly = ((y - this->_viewpoint.y) - offset->y);
    for (int32_t x = viewpoint->x; x < (viewpoint->x + viewpoint->width); x++) {
      int32_t lx = ((x - this->_viewpoint.x) - offset->x);
      int32_t index = ((ly * ((this->_viewpoint.width + 1) / 2)) + (lx / 2));
      uint8_t palette_index = (this->_buffer.ptr[index] & 0x0F);
      if (((lx & 1) == 0)) {
        (palette_index = ((this->_buffer.ptr[index] >> 4) & 0x0F));
      }
      uint16_t color = this->_palette.ptr[palette_index];
      if ((color != palette__TRANSPARENT)) {
        Point point = (Point){x, y};
        RenderContext_set_pixel(context, (&point), color);
      }
    }
  }
}

static void Canvas__render_index8(Canvas* this, RenderContext* context, Rect* viewpoint, Point* offset) {
  for (int32_t y = viewpoint->y; y < (viewpoint->y + viewpoint->height); y++) {
    int32_t ly = ((y - this->_viewpoint.y) - offset->y);
    for (int32_t x = viewpoint->x; x < (viewpoint->x + viewpoint->width); x++) {
      int32_t lx = ((x - this->_viewpoint.x) - offset->x);
      uint16_t color = this->_palette.ptr[this->_buffer.ptr[((ly * this->_viewpoint.width) + lx)]];
      if ((color != palette__TRANSPARENT)) {
        Point point = (Point){x, y};
        RenderContext_set_pixel(context, (&point), color);
      }
    }
  }
}

static inline int32_t Canvas__abs(Canvas* this, int32_t v) {
  if ((v < 0)) {
    return (-v);
  }
  return v;
}

static inline int32_t Canvas__sign(Canvas* this, int32_t v) {
  if ((v > 0)) {
    return 1;
  }
  if ((v < 0)) {
    return (-1);
  }
  return 0;
}

static inline int32_t Canvas__sqrt(Canvas* this, int32_t n) {
  if ((n <= 0)) {
    return 0;
  }
  int32_t x = n;
  int32_t y = ((x + 1) / 2);
  while ((y < x)) {
    (x = y);
    (y = ((x + (n / x)) / 2));
  }
  return x;
}

void Allocator_init(Allocator* this, __Slice_uint8_t mem) {
  (this->_memory = mem);
  (this->_cursor = 0);
}

static inline void* Allocator_allocate(Allocator* this, size_t __sizeof_T) {
  uint64_t size = __sizeof_T;
  if (((this->_cursor + size) > this->_memory.size)) {
    return NULL;
  }
  void* ptr = (void*)((&this->_memory.ptr[this->_cursor]));
  (this->_cursor = (((this->_cursor + size) + 3) & (~((int32_t)(3)))));
  return ptr;
}

static inline int32_t Allocator_available(Allocator* this) {
  return (this->_memory.size - this->_cursor);
}

static inline void Allocator_reset(Allocator* this) {
  (this->_cursor = 0);
}

static inline int32_t math__floor_q16(int32_t value) {
  return (value & (-65536));
}

static inline int32_t math__ceil_q16(int32_t value) {
  int32_t result = (value & (-65536));
  if ((value != result)) {
    return (result + 65536);
  }
  return result;
}

static inline int32_t math__round_q16(int32_t value) {
  return ((value + 32768) & (-65536));
}

static inline int32_t math__floor_fixed(int32_t value) {
  return (value & (-65536));
}

static inline int32_t math__ceil_fixed(int32_t value) {
  int32_t result = (value & (-65536));
  if ((value != result)) {
    return (result + 65536);
  }
  return result;
}

static inline int32_t math__round_fixed(int32_t value) {
  return ((value + 32768) & (-65536));
}

static inline bool RenderContext_intersect(RenderContext* this, Rect* rect) {
  return Rect_intersect((&this->viewpoint), rect);
}

void RenderContext_set_pixel(RenderContext* this, Point* point, uint16_t color) {
  if ((!Rect_contains((&this->viewpoint), point))) {
    return;
  }
  int32_t px = (point->x - this->viewpoint.x);
  int32_t py = (point->y - this->viewpoint.y);
  if ((this->format == PixelFormat_Mono)) {
    RenderContext__set_pixel_mono(this, px, py, color);
  } else {
    RenderContext__set_pixel_rgb565(this, px, py, color);
  }
}

void RenderContext_fill_rect(RenderContext* this, Rect* rect, uint16_t color) {
  for (int32_t py = rect->y; py < (rect->y + rect->height); py++) {
    if (((py >= this->viewpoint.y) && (py < (this->viewpoint.y + this->viewpoint.height)))) {
      for (int32_t px = rect->x; px < (rect->x + rect->width); px++) {
        if ((this->format == PixelFormat_Mono)) {
          RenderContext__set_pixel_mono(this, px, (py - this->viewpoint.y), color);
        } else {
          RenderContext__set_pixel_rgb565(this, px, (py - this->viewpoint.y), color);
        }
        (px += 1);
      }
    }
  }
}

static inline void RenderContext__set_pixel_mono(RenderContext* this, int32_t x, int32_t y, uint16_t color) {
  int32_t index = (((y * (this->viewpoint.width + 7)) / 8) + (x / 8));
  if ((color == 0)) {
    (this->buffer.ptr[index] = (this->buffer.ptr[index] & ((uint8_t)((~(0x80 >> (x & 7)))))));
  } else {
    (this->buffer.ptr[index] = (this->buffer.ptr[index] | ((uint8_t)((0x80 >> (x & 7))))));
  }
}

static inline void RenderContext__set_pixel_rgb565(RenderContext* this, int32_t x, int32_t y, uint16_t color) {
  int32_t index = (((y * this->viewpoint.width) + x) * 2);
  (this->buffer.ptr[index] = ((uint8_t)((color >> 8))));
  (this->buffer.ptr[(index + 1)] = ((uint8_t)(color)));
}

static inline void memory__memory_set(__Slice_uint8_t dst, uint8_t value) {
  memset(dst.ptr, value, dst.size);
}

static inline void memory__memory_copy(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size) {
  memcpy(dst.ptr, src.ptr, size);
}

static inline void memory__memory_move(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size) {
  memmove(dst.ptr, src.ptr, size);
}

static inline void memory__memory_zero(__Slice_uint8_t dst) {
  memory__memory_set(dst, ((uint8_t)(0)));
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

static inline int32_t math__min_int32_t(int32_t a, int32_t b) {
  if ((a < b)) {
    return a;
  }
  return b;
}

static inline int32_t math__max_int32_t(int32_t a, int32_t b) {
  if ((a > b)) {
    return a;
  }
  return b;
}

static inline Canvas* Allocator_allocate_Canvas(Allocator* this) {
  uint64_t size = sizeof(Canvas);
  if (((this->_cursor + size) > this->_memory.size)) {
    return NULL;
  }
  Canvas* ptr = ((Canvas*)((&this->_memory.ptr[this->_cursor])));
  (this->_cursor = (((this->_cursor + size) + 3) & (~((int32_t)(3)))));
  return ptr;
}

static inline __Slice_uint8_t Allocator_allocate_array_uint8_t(Allocator* this, int32_t length) {
  uint64_t size = (sizeof(uint8_t) * length);
  if (((this->_cursor + size) > this->_memory.size)) {
    return (__Slice_uint8_t){NULL, 0};
  }
  uint8_t* ptr = ((uint8_t*)((&this->_memory.ptr[this->_cursor])));
  (this->_cursor = (((this->_cursor + size) + 3) & (~((int32_t)(3)))));
  return (__Slice_uint8_t){ptr, length};
}

void app_main(void) { main__main(); }

