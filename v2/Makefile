DEPS := ./deps
GCC := $(DEPS)/gcc-arm-none-eabi-7-2017-q4-major
SDK := $(DEPS)/nRF5_SDK_15.0.0_a53641a

include Makefile.nrf

OPENOCD := openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg
GCC_BIN := $(GCC)/bin

CC := $(GCC_BIN)/arm-none-eabi-gcc
AR := $(GCC_BIN)/arm-none-eabi-ar
CFLAGS := \
	-Wall \
	-Werror \
	-mthumb \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \
	-DNRF52 \
	-DBOARD_CUSTOM \
	-DNRF_LOG_ENABLED=0 \
	-DNRF_SD_BLE_API_VERSION=6 \
	-I. $(addprefix -I,$(NRF_INC_FOLDERS)) \
	-L$(SDK)/modules/nrfx/mdk/ \
	-L$(SDK)/components/softdevice/s132/toolchain/armgcc/

DEBUG_CFLAGS := \
	--specs=rdimon.specs \
	-g \
	-lc \
	-lrdimon \

REL_CFLAGS := \
	--specs=nosys.specs \
	-flto \

NRF_OBJ_FILES := $(patsubst %.c,%.o,$(NRF_SRC_FILES))

default: main.hex

__PHONY:

$(DEPS):
	mkdir -p $(DEPS)

$(GCC): $(DEPS)
	cd $(DEPS); curl -LOJ https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2017q4/gcc-arm-none-eabi-7-2017-q4-major-mac.tar.bz2
	cd $(DEPS); tar -xjf gcc-arm-none-eabi-7-2017-q4-major-mac.tar.bz2

$(SDK): $(DEPS)
	cd $(DEPS); curl -LOJ https://www.nordicsemi.com/eng/nordic/download_resource/59011/70/84826398/116085
	cd $(DEPS); unzip nRF5_SDK_15.0.0_a53641a.zip

deps: $(GCC) $(SDK)

nuke:
	$(OPENOCD) -c 'init;reset halt;nrf5 mass_erase;exit'

halt:
	$(OPENOCD) -c 'init;reset halt;exit'

reset:
	$(OPENOCD) -c 'init;reset;exit'

debug-run:
	$(OPENOCD) -c 'init;arm semihosting enable;reset'

upload-softdevice: __PHONY
	$(OPENOCD) -c 'init;reset halt;program $(SDK)/components/softdevice/s132/hex/s132_nrf52_6.0.0_softdevice.hex reset verify exit'

upload-%: %.hex __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify exit'

semihost-%: %_debug.hex __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify;arm semihosting enable;reset'

debug-%: %_debug.out upload-%
	$(GCC_BIN)/arm-none-eabi-gdb "$<" -ex 'target remote | $(OPENOCD) -c "gdb_port pipe; init; reset halt"'

%.bin: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O binary "$<" "$@"

%_debug.hex: %_debug.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O ihex "$<" "$@"

%.hex: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O ihex "$<" "$@"

%_debug.out: %.c %.ld $(NRF_OBJ_FILES) Makefile
	$(CC) -o "$@" $(DEBUG_CFLAGS) $(CFLAGS) -Wl,-T,$*.ld $< $(NRF_OBJ_FILES)

%.out: %.c %.ld $(NRF_OBJ_FILES) Makefile
	$(CC) -o "$@" $(REL_CFLAGS) $(CFLAGS) -Wl,-T,$*.ld $< $(NRF_OBJ_FILES)
