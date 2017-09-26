#include <bcm2835.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define xstr(s) str(s)
#define str(s) #s

#define RX 7
#define TX 8

#define BIT_DUR (2000)
#define SPACE_DUR (BIT_DUR * 200)

#define GPIO_PATH(PIN, NODE) "/sys/class/gpio/gpio" str(PIN) "/" NODE

static uint64_t ustime() {
	static struct timespec start = {0, 0};
	if (start.tv_sec == 0) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		return 0;
	}
	struct timespec ts = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((ts.tv_sec - start.tv_sec) * 1000000) + (ts.tv_nsec - start.tv_nsec) / 1000;
}

static void export_pins() {
	int fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, xstr(RX) "\n", sizeof(xstr(RX) "\n"));
	//write(fd, str(TX) "\n", sizeof(str(TX) "\n"));
	close(fd);
}

static void unexport_pins() {
	int fd = open("/sys/class/gpio/unexport", O_WRONLY);
	write(fd, xstr(RX) "\n", sizeof(xstr(RX) "\n"));
	//write(fd, str(TX) "\n", sizeof(str(TX) "\n"));
	close(fd);
}

static void wait_pin(int fd) {
	char buf[8];
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, sizeof(buf));
	struct pollfd pfd = {fd, POLLPRI | POLLERR, 0};
	poll(&pfd, 1, -1);
}

int main() {

	if (!bcm2835_init())
		return 1;

	bcm2835_gpio_fsel(RX, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(RX, BCM2835_GPIO_PUD_UP);

	bcm2835_gpio_fsel(TX, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_clr(TX);

	export_pins();
	atexit(unexport_pins);

	{
		int rx_edge = open(GPIO_PATH(RX, "edge"), O_WRONLY);
		write(rx_edge, "both", sizeof("both"));
		close(rx_edge);
	}

	int rx = open(GPIO_PATH(RX, "value"), O_RDONLY);

	{
		struct sched_param sp = {0};
		sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
		if (-1 == sched_setscheduler(0, SCHED_FIFO, &sp)) {
			return 2;
		}
		mlockall(MCL_CURRENT | MCL_FUTURE);
	}

	uint64_t last_time = 0;

	for (;;) {
		printf("\n");

		bcm2835_gpio_clr(TX);

		// Wait for a fresh packet.
		for (uint64_t last = 0, now = 0;; last = now) {
			if (bcm2835_gpio_lev(RX))
				continue;
			wait_pin(rx);
			now = ustime();
			if ((now - last) > SPACE_DUR)
				break;
		}

		// OK, let's do this. Start bit.
		if (!bcm2835_gpio_lev(RX)) {
			fprintf(stderr, "Weird, expected a leading 1.\n");
			continue;
		}

		// Then we seem to have a zero byte.
		wait_pin(rx);
		if (bcm2835_gpio_lev(RX)) {
			fprintf(stderr, "Er, that should have been followed by a zero.\n");
			continue;
		}


#if 0
		bcm2835_gpio_ren(RX);
		bcm2835_gpio_set_eds(RX);

		uint64_t sync_start = ustime();
		while (!bcm2835_gpio_eds(RX));
		bcm2835_gpio_set_eds(RX);
		uint64_t bit_dur = (ustime() - sync_start) / 8;
		if (bit_dur < (BIT_DUR * 0.9) || bit_dur > (BIT_DUR * 1.1)) {
			fprintf(stderr, "Got a bit duration of %llu, timing seems to be off.\n", bit_dur);
			continue;
		}

		bcm2835_gpio_fen(RX);
		bcm2835_gpio_set_eds(RX);
		bcm2835_gpio_set(TX);

		// Drop us in the middle of the first "real" bit.
#endif
		//usleep(BIT_DUR * 0.0);

		bcm2835_gpio_set(TX);

		usleep(BIT_DUR);

		for (int i = 0; i < 20; i++) {
			char byte = 0;

			// // Start bit
			// if (!bcm2835_gpio_lev(RX)) {
			// 	fprintf(stderr, "No start bit, bail.\n");
			// 	break;
			// }
			for (int i = 0; i < 8; i++) {
				byte <<= 1;
				byte |= bcm2835_gpio_lev(RX);
				usleep(BIT_DUR);
			}

			printf("%02X ", byte);

			usleep(BIT_DUR);

			// Lel
			if (i == 8) {
				wait_pin(rx);
				printf("| ");
				i++;
			}
		}
	}

#if 0
	if (!bcm2835_init())
		return 1;

	{
		struct sched_param sp = {0};
		sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
		if (-1 == sched_setscheduler(0, SCHED_FIFO, &sp)) {
			return 2;
		}
		mlockall(MCL_CURRENT | MCL_FUTURE);
	}

	bcm2835_gpio_fsel(RX, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(RX, BCM2835_GPIO_PUD_UP);
	//bcm2835_gpio_ren(RX);
	bcm2835_gpio_clr_ren(RX);
	//bcm2835_gpio_fen(RX);
	bcm2835_gpio_clr_fen(RX);
	//bcm2835_gpio_set_eds(RX);

	bcm2835_gpio_fsel(TX, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_clr(TX);

	struct timespec ts;
	int badcount = 0;
	int fallcount = 0;

	int on = 0;


	for (;;) {
		if (bcm2835_gpio_lev(RX))
			bcm2835_gpio_set(TX);
		else
			bcm2835_gpio_clr(TX);
#if 0
		while (bcm2835_gpio_lev(RX))
			badcount++;
			continue;
		fallcount += 1;
		if (fallcount % 10 == 0) {
			clock_gettime(CLOCK_MONOTONIC, &ts);
			printf("Fall: %d %d %d %d\n", badcount, fallcount, ts.tv_sec, ts.tv_nsec);
		}
		//printf("%d -> %d\n", pin, bcm2835_gpio_lev(RX));
		//usleep(1000);
		//bcm2835_gpio_set_eds(RX);
#endif
	}
#endif
}
