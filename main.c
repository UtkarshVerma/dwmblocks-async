#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define STR2(a) #a
#define STR(a) STR2(a)
#define BLOCK(cmd, interval, signal) {"echo \"" STR(__COUNTER__) "$(" cmd ")\"", interval, signal},
typedef struct {
	const char *command;
	const unsigned int interval;
	const unsigned int signal;
} Block;
#include "config.h"

static Display *dpy;
static int screen;
static Window root;
static char outputs[LEN(blocks)][CMDLENGTH + 2];
static char statusBar[2][LEN(blocks) * ((LEN(outputs[0]) - 1) + (LEN(DELIMITER) - 1)) + 1];
static int statusContinue = 1;
static volatile sig_atomic_t updatedBlocks = 0;
void (*writeStatus)();
static int pipeFD[2];

int gcd(int a, int b) {
	int temp;
	while (b > 0) {
		temp = a % b;
		a = b;
		b = temp;
	}
	return a;
}

void replace(char *str, char old, char new) {
	for (char *ch = str; *ch; ch++)
		if (*ch == old)
			*ch = new;
}

void getCommand(int i, const char *button) {
	if (fork() == 0) {
		dup2(pipeFD[1], STDOUT_FILENO);
		close(pipeFD[0]);

		if (button)
			setenv("BLOCK_BUTTON", button, 1);
		execl("/bin/sh", "sh", "-c", blocks[i].command);
	}
}

void getCommands(int time) {
	for (int i = 0; i < LEN(blocks); i++)
		if (time == 0 || (blocks[i].interval != 0 && time % blocks[i].interval == 0))
			getCommand(i, NULL);
}

void getSignalCommand(int signal) {
	for (int i = 0; i < LEN(blocks); i++)
		if (blocks[i].signal == signal)
			getCommand(i, NULL);
}

int getStatus(char *new, char *old) {
	strcpy(old, new);
	new[0] = 0;

	for (int i = 0; i < LEN(blocks); i++) {
		#ifdef TRAILING_DELIMITER
		if (strlen(outputs[i]) > (blocks[i].signal > 0))
		#else
		if (strlen(new) && strlen(outputs[i]) > (blocks[i].signal > 0))
		#endif
			strcat(new, DELIMITER);
		strcat(new, outputs[i]);
	}
	new[strlen(new)] = 0;
	return strcmp(new, old);
}

void debug() {
	// Only write out if text has changed
	if (!getStatus(statusBar[0], statusBar[1]))
		return;

	write(STDOUT_FILENO, statusBar[0], strlen(statusBar[0]));
	write(STDOUT_FILENO, "\n", 1);
}

void setRoot() {
	// Only set root if text has changed
	if (!getStatus(statusBar[0], statusBar[1]))
		return;

	Display *d = XOpenDisplay(NULL);
	if (d)
		dpy = d;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusBar[0]);
	XCloseDisplay(dpy);
}

void signalHandler(int sig, siginfo_t *si, void *ucontext) {
	sig -= SIGRTMIN;
	int i = 0;
	while (blocks[i].signal != sig)
		i++;
	const char button[2] = {'0' + si->si_value.sival_int & 0xff, 0};
	getCommand(i, button);
}

void termHandler(int signal) {
	statusContinue = 0;
	exit(EXIT_SUCCESS);
}

void childHandler() {
	char i;
	read(pipeFD[0], &i, 1);
	i -= '0';

	char ch;
	char buffer[LEN(outputs[0]) - 1];
	int j = 0;
	while (j < LEN(buffer) - 1 && read(pipeFD[0], &ch, 1) == 1 && ch != '\n')
		buffer[j++] = ch;
	buffer[j] = 0;

	// Clear the pipe until newline
	while (ch != '\n' && read(pipeFD[0], &ch, 1) == 1);

	char *output = outputs[i];
	if (blocks[i].signal > 0) {
		output[0] = blocks[i].signal;
		output++;
	}

	// Don't write stale output from the pipe. This only happens when signals
	// are received in a rapid succession.
	if ((updatedBlocks & (1 << i)) == 0) {
		updatedBlocks |= 1 << i;
		strcpy(output, buffer);
		writeStatus();
	}
	updatedBlocks &= ~(1 << i);
}

void setupSignals() {
	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);

	// Handle block update signals
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = signalHandler;
	for (int i = 0; i < LEN(blocks); i++) {
		if (blocks[i].signal > 0)
			sigaction(SIGRTMIN + blocks[i].signal, &sa, NULL);
	}

	// Handle exit of forks
	signal(SIGCHLD, childHandler);
}

void statusLoop() {
	getCommands(0);

	unsigned int sleepInterval = -1;
	for (int i = 0; i < LEN(blocks); i++)
		if (blocks[i].interval)
			sleepInterval = gcd(blocks[i].interval, sleepInterval);

	unsigned int i = 0;
	struct timespec sleepTime = {sleepInterval, 0};
	struct timespec toSleep = sleepTime;

	while (statusContinue) {
		// Sleep for `sleepTime` even on being interrupted
		if (nanosleep(&toSleep, &toSleep) == -1)
			continue;

		// Write to status after sleeping
		getCommands(i);

		// After sleep, reset timer and update counter
		toSleep = sleepTime;
		i += sleepInterval;
	}
}

int main(int argc, char **argv) {
	pipe(pipeFD);
	writeStatus = setRoot;
	for (int i = 0; i < argc; i++)
		if (!strcmp("-d", argv[i]))
			writeStatus = debug;

	setupSignals();
	statusLoop();
}
