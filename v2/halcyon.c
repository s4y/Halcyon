#include "app_config.h"

#include "halcyon/halcyon_ble.h"

#include "app_error.h"
#include "app_timer.h"
#include "nordic_common.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"

#include "halcyon_boards.h"
#include "halcyon/bridge.h"
#include "halcyon/invert.h"

int main(void) {
	NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    APP_ERROR_CHECK(nrf_pwr_mgmt_init());
	APP_ERROR_CHECK(nrf_sdh_enable_request());
	APP_ERROR_CHECK(app_timer_init());

	halcyon_bridge_t bridge = {NRFX_UARTE_INSTANCE(0)};
	halcyon_bridge_init(&bridge);
	invert_init();

	halcyon_ble_config_t ble = {
		.services = (halcyon_ble_service_t*[]){
			&(halcyon_ble_service_t){
				.uuid = {0},
			},
			NULL
		},
	};

	halcyon_ble_init(&ble);

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
