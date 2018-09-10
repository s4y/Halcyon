#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_config.h"

#include "nrf_log.h"
#include "nrf_uarte.h"
#include "app_timer.h"
#include "nrf_log_ctrl.h"
#include "nrf_gpio.h"

#include "bridge.h"
#include "halcyon_boards.h"

static uint8_t rx_buf[8];
#define RX_BUF_COUNT (sizeof(rx_buf) / sizeof(*rx_buf))

APP_TIMER_DEF(rx_timeout_timer);
APP_TIMER_DEF(rx_sync_timer);

static enum {
  RX_STATE_WAIT_SYNC,
  RX_STATE_WAIT_PACKET,
  RX_STATE_READ_PACKET,
} rx_state;

static void wait_sync() {
  rx_state = RX_STATE_WAIT_SYNC;
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_STOPRX);
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_STARTRX);
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_FLUSHRX);
  app_timer_stop(rx_timeout_timer);
  app_timer_stop(rx_sync_timer);
  app_timer_start(rx_sync_timer, 300, NULL);
}

static void handle_rx_timeout_timer() {
  wait_sync();
}

static void handle_rx_sync_timer() {
  rx_state = RX_STATE_WAIT_PACKET;
}

static inline bool check_clear(nrf_uarte_event_t event) {
  bool was_set = nrf_uarte_event_check(UARTE, event);
  nrf_uarte_event_clear(UARTE, event);
  return was_set;
}

void UARTE0_UART0_IRQHandler() {
  if (check_clear(NRF_UARTE_EVENT_ERROR)) {
    uint32_t errorsrc = nrf_uarte_errorsrc_get_and_clear(UARTE);
    NRF_LOG_INFO("UART RX error: %d", errorsrc);
    wait_sync();
  }
  if (check_clear(NRF_UARTE_EVENT_RXSTARTED)) {
    nrf_uarte_int_enable(UARTE, NRF_UARTE_INT_RXDRDY_MASK);
  }
  if (check_clear(NRF_UARTE_EVENT_RXDRDY)) {
    switch (rx_state) {
      case RX_STATE_WAIT_SYNC:
        wait_sync();
        break;
      case RX_STATE_WAIT_PACKET:
        app_timer_start(rx_timeout_timer, APP_TIMER_TICKS(200), NULL);
        nrf_uarte_int_disable(UARTE, NRF_UARTE_INT_RXDRDY_MASK);
        rx_state = RX_STATE_READ_PACKET;
        break;
      case RX_STATE_READ_PACKET:
        break;
    }
  }
  if (check_clear(NRF_UARTE_EVENT_ENDRX)) {
    app_timer_stop(rx_timeout_timer);
    if (rx_state == RX_STATE_READ_PACKET) {
      rx_state = RX_STATE_WAIT_PACKET;
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
      halcyon_bridge_rx(rx_buf, sizeof(rx_buf));
    }
  }
  if (check_clear(NRF_UARTE_EVENT_ENDTX)) {
  }
  if (check_clear(NRF_UARTE_EVENT_TXSTARTED)) {
  }
  if (check_clear(NRF_UARTE_EVENT_TXSTOPPED)) {
  }
  NRF_LOG_FLUSH();
}

void halcyon_bridge_init() {
  // Set up the UART, which interfaces with the bus.

  app_timer_create(&rx_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, &handle_rx_timeout_timer);
  app_timer_create(&rx_sync_timer, APP_TIMER_MODE_SINGLE_SHOT, &handle_rx_sync_timer);

  nrf_gpio_cfg_output(TX_PIN);
  nrf_gpio_pin_set(TX_PIN);
  nrf_gpio_cfg_input(RX_PIN, NRF_GPIO_PIN_NOPULL);

  nrf_uarte_txrx_pins_set(UARTE, TX_PIN, RX_PIN);
  nrf_uarte_configure(UARTE, NRF_UARTE_PARITY_INCLUDED, NRF_UARTE_HWFC_DISABLED);
  nrf_uarte_baudrate_set(UARTE, 0x21000); // Roughly 500 baud
  nrf_uarte_rx_buffer_set(UARTE, rx_buf, sizeof(rx_buf));
  nrf_uarte_int_enable(UARTE, NRF_UARTE_INT_ENDRX_MASK | NRF_UARTE_INT_ENDTX_MASK | NRF_UARTE_INT_RXSTARTED_MASK);
  nrf_uarte_shorts_enable(UARTE, NRF_UARTE_SHORT_ENDRX_STARTRX);
  nrf_uarte_enable(UARTE);

  NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(UARTE),
      NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  NRFX_IRQ_ENABLE(nrfx_get_irq_number(UARTE));

  wait_sync();
}
