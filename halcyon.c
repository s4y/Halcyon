#include <asm/termbits.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define SERIAL_PORT "/dev/ttyAMA0"
#define SERIAL_DIVISOR 6000

#define LISTEN_PORT 9972

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
		setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
		if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)))
			return 2;
	}
	listen(lsock, 0);

	for (;;) {
		int rsock = accept(lsock, NULL, 0);
		int serial = openserial();
		if (serial < 0)
			return 1;

		unsigned char ceil_buf[7] = {0};
		unsigned char therm_buf[7] = {0};
		unsigned char our_buf[8] = {
			(uint8_t)~0x21,
			(uint8_t)~0x81,
			(uint8_t)~0x00,
			(uint8_t)~0x00,
			(uint8_t)~0x00,
			(uint8_t)~0x20,
			(uint8_t)~0x00,
			(uint8_t)~0x00,
		};

		unsigned char buf[64];
		ssize_t len;
		while ((len = read(serial, buf, sizeof(buf) / sizeof(*buf))) > 0) {
			unsigned char* known_buf = 0;
			if (len == 8) {
				switch (buf[0]) {
					case (uint8_t)~0x00:
						known_buf = ceil_buf;
						break;
					case (uint8_t)~0x20:
						known_buf = therm_buf;
						break;
					default:
						break;
				}
			}
			if (known_buf) {
				int changed = 0;
				for (ssize_t i = 1; i < len; i++) {
					if (buf[i] != known_buf[i-1]) {
						changed = 1;
						known_buf[i-1] = buf[i];
					}
				}
				if (buf[0] == (uint8_t)~0x20) {
					fprintf(stderr, "copy time....\n");
					for (size_t i = 1; i < (sizeof(our_buf) / sizeof(*our_buf)); i++) {
						our_buf[i] = known_buf[i-1];
					}
					our_buf[1] = (uint8_t)~0x81;
					//our_buf[2] = (uint8_t)~0x00;
					our_buf[5] = ceil_buf[2] == (uint8_t)~0x10 ? 0xff : (uint8_t)~0x20;
					fprintf(stderr, "write wait...\n");
					usleep(10000);
					fprintf(stderr, "write time.\n");
					int wrote;
					if ((wrote = write(serial, our_buf, sizeof(our_buf))) > 0) {
						fprintf(stderr, "write good: %d\n", wrote);
					} else {
						fprintf(stderr, "write bad: %d\n", errno);
					}
				}

				if (!changed)
					continue;
			}

			for (ssize_t i = 0; i < len; i++) {
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
