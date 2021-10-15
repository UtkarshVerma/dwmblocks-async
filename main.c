#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <time.h>
#include <unistd.h>

#define LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define BLOCK(cmd, interval, signal) {"echo \"_$(" cmd ")\"", interval, signal},
typedef struct {
	const char* command;
	const unsigned int interval;
	const unsigned int signal;
} Block;
#include "config.h"

static Display* dpy;
static int screen;
static Window root;
static unsigned short int statusContinue = 1;
static char outputs[LEN(blocks)][CMDLENGTH + 2];
static char statusBar[2][LEN(blocks) * ((LEN(outputs[0]) - 1) + (LEN(DELIMITER) - 1)) + 1];
static struct epoll_event event, events[LEN(blocks) + 2];
static int pipes[LEN(blocks)][2];
static int timerFD[2];
static int signalFD;
static int epollFD;
void (*writeStatus)();

int gcd(int a, int b) {
	int temp;
	while (b > 0) {
		temp = a % b;
		a = b;
		b = temp;
	}
	return a;
}

void execBlock(int i, const char* button) {
	if (fork() == 0) {
		dup2(pipes[i][1], STDOUT_FILENO);
		close(pipes[i][0]);

		if (button)
			setenv("BLOCK_BUTTON", button, 1);
		execl("/bin/sh", "sh", "-c", blocks[i].command);
	}
}

void execBlocks(unsigned long long int time) {
	for (int i = 0; i < LEN(blocks); i++)
		if (time == 0 || (blocks[i].interval != 0 && time % blocks[i].interval == 0))
			execBlock(i, NULL);
}

int getStatus(char* new, char* old) {
	strcpy(old, new);
	new[0] = '\0';

	for (int i = 0; i < LEN(blocks); i++) {
#ifdef TRAILING_DELIMITER
		if (strlen(outputs[i]) > (blocks[i].signal > 0))
#else
		if (strlen(new) && strlen(outputs[i]) > (blocks[i].signal > 0))
#endif
			strcat(new, DELIMITER);
		strcat(new, outputs[i]);
	}
	return strcmp(new, old);
}

void updateBlock(int i) {
	char* output = outputs[i];
	char buffer[LEN(outputs[0])];
	int bytesRead = read(pipes[i][0], buffer, LEN(buffer));
	buffer[bytesRead - 1] = '\0';

	// Clear the pipe
	if (bytesRead == LEN(buffer)) {
		char ch;
		while (read(pipes[i][0], &ch, 1) == 1 && ch != '\n')
			;
	}

	if (blocks[i].signal > 0)
		buffer[0] = blocks[i].signal;

	strcpy(output, buffer);
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

	Display* d = XOpenDisplay(NULL);
	if (d)
		dpy = d;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusBar[0]);
	XCloseDisplay(dpy);
}

void signalHandler() {
	struct signalfd_siginfo info;
	read(signalFD, &info, sizeof(info));

	for (int j = 0; j < LEN(blocks); j++) {
		if (blocks[j].signal == info.ssi_signo - SIGRTMIN) {
			char button[] = {'0' + info.ssi_int & 0xff, 0};
			execBlock(j, button);
			break;
		}
	}
}

void termHandler() {
	statusContinue = 0;
}

void setupSignals() {
	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);

	// Avoid zombie subprocesses
	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &sa, 0);

	// Handle block update signals
	sigset_t sigset;
	sigemptyset(&sigset);
	for (int i = 0; i < LEN(blocks); i++)
		if (blocks[i].signal > 0)
			sigaddset(&sigset, SIGRTMIN + blocks[i].signal);

	signalFD = signalfd(-1, &sigset, 0);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	event.data.u32 = LEN(blocks) + 1;
	epoll_ctl(epollFD, EPOLL_CTL_ADD, signalFD, &event);
}

void init() {
	epollFD = epoll_create(LEN(blocks) + 1);
	event.events = EPOLLIN;

	for (int i = 0; i < LEN(blocks); i++) {
		pipe(pipes[i]);
		event.data.u32 = i;
		epoll_ctl(epollFD, EPOLL_CTL_ADD, pipes[i][0], &event);
	}

	pipe(timerFD);
	event.data.u32 = LEN(blocks);
	epoll_ctl(epollFD, EPOLL_CTL_ADD, timerFD[0], &event);

	setupSignals();
}

void statusLoop() {
	execBlocks(0);

	while (statusContinue) {
		int eventCount = epoll_wait(epollFD, events, LEN(events), 1000);

		for (int i = 0; i < eventCount; i++) {
			unsigned int id = events[i].data.u32;

			if (id == LEN(blocks)) {
				unsigned long long int j = 0;
				read(timerFD[0], &j, sizeof(j));
				execBlocks(j);
			} else if (id < LEN(blocks)) {
				updateBlock(events[i].data.u32);
			} else {
				signalHandler();
			}
		}
		if (eventCount)
			writeStatus();

		// Poll every 100ms
		struct timespec toSleep = {0, 100000L};
		nanosleep(&toSleep, &toSleep);
	}
}

void timerLoop() {
	close(timerFD[0]);

	unsigned int sleepInterval = -1;
	for (int i = 0; i < LEN(blocks); i++)
		if (blocks[i].interval)
			sleepInterval = gcd(blocks[i].interval, sleepInterval);

	unsigned long long int i = 0;
	struct timespec sleepTime = {sleepInterval, 0};
	struct timespec toSleep = sleepTime;

	while (1) {
		// Sleep for `sleepTime` even on being interrupted
		if (nanosleep(&toSleep, &toSleep) == -1)
			continue;

		// Notify parent to update blocks
		write(timerFD[1], &i, sizeof(i));

		// After sleep, reset timer and update counter
		toSleep = sleepTime;
		i += sleepInterval;
	}
}

int main(const int argc, const char* argv[]) {
	writeStatus = setRoot;
	for (int i = 0; i < argc; i++)
		if (!strcmp("-d", argv[i]))
			writeStatus = debug;

	init();

	if (fork() == 0)
		timerLoop();
	else
		statusLoop();

	close(epollFD);
	return 0;
}
