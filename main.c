#include <X11/Xlib.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

#define LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define MIN(x, y) (x > y ? y : x)

static Display *dpy;
static int screen;
static Window root;
static char statusBar[2][LEN(blocks) * (CMDLENGTH + LEN(DELIMITER) - 1) + 1];
static int statusContinue = 1;
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

void replace(char *str, char old, char new) {
	for (char *ch = str; *ch; ch++)
		if (*ch == old)
			*ch = new;
}

void getCommand(int i, const char *button) {
	Block *block = blocks + i;

	if (fork() == 0) {
		dup2(block->pipe[1], STDOUT_FILENO);
		close(block->pipe[0]);
		close(block->pipe[1]);

		// Temporary hack
		char cmd[1024];
		sprintf(cmd, "echo \"_$(%s)\"", block->command);

		char *command[] = {"/bin/sh", "-c", cmd, NULL};
		if (button) setenv("BLOCK_BUTTON", button, 1);
		setsid();
		execvp(command[0], command);
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
	new[0] = '\0';
	for (int i = 0; i < LEN(blocks); i++) {
		Block *block = blocks + i;
		if (strlen(block->output) > (block->signal != 0))
			strcat(new, DELIMITER);
		strcat(new, block->output);
	}
	new[strlen(new)] = '\0';
	return strcmp(new, old);
}

void debug() {
	// Only write out if text has changed
	if (!getStatus(statusBar[0], statusBar[1])) return;

	write(STDOUT_FILENO, statusBar[0], strlen(statusBar[0]));
	write(STDOUT_FILENO, "\n", 1);
}

void setRoot() {
	// Only set root if text has changed
	if (!getStatus(statusBar[0], statusBar[1])) return;

	Display *d = XOpenDisplay(NULL);
	if (d) dpy = d;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusBar[0]);
	XCloseDisplay(dpy);
}

void buttonHandler(int sig, siginfo_t *si, void *ucontext) {
	sig = si->si_value.sival_int >> 8;

	int i = 0;
	while (blocks[i].signal != sig) i++;
	const char button[2] = {'0' + si->si_value.sival_int & 0xff, '\0'};
	getCommand(i, button);
}

void signalHandler(int signal) { getSignalCommand(signal - SIGRTMIN); }

void termHandler(int signal) {
	statusContinue = 0;
	exit(EXIT_SUCCESS);
}

void childHandler() {
	for (int i = 0; i < LEN(blocks); i++) {
		Block *block = blocks + i;

		int bytesToRead = CMDLENGTH;
		char *output = block->output;
		if (block->signal) output++, bytesToRead--;

		char placebo;
		if (read(block->pipe[0], &placebo, 1) == 1) {
			char buffer[PIPE_BUF];
			read(block->pipe[0], buffer, PIPE_BUF);
			replace(buffer, '\n', '\0');
			strncpy(output, buffer, bytesToRead);
			break;
		}
	}
	writeStatus();
}

void setupSignals() {
	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);

	// Handle block update signals
	struct sigaction sa;
	for (int i = 0; i < LEN(blocks); i++) {
		if (blocks[i].signal > 0) {
			signal(SIGRTMIN + blocks[i].signal, signalHandler);
			sigaddset(&sa.sa_mask, SIGRTMIN + blocks[i].signal);
		}
	}

	// Handle mouse events
	sa.sa_sigaction = buttonHandler;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sa, NULL);

	// Handle exit of forks
	struct sigaction sigchld_action = {
		.sa_handler = childHandler,
		.sa_flags = SA_NOCLDWAIT,
	};
	sigaction(SIGCHLD, &sigchld_action, NULL);
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
		if (nanosleep(&toSleep, &toSleep) == -1) continue;

		// Write to status after sleeping
		getCommands(i);

		// After sleep, reset timer and update counter
		toSleep = sleepTime;
		i += sleepInterval;
	}
}

int main(int argc, char **argv) {
	writeStatus = setRoot;
	for (int i = 0; i < argc; i++)
		if (!strcmp("-d", argv[i])) writeStatus = debug;

	for (int i = 0; i < LEN(blocks); i++) {
		Block *block = blocks + i;
		pipe(block->pipe);
		fcntl(block->pipe[0], F_SETFL, O_NONBLOCK);

		if (block->signal) block->output[0] = block->signal;
	}

	setupSignals();
	statusLoop();
}
