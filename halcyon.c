#include <bcm2835.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
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

    struct serial_struct serinfo = {0};

	if (ioctl(rx, TIOCGSERIAL, &serinfo) < 0) {
		fprintf(stderr, "TIOCGSERIAL: %d", errno);
		return 1;
	}

	serinfo.flags &= ~ASYNC_SPD_MASK;
	serinfo.flags |= ASYNC_SPD_CUST;
	serinfo.custom_divisor = SERIAL_DIVISOR;

	if (ioctl(rx, TIOCSSERIAL, &serinfo) < 0) {
		fprintf(stderr, "TIOCSSERIAL: %d", errno);
		return 1;
	}

	if (ioctl(rx, TIOCGSERIAL, &serinfo) < 0) {
		fprintf(stderr, "TIOCGSERIAL: %d", errno);
		return 1;
	}

	printf("rchar: %d, baud_base: %d, custom_divisor: %d\n", serinfo.reserved_char, serinfo.baud_base, serinfo.custom_divisor);

	struct termios tty = {0};

	if (tcgetattr(rx, &tty)) {
		fprintf(stderr, "tcgetattr: %d", errno);
		return 1;
	}

	cfsetospeed(&tty, B38400);
	cfsetispeed(&tty, B38400);
	cfmakeraw(&tty);

	tty.c_cflag &= ~CRTSCTS;
	tty.c_cflag |= CS7|PARENB|PARODD|CREAD|CLOCAL;

	printf("c_cflag %d\n", tty.c_cflag);
	printf("vtime was: %d\n", tty.c_cc[VTIME]);
	tty.c_cc[VMIN] = 16;
	tty.c_cc[VTIME] = 2;

	if (tcsetattr(rx, TCSANOW, &tty)) {
		fprintf(stderr, "tcsetattr: %d", errno);
		return 1;
	}

	unsigned char buf[256];
	ssize_t len;
	while ((len = read(rx, buf, sizeof(buf) / sizeof(*buf))) > 0) {
		printf("read %d bytes\n", len);
		for (size_t i = 0; i < len; i++)
			printf("%02x ", buf[i]);
		printf("\n");
	}
}

