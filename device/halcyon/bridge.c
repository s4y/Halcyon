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

static enum {
  RX_STATE_WAIT_SYNC,
  RX_STATE_WAIT_PACKET,
  RX_STATE_READ_PACKET,
} rx_state;

halcyon_bus_t halcyon_bus = {
  { .address = 0x00 },
  { .address = 0x20 },
  { .address = 0x21, .byte1 = 0x82, },
};

uint8_t halcyon_bridge_last_rx_buf[HALCYON_BRIDGE_PACKET_LENGTH];
uint8_t halcyon_bridge_state_buf[3] = {0};

static uint8_t rx_buf[HALCYON_BRIDGE_PACKET_LENGTH];
static uint8_t tx_buf[HALCYON_BRIDGE_PACKET_LENGTH];

APP_TIMER_DEF(rx_timeout_timer);
APP_TIMER_DEF(rx_sync_timer);
APP_TIMER_DEF(tx_delay_timer);

static void wait_sync() {
  rx_state = RX_STATE_WAIT_SYNC;
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_STOPRX);
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_STARTRX);
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_FLUSHRX);
  app_timer_stop(rx_timeout_timer);
  app_timer_stop(rx_sync_timer);
  app_timer_start(rx_sync_timer, APP_TIMER_TICKS(200), NULL);
}

static void handle_rx_timeout_timer() {
  wait_sync();
}

static void handle_rx_sync_timer() {
  rx_state = RX_STATE_WAIT_PACKET;
}

static void handle_tx() {
  memcpy(tx_buf, &halcyon_bus.remote1, sizeof(tx_buf));
  memcpy(tx_buf + 2, halcyon_bridge_state_buf, sizeof(halcyon_bridge_state_buf));
  for (size_t i = 0; i < sizeof(tx_buf) / sizeof(*tx_buf); i++)
    tx_buf[i] = ~tx_buf[i];
  nrf_uarte_task_trigger(UARTE, NRF_UARTE_TASK_STARTTX);
  if (halcyon_bridge_state_buf[0] & 0x08)  {
    halcyon_bridge_state_buf[0] &= ~(uint8_t)0x08;
    halcyon_bridge_state_change_cb();
  }
}

static void handle_rx() {
#if NRF_LOG_LEVEL >= NRF_LOG_SEVERITY_ERROR
      char buf_str[HALCYON_BRIDGE_PACKET_LENGTH * 3 + 1];
      snprintf(
          buf_str, sizeof(buf_str) / sizeof(*buf_str),
          "%02x %02x %02x %02x %02x %02x %02x %02x",
          rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7]);
      NRF_LOG_INFO("UARTE rx: %s", buf_str);
      NRF_LOG_FLUSH();
#endif

  memcpy(halcyon_bridge_last_rx_buf, rx_buf, sizeof(halcyon_bridge_last_rx_buf));

  halcyon_node_state_t* node_state = NULL;
  switch (rx_buf[0]) {
    case 0x00:
      node_state = &halcyon_bus.blower;
      break;
    case 0x20:
      node_state = &halcyon_bus.remote0;
      break;
    case 0x21:
      node_state = &halcyon_bus.remote1;
      break;
    default:
      break;
  }
  if (node_state) {
    if (memcmp(node_state, rx_buf, sizeof(*node_state)) != 0) {
      memcpy(node_state, rx_buf, sizeof(*node_state));
      halcyon_bridge_node_change_cb(node_state);
    }
  } else {
    NRF_LOG_ERROR("Got a packet for an unknown node: %s", buf_str);
    NRF_LOG_FLUSH();
  }

  halcyon_bridge_rx_cb();

  halcyon_node_state_t state = *(halcyon_node_state_t*)rx_buf;

  // If remote0 just spoke and wants remote1 to speak,
  if (state.address == 0x20 && state.byte1 == 0xa1) {

    // If remote1 isn't planning to change state, and doesn't match remote0,
    if (!(halcyon_bridge_state_buf[0] & 0x08) && (halcyon_bridge_state_buf[1] != state.state0 || halcyon_bridge_state_buf[2] != state.state1)) {
      // then copy remote0's state.
      halcyon_bridge_state_buf[1] = state.state0;
      halcyon_bridge_state_buf[2] = state.state1;
      halcyon_bridge_state_change_cb();
    }

    // (I forget what this is for and I'm just copying my old code. Will revise.)
    // halcyon_bus.remote1.byte5 = (state.byte5 & 0x10) ? 0x00 : 0x20;

    // Finally, schedule transmission.
    app_timer_start(tx_delay_timer, APP_TIMER_TICKS(50), NULL);
  }
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
      handle_rx();
    }
  }
  NRF_LOG_FLUSH();
}

void halcyon_bridge_init() {
  // Set up the UART, which interfaces with the bus.

  app_timer_create(&rx_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, &handle_rx_timeout_timer);
  app_timer_create(&rx_sync_timer, APP_TIMER_MODE_SINGLE_SHOT, &handle_rx_sync_timer);
  app_timer_create(&tx_delay_timer, APP_TIMER_MODE_SINGLE_SHOT, &handle_tx);

  nrf_gpio_cfg_output(TX_PIN);
  nrf_gpio_pin_set(TX_PIN);
  nrf_gpio_cfg_input(RX_PIN, NRF_GPIO_PIN_NOPULL);

  nrf_uarte_txrx_pins_set(UARTE, TX_PIN, RX_PIN);
  nrf_uarte_configure(UARTE, NRF_UARTE_PARITY_INCLUDED, NRF_UARTE_HWFC_DISABLED);
  nrf_uarte_baudrate_set(UARTE, 0x21000); // Roughly 500 baud
  nrf_uarte_rx_buffer_set(UARTE, rx_buf, sizeof(rx_buf));
  nrf_uarte_tx_buffer_set(UARTE, tx_buf, sizeof(tx_buf));
  nrf_uarte_int_enable(UARTE, NRF_UARTE_INT_ENDRX_MASK | NRF_UARTE_INT_RXSTARTED_MASK);
  nrf_uarte_shorts_enable(UARTE, NRF_UARTE_SHORT_ENDRX_STARTRX);
  nrf_uarte_enable(UARTE);

  NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(UARTE),
      NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  NRFX_IRQ_ENABLE(nrfx_get_irq_number(UARTE));

  wait_sync();
}
