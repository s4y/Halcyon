#include <inttypes.h>

#include "app_config.h"

#include "nrf_log.h"
#include "app_timer.h"
#include "nrf_log_ctrl.h"
#include "nrf_gpio.h"

#include "bridge.h"
#include "halcyon_boards.h"

static const nrfx_uarte_t g_uart = NRFX_UARTE_INSTANCE(0);
static uint8_t rx_buf[8];
static bool need_sync = true;
#define RX_BUF_COUNT (sizeof(rx_buf) / sizeof(*rx_buf))

APP_TIMER_DEF(rx_timeout_timer);
APP_TIMER_DEF(rx_sync_timer);

static void sync() {
  NRF_LOG_INFO("%s %d", __func__, app_timer_cnt_get())
  need_sync = true;
  nrfx_uarte_rx_abort(&g_uart);
  app_timer_stop(rx_sync_timer);
  app_timer_start(rx_sync_timer, 300, NULL);
}

static void bridge_rx() {
  NRF_LOG_INFO("%s %d", __func__, app_timer_cnt_get())
  need_sync = false;
  APP_ERROR_CHECK(nrfx_uarte_rx(&g_uart, rx_buf, 1));
  APP_ERROR_CHECK(nrfx_uarte_rx(&g_uart, rx_buf + 1, sizeof(rx_buf)));
}

static void halcyon_bridge_handle_uarte_event(
  nrfx_uarte_event_t const *p_event, void *p_context
) {
  NRF_LOG_INFO("%s %d", __func__, app_timer_cnt_get())
  switch (p_event->type) {
    case NRFX_UARTE_EVT_TX_DONE:
      {
        uint8_t* buf = p_event->data.rxtx.p_data;
#if NRF_LOG_LEVEL >= NRF_LOG_SEVERITY_INFO
        char buf_str[17];
        snprintf(
            buf_str, sizeof(buf_str) / sizeof(*buf_str),
            "%02x%02x%02x%02x%02x%02x%02x%02x",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        NRF_LOG_INFO("UARTE tx: %s", buf_str);
#endif
      } break;
    case NRFX_UARTE_EVT_RX_DONE:
      {
        nrfx_uarte_xfer_evt_t rx = p_event->data.rxtx;
        if (need_sync || rx.bytes != sizeof(rx_buf)) {
          sync();
          break;
        }
        if (rx.bytes == 1) {
          app_timer_start(rx_timeout_timer, APP_TIMER_TICKS(200), NULL);
          break;
        }
        NRFX_ASSERT(rx.p_data == rx_buf + 1);
        NRFX_ASSERT(rx.bytes == RX_BUF_COUNT - 1);
        app_timer_stop(rx_timeout_timer);

        for (size_t i = 0; i < RX_BUF_COUNT; i++)
          rx_buf[i] = ~rx_buf[i];

#if NRF_LOG_LEVEL >= NRF_LOG_SEVERITY_INFO
        char buf_str[17];
        snprintf(
            buf_str, sizeof(buf_str) / sizeof(*buf_str),
            "%02x%02x%02x%02x%02x%02x%02x%02x",
            rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7]);
        NRF_LOG_INFO("UARTE rx: %s", buf_str);
        NRF_LOG_FLUSH();
#endif
        bridge_rx();
      } break;
    case NRFX_UARTE_EVT_ERROR:
      {
        NRF_LOG_ERROR("UARTE error: %"PRIu32, p_event->data.error.error_mask);
        sync();
      } break;
  }
}

void halcyon_bridge_init(halcyon_bridge_t* bridge) {
  // Set up the UART, which interfaces with the bus.

  nrf_gpio_cfg_output(TX_PIN);
  nrf_gpio_pin_set(TX_PIN);
  nrf_gpio_cfg_input(RX_PIN, NRF_GPIO_PIN_NOPULL);

  static nrfx_uarte_config_t cfg = NRFX_UARTE_DEFAULT_CONFIG;
  cfg.pseltxd = TX_PIN;
  cfg.pselrxd = RX_PIN;
  cfg.p_context = bridge;
  cfg.hwfc = NRF_UARTE_HWFC_DISABLED;
  cfg.parity = NRF_UARTE_PARITY_INCLUDED;
  // Custom baud rate around 500.
  cfg.baudrate = 0x21000;
  APP_ERROR_CHECK(nrfx_uarte_init(&bridge->uarte, &cfg, halcyon_bridge_handle_uarte_event));

  app_timer_create(&rx_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, &sync);
  app_timer_create(&rx_sync_timer, APP_TIMER_MODE_SINGLE_SHOT, &bridge_rx);

  sync();
}
