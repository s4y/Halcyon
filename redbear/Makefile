SDK_ROOT := ./dump/nRF5_SDK_14.2.0_17b948a

include Makefile.nrf

OPENOCD := openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg
GCC_BIN := ./dump/gcc-arm-none-eabi-6-2017-q2-update/bin

CC := $(GCC_BIN)/arm-none-eabi-gcc
AR := $(GCC_BIN)/arm-none-eabi-ar
CFLAGS := \
	-Wall \
	-Werror \
	--specs=nosys.specs \
	-mthumb \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-DNRF52 \
	-DBOARD_CUSTOM \
	-DNRF_LOG_ENABLED=0 \
	-DNRF_SD_BLE_API_VERSION=5 \
	-I. \
	$(addprefix -I,$(NRF_INC_FOLDERS))

NRF_OBJ_FILES := $(patsubst %.c,%.o,$(NRF_SRC_FILES))

default: main.hex

__PHONY:

upload-softdevice: __PHONY
	$(OPENOCD) -c 'init;reset halt;program $(SDK_ROOT)/components/softdevice/s132/hex/s132_nrf52_5.0.0_softdevice.hex reset verify exit'

upload-%: %.hex __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< reset verify exit'

debug-%: %.out upload-%
	$(GCC_BIN)/arm-none-eabi-gdb helloworld.out -ex 'target remote | $(OPENOCD) -c "gdb_port pipe; init; reset halt"'

%.bin: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O binary "$<" "$@"

%.hex: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O ihex "$<" "$@"

libsdk.a: $(patsubst %.c,%.o,$(NRF_SRC_FILES))
	$(AR) cr $@ $^

%.out: %.c %.ld $(NRF_OBJ_FILES) Makefile
	$(CC) \
		-o "$@" \
		-flto \
		$(CFLAGS) \
		-L$(SDK_ROOT)/components/toolchain/gcc/ \
 	 	-Wl,-T,$*.ld \
		$< \
		$(NRF_OBJ_FILES)
