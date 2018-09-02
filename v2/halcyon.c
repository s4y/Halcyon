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

	// const ble_uuid128_t vendor_uuid = { .uuid128 = {
	// 	0x79, 0x09, 0xe6, 0x9a, 0x90, 0x4d, 0x46, 0x19,
	// 	0x93, 0xac, 0x72, 0x2d, 0x1d, 0x13, 0xc7, 0xab,
	// } };
	// uint8_t uuid_type;
	// APP_ERROR_CHECK(sd_ble_uuid_vs_add(&vendor_uuid, &uuid_type));

	halcyon_ble_characteristic_t characteristics[] = {
		{
			.params = {
				.uuid = 0xB00F,
				.uuid_type = BLE_UUID_TYPE_BLE,
				.max_len = 8,
				.init_len = 8,
				.p_init_value = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
				.char_props = { .read = 1, .notify = 1 },
				.read_access = SEC_OPEN,
				.cccd_write_access = SEC_OPEN,
			},
		},
		{
			.params = {
				.uuid = 0xAC00,
				.uuid_type = BLE_UUID_TYPE_BLE,
				.max_len = 7,
				.init_len = 7,
				.p_init_value = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
				.char_props = { .read = 1, .notify = 1 },
				.read_access = SEC_OPEN,
				.cccd_write_access = SEC_OPEN,
			},
		},
		{
			.params = {
				.uuid = 0xAC20,
				.uuid_type = BLE_UUID_TYPE_BLE,
				.max_len = 7,
				.init_len = 7,
				.p_init_value = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
				.char_props = { .read = 1, .notify = 1 },
				.read_access = SEC_OPEN,
				.cccd_write_access = SEC_OPEN,
			},
		},
		{
			.params = {
				.uuid = 0xAC21,
				.uuid_type = BLE_UUID_TYPE_BLE,
				.max_len = 7,
				.init_len = 7,
				.p_init_value = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
				.char_props = { .read = 1, .notify = 1 },
				.read_access = SEC_OPEN,
				.cccd_write_access = SEC_OPEN,
			},
		},
		{
			.params = {
				.uuid = 0xACED,
				.uuid_type = BLE_UUID_TYPE_BLE,
				.max_len = 2,
				.init_len = 2,
				.p_init_value = (uint8_t[]){0x00, 0x00},
				.char_props = { .read = 1, .notify = 1, .write = 1 },
				.read_access = SEC_OPEN,
				.write_access = SEC_JUST_WORKS,
				.cccd_write_access = SEC_OPEN,
			},
		},
	};

	halcyon_ble_service_t services[] = {
		{
			.uuid = {0xACAC, BLE_UUID_TYPE_BLE},
			.characteristics = characteristics,
			.n_characteristics = sizeof(characteristics) / sizeof(*characteristics),
		},
	};

	halcyon_ble_config_t ble = {
		.services = services,
		.n_services = sizeof(services) / sizeof(*services),
		.allow_bonding = false, // TODO: button
		.delete_bonds = false, // TODO: button
	};

	halcyon_ble_init(&ble);

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
