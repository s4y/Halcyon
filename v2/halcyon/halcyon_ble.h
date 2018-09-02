#include <stddef.h>

#include <ble_types.h>
#include <ble_gatts.h>

typedef struct {
	ble_uuid_t uuid;
	struct {
		int read: 1;
		int write: 1;
		int notify: 1;
		int read_and_notify_public: 1;
		int write_public: 1;
	} mode;
	uint8_t *value;
	size_t length;

	ble_gatts_char_handles_t _handles;
} halcyon_ble_characteristic_t;

typedef struct {
	ble_uuid_t uuid;
	halcyon_ble_characteristic_t **characteristics; // {…, NULL}
} halcyon_ble_service_t;

typedef struct {
	halcyon_ble_service_t **services; // {…, NULL}
} halcyon_ble_config_t;

void halcyon_ble_init(halcyon_ble_config_t* config);
