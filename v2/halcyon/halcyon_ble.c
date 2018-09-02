#include "halcyon_ble.h"

#include "app_config.h"
#include "halcyon_boards.h"

#include "nordic_common.h"

#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_gatts.h"
#include "nrf_ble_gatt.h"
#include "nrf_log.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "softdevice/s132/headers/ble.h"

#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3
#define APP_ADV_INTERVAL 1024

#define MIN_CONN_INTERVAL MSEC_TO_UNITS(20, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL MSEC_TO_UNITS(75, UNIT_1_25_MS)
#define CONN_SUP_TIMEOUT MSEC_TO_UNITS(4000, UNIT_10_MS)

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)
#define MAX_CONN_PARAMS_UPDATE_COUNT    3

NRF_BLE_GATT_DEF(m_gatt);
BLE_ADVERTISING_DEF(m_advertising);

ble_gatts_char_handles_t buf0_handles = {0};

static void on_adv_evt(ble_adv_evt_t ble_adv_evt) {
	// NRF_LOG_WARNING("on_adv_evt(%d)\n", ble_adv_evt);

}

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {
	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			NRF_LOG_INFO("Connected");
			// g_tick_context.conn_handles[g_tick_context.conn_count++] = p_ble_evt->evt.gap_evt.conn_handle;
			// m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			NRF_LOG_INFO("Disconnected");
			break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
			{
				const ble_gap_phys_t phys = {
					.rx_phys = BLE_GAP_PHY_AUTO,
					.tx_phys = BLE_GAP_PHY_AUTO,
				};
				APP_ERROR_CHECK(sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys));
			}
			break;
		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
			APP_ERROR_CHECK(sd_ble_gap_conn_param_update(p_ble_evt->evt.gap_evt.conn_handle, &p_ble_evt->evt.gap_evt.params.conn_param_update_request.conn_params));
			break;
		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			// Pairing not supported (yet!)
			APP_ERROR_CHECK(sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL));
			break;
		case BLE_GAP_EVT_SEC_INFO_REQUEST:
			APP_ERROR_CHECK(sd_ble_gap_sec_info_reply(p_ble_evt->evt.gap_evt.conn_handle, NULL, NULL, NULL));
			break;
		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			// No system attributes have been stored.
			APP_ERROR_CHECK(sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, NULL, 0, 0));
			break;
		case BLE_GATTC_EVT_TIMEOUT:
			// Disconnect on GATT Client timeout event.
			APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
			break;
		case BLE_GATTS_EVT_TIMEOUT:
			// Disconnect on GATT Server timeout event.
			APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
			break;

		// case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
		// case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
        // case BLE_GATTC_EVT_EXCHANGE_MTU_RSP:
		// 	break;

		default:
			// NRF_LOG_ERROR("Unhandled event: 0x%x", p_ble_evt->header.evt_id);
			break;
	}
}

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt) {
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        APP_ERROR_CHECK(sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE));
    }
}

static void conn_params_error_handler(uint32_t nrf_error) {
    APP_ERROR_HANDLER(nrf_error);
}

void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt) {
	NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
			p_gatt->att_mtu_desired_central,
			p_gatt->att_mtu_desired_periph);
}

void halcyon_ble_init(halcyon_ble_config_t* config) {
	{
		uint32_t ram_start = 0;
		APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start));
		APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));
	}

    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

	{
		ble_gap_conn_sec_mode_t sec_mode;
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

		APP_ERROR_CHECK(sd_ble_gap_device_name_set(&sec_mode,
					(const uint8_t *) DEVICE_NAME,
					strlen(DEVICE_NAME)));

		ble_gap_conn_params_t   gap_conn_params = {
			.min_conn_interval = MIN_CONN_INTERVAL,
			.max_conn_interval = MAX_CONN_INTERVAL,
			.conn_sup_timeout  = CONN_SUP_TIMEOUT,
		};
		APP_ERROR_CHECK(sd_ble_gap_ppcp_set(&gap_conn_params));
	}

    APP_ERROR_CHECK(nrf_ble_gatt_init(&m_gatt, gatt_evt_handler));
    APP_ERROR_CHECK(nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE));

	const uint8_t uuid_type = BLE_UUID_TYPE_BLE;
	// const ble_uuid128_t vendor_uuid = { .uuid128 = {
	// 	0x79, 0x09, 0xe6, 0x9a, 0x90, 0x4d, 0x46, 0x19,
	// 	0x93, 0xac, 0x72, 0x2d, 0x1d, 0x13, 0xc7, 0xab,
	// } };
	// uint8_t uuid_type;
	// APP_ERROR_CHECK(sd_ble_uuid_vs_add(&vendor_uuid, &uuid_type));

	ble_uuid_t m_adv_uuids[] = {
		{0xACAC, uuid_type}
	};

	{
		ble_advertising_init_t init = {
			.advdata.name_type = BLE_ADVDATA_FULL_NAME,
			.advdata.flags     = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,

			.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(*m_adv_uuids),
			.srdata.uuids_complete.p_uuids  = m_adv_uuids,

			.config.ble_adv_fast_enabled  = true,
			.config.ble_adv_fast_interval = APP_ADV_INTERVAL,
			.config.ble_adv_slow_enabled  = true,
			.config.ble_adv_slow_interval = APP_ADV_INTERVAL,
			.evt_handler = on_adv_evt,
		};

		APP_ERROR_CHECK(ble_advertising_init(&m_advertising, &init));
		ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
	}

	{
		uint16_t service;
		APP_ERROR_CHECK(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &m_adv_uuids[0], &service));

		ble_add_char_params_t char_params = {
			.uuid = 0xAC00,
			.uuid_type = uuid_type,
			.max_len = 7,
			.init_len = 7,
			.p_init_value = (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			.char_props = { .read = 1, .notify = 1 },
			.read_access = SEC_OPEN,
			.cccd_write_access = SEC_OPEN,
		};
		(void)char_params;

		APP_ERROR_CHECK(characteristic_add(service, &char_params, &buf0_handles));

		// g_tick_context.value_handle = buf0_handles.value_handle;
		// APP_ERROR_CHECK(app_timer_create(&tick_timer, APP_TIMER_MODE_REPEATED, &tick_handler));
		// APP_ERROR_CHECK(app_timer_start(tick_timer, APP_TIMER_TICKS(500), NULL));
	}

	{
		ble_conn_params_init_t cp_init = {
			.p_conn_params                  = NULL,
			.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY,
			.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY,
			.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT,
			.start_on_notify_cccd_handle    = buf0_handles.cccd_handle,
			.disconnect_on_fail             = false,
			.evt_handler                    = on_conn_params_evt,
			.error_handler                  = conn_params_error_handler,
		};

		APP_ERROR_CHECK(ble_conn_params_init(&cp_init));
	}

    APP_ERROR_CHECK(ble_advertising_start(&m_advertising, BLE_ADV_MODE_SLOW));

}
