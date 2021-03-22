#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define len(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct {
	char *command;
	unsigned int interval;
	unsigned int signal;
} Block;
#include "config.h"

void getCommand(const Block *, char *);
void getCommands(int);
int getStatus(char *, char *);
void setRoot();
void debug();
void statusLoop();
void termHandler(int);
void signalHandler(int num);
void buttonHandler(int sig, siginfo_t *si, void *ucontext);
void getSignalCommands(int signal);
void setupSignals();
void signalHandler(int signum);
void sleepMillis(unsigned long);

static Display *dpy;
static int screen;
static Window root;
static char commands[len(blocks)][256];
static char statusbar[len(blocks)][CMDLENGTH];
static char statusStr[2][len(blocks) * CMDLENGTH +
						 (len(delimiter) - 1) * (len(blocks) - 1) + 1];
static int statusContinue = 1;
static void (*writestatus)() = setRoot;

void replace(char *str, char old, char new) {
	int i = 0;
	while (str[i] != '\0') {
		if (str[i] == old) {
			str[i] = new;
			return;
		}
		i++;
	}
}

void getSignalCommands(int signal) {
	const Block *current;
	for (int i = 0; i < len(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			getCommand(current, statusbar[i]);
	}
}

void setupSignals() {
	struct sigaction sa;

	// Handle block update signals
	for (int i = 0; i < len(blocks); i++) {
		if (blocks[i].signal > 0) {
			signal(SIGRTMIN + blocks[i].signal, signalHandler);
			sigaddset(&sa.sa_mask, SIGRTMIN + blocks[i].signal);
		}
	}

	// Handle mouse events
	sa.sa_sigaction = buttonHandler;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sa, NULL);
	struct sigaction sigchld_action = {
		.sa_handler = SIG_DFL,
		.sa_flags = SA_NOCLDWAIT,
	};
	sigaction(SIGCHLD, &sigchld_action, NULL);
}

// Open process *command and store output in *output
void getCommand(const Block *block, char *output) {
	// Leave signal in output for dwm
	if (block->signal) {
		output[0] = block->signal;
		output++;
	}

	char *cmd = block->command;
	FILE *cmdFile = popen(cmd, "r");
	if (!cmdFile) return;

	int fd = fileno(cmdFile);
	if (!read(fd, output, CMDLENGTH - 2))
		output[0] = '\0';
	else {
		output[CMDLENGTH - 2] = '\0';
		replace(output, '\n', '\0');
	}
	pclose(cmdFile);
}

void getCommands(int time) {
	const Block *current;
	for (int i = 0; i < len(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) ||
			time == -1)
			getCommand(current, statusbar[i]);
	}
}

int getStatus(char *new, char *old) {
	strcpy(old, new);
	new[0] = '\0';
	for (int i = 0; i < len(blocks); i++) {
		strcat(new, statusbar[i]);
		if (strlen(statusbar[i]) > (blocks[i].signal != 0) &&
			i != len(blocks) - 1)
			strcat(new, delimiter);
	}
	new[strlen(new)] = '\0';
	return strcmp(new, old);
}

void setRoot() {
	// Only set root if text has changed
	if (!getStatus(statusStr[0], statusStr[1])) return;

	Display *d = XOpenDisplay(NULL);
	if (d) dpy = d;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusStr[0]);
	XCloseDisplay(dpy);
}

void debug() {
	// Only write out if text has changed
	if (!getStatus(statusStr[0], statusStr[1])) return;

	write(STDOUT_FILENO, statusStr[0], strlen(statusStr[0]));
	write(STDOUT_FILENO, "\n", 1);
}

void statusLoop() {
	setupSignals();
	unsigned long int i = 0;
	getCommands(-1);
	while (statusContinue) {
		getCommands(i);
		writestatus();
		sleep(1.0);
		i++;
	}
}

void signalHandler(int signum) {
	getSignalCommands(signum - SIGRTMIN);
	writestatus();
}

void buttonHandler(int sig, siginfo_t *si, void *ucontext) {
	char button[2] = {'0' + si->si_value.sival_int & 0xff, '\0'};
	sig = si->si_value.sival_int >> 8;
	if (fork() == 0) {
		const Block *current;
		char *shCommand = NULL;
		for (int i = 0; i < len(blocks); i++) {
			current = blocks + i;
			if (current->signal == sig) {
				shCommand = commands[i];
				break;
			}
		}

		if (shCommand) {
			char *command[] = {"/bin/sh", "-c", shCommand, NULL};
			setenv("BLOCK_BUTTON", button, 1);
			setsid();
			execvp(command[0], command);
		}
		exit(EXIT_SUCCESS);
	}
}

void termHandler(int sigNum) {
	statusContinue = 0;
	exit(0);
}

int main(int argc, char **argv) {
	const int processID = getpid();
	for (int i = 0; i < len(blocks); i++)
		sprintf(commands[i], "%s && kill -%d %d", blocks[i].command,
				SIGRTMIN + blocks[i].signal, processID);

	for (int i = 0; i < argc; i++)
		if (!strcmp("-d", argv[i])) writestatus = debug;

	signal(SIGTERM, termHandler);
	signal(SIGINT, termHandler);
	statusLoop();
}
