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
#include "peer_manager.h"
#include "peer_manager_handler.h"

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

typedef struct {
	uint16_t conn_handle;
	ble_gap_enc_key_t enc_key;
} halcyon_peer_t;

halcyon_peer_t *peer_get(uint16_t conn_handle) {
	static halcyon_peer_t peers[NRF_SDH_BLE_PERIPHERAL_LINK_COUNT] = {{0}};
	NRF_LOG_INFO("Peers:");
	for (halcyon_peer_t *peer = peers, *end = peers + NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; peer < end; peer++)
		NRF_LOG_INFO("- %d", peer->conn_handle);
	for (halcyon_peer_t *peer = peers, *end = peers + NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; peer < end; peer++) {
		if (peer->conn_handle == conn_handle)
			return peer;
	}
	return NULL;
}

halcyon_ble_config_t* g_config;

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {
	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			{
				halcyon_peer_t *peer = peer_get(0);
				if (!peer)
					break;
				peer->conn_handle = p_ble_evt->evt.gap_evt.conn_handle + 1;
			}
			break;
		case BLE_GAP_EVT_DISCONNECTED:
			{
				halcyon_peer_t *peer = peer_get(p_ble_evt->evt.gap_evt.conn_handle + 1);
				if (!peer)
					break;
				*peer = (halcyon_peer_t){0};
			}
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

static void pm_evt_handler(pm_evt_t const * p_evt) {
	pm_handler_on_pm_evt(p_evt);
	pm_handler_flash_clean(p_evt);

	// switch (p_evt->evt_id) {
	// 	case PM_EVT_PEERS_DELETE_SUCCEEDED:
	// 		advertising_start(false);
	// 		break;
	// 	default:
	// 		break;
	// }
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

	g_config = config;
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

	{
		ble_advertising_init_t init = {
			.advdata.name_type = BLE_ADVDATA_FULL_NAME,
			.advdata.flags     = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,

			.config.ble_adv_fast_enabled  = true,
			.config.ble_adv_fast_interval = APP_ADV_INTERVAL,
			.config.ble_adv_slow_enabled  = true,
			.config.ble_adv_slow_interval = APP_ADV_INTERVAL,
		};

		APP_ERROR_CHECK(ble_advertising_init(&m_advertising, &init));
		ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
	}

	for (halcyon_ble_service_t *service = config->services, *end = service + config->n_services; service < end; service++) {
		APP_ERROR_CHECK(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service->uuid, &service->_service_id));

		for (halcyon_ble_characteristic_t *characteristic = service->characteristics, *end = characteristic + service->n_characteristics; characteristic < end; characteristic++)
			APP_ERROR_CHECK(characteristic_add(service->_service_id, &characteristic->params, &characteristic->_handles));
	}

	{
		ble_conn_params_init_t cp_init = {
			.p_conn_params                  = NULL,
			.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY,
			.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY,
			.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT,
			.disconnect_on_fail             = false,
			.error_handler                  = conn_params_error_handler,
		};

		APP_ERROR_CHECK(ble_conn_params_init(&cp_init));
	}

	{
		APP_ERROR_CHECK(pm_init());

		if (config->allow_bonding) {
			ble_gap_sec_params_t sec_param = {
				.bond = 1,
				.io_caps = BLE_GAP_IO_CAPS_NONE,
				.min_key_size = 7,
				.max_key_size = 16,
				.kdist_own.enc  = 1,
				.kdist_own.id   = 1,
				.kdist_peer.enc = 1,
				.kdist_peer.id  = 1,
			};

			APP_ERROR_CHECK(pm_sec_params_set(&sec_param));
		} else {
			APP_ERROR_CHECK(pm_sec_params_set(NULL));
		}

		APP_ERROR_CHECK(pm_register(pm_evt_handler));
		if (config->delete_bonds)
			APP_ERROR_CHECK(pm_peers_delete());
	}

    APP_ERROR_CHECK(ble_advertising_start(&m_advertising, BLE_ADV_MODE_SLOW));
}
