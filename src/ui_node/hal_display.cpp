#include "hal_display.h"

#include <Arduino.h>
#include "pin_config.h"
#include "xl9555.h"
#include "gt911.h"

// ESP-IDF headers, exposed by the arduino-esp32 core (verified against IDF 5.x)
#include "esp_lcd_panel_rgb.h" // esp_lcd_rgb_panel_config_t
#include "esp_lcd_panel_ops.h" // esp_lcd_new_rgb_panel / init / draw_bitmap
#include "hal/lcd_types.h"     // LCD_CLK_SRC_DEFAULT
#include "soc/gpio_num.h"      // GPIO_NUM_NC
#include "esp_heap_caps.h"

static XL9555 io_exp;
static GT911 touch;

static esp_lcd_panel_handle_t s_panel = nullptr;
static lv_display_t *s_disp = nullptr;
static lv_indev_t *s_indev = nullptr;

// ------------------------------------------------------------------ LVGL flush
// LVGL renders RGB565 partial areas; copy them into the RGB panel framebuffer.
// (LVGL's little-endian RGB565 matches the S3 RGB DMA byte order — no swap.)
static void flush_cb(lv_display_t *disp, const lv_area_t *area,
                     uint8_t *px_map) {
  if (s_panel) {
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1, area->x2 + 1,
                              area->y2 + 1, px_map);
  }
  lv_display_flush_ready(disp);
}

// ----------------------------------------------------------- LVGL touch indev
static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  int16_t x, y;
  if (touch.readTouch(x, y)) {
    if (TOUCH_SWAP_XY) {
      int16_t t = x;
      x = y;
      y = t;
    }
    if (TOUCH_INVERT_X)
      x = LCD_H_RES - 1 - x;
    if (TOUCH_INVERT_Y)
      y = LCD_V_RES - 1 - y;
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// ---------------------------------------------------------------- init helpers
static bool expander_and_touch_init() {
  if (!io_exp.begin(Wire, 0x20)) {
    Serial.println("[HAL] XL9555 expander not found on 0x20");
    return false;
  }
  // EXIO1 = TP_RST and EXIO2 = DISP (backlight) become outputs, both high.
  if (!io_exp.setOutputs((1 << EXIO1_TP_RST) | (1 << EXIO2_DISP))) {
    Serial.println("[HAL] failed to configure expander outputs");
    return false;
  }
  io_exp.write(0xFFFF); // touch reset released, backlight off for now

  // GT911 reset pulse through the expander
  io_exp.write(0xFFFF & ~(1 << EXIO1_TP_RST)); // TP_RST low
  delay(10);
  io_exp.write(0xFFFF); // TP_RST high
  delay(60);

  if (!touch.begin(Wire))
    Serial.println("[HAL] GT911 not found (continuing without touch)");
  else
    Serial.println("[HAL] GT911 found");
  return true;
}

static bool panel_init() {
  // data lane order for the 16-bit bus: B3..B7, G2..G7, R3..R7
  gpio_num_t data_pins[16] = {
      (gpio_num_t)PIN_LCD_B3, (gpio_num_t)PIN_LCD_B4, (gpio_num_t)PIN_LCD_B5,
      (gpio_num_t)PIN_LCD_B6, (gpio_num_t)PIN_LCD_B7, (gpio_num_t)PIN_LCD_G2,
      (gpio_num_t)PIN_LCD_G3, (gpio_num_t)PIN_LCD_G4, (gpio_num_t)PIN_LCD_G5,
      (gpio_num_t)PIN_LCD_G6, (gpio_num_t)PIN_LCD_G7, (gpio_num_t)PIN_LCD_R3,
      (gpio_num_t)PIN_LCD_R4, (gpio_num_t)PIN_LCD_R5, (gpio_num_t)PIN_LCD_R6,
      (gpio_num_t)PIN_LCD_R7};

  esp_lcd_rgb_panel_config_t cfg = {};
  cfg.clk_src = LCD_CLK_SRC_DEFAULT;
  cfg.timings.pclk_hz = LCD_PCLK_HZ;
  cfg.timings.h_res = LCD_H_RES;
  cfg.timings.v_res = LCD_V_RES;
  cfg.timings.hsync_pulse_width = LCD_HSW;
  cfg.timings.hsync_back_porch = LCD_HBP;
  cfg.timings.hsync_front_porch = LCD_HFP;
  cfg.timings.vsync_pulse_width = LCD_VSW;
  cfg.timings.vsync_back_porch = LCD_VBP;
  cfg.timings.vsync_front_porch = LCD_VFP;
  cfg.timings.flags.pclk_active_neg = 1; // ST7262 samples pclk on falling edge
  cfg.data_width = 16;                   // RGB565 in parallel mode
  cfg.bits_per_pixel = 16;
  cfg.num_fbs = 1;
  cfg.psram_trans_align = 64;
  cfg.hsync_gpio_num = PIN_LCD_HSYNC;
  cfg.vsync_gpio_num = PIN_LCD_VSYNC;
  cfg.de_gpio_num = PIN_LCD_DE;
  cfg.pclk_gpio_num = PIN_LCD_PCLK;
  cfg.disp_gpio_num = GPIO_NUM_NC; // no power-enable pin; backlight via XL9555
  for (int i = 0; i < 16; i++)
    cfg.data_gpio_nums[i] = data_pins[i];
  cfg.flags.fb_in_psram = 1; // 800*480*2 = 750 kB -> must live in PSRAM

  if (esp_lcd_new_rgb_panel(&cfg, &s_panel) != ESP_OK) {
    Serial.println("[HAL] esp_lcd_new_rgb_panel failed");
    return false;
  }
  if (esp_lcd_panel_init(s_panel) != ESP_OK) {
    Serial.println("[HAL] esp_lcd_panel_init failed");
    return false;
  }

  // Backlight on (EXIO2 high). If your panel stays dark, flip this polarity.
  io_exp.write(0xFFFF);
  return true;
}

static bool lvgl_init() {
  lv_init();

  s_disp = lv_display_create(LCD_H_RES, LCD_V_RES);
  lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);

  // Two partial-render buffers of 40 lines each, in fast internal RAM
  const size_t buf_bytes = (size_t)LCD_H_RES * 40 * 2;
  void *b1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  void *b2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!b1 || !b2) {
    Serial.println("[HAL] LVGL draw buffer allocation failed");
    return false;
  }
  lv_display_set_buffers(s_disp, b1, b2, buf_bytes,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(s_disp, flush_cb);

  s_indev = lv_indev_create(LV_INDEV_TYPE_POINTER, LV_INDEV_MODE_TIMER);
  lv_indev_set_read_cb(s_indev, touch_cb);
  lv_indev_set_display(s_indev, s_disp);
  return true;
}

bool display_init() {
  Wire.begin(PIN_TP_SDA, PIN_TP_SCL, 400000U);
  if (!expander_and_touch_init())
    return false;
  if (!panel_init())
    return false;
  if (!lvgl_init())
    return false;
  Serial.println("[HAL] display ready");
  return true;
}

void display_loop() { lv_timer_handler(); }
