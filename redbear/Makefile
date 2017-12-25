SDK_ROOT := ./dump/nRF5_SDK_14.2.0_17b948a/components

OPENOCD := openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg
GCC_BIN := ./dump/gcc-arm-none-eabi-6-2017-q2-update/bin

__PHONY:

upload-softdevice: __PHONY
	$(OPENOCD) -c 'init;reset halt;program $(SDK_ROOT)/softdevice/s132/hex/s132_nrf52_5.0.0_softdevice.hex reset verify exit'

upload-%: %.hex __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< reset verify exit'

debug-%: %.out upload-%
	$(GCC_BIN)/arm-none-eabi-gdb helloworld.out -ex 'target remote | $(OPENOCD) -c "gdb_port pipe; init; reset halt"'

%.bin: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O binary "$<" "$@"

%.hex: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O ihex "$<" "$@"

%.out: %.c %.ld Makefile
	$(GCC_BIN)/arm-none-eabi-gcc \
		-g \
		-Wall \
		-Werror \
 		--specs=nosys.specs \
		-mthumb \
		-mcpu=cortex-m4 \
		-mfloat-abi=hard \
		-mfpu=fpv4-sp-d16 \
		-DNRF52 \
		-DNRF_SECTION_ITER_ENABLED=1 \
		-DNRF_SDH_ENABLED=1 \
		-DNRF_SDH_BLE_ENABLED=1 \
		-DNRF_LOG_ENABLED=0 \
		-DNRF_SDH_CLOCK_LF_SRC=1 \
		-DNRF_SDH_CLOCK_LF_RC_CTIV=0 \
		-DNRF_SDH_CLOCK_LF_RC_TEMP_CTIV=0 \
		-DNRF_SDH_CLOCK_LF_XTAL_ACCURACY=7 \
		-o "$@" \
		-I$(SDK_ROOT)/toolchain/ \
		-I$(SDK_ROOT)/toolchain/cmsis/include/ \
		-I$(SDK_ROOT)/device/ \
		-I$(SDK_ROOT)/softdevice/common/ \
		-I$(SDK_ROOT)/libraries/util/ \
		-I$(SDK_ROOT)/libraries/experimental_section_vars/ \
		-I$(SDK_ROOT)/libraries/experimental_log/ \
		-I$(SDK_ROOT)/libraries/experimental_log/src/ \
		-I$(SDK_ROOT)/ \
		-I$(SDK_ROOT)/softdevice/s132/headers/ \
		-L$(SDK_ROOT)/toolchain/gcc/ \
		-I. \
 	 	-Wl,-T,$*.ld \
 		$(SDK_ROOT)/toolchain/gcc/gcc_startup_nrf52.S \
 		$(SDK_ROOT)/softdevice/common/nrf_sdh.c \
 		$(SDK_ROOT)/libraries/util/app_error.c \
		$(SDK_ROOT)/libraries/experimental_section_vars/nrf_section_iter.c \
		$<
