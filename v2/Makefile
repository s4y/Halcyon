DEPS := deps
GCC := $(DEPS)/gcc-arm-none-eabi-7-2018-q2-update
SDK := $(DEPS)/nRF5_SDK_15.1.0_a8c0c4d

include Makefile.nrf

OPENOCD := openocd -f interface/cmsis-dap.cfg -c "transport select swd" -f target/nrf52.cfg
GCC_BIN := $(GCC)/bin

TARGET := halcyon
BOARD := BLE_NANO_2

DEBUG_HEX = build/Debug/$(TARGET).hex
RELEASE_HEX = build/Release/$(TARGET).hex

LOCAL_CC := $(CC)
CC := $(GCC_BIN)/arm-none-eabi-gcc
AR := $(GCC_BIN)/arm-none-eabi-ar
CFLAGS := \
	-Wall \
	-Werror \
	-Wno-expansion-to-defined \
	-DNRF52 \
	-DS132 \
	-DNRF52832_XXAA \
	-DBOARD_CUSTOM \
	-DSOFTDEVICE_PRESENT \
	-DNRF_SD_BLE_API_VERSION=6 \
	-DUSE_APP_CONFIG \
	-DBOARD_$(BOARD) \
	-I. \
	$(addprefix -isystem,$(NRF_INC_FOLDERS)) \
	-L$(SDK)/modules/nrfx/mdk \

TARGET_CFLAGS := $(CFLAGS) \
	-mthumb \
	-mcpu=cortex-m4 \
	-mfloat-abi=hard \
	-mfpu=fpv4-sp-d16 \

TEST_CFLAGS := $(CFLAGS) \
	-include test/pch.h \
	-I test/mock \
	-DDEBUG \
	-g \

DEBUG_CFLAGS := $(TARGET_CFLAGS) \
	--specs=rdimon.specs \
	-DDEBUG \
	-g \
	-lc \
	-lrdimon \

RELEASE_CFLAGS := $(TARGET_CFLAGS) \
	--specs=nosys.specs \
	-Os \

