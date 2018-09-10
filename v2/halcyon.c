#include "app_config.h"

#include "halcyon/halcyon_ble.h"

#include "app_error.h"
#include "app_timer.h"
#include "ble_dfu.h"
#include "nordic_common.h"
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"

#include "halcyon_boards.h"
#include "halcyon/bridge.h"
#include "halcyon/invert.h"

static halcyon_ble_characteristic_t characteristics[] = {
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
      .is_value_user = true,
    },
  },
  {
    .params = {
      .uuid = 0xAC00,
      .uuid_type = BLE_UUID_TYPE_BLE,
      .max_len = 7,
      .init_len = 7,
      .p_init_value = halcyon_bus_node[0].buf,
      .char_props = { .read = 1, .notify = 1 },
      .read_access = SEC_OPEN,
      .cccd_write_access = SEC_OPEN,
      .is_value_user = true,
    },
  },
  {
    .params = {
      .uuid = 0xAC20,
      .uuid_type = BLE_UUID_TYPE_BLE,
      .max_len = 7,
      .init_len = 7,
      .p_init_value = halcyon_bus_node[1].buf,
      .char_props = { .read = 1, .notify = 1 },
      .read_access = SEC_OPEN,
      .cccd_write_access = SEC_OPEN,
      .is_value_user = true,
    },
  },
  {
    .params = {
      .uuid = 0xAC21,
      .uuid_type = BLE_UUID_TYPE_BLE,
      .max_len = 7,
      .init_len = 7,
      .p_init_value = halcyon_bus_node[2].buf,
      .char_props = { .read = 1, .notify = 1 },
      .read_access = SEC_OPEN,
      .cccd_write_access = SEC_OPEN,
      .is_value_user = true,
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
      .is_value_user = true,
    },
  },
};

static halcyon_ble_service_t services[] = {
  {
    .uuid = {0xACAC, BLE_UUID_TYPE_BLE},
    .characteristics = characteristics,
    .n_characteristics = sizeof(characteristics) / sizeof(*characteristics),
  },
};

void halcyon_bridge_rx_cb() {
  halcyon_ble_notify_changed(&characteristics[0]);
}

void halcyon_bridge_state_change_cb(halcyon_bus_node_t id) {
  halcyon_ble_characteristic_t* characteristic = NULL;
  switch (id) {
    case HALCYON_BUS_NODE_BLOWER:
      characteristic = &characteristics[1];
      break;
    case HALCYON_BUS_NODE_THERMOSTAT_ZERO:
      characteristic = &characteristics[2];
      break;
    case HALCYON_BUS_NODE_THERMOSTAT_ONE:
      characteristic = &characteristics[3];
      break;
    case MAX_HALCYON_BUS_NODE:
      break;
  }
  halcyon_ble_notify_changed(characteristic);
}


int main(void) {
  NRF_LOG_INIT(NULL);
  NRF_LOG_DEFAULT_BACKENDS_INIT();

  APP_ERROR_CHECK(nrf_pwr_mgmt_init());
  APP_ERROR_CHECK(app_timer_init());
  APP_ERROR_CHECK(nrf_sdh_enable_request());

  halcyon_bridge_init();
  invert_init();

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
