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
  bool in_use;
  uint16_t conn_handle;
} halcyon_peer_t;

static halcyon_peer_t halcyon_peers[NRF_SDH_BLE_PERIPHERAL_LINK_COUNT] = {{0}};

halcyon_peer_t *peer_get(uint16_t conn_handle, bool create) {
  NRF_LOG_INFO("Peers:");
  for (halcyon_peer_t *peer = halcyon_peers, *end = halcyon_peers + NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; peer < end; peer++)
    NRF_LOG_INFO("- %d", peer->conn_handle);
  for (halcyon_peer_t *peer = halcyon_peers, *end = halcyon_peers + NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; peer < end; peer++) {
    if (peer->in_use && peer->conn_handle == conn_handle) {
      return peer;
    } else if (create && !peer->in_use) {
      peer->in_use = true;
      *peer = (halcyon_peer_t){true, conn_handle};
      return peer;
    }
  }
  return NULL;
}

halcyon_ble_config_t* g_config;

static halcyon_ble_characteristic_t* characteristic_get_by_handle(uint16_t value_handle) {
  for (halcyon_ble_service_t *service = g_config->services, *end = service + g_config->n_services; service < end; service++) {
    for (halcyon_ble_characteristic_t *characteristic = service->characteristics, *end = characteristic + service->n_characteristics; characteristic < end; characteristic++) {
      if (characteristic->_handles.value_handle == value_handle) {
        return characteristic;
      }
    }
  }
  return NULL;
}

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {
  switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
      peer_get(p_ble_evt->evt.gap_evt.conn_handle, true);
      break;
    case BLE_GAP_EVT_DISCONNECTED:
      {
        halcyon_peer_t* peer = peer_get(p_ble_evt->evt.gap_evt.conn_handle, false);
        if (peer)
          peer->in_use = false;
      }
      break;
    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
      APP_ERROR_CHECK(sd_ble_gap_phy_update(
        p_ble_evt->evt.gap_evt.conn_handle,
        &(ble_gap_phys_t){
          .rx_phys = BLE_GAP_PHY_AUTO,
          .tx_phys = BLE_GAP_PHY_AUTO,
        }
      ));
      break;
    case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
      APP_ERROR_CHECK(sd_ble_gap_conn_param_update(p_ble_evt->evt.gap_evt.conn_handle, &p_ble_evt->evt.gap_evt.params.conn_param_update_request.conn_params));
      break;
    case BLE_GATTS_EVT_WRITE:
      {
        halcyon_ble_characteristic_t* characteristic = characteristic_get_by_handle(p_ble_evt->evt.gatts_evt.params.write.handle);
        if (characteristic)
          halcyon_ble_characteristic_written_cb(characteristic);
      }
      break;
    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
      APP_ERROR_CHECK(sd_ble_gatts_sys_attr_set(p_ble_evt->evt.gap_evt.conn_handle, NULL, 0, 0));
      break;
    case BLE_GATTC_EVT_TIMEOUT:
      APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
      break;
    case BLE_GATTS_EVT_TIMEOUT:
      APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
      break;
    default:
      break;
  }
}

static void pm_evt_handler(pm_evt_t const * p_evt) {
  pm_handler_on_pm_evt(p_evt);
  pm_handler_flash_clean(p_evt);
}

static void conn_params_error_handler(uint32_t nrf_error) {
  APP_ERROR_HANDLER(nrf_error);
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

  APP_ERROR_CHECK(nrf_ble_gatt_init(&m_gatt, NULL));
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
      APP_ERROR_CHECK(pm_sec_params_set(&(ble_gap_sec_params_t){
        .bond = 1,
        .io_caps = BLE_GAP_IO_CAPS_NONE,
        .min_key_size = 7,
        .max_key_size = 16,
        .kdist_own.enc  = 1,
        .kdist_own.id   = 1,
        .kdist_peer.enc = 1,
        .kdist_peer.id  = 1,
      }));
    } else {
      APP_ERROR_CHECK(pm_sec_params_set(NULL));
    }
    APP_ERROR_CHECK(pm_register(pm_evt_handler));
    if (config->delete_bonds)
      APP_ERROR_CHECK(pm_peers_delete());
  }

  APP_ERROR_CHECK(ble_advertising_start(&m_advertising, BLE_ADV_MODE_SLOW));

  g_config = config;
}

void halcyon_ble_notify_changed(halcyon_ble_characteristic_t* characteristic) {
  ble_gatts_hvx_params_t hvx_params = {
    .handle = characteristic->_handles.value_handle,
    .type   = BLE_GATT_HVX_NOTIFICATION,
    .offset = 0,
    .p_len  = &characteristic->params.init_len,
    .p_data = characteristic->params.p_init_value,
  };
  for (halcyon_peer_t *peer = halcyon_peers, *end = halcyon_peers + NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; peer < end; peer++) {
    if (!peer->in_use)
      return;
    int err = sd_ble_gatts_hvx(peer->conn_handle, &hvx_params);
    if (
        err != NRF_ERROR_INVALID_STATE &&
        err != BLE_ERROR_GATTS_SYS_ATTR_MISSING
       ) APP_ERROR_CHECK(err);
  }
}
