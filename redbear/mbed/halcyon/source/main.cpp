#include "mbed.h"
#include "BLE.h"

extern "C" {
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_soc.h"
}

#include <array>
#include <functional>

#define WHICH_BOARD 3

#if WHICH_BOARD == 0

#define RX_PIN P0_5
#define TX_PIN P0_11
#define TX_INV_PIN P0_4

#elif WHICH_BOARD == 1

#define RX_PIN P0_5
#define TX_PIN P0_7
#define TX_INV_PIN P0_3

#elif WHICH_BOARD == 2

#define RX_PIN P0_26
#define TX_PIN P0_23
#define TX_INV_PIN P0_2

#elif WHICH_BOARD == 3

#define RX_PIN P0_13
#define TX_PIN P0_26
#define TX_INV_PIN P0_20

#endif

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
	std::array<uint8_t, 7> ourState{{0x81, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00}};
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

		// Copy the primary thermostat unless we're broadcasting a change.
		if (serialBuf[0] == 0x20 && !(ourState[1] & 0x08)) {
			ourState[2] = stateBuf[2];
			ourState[3] = stateBuf[3];
			ourState[4] = (stateBuf[1] & 0x10) ? 0x00 : 0x20;
		}

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

static void invert_init()
{
	nrf_drv_gpiote_in_config_t in_cfg{};
	in_cfg.is_watcher = true;
	in_cfg.hi_accuracy = true;
	in_cfg.sense = NRF_GPIOTE_POLARITY_TOGGLE;
	nrf_drv_gpiote_in_init(TX_PIN, &in_cfg, NULL);

	nrf_drv_gpiote_out_config_t out_cfg{};
	out_cfg.init_state = NRF_GPIOTE_INITIAL_VALUE_LOW;
	out_cfg.task_pin = true;
	out_cfg.action = NRF_GPIOTE_POLARITY_TOGGLE;

	nrf_drv_gpiote_out_init(TX_INV_PIN, &out_cfg);

	nrf_ppi_channel_t inv_channel;
	nrf_drv_ppi_channel_alloc(&inv_channel);
	nrf_drv_ppi_channel_assign(
		inv_channel,
		nrf_drv_gpiote_in_event_addr_get(TX_PIN),
		nrf_drv_gpiote_out_task_addr_get(TX_INV_PIN)
	);

	nrf_drv_ppi_channel_enable(inv_channel);
	nrf_drv_gpiote_in_event_enable(TX_PIN, 0);
	nrf_drv_gpiote_out_task_enable(TX_INV_PIN);
}

void on_ble_init(BLE::InitializationCompleteCallbackContext *context) {
	sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);
}

int main() {
	nrf_drv_gpiote_init();
	nrf_drv_ppi_init();

#ifdef P_GROUND
	nrf_gpio_cfg_output(P_GROUND);
#endif

	BLE &ble = BLE::Instance();
	ble.init();
	ble.securityManager().init(false, false);

	HalcyonBus bridge(TX_PIN, RX_PIN);
	BLEService service(&bridge);

	invert_init();

	for (;;)
		ble.processEvents();
}
