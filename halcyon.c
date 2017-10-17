#include <asm/termbits.h>
#include <bcm2835.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define SERIAL_DIVISOR 6000

uint64_t ustime() {
	struct timespec time;
	if (clock_gettime(CLOCK_MONOTONIC, &time) != 0)
		return errno;
	return (time.tv_sec * 1000000) + (time.tv_nsec / 1000);
}

int main() {

#if 0
	if (!bcm2835_init())
		return 1;

	bcm2835_gpio_fsel(RX, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(RX, BCM2835_GPIO_PUD_UP);

	bcm2835_gpio_fsel(TX, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_clr(TX);
#endif

	int rx = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
	fcntl(rx, F_SETFL, 0);

	// https://stackoverflow.com/a/19992472/84745
	struct termios2 tio;

	ioctl(rx, TCGETS2, &tio);
	tio.c_cflag &= ~(CBAUD | CSIZE | CSTOPB);
	tio.c_cflag |= CS8|PARENB|PARODD|BOTHER|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO);

	tio.c_ispeed = 500;
	tio.c_ospeed = 500;

	tio.c_cc[VMIN] = 8;
	tio.c_cc[VTIME] = 0;

	if (ioctl(rx, TCSETS2, &tio)) {
		return 1;
	}

	unsigned char outbuf[][8] = {
		{0x04, 0x81, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x04, 0x85, 0x00, 0x69, 0x18, 0x00, 0xcc, 0x00},
		{0x04, 0x81, 0x00, 0x69, 0x18, 0x00, 0xcc, 0x00},
		{0x04, 0x81, 0x10, 0xe8, 0x18, 0x00, 0xcc, 0x00},
		{0x04, 0x81, 0x00, 0xe8, 0x18, 0x00, 0x2c, 0x00},
	};
	for (size_t j = 0; j < (sizeof(outbuf) / sizeof(*outbuf[0])); j++) {
		for (size_t i = 0; i < (sizeof(outbuf[j]) / sizeof(*outbuf[j])); i++) {
			outbuf[j][i] = ~outbuf[j][i];
		}
	}

	int seq = 0;
	//int sendbudget = 4;

	unsigned char buf[64];
	ssize_t len;
	while ((len = read(rx, buf, sizeof(buf) / sizeof(*buf))) > 0) {
		printf("%lld ", ustime());
		for (size_t i = 0; i < len; i++) {
			buf[i] = ~buf[i];
			printf("%02x ", buf[i]);
		}
		printf("\n");
#if 0
		if (len == 8 && buf[0] == 0x00 && buf[7] == 0x04) {
			write(rx, outbuf[seq], sizeof(outbuf[seq]) / sizeof(*outbuf[seq]));
			if (seq < 4) seq++;
		}
#endif
	}
}

