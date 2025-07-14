/* SPDX-License-Identifier: GPL-2.0+
 *
 * ft800-splash.c – ultra-small early-boot splash for Bridgetek FT800
 *
 * - waits for /dev/ft800 (handled by systemd unit “After=dev-ft800.device”)
 * - pushes a dark-blue background and centred “Loading…” text
 * - swaps the list and polls until the FIFO drains
 *
 * Tested on AM335x BeagleBone-Black, Yocto “scarthgap”, busybox userspace.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ft800_uapi.h"           /* installed by your kernel headers */

/* -------------------------------------------------------------------- */
#define ARRAY_LEN(x)   (sizeof(x) / sizeof((x)[0]))

static int wait_fifo_empty(int fd)
{
	struct ft800_status st;
	const struct timespec slp = { 0, 10 * 1000 * 1000 };	/* 10 ms */
	int ms = 0;

	for (;;) {
		if (ioctl(fd, FT800_IOCTL_GET_STATUS, &st) < 0)
			return -errno;

		if (st.cmd_read == st.cmd_write)
			return 0;                       /* drained */

		if (ms >= 500)                       /* 0.5 s safety net */
			return -ETIMEDOUT;

		nanosleep(&slp, NULL);
		ms += 10;
	}
}

static int show_splash(int fd)
{
	struct ft800_cmd cmd[6] = { { 0 } };     /* 6 slots = 24 bytes */

	/* build the command list */
	cmd[0].cmd = CMD_DLSTART;
	cmd[1].cmd = CLEAR_COLOR_RGB(0, 0, 60);  /* navy blue bg       */
	cmd[2].cmd = CLEAR(1, 1, 1);             /* wipe all buffers   */

	cmd[3].cmd     = CMD_TEXT;               /* “Loading…” string  */
	cmd[3].x       = 240;                    /* centre on 480×272  */
	cmd[3].y       = 136;
	cmd[3].font    = 31;                     /* largest ROM font   */
	cmd[3].options = 0;                      /* OPT_CENTER         */
	strncpy(cmd[3].text, "Loading…", FT800_MAX_CMD_TEXT_LEN);

	cmd[4].cmd = DISPLAY();                  /* terminate DL       */
	cmd[5].cmd = CMD_SWAP;                   /* flip when ready    */

	struct ft800_uapi_exec_cmds ex = {
		.user_ptr = (uint64_t)(uintptr_t)cmd,
		.count    = ARRAY_LEN(cmd)
	};

	if (ioctl(fd, FT800_IOCTL_EXEC_CMDS, &ex) < 0)
		return -errno;

	return wait_fifo_empty(fd);
}
/* -------------------------------------------------------------------- */

int main(void)
{
	int fd = open("/dev/ft800", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		perror("open /dev/ft800");
		return 1;
	}

	int rc = show_splash(fd);
	if (rc < 0)
		fprintf(stderr, "ft800-splash: %s\n", strerror(-rc));

	close(fd);
	return rc ? 1 : 0;
}

