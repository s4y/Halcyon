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

#define RX 23
#define TX 24

#define BIT_DUR (2000)
#define SHORT_SPACE_DUR (BIT_DUR * 20)
#define LONG_SPACE_DUR (BIT_DUR * 200)

#define GPIO_PATH(PIN, NODE) "/sys/class/gpio/gpio" str(PIN) "/" NODE

// https://stackoverflow.com/questions/44116820/
static int iround(int x, int multiple) {
    return (x + (multiple / 2)) / multiple * multiple;
}

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

typedef enum {
	INPUT_STATE_WAIT_FRAME,
	INPUT_STATE_HAVE_BYTE,
	INPUT_STATE_WANT_STOP_BIT,
	INPUT_STATE_WANT_PARITY_BIT,
	INPUT_STATE_BIT_7,
	INPUT_STATE_BIT_6,
	INPUT_STATE_BIT_5,
	INPUT_STATE_BIT_4,
	INPUT_STATE_BIT_3,
	INPUT_STATE_BIT_2,
	INPUT_STATE_BIT_1,
	INPUT_STATE_BIT_0,
} input_state;

typedef struct {
	input_state state;
	int newframe;
	char byte;
	const char* err;
} input_context;

void input_machine(input_context* context, int lev) {
	switch (context->state) {
		case INPUT_STATE_WAIT_FRAME:
			break;
		case INPUT_STATE_HAVE_BYTE:
			if (!lev) {
				context->err = "Definitely expected a start bit.";
				goto fail;
			}
			context->byte = 0;
			context->state = INPUT_STATE_BIT_0;
			break;
		case INPUT_STATE_WANT_STOP_BIT:
		case INPUT_STATE_WANT_PARITY_BIT:
			context->state--;
			// TODO
			break;
		case INPUT_STATE_BIT_7:
		case INPUT_STATE_BIT_6:
		case INPUT_STATE_BIT_5:
		case INPUT_STATE_BIT_4:
		case INPUT_STATE_BIT_3:
		case INPUT_STATE_BIT_2:
		case INPUT_STATE_BIT_1:
		case INPUT_STATE_BIT_0:
			context->byte <<= 1;
			context->byte |= lev;
			context->state--;
			break;
	}
	return;
fail:
	context->state = INPUT_STATE_WAIT_FRAME;
}

void input(input_context* context, int pin, int fd) {
	const uint64_t start = ustime();
	wait_pin(fd);
	const uint64_t dur = ustime() - start;
	const int lev = bcm2835_gpio_lev(pin);

	//fprintf(stderr, "read %u after %llu\n", bcm2835_gpio_lev(pin), dur);

	context->err = NULL;

	if (dur > SHORT_SPACE_DUR) {
		context->state = INPUT_STATE_HAVE_BYTE;
		context->newframe = (dur > LONG_SPACE_DUR) ? 2 : 1;
	} else {
		const int bits = iround(dur, BIT_DUR) / BIT_DUR;
		for (int i = 0; i < bits; i++)
			input_machine(context, !lev);
	}
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

	input_context ctx = {0};
	for (;;) {
		input(&ctx, RX, rx);
		if (ctx.err) {
			fprintf(stderr, "state: %u, byte: %02x, err: %s\n", ctx.state, ctx.byte, ctx.err);
		}
		if (ctx.newframe == 2)
			printf("\n");
		else if (ctx.newframe == 1)
			printf("| ");
		ctx.newframe = 0;
		if (ctx.state == INPUT_STATE_HAVE_BYTE)
			printf("%02x ", ctx.byte);
	}

#if 0
	uint64_t last_time = 0;

	for (;;) {
		printf("\n");

		//bcm2835_gpio_clr(TX);

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

		//bcm2835_gpio_set(TX);

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
#endif
}
