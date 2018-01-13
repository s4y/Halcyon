#include "mbed.h"
#include "BLE.h"

#include <array>

const char* const kDeviceName = "Halcyon bridge";

class HalcyonBus {
	public:
	enum class Node {
		Blower,
		PrimaryRemote,
		SecondaryRemote,
	};

	struct DebugObserver {
		virtual void didReadPacket(const std::array<uint8_t, 8>& buf) {}
		virtual void nodeDidChangeState(Node node, const std::array<uint8_t, 7>& buf) {}
	};

	DebugObserver* observer = nullptr;

	private:
	Serial uart;
	Timeout timeout;

	std::array<uint8_t, 8> serialBuf{{0}};

	uint8_t ourAddr = 0x21;
	std::array<uint8_t, 7> ourState{{0x81, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00}};
	std::array<uint8_t, 8> txBuf{{0}};
	Timeout txTimeout;

	void handleReadComplete(int) {
		timeout.detach();
		scheduleRead();

		for (size_t i = 0; i < serialBuf.size(); i++)
			serialBuf[i] = ~serialBuf[i];

		if (observer) observer->didReadPacket(serialBuf);

		std::array<uint8_t, 7> stateBuf;
		std::copy(serialBuf.begin() + 1, serialBuf.end(), stateBuf.begin());

		switch (serialBuf[0]) {
			case 0x00:
				if (observer) observer->nodeDidChangeState(Node::Blower, stateBuf);
				break;
			case 0x20:
				if (observer) observer->nodeDidChangeState(Node::PrimaryRemote, stateBuf);
				break;
			case 0x21:
				break;
			default:
				break;
		}

		if (!(stateBuf[1] & 0x20)) {
			ourState[2] = stateBuf[2];
			ourState[3] = stateBuf[3];
			ourState[1] &= ~0x20;
		}

#if 0
		// AC must be turned off at all times >:(
		// (Yes, this is just a PoC/test.)
		if ((stateBuf[2] & 0x01)) {
			ourState[1] |= 0x08;
			ourState[2] &= ~0x01;
		}
#endif

		// Our time to shine!
		if (ourState[0] == 0xa1)
			txTimeout.attach(callback(this, &HalcyonBus::handleTx), 0.1);
	}

	void handleTx() {
		txBuf[0] = ~ourAddr;
		for (size_t i = 0; i < ourState.size(); i++)
			txBuf[i+1] = ~ourState[i];
		uart.write(txBuf.data(), txBuf.size(), callback(this, &HalcyonBus::handleTxComplete));
		ourState[2] &= ~0x08;
	}

	void handleTxComplete(int) {
	}

	void handleTimeout() {
		uart.abort_read();
		scheduleRead();
	}

	void handleReadStart() {
		uart.attach(nullptr);
		uart.read(serialBuf.data(), serialBuf.size(), callback(this, &HalcyonBus::handleReadComplete));
		timeout.attach(callback(this, &HalcyonBus::handleTimeout), 0.2);
	}

	void scheduleRead() {
		uart.attach(callback(this, &HalcyonBus::handleReadStart));
	}

	public:
	HalcyonBus(PinName tx, PinName rx) : uart(tx, rx, 500) {
		// Even parity.
		uart.format(8, SerialBase::Even);

		// Custom baud rate around 500.
		NRF_UART0->BAUDRATE = 0x21000;

		// Pseudo-ground.
		nrf_gpio_cfg_output(P0_3);

		// Attach a pullup resistor to rx; it's connected to an optocoupler which only pulls down.
		nrf_gpio_cfg_input(rx, NRF_GPIO_PIN_PULLUP);

		scheduleRead();
	}

};

class BLEService: HalcyonBus::DebugObserver {

	template <size_t Size>
	class ROBufferCharacteristic {
		std::array<uint8_t, Size> buf{{0}};
		GattCharacteristic characteristic{0x0, buf.data(), buf.size(), buf.size(), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false};

		public:
		ROBufferCharacteristic(const UUID uuid) :
			characteristic{uuid, buf.data(), Size, Size, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY, NULL, 0, false}
		{}

		void update(const std::array<uint8_t, Size>& from) {
			// TODO: Move this higher up the chain.
			if (buf == from)
				return;

			buf = from;
			BLE::Instance().gattServer().write(
				characteristic.getValueAttribute().getHandle(), buf.data(), buf.size()
			);
		}

		operator GattCharacteristic*() { return &characteristic; }
	};

	ROBufferCharacteristic<8> bufCharacteristic{0xb00f};
	ROBufferCharacteristic<7> blowerCharacteristic{0xac00};
	ROBufferCharacteristic<7> themostatCharacteristic{0xac20};

	GattService service{[&]{
		GattCharacteristic* characteristics[] = {
			bufCharacteristic,
			blowerCharacteristic,
			themostatCharacteristic,
		};
		return GattService{0xACAC, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *)};
	}()};

	void didReadPacket(const std::array<uint8_t, 8>& buf) override {
		bufCharacteristic.update(buf);
	}

	void nodeDidChangeState(HalcyonBus::Node node, const std::array<uint8_t, 7>& buf) override {
		switch (node) {
			case HalcyonBus::Node::Blower:
				blowerCharacteristic.update(buf);
				break;
			case HalcyonBus::Node::PrimaryRemote:
				themostatCharacteristic.update(buf);
				break;
			default:
				break;
		}
	}

	void didDisconnect(const Gap::DisconnectionCallbackParams_t*) {
		BLE::Instance().gap().startAdvertising();
	}

	public:
	BLEService(HalcyonBus* bridge) {
		BLE &ble = BLE::Instance();

		//ble.gap().onConnection(&onConnection);
		ble.gap().onDisconnection(this, &BLEService::didDisconnect);

		ble.gattServer().addService(service);
		ble.gap().setDeviceName(reinterpret_cast<const uint8_t*>(kDeviceName));
		ble.gap().setAppearance(GapAdvertisingData::Appearance(1537) /* HVAC -> Thermostat */);
		ble.gap().startAdvertising();

		bridge->observer = this;
	}
};

int main() {
	BLE &ble = BLE::Instance();
	ble.init();

	// Pseudo ground.
	nrf_gpio_cfg_output(P0_3);

	HalcyonBus bridge(P0_29, P0_5);
	BLEService service(&bridge);

	for (;;)
		ble.processEvents();

}
