#include <asm/termbits.h>
#include <bcm2835.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define SERIAL_DIVISOR 6000

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

	// https://stackoverflow.com/a/19992472/84745
	struct termios2 tio;

	ioctl(rx, TCGETS2, &tio);
	tio.c_cflag &= ~(CBAUD | CSIZE);
	tio.c_cflag |= CS8|PARENB|PARODD|BOTHER;
	tio.c_lflag &= ~ICANON;

	tio.c_ispeed = 500;
	tio.c_ospeed = 500;

	tio.c_cc[VMIN] = 16;
	tio.c_cc[VTIME] = 5;

	if (ioctl(rx, TCSETS2, &tio)) {
		return 1;
	}

	unsigned char buf[256];
	ssize_t len;
	while ((len = read(rx, buf, sizeof(buf) / sizeof(*buf))) > 0) {
		for (size_t i = 0; i < len; i++) {
			buf[i] = ~buf[i];
			printf("%02x ", buf[i]);
		}
		printf("\n");
	}
}

