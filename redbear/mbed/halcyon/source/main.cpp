#include "mbed.h"
#include "BLE.h"

const char* const kDeviceName = "Halcyon bridge";

uint8_t serialBuf[8];

uint8_t ceilBuf[7];
uint8_t thermBuf[7];

DigitalOut led(LED1, 0);

#if 0
GattCharacteristic bufCharacteristic(0xb00f, ceilBuf, sizeof(serialBuf), sizeof(serialBuf), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false);
GattCharacteristic ceilCharacteristic(0xACff, ceilBuf, sizeof(ceilBuf), sizeof(ceilBuf), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false);
GattCharacteristic thermCharacteristic(0xACdf, thermBuf, sizeof(thermBuf), sizeof(thermBuf), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false);

GattCharacteristic* characteristics[] = {
	&bufCharacteristic,
	&ceilCharacteristic,
	&thermCharacteristic,
};
GattService acService(0xACAC, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));
#endif

class HalcyonBus {
	public:
	enum Node {
		Blower,
		PrimaryRemote,
		SecondaryRemote,
	};

	struct DebugObserver {
		virtual void didReadPacket(const uint8_t* buf) {}
		virtual void nodeDidChangeState(Node node, const uint8_t* buf) {}
	};

	DebugObserver* observer = nullptr;

	private:
	Serial uart;
	Timeout timeout;

	uint8_t serialBuf[8];

	void handleRead(int) {
		led = 0;
		scheduleRead();

		if (observer) observer->didReadPacket(serialBuf);
	}

	void handleTimeout() {
		led = !led;
		uart.abort_read();
		scheduleRead();
	}

	void scheduleRead() {
		timeout.detach();
		uart.read(serialBuf, sizeof(serialBuf), callback(this, &HalcyonBus::handleRead));
		timeout.attach(callback(this, &HalcyonBus::handleTimeout), 0.5);
	}

	public:
	HalcyonBus(PinName rx, PinName tx) : uart(rx, tx, 1200) {
		// Even parity.
		uart.format(8, SerialBase::Even);

		// Custom baud rate around 500.
		NRF_UART0->BAUDRATE = 0x21000;

		// Attach a pullup resistor to rx; it's connected to an optocoupler which only pulls down.
		nrf_gpio_cfg_input(rx, NRF_GPIO_PIN_PULLUP);

		scheduleRead();
	}

};

class BLEService: HalcyonBus::DebugObserver {
	GattCharacteristic bufCharacteristic{0xb00f, NULL, 0, 0, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false};
	GattCharacteristic ceilCharacteristic{0xACff, NULL, 0, 0, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false};
	GattCharacteristic thermCharacteristic{0xACdf, NULL, 0, 0, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false};

	GattService service{[&]{
		GattCharacteristic* characteristics[] = {
			&bufCharacteristic,
			&ceilCharacteristic,
			&thermCharacteristic,
		};
		return GattService{0xACAC, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *)};
	}()};

	void didReadPacket(const uint8_t* buf) override {
		BLE::Instance().updateCharacteristicValue(bufCharacteristic.getValueAttribute().getHandle(), serialBuf, 8);
	}

	public:
	BLEService(HalcyonBus* bridge) {
		BLE &ble = BLE::Instance();

		// ble.gap().onConnection(&onConnection);
		// ble.gap().onDisconnection(&onDisconnection);

		ble.gattServer().addService(service);
		ble.gap().setDeviceName(reinterpret_cast<const uint8_t*>(kDeviceName));
		ble.gap().setAppearance(GapAdvertisingData::Appearance(1537) /* HVAC -> Thermostat */);
		ble.gap().startAdvertising();

		bridge->observer = this;
	}
};

#if 0

void scheduleRead();

void onRead(int) {
	scheduleRead();

	uint8_t* targetBuf = NULL;
	GattCharacteristic* targetCharacteristic = NULL;

	for (size_t i = 1; i < sizeof(serialBuf) / sizeof(*serialBuf); i++)
		serialBuf[i] = ~serialBuf[i];
	BLE::Instance().updateCharacteristicValue(bufCharacteristic.getValueAttribute().getHandle(), serialBuf, sizeof(serialBuf));

	switch (serialBuf[0]) {
		case 0xff:
			targetBuf = ceilBuf;
			targetCharacteristic = &ceilCharacteristic;
			break;
		case 0xdf:
			targetBuf = thermBuf;
			targetCharacteristic = &thermCharacteristic;
			break;
		default:
			led = 1;
			return;
	}
	led = 0;

	if (!targetCharacteristic)
		return;

	bool changed = false;
	for (size_t i = 1; i < sizeof(serialBuf) / sizeof(*serialBuf); i++) {
		if (targetBuf[i] != serialBuf[i]) {
			targetBuf[i] = serialBuf[i];
			changed = 1;
		}
	}

	if (changed)
		BLE::Instance().updateCharacteristicValue(targetCharacteristic->getValueAttribute().getHandle(), serialBuf + 1, sizeof(serialBuf) - 1);
}

void scheduleRead() {
	uart.read(serialBuf, sizeof(serialBuf), &onRead);
}

Ticker stillAliveTicker;

void onTick() {
	//const char* buf = "hi\r\n";
	//uart.write(reinterpret_cast<const uint8_t*>(buf), sizeof(buf));
}


void onConnection(const Gap::ConnectionCallbackParams_t* params) {
}

void onDisconnection(const Gap::DisconnectionCallbackParams_t* params) {
	BLE::Instance().gap().startAdvertising();
}


void bleInitialized(BLE::InitializationCompleteCallbackContext* ctx) {
	// if (ctx->error) {
	// 	led = 1;
	// 	return;
	// }
	// BLE &ble = BLE::Instance();

	// ble.gap().onConnection(&onConnection);
	// ble.gap().onDisconnection(&onDisconnection);

	// ble.gattServer().addService(acService);
	// ble.gap().setDeviceName(reinterpret_cast<const uint8_t*>(kDeviceName));
	// ble.gap().startAdvertising();
}

#endif

int main() {
	//uart.format(8, SerialBase::Even);
	//NRF_UART0->BAUDRATE = 0x21000;
	//nrf_gpio_cfg_input(P0_30, NRF_GPIO_PIN_PULLUP);
	//stillAliveTicker.attach(&onTick, 1);
	//scheduleRead();

	BLE &ble = BLE::Instance();
	ble.init();

	HalcyonBus bridge(P0_29, P0_30);
	BLEService service(&bridge);

	for (;;)
		ble.processEvents();

}
