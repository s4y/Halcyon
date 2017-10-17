#include <asm/termbits.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define SERIAL_DIVISOR 6000

#define LISTEN_PORT 9972

uint64_t ustime() {
	struct timespec time;
	if (clock_gettime(CLOCK_MONOTONIC, &time) != 0)
		return errno;
	return (time.tv_sec * 1000000) + (time.tv_nsec / 1000);
}

int openserial() {
	int ret = open(SERIAL_PORT, O_RDWR | O_NOCTTY);

	// https://stackoverflow.com/a/19992472/84745
	struct termios2 tio;

	ioctl(ret, TCGETS2, &tio);
	tio.c_cflag &= ~(CBAUD | CSIZE | CSTOPB);
	tio.c_cflag |= CS8|PARENB|PARODD|BOTHER|CLOCAL;
	tio.c_lflag &= ~(ICANON | ECHO);

	tio.c_ispeed = 500;
	tio.c_ospeed = 500;

	tio.c_cc[VMIN] = 8;
	tio.c_cc[VTIME] = 0;

	if (ioctl(ret, TCSETS2, &tio))
		return -1;
	return ret;
}

int main() {

	signal(SIGPIPE, SIG_IGN);

	int lsock = socket(AF_INET, SOCK_STREAM, 0);
	{
		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(LISTEN_PORT);
		bind(lsock, (struct sockaddr *)&addr, sizeof(addr));
	}
	listen(lsock, 0);

	for (;;) {
		int rsock = accept(lsock, NULL, 0);
		int serial = openserial();
		if (serial < 0)
			return 1;

		unsigned char buf[64];
		ssize_t len;
		while ((len = read(serial, buf, sizeof(buf) / sizeof(*buf))) > 0) {
			dprintf(rsock, "%lld ", ustime());
			for (size_t i = 0; i < len; i++) {
				buf[i] = ~buf[i];
				dprintf(rsock, "%02x ", buf[i]);
			}
			if (send(rsock, "\n", 1, 0) < 0)
				break;
		}
		close(rsock);
		close(serial);
	}
}
