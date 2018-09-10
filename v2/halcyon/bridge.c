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

// inline halcyon_bus_node_state_t node_state_parse(uint8_t* buf) {
//   return (halcyon_bus_node_state_t) {
//     .byte0 = buf[0],
//     .byte1 = buf[1],
//     .byte2 = buf[2],
//     .byte3 = buf[3],
//     .byte4 = buf[4],
//     .byte5 = buf[5],
//     .byte6 = buf[6],
//     .byte7 = buf[7],
//   };
// }

// inline bool node_state_equals(halcyon_bus_node_state_t a, halcyon_bus_node_state_t b) {
//   return memcmp(a.buf, b.buf, sizeof(a.buf)) == 0;
// }

halcyon_bus_node_state_t halcyon_bus_node[MAX_HALCYON_BUS_NODE];

halcyon_bus_node_state_t blower_node_state;
halcyon_bus_node_state_t first_thermostat_node_state;
halcyon_bus_node_state_t second_thirmostat_node_state;

static enum {
  RX_STATE_WAIT_SYNC,
  RX_STATE_WAIT_PACKET,
  RX_STATE_READ_PACKET,
} rx_state;

uint8_t rx_buf[HALCYON_BRIDGE_PACKET_LENGTH];

APP_TIMER_DEF(rx_timeout_timer);
APP_TIMER_DEF(rx_sync_timer);

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
      for (size_t i = 0; i < HALCYON_BRIDGE_PACKET_LENGTH; i++)
        rx_buf[i] = ~rx_buf[i];
#if NRF_LOG_LEVEL >= NRF_LOG_SEVERITY_ERROR
      char buf_str[HALCYON_BRIDGE_PACKET_LENGTH * 2 + 1];
      snprintf(
          buf_str, sizeof(buf_str) / sizeof(*buf_str),
          "%02x%02x%02x%02x%02x%02x%02x%02x",
          rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7]);
      NRF_LOG_INFO("UARTE rx: %s", buf_str);
#endif
      halcyon_bridge_rx_cb();

      halcyon_bus_node_t node_id;
      halcyon_bus_node_state_t* node_state = NULL;
      switch (rx_buf[0]) {
        case 0x00:
          node_id = 0;
          node_state = &halcyon_bus_node[node_id];
          break;
        case 0x20:
          node_id = 1;
          node_state = &halcyon_bus_node[node_id];
          break;
        case 0x21:
          node_id = 1;
          node_state = &halcyon_bus_node[node_id];
          break;
      }
      if (node_state) {
        if (memcmp(node_state->buf, rx_buf + 1, sizeof(node_state->buf)) != 0) {
          memcpy(node_state->buf, rx_buf + 1, sizeof(node_state->buf));
          halcyon_bridge_state_change_cb(node_id);
        }
      } else {
        NRF_LOG_ERROR("Got a packet for an unknown node: %s", buf_str);
      }
      // buf_str will no longer exist after this line, so flush any logs that need it.
      NRF_LOG_FLUSH();
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
