%.hex: %.out
	./dump/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-objcopy -O ihex "$<" "$@"

%.out: %.lds %.o
	./dump/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-ld -o "$@" -T $^

%.o: %.c
	./dump/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-gcc --specs=nosys.specs -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o "$@" -c "$<"
