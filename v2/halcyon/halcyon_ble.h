#include <stdbool.h>
#include <stdlib.h>

#include "ble_srv_common.h"

typedef struct {
  ble_add_char_params_t params;
  ble_gatts_char_handles_t _handles;
} halcyon_ble_characteristic_t;

typedef struct {
  ble_uuid_t uuid;
  halcyon_ble_characteristic_t *characteristics;
  size_t n_characteristics;
  uint16_t _service_id;
} halcyon_ble_service_t;

typedef struct {
  halcyon_ble_service_t *services;
  size_t n_services;
  bool allow_bonding;
  bool delete_bonds;
} halcyon_ble_config_t;

void halcyon_ble_init(halcyon_ble_config_t* config);
void halcyon_ble_notify_changed(halcyon_ble_characteristic_t* characteristic);
