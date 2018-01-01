/*
 * pigpio-clackws.c:
 *
 * Copyright (c) 2017 Peter Lieven
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <pigpio.h>

#define MAX_PINS 54
#define VOLATILE_DIR "/run/pigpio-clackws"
#define STATIC_DIR "/var/lib/pigpio-clackws"
#define INT_TIMEOUT_MS 300000

static unsigned g_monitor, g_debug, g_exit, g_dumpint = 300000000;
static char *g_arg0;

void usage(void) {
	fprintf(stderr, "Usage: %s [-d] [-m] [-t dumpIntSecs]\n", g_arg0);
	fprintf(stderr, "\n         -t  dump counters and periods each dumpIntSecs to non-volatile files (default 300s)");
	fprintf(stderr, "\n         -m  monitor mode, do not read or write any files (implies -d)");
	fprintf(stderr, "\n         -d  debug mode, log every interrupt\n");
	exit(1);
}

void signal_handler(int signal) {
	g_exit = 1;
}

void ignore_signal(int signal) {
}

int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int c;
	g_arg0 = argv[0];

	while ((c = getopt(argc, argv, "dmw:UDt:p:P:")) != -1) {
		switch (c) {
		case 'd':
			g_debug = 1;
			break;
		case 'm':
			g_monitor = 1;
			g_debug = 1;
			break;
		case 't':
			g_dumpint = atoi(optarg) * 1000000;
			break;
		default:
			usage();
		}
	}

	if (optind != argc) usage();

	if (g_monitor) {
		fprintf(stderr, "MONITOR MODE, will not read or write any files\n");
	} else {
		fprintf(stderr, "will read/write static counter information to %s\n", STATIC_DIR);
		if (mkdir(STATIC_DIR, 0755) && errno != EEXIST) {
			fprintf(stderr, "failed to create directory: %s\n", strerror(errno));
			exit(1);
		}
		fprintf(stderr, "will read/write volatile counter information to %s\n", VOLATILE_DIR);
		if (mkdir(VOLATILE_DIR, 0755) && errno != EEXIST) {
			fprintf(stderr, "failed to create directory: %s\n", strerror(errno));
			exit(1);
		}
	}

	if (gpioInitialise() == PI_INIT_FAILED) {
		fprintf(stderr, "gpioInitialise() failed\n");
		exit(1);
	}

	gpioSetSignalFunc(SIGTERM, &signal_handler);
	gpioSetSignalFunc(SIGINT, &signal_handler);
	gpioSetSignalFunc(SIGCONT, &ignore_signal);

	while (1) {
		if (g_exit) break;
		gpioDelay(20000);
	}

	gpioTerminate();
	exit(0);
}
