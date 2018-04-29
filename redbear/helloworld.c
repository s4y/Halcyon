int main() {

	*(unsigned int*)0x50000518 = 1UL << 11;

	*(unsigned int*)0x4001C560 = 11;
	*(unsigned int*)0x4001C500 = 1;

	*(unsigned int*)0x4001C200 = 1 << 2;

	*(unsigned int*)0x4001C504 = 1;
	*(unsigned int*)0x4001C50C = 4;
	*(unsigned int*)0x4001C514 = 1;

	unsigned short pwm_seq0[32];
	for (int i = 0; i < 32; i++)
		pwm_seq0[i] = (1024 / 32) * (i + 1);
	*(const unsigned short**)0x4001C520 = pwm_seq0;
	*(unsigned int*)0x4001C524 = sizeof(pwm_seq0) / sizeof(*pwm_seq0);
	*(unsigned int*)0x4001C52C = 1024;

	unsigned short pwm_seq1[8];
	for (int i = 0; i < 8; i++)
		pwm_seq1[i] = (1024 / 8) * (8 - i);
	*(const unsigned short**)0x4001C540 = pwm_seq1;
	*(unsigned int*)0x4001C544 = sizeof(pwm_seq1) / sizeof(*pwm_seq1);

	*(unsigned int*)0x4001C00C = 1;

	for (;;) __asm__ ("wfe");
	return 0;
}
