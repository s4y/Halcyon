#include "mbed.h"
#include "BLE.h"

#include <array>
#include <functional>

const char* const kDeviceName = "Halcyon bridge";

class HalcyonBus {
	public:
	enum class Node {
		Blower,
		PrimaryRemote,
		SecondaryRemote,
	};

	struct Observer {
		virtual void didReadPacket(const std::array<uint8_t, 8>& buf) {}
		virtual void nodeDidChangeState(Node node, const std::array<uint8_t, 7>& buf) {}
	};

	Observer* observer = nullptr;

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
				if (observer) observer->nodeDidChangeState(Node::SecondaryRemote, stateBuf);
			default:
				break;
		}

		// Update our own state if:
		//   - (Another device is changing state,
		//   - OR we don't have state),
		//   - AND we're not changing state.
		if ((stateBuf[1] & 0x08 || ourState[1] & 0x20) && !(ourState[1] & 0x08)) {
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
		if (stateBuf[0] == 0xa1)
			txTimeout.attach(callback(this, &HalcyonBus::handleTx), 0.1);
	}

	void handleTx() {
		txBuf[0] = ~ourAddr;
		for (size_t i = 0; i < ourState.size(); i++)
			txBuf[i+1] = ~ourState[i];
		uart.write(txBuf.data(), txBuf.size(), callback(this, &HalcyonBus::handleTxComplete));
		ourState[1] &= ~0x08;
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

	void putState(std::array<uint8_t, 2> state) {
		ourState[1] |= 0x08;
		ourState[2] = state[0];
		ourState[3] = state[1];
	}

};

class BLEService: HalcyonBus::Observer {

	template <size_t Size>
	class ROCharacteristic {
		protected:
		std::array<uint8_t, Size> buf{{0}};
		GattCharacteristic characteristic;

		ROCharacteristic(const UUID uuid, uint8_t props) :
			characteristic{uuid, buf.data(), Size, Size, props, NULL, 0, false}
		{}

		public:
		ROCharacteristic(const UUID uuid) :
			ROCharacteristic{uuid, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY}
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

	template <size_t Size>
	class RWCharacteristic: public ROCharacteristic<Size> {

		void onDataWritten(const GattWriteCallbackParams* params) {
			if (params->handle != this->characteristic.getValueAttribute().getHandle())
				return;

			if (params->len != Size)
				return;

			std::array<uint8_t, Size> newval{params->data[0], params->data[1]};
			if (this->buf == newval)
				return;

			this->update(newval);
			if (onWritten) onWritten(newval);
		}

		public:
		RWCharacteristic(const UUID uuid) :
			ROCharacteristic<Size>{uuid, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY}
		{
			this->characteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM);
			BLE::Instance().gattServer().onDataWritten().add(FunctionPointerWithContext<const GattWriteCallbackParams*>(this, &RWCharacteristic::onDataWritten));
		}

		~RWCharacteristic() {
			BLE::Instance().gattServer().onDataWritten().detach(FunctionPointerWithContext<const GattWriteCallbackParams*>(this, &RWCharacteristic::onDataWritten));
		}

		std::function<void(const std::array<uint8_t, Size>&)> onWritten = nullptr;
	};

	ROCharacteristic<8> bufCharacteristic{0xb00f};
	ROCharacteristic<7> blowerCharacteristic{0xac00};
	ROCharacteristic<7> themostatCharacteristic{0xac20};
	ROCharacteristic<7> secondaryRemoteCharacteristic{0xac21};

	RWCharacteristic<2> stateCharacteristic{0xaced};

	GattService service{[&]{
		GattCharacteristic* characteristics[] = {
			bufCharacteristic,
			blowerCharacteristic,
			themostatCharacteristic,
			secondaryRemoteCharacteristic,
			stateCharacteristic
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
			case HalcyonBus::Node::SecondaryRemote:
				secondaryRemoteCharacteristic.update(buf);
				stateCharacteristic.update({buf[2], buf[3]});
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

		// ble.gap().onConnection(this, &BLEService::onConnection);
		ble.gap().onDisconnection(this, &BLEService::didDisconnect);

		ble.gattServer().addService(service);
		ble.gap().setDeviceName(reinterpret_cast<const uint8_t*>(kDeviceName));
		ble.gap().setAppearance(GapAdvertisingData::Appearance(1537) /* HVAC -> Thermostat */);
		ble.gap().startAdvertising();

		stateCharacteristic.onWritten = [=](const std::array<uint8_t, 2> newval) {
			bridge->putState(newval);
		};

		bridge->observer = this;
	}
};

int main() {
	BLE &ble = BLE::Instance();
	ble.init();
	ble.securityManager().init(false, false);

	// Pseudo ground.
	nrf_gpio_cfg_output(P0_3);

	HalcyonBus bridge(P0_29, P0_5);
	BLEService service(&bridge);

	for (;;)
		ble.processEvents();

}