TEST_HEADER_FILES := $(wildcard test/*.h)
TEST_SOURCE_FILES := $(wildcard halcyon/*_test.c)
TEST_SOURCE_FILES := $(patsubst %_test.c,%.c,$(TEST_SOURCE_FILES)) $(TEST_SOURCE_FILES)
TEST_SOURCE_FILES := $(wildcard test/*.c) $(TEST_SOURCE_FILES)
TEST_SOURCE_FILES := $(wildcard test/mock/*.c) $(TEST_SOURCE_FILES)
TEST_SOURCE_FILES := $(wildcard halcyon/*_test.c) $(TEST_SOURCE_FILES)

SUPPORT_HEADER_FILES := $(wildcard *.h) $(wildcard halcyon/*.h)
SUPPORT_SOURCE_FILES := $(wildcard halcyon/*.c)
SUPPORT_SOURCE_FILES := $(filter-out %_test.c,$(SUPPORT_SOURCE_FILES))

NRF_OBJ_FILES := $(NRF_SRC_FILES)
NRF_OBJ_FILES := $(patsubst %.c,%.o,$(NRF_OBJ_FILES))
NRF_OBJ_FILES := $(patsubst %.S,%.o,$(NRF_OBJ_FILES))

NRF_DEBUG_OBJ_FILES := $(NRF_DEBUG_SRC_FILES)
NRF_DEBUG_OBJ_FILES := $(patsubst %.c,%.o,$(NRF_DEBUG_OBJ_FILES))
NRF_DEBUG_OBJ_FILES := $(patsubst %.S,%.o,$(NRF_DEBUG_OBJ_FILES))
NRF_DEBUG_OBJ_FILES := $(NRF_OBJ_FILES) $(NRF_DEBUG_OBJ_FILES)

default: $(RELEASE_HEX)

ifndef VERBOSE
.SILENT:
endif

__PHONY:

deps/warning:
	@echo "Gotta fetch some dependencies. This'll take about 230MB." >&2
	@echo "Hit enter to continue, or ^C to bail." >&2
	@read -p "? "
	touch deps/warning

$(NRF_SRC_FILES) : $(DEPS)/stamp

$(GCC): | deps/warning
	mkdir -p $(DEPS)
	cd $(DEPS) && curl -LOJ https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2
	cd $(DEPS) && tar -xjf gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2

$(SDK): | deps/warning
	mkdir -p $(DEPS)
	cd $(DEPS) && curl -LOJ https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2
	cd $(DEPS) && tar -xjf gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2

$(DEPS)/stamp: $(GCC) $(SDK)
	touch $(DEPS)/stamp

nuke: __PHONY
	$(OPENOCD) -c 'init;reset halt;nrf5 mass_erase;exit'

halt: __PHONY
	$(OPENOCD) -c 'init;reset halt;exit'

reset: __PHONY
	$(OPENOCD) -c 'init;reset;exit'

openocd: __PHONY
	$(OPENOCD) -c 'init' $(ARGS)

run-attached: $(DEBUG_HEX) __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify;arm semihosting enable;reset'

upload-softdevice: __PHONY
	$(OPENOCD) -c 'init;reset halt;program $(SDK)/components/softdevice/s132/hex/s132_nrf52_6.1.0_softdevice.hex reset verify exit'

upload: $(RELEASE_HEX) __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify exit'

run: $(RELEASE_HEX) __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify reset exit'

debugserver: $(DEBUG_HEX) __PHONY
	$(OPENOCD) -c 'init;reset halt;program $< verify;arm semihosting enable'

debugclient: build/Debug/$(TARGET).out __PHONY
	$(GCC_BIN)/arm-none-eabi-gdb $< -ex 'target extended-remote 127.0.0.1:3333'

debug: build/Debug/$(TARGET).out upload __PHONY
	$(GCC_BIN)/arm-none-eabi-gdb "$<" -ex 'target remote | $(OPENOCD) -c "gdb_port pipe; init; arm semihosting enable; reset halt"'

test: build/Debug/$(TARGET)_tests __PHONY
	"$<"

%.hex: %.out
	$(GCC_BIN)/arm-none-eabi-objcopy -O ihex "$<" "$@"

define builddir
.PRECIOUS: $(1)/$$(SDK)/%.o

$(1)/$$(SDK)/%.o: $$(SDK)/%.c app_config.h Makefile Makefile.nrf
	@mkdir -p $$(dir $$@)
	$$(CC) -c -o "$$@" $(2) $$(TARGET_CFLAGS) "$$<"

$(1)/$$(SDK)/%.o: $$(SDK)/%.S app_config.h Makefile Makefile.nrf
	@mkdir -p $$(dir $$@)
	$$(CC) -c -o "$$@" $(2) $$(TARGET_CFLAGS) "$$<"

$(1)/%.out: %.ld %.c $$(SUPPORT_HEADER_FILES) $$(SUPPORT_SOURCE_FILES) $$(addprefix $(1)/,$(3))
	@mkdir -p $$(dir $$@)
	$$(CC) -o "$$@" $(2) -Wpedantic -L$(1) -Wl,-T,$$(filter-out %.h,$$^)

$(1)/%_tests: %_tests.c $$(TEST_HEADER_FILES) $$(TEST_SOURCE_FILES) $$(SUPPORT_HEADER_FILES)
	@mkdir -p $$(dir $$@)
	$$(LOCAL_CC) -o "$$@" $(TEST_CFLAGS) -Wpedantic -L$(1) $$(filter-out %.h,$$^)
endef

$(eval $(call builddir,build/Debug,$(DEBUG_CFLAGS),$(NRF_DEBUG_OBJ_FILES)))
$(eval $(call builddir,build/Release,$(RELEASE_CFLAGS),$(NRF_OBJ_FILES)))

%/.ycm_debug_flags: __PHONY
	@echo \
		--sysroot=$(GCC)/arm-none-eabi \
		--target=armv7m-none-eabi \
		$(DEBUG_CFLAGS) \
		-x c
