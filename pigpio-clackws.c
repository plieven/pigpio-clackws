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

#define FLOW_PIN 17
#define VOLATILE_DIR "/run/pigpio-clackws"
#define STATIC_DIR "/var/lib/pigpio-clackws"
#define SAMPLE_DELAY 10000000

static volatile uint64_t g_totalCount;
static volatile uint64_t g_lastInterrupt;
static unsigned g_monitor, g_debug, g_exit, g_dumpint = 300000000;
static char *g_arg0;

void myInterrupt(int pin, int level, uint32_t tick) {
	g_totalCount++;
	g_lastInterrupt = tick;
}

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
	unsigned lastdump, lastPeriod;
	g_arg0 = argv[0];
	char buf[PATH_MAX];
	FILE *file;
	uint64_t last;

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

	fprintf(stderr, "setting GPIO%d for direction IN -> ", FLOW_PIN);
	if (gpioSetMode(FLOW_PIN, PI_INPUT)) fprintf(stderr, "FAILED\n"); else fprintf(stderr, "OK\n");
	fprintf(stderr, "disabling pull-ups for GPIO%d -> ", FLOW_PIN);
	if (gpioSetPullUpDown(FLOW_PIN, PI_PUD_OFF)) fprintf(stderr, "FAILED\n"); else fprintf(stderr, "OK\n"); 
	fprintf(stderr, "setting up interrupt handler for GPIO%d -> ", FLOW_PIN);
	if (gpioSetISRFunc(FLOW_PIN, EITHER_EDGE, 0, &myInterrupt)) fprintf(stderr, "FAILED\n"); else fprintf(stderr, "OK\n");

	if (!g_monitor) {
		snprintf(buf, PATH_MAX, "%s/totalCount", STATIC_DIR);
		file = fopen(buf, "r");
		if (file) {
			if (fscanf (file, "%"PRIu64, &g_totalCount) == 1) {
				fprintf(stderr, "initialized totalCount for GPIO%d to %" PRIu64 " from %s\n", FLOW_PIN, g_totalCount, buf);
			}
			fclose(file);
		}
	}

	lastdump = gpioTick();

	while (1) {
		unsigned now = gpioTick();
		unsigned diff = now - lastdump;
		if (!g_monitor && (g_exit || diff >= g_dumpint)) {
			snprintf(buf, PATH_MAX, "%s/totalCount", STATIC_DIR);
			file = fopen(buf, "w");
			if (file) {
				fprintf(file, "%" PRIu64 "\n", g_totalCount);
				fprintf(stderr, "updated %s for GPIO%d to %" PRIu64 "\n", buf, FLOW_PIN, g_totalCount);
				fclose(file);
			} else {
				fprintf(stderr, "cannot open %s for writing: %s", buf, strerror(errno));
				exit(1);
			}
			lastdump = now;
		}
		if (g_exit) break;
		last = g_totalCount;
		gpioDelay(SAMPLE_DELAY);
		last = g_totalCount - last;
		lastPeriod = last ? (SAMPLE_DELAY / last) : -1;
		lastPeriod /= 1000;
		snprintf(buf, PATH_MAX, "%s/totalCount", VOLATILE_DIR);
		file = fopen(buf, "w");
		if (file) {
			fprintf(file, "%" PRIu64 "\n", g_totalCount);
			if (g_debug) fprintf(stderr, "updated %s for GPIO%d to %" PRIu64 "\n", buf, FLOW_PIN, g_totalCount);
			fclose(file);
		}
		snprintf(buf, PATH_MAX, "%s/lastPeriod", VOLATILE_DIR);
		file = fopen(buf, "w");
		if (file) {
			fprintf(file, "%u\n", lastPeriod);
			if (g_debug) fprintf(stderr, "updated %s for GPIO%d to %u\n", buf, FLOW_PIN, lastPeriod);
			fclose(file);
		}
		if (g_debug) {
			fprintf(stderr, "totalCount %" PRIu64 " lastPeriod %u ms\n", g_totalCount, lastPeriod);
		}
	}

	gpioTerminate();
	exit(0);
}
