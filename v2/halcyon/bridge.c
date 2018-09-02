#include <inttypes.h>

#include "app_config.h"

#include "nrf_log.h"
#include "nrf_gpio.h"

#include "bridge.h"
#include "halcyon_boards.h"

static void halcyon_bridge_rx(halcyon_bridge_t* bridge) {
	nrfx_uarte_rx(
			&bridge->uarte,
			bridge->rx_buf,
			sizeof(bridge->rx_buf) / sizeof(*bridge->rx_buf));
}

static void halcyon_bridge_handle_uarte_event(
	nrfx_uarte_event_t const *p_event, void *p_context
) {
	halcyon_bridge_t *bridge = p_context;
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
				uint8_t* buf = p_event->data.rxtx.p_data;
#if NRF_LOG_LEVEL >= NRF_LOG_SEVERITY_INFO
				char buf_str[17];
				snprintf(
						buf_str, sizeof(buf_str) / sizeof(*buf_str),
						"%02x%02x%02x%02x%02x%02x%02x%02x",
						buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
				NRF_LOG_INFO("UARTE rx: %s", buf_str);
#endif
				halcyon_bridge_rx(bridge);
			} break;
		case NRFX_UARTE_EVT_ERROR:
			{
				NRF_LOG_ERROR("UARTE error: %"PRIu32, p_event->data.error.error_mask);
			} break;
	}
}

void halcyon_bridge_init(halcyon_bridge_t* bridge) {
	// Set up the UART, which interfaces with the bus.
	nrfx_uarte_config_t cfg = NRFX_UARTE_DEFAULT_CONFIG;
	cfg.pseltxd = TX_PIN;
	cfg.pselrxd = RX_PIN;
	// Custom baud rate around 500.
	cfg.baudrate = 0x21000;
	APP_ERROR_CHECK(nrfx_uarte_init(&bridge->uarte, &cfg, halcyon_bridge_handle_uarte_event));
	halcyon_bridge_rx(bridge);
}

