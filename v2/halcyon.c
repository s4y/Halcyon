#include "app_config.h"

#include "halcyon/ble.h"

#include "app_error.h"
#include "nordic_common.h"

#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
// #include "app_scheduler.h"

#include "nrf_log_default_backends.h"

#include "halcyon_boards.h"
#include "halcyon/bridge.h"
#include "halcyon/invert.h"
#include "halcyon/logging.h"

#define DEVICE_NAME "Halcyon Bridge"

// APP_TIMER_DEF(tick_timer);
// 
// struct {
// 	uint32_t ticker_value;
// 	uint16_t value_handle;
// 	size_t conn_count;
// 	uint16_t conn_handles[NRF_SDH_BLE_PERIPHERAL_LINK_COUNT];
// } g_tick_context;
// 
// void tick_handler(void* context) {
// 	uint16_t hvx_len = 7;
// 	ble_gatts_hvx_params_t hvx_params = {
// 		.handle = g_tick_context.value_handle,
// 		.type   = BLE_GATT_HVX_NOTIFICATION,
// 		.offset = 0,
// 		.p_len  = &hvx_len,
// 		.p_data = (uint8_t[]){0x00, 0x00, 0x00, g_tick_context.ticker_value >> 24, g_tick_context.ticker_value >> 16, g_tick_context.ticker_value >> 8, g_tick_context.ticker_value},
// 	};
// 	for (size_t i = 0; i < g_tick_context.conn_count; i++) {
// 		uint16_t conn_handle = g_tick_context.conn_handles[i];
// 		int err = sd_ble_gatts_hvx(conn_handle, &hvx_params);
// 		if (
// 			err != NRF_ERROR_INVALID_STATE &&
// 			err != BLE_ERROR_GATTS_SYS_ATTR_MISSING
// 	   ) APP_ERROR_CHECK(err);
// 	}
// 	g_tick_context.ticker_value++;
// }

int main(void) {
	NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
	// log_init();

    APP_ERROR_CHECK(nrf_pwr_mgmt_init());
	APP_ERROR_CHECK(nrf_sdh_enable_request());
	// APP_ERROR_CHECK(app_timer_init());
	// APP_SCHED_INIT(APP_TIMER_SCHED_EVENT_DATA_SIZE, 16);

	// halcyon_bridge_t bridge = {NRFX_UARTE_INSTANCE(0)};
	// halcyon_bridge_init(&bridge);

	invert_init();

	ble_init();

	for (;;) {
		NRF_LOG_FLUSH();
		// app_sched_execute();
		nrf_pwr_mgmt_run();
	}
}
