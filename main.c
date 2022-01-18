#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <time.h>
#include <unistd.h>

#define LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define MAX(a, b) (a > b ? a : b)

#define POLL_INTERVAL 50
#define BLOCK(cmd, interval, signal) \
	{ "echo \"$(" cmd ")\"", interval, signal }

typedef const struct {
	const char* command;
	const unsigned int interval;
	const unsigned int signal;
} Block;
#include "config.h"

#ifdef CLICKABLE_BLOCKS
#undef CLICKABLE_BLOCKS
#define CLICKABLE_BLOCKS 1
#else
#define CLICKABLE_BLOCKS 0
#endif

#ifdef LEADING_DELIMITER
#undef LEADING_DELIMITER
#define LEADING_DELIMITER 1
#else
#define LEADING_DELIMITER 0
#endif

static Display* dpy;
static int screen;
static Window root;
static unsigned short int statusContinue = 1;
static char outputs[LEN(blocks)][CMDLENGTH + 1 + CLICKABLE_BLOCKS];
static char statusBar[2][LEN(blocks) * (LEN(outputs[0]) - 1) + (LEN(blocks) - 1 + LEADING_DELIMITER) * (LEN(DELIMITER) - 1) + 1];
static struct epoll_event event, events[LEN(blocks) + 2];
static int pipes[LEN(blocks)][2];
static int timerPipe[2];
static int signalFD;
static int epollFD;
static unsigned int execLock = 0;
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

void closePipe(int* pipe) {
	close(pipe[0]);
	close(pipe[1]);
}

void execBlock(int i, const char* button) {
	// Ensure only one child process exists per block at an instance
	if (execLock & 1 << i)
		return;
	// Lock execution of block until current instance finishes execution
	execLock |= 1 << i;

	if (fork() == 0) {
		close(pipes[i][0]);
		dup2(pipes[i][1], STDOUT_FILENO);

		if (button)
			setenv("BLOCK_BUTTON", button, 1);
		execl("/bin/sh", "sh", "-c", blocks[i].command, (char*)NULL);
	}
}

void execBlocks(unsigned int time) {
	for (int i = 0; i < LEN(blocks); i++)
		if (time == 0 || (blocks[i].interval != 0 && time % blocks[i].interval == 0))
			execBlock(i, NULL);
}

int getStatus(char* new, char* old) {
	strcpy(old, new);
	new[0] = '\0';

	for (int i = 0; i < LEN(blocks); i++) {
#if LEADING_DELIMITER
		if (strlen(outputs[i]))
#else
		if (strlen(new) && strlen(outputs[i]))
#endif
			strcat(new, DELIMITER);
		strcat(new, outputs[i]);
	}
	return strcmp(new, old);
}

void updateBlock(int i) {
	char* output = outputs[i];
	char buffer[LEN(outputs[0]) - CLICKABLE_BLOCKS];
	int bytesRead = read(pipes[i][0], buffer, LEN(buffer));

	// Trim UTF-8 characters properly
	int j = bytesRead - 1;
	while ((buffer[j] & 0b11000000) == 0x80)
		j--;
	buffer[j] = ' ';

	// Trim trailing spaces
	while (buffer[j] == ' ')
		j--;
	buffer[j + 1] = '\0';

	if (bytesRead == LEN(buffer)) {
		// Clear the pipe
		char ch;
		while (read(pipes[i][0], &ch, 1) == 1 && ch != '\n')
			;
	}

#if CLICKABLE_BLOCKS
	if (bytesRead > 1 && blocks[i].signal > 0) {
		output[0] = blocks[i].signal;
		output++;
	}
#endif

	strcpy(output, buffer);

	// Remove execution lock for the current block
	execLock &= ~(1 << i);
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

	// Clear the pipe after each poll to limit number of signals handled
	while (read(signalFD, &info, sizeof(info)) != -1)
		;
}

void termHandler() {
	statusContinue = 0;
}

void dummysighandler(int signum)
{
	return;
}

void setupSignals() {
	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);

	// initialize signals with dummy handler
	for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
		signal(i, dummysighandler);

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
	fcntl(signalFD, F_SETFL, O_NONBLOCK);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	event.data.u32 = LEN(blocks) + 1;
	epoll_ctl(epollFD, EPOLL_CTL_ADD, signalFD, &event);
}

void statusLoop() {
	// Poll every `POLL_INTERVAL` milliseconds
	const struct timespec pollInterval = {.tv_nsec = POLL_INTERVAL * 1000000L};
	struct timespec toSleep = pollInterval;

	while (statusContinue) {
		int eventCount = epoll_wait(epollFD, events, LEN(events), 1);
		for (int i = 0; i < eventCount; i++) {
			unsigned int id = events[i].data.u32;

			if (id == LEN(blocks)) {
				unsigned int j = 0;
				read(timerPipe[0], &j, sizeof(j));
				execBlocks(j);
			} else if (id < LEN(blocks)) {
				updateBlock(events[i].data.u32);
			} else {
				signalHandler();
			}
		}
		if (eventCount != -1)
			writeStatus();

		// Sleep for `pollInterval` even on being interrupted
		while (nanosleep(&toSleep, &toSleep) == -1)
			;
		toSleep = pollInterval;
	}
}

void timerLoop() {
	close(timerPipe[0]);

	unsigned int sleepInterval = 0;
	unsigned int maxInterval = 0;
	for (int i = 0; i < LEN(blocks); i++)
		if (blocks[i].interval) {
			maxInterval = MAX(blocks[i].interval, maxInterval);
			sleepInterval = gcd(blocks[i].interval, sleepInterval);
		}

	unsigned int i = 0;
	struct timespec sleepTime = {sleepInterval, 0};
	struct timespec toSleep = sleepTime;

	while (1) {
		// Notify parent to update blocks
		write(timerPipe[1], &i, sizeof(i));

		// Wrap `i` to the interval [1, `maxInterval`]
		i = (i + sleepInterval - 1) % maxInterval + 1;

		// Sleep for `sleepTime` even on being interrupted
		while (nanosleep(&toSleep, &toSleep) == -1)
			;
		toSleep = sleepTime;
	}

	close(timerPipe[1]);
}

void init() {
	epollFD = epoll_create(LEN(blocks) + 1);
	event.events = EPOLLIN;

	for (int i = 0; i < LEN(blocks); i++) {
		pipe(pipes[i]);
		event.data.u32 = i;
		epoll_ctl(epollFD, EPOLL_CTL_ADD, pipes[i][0], &event);
	}

	pipe(timerPipe);
	event.data.u32 = LEN(blocks);
	epoll_ctl(epollFD, EPOLL_CTL_ADD, timerPipe[0], &event);

	setupSignals();
}

int main(const int argc, const char* argv[]) {
	writeStatus = setRoot;
	for (int i = 0; i < argc; i++)
		if (!strcmp("-d", argv[i]))
			writeStatus = debug;

	init();

	if (fork())
		statusLoop();
	else
		timerLoop();

	close(epollFD);
	close(signalFD);
	closePipe(timerPipe);
	for (int i = 0; i < LEN(pipes); i++)
		closePipe(pipes[i]);
	return 0;
}

