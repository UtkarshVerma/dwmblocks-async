#define _GNU_SOURCE
#include <X11/Xlib.h>
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
#define BLOCK(cmd, interval, signal) \
    { "echo \"$(" cmd ")\"", interval, signal }

typedef const struct {
    const char *command;
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

static Display *dpy;
static Window root;
static unsigned short statusContinue = 1;
static struct epoll_event event;
static int pipes[LEN(blocks)][2];
static int timer = 0, timerTick = 0, maxInterval = 1;
static int signalFD;
static int epollFD;
static int execLock = 0;

// Longest UTF-8 character is 4 bytes long
static char outputs[LEN(blocks)][CMDLENGTH * 4 + 1 + CLICKABLE_BLOCKS];
static char
    statusBar[2]
             [LEN(blocks) * (LEN(outputs[0]) - 1) +
              (LEN(blocks) - 1 + LEADING_DELIMITER) * (LEN(DELIMITER) - 1) + 1];

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

void closePipe(int *pipe) {
    close(pipe[0]);
    close(pipe[1]);
}

void execBlock(int i, const char *button) {
    // Ensure only one child process exists per block at an instance
    if (execLock & 1 << i) return;
    // Lock execution of block until current instance finishes execution
    execLock |= 1 << i;

    if (fork() == 0) {
        close(pipes[i][0]);
        dup2(pipes[i][1], STDOUT_FILENO);
        close(pipes[i][1]);

        if (button) setenv("BLOCK_BUTTON", button, 1);
        execl("/bin/sh", "sh", "-c", blocks[i].command, (char *)NULL);
        exit(EXIT_FAILURE);
    }
}

void execBlocks(unsigned int time) {
    for (int i = 0; i < LEN(blocks); i++)
        if (time == 0 ||
            (blocks[i].interval != 0 && time % blocks[i].interval == 0))
            execBlock(i, NULL);
}

int getStatus(char *new, char *old) {
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
    char *output = outputs[i];
    char buffer[LEN(outputs[0]) - CLICKABLE_BLOCKS];
    int bytesRead = read(pipes[i][0], buffer, LEN(buffer));

    // Trim UTF-8 string to desired length
    int count = 0, j = 0;
    while (buffer[j] != '\n' && count < CMDLENGTH) {
        count++;

        // Skip continuation bytes, if any
        char ch = buffer[j];
        int skip = 1;
        while ((ch & 0xc0) > 0x80) ch <<= 1, skip++;
        j += skip;
    }

    // Cache last character and replace it with a trailing space
    char ch = buffer[j];
    buffer[j] = ' ';

    // Trim trailing spaces
    while (j >= 0 && buffer[j] == ' ') j--;
    buffer[j + 1] = 0;

    // Clear the pipe
    if (bytesRead == LEN(buffer)) {
        while (ch != '\n' && read(pipes[i][0], &ch, 1) == 1)
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
    if (!getStatus(statusBar[0], statusBar[1])) return;

    write(STDOUT_FILENO, statusBar[0], strlen(statusBar[0]));
    write(STDOUT_FILENO, "\n", 1);
}

int setupX() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;

    root = DefaultRootWindow(dpy);
    return 0;
}

void setRoot() {
    // Only set root if text has changed
    if (!getStatus(statusBar[0], statusBar[1])) return;

    XStoreName(dpy, root, statusBar[0]);
    XFlush(dpy);
}

void signalHandler() {
    struct signalfd_siginfo info;
    read(signalFD, &info, sizeof(info));
    unsigned int signal = info.ssi_signo;

    switch (signal) {
        case SIGALRM:
            // Schedule the next timer event and execute blocks
            alarm(timerTick);
            execBlocks(timer);

            // Wrap `timer` to the interval [1, `maxInterval`]
            timer = (timer + timerTick - 1) % maxInterval + 1;
            return;
        case SIGUSR1:
            // Update all blocks on receiving SIGUSR1
            execBlocks(0);
            return;
    }

    for (int j = 0; j < LEN(blocks); j++) {
        if (blocks[j].signal == signal - SIGRTMIN) {
            char button[4];  // value can't be more than 255;
            sprintf(button, "%d", info.ssi_int & 0xff);
            execBlock(j, button);
            break;
        }
    }
}

void termHandler() { statusContinue = 0; }

void setupSignals() {
    sigset_t handledSignals;
    sigemptyset(&handledSignals);
    sigaddset(&handledSignals, SIGUSR1);
    sigaddset(&handledSignals, SIGALRM);

    // Append all block signals to `handledSignals`
    for (int i = 0; i < LEN(blocks); i++)
        if (blocks[i].signal > 0)
            sigaddset(&handledSignals, SIGRTMIN + blocks[i].signal);

    // Create a signal file descriptor for epoll to watch
    signalFD = signalfd(-1, &handledSignals, 0);
    event.data.u32 = LEN(blocks);
    epoll_ctl(epollFD, EPOLL_CTL_ADD, signalFD, &event);

    // Block all realtime and handled signals
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++) sigaddset(&handledSignals, i);
    sigprocmask(SIG_BLOCK, &handledSignals, NULL);

    // Handle termination signals
    signal(SIGINT, termHandler);
    signal(SIGTERM, termHandler);

    // Avoid zombie subprocesses
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, 0);
}

void statusLoop() {
    // Update all blocks initially
    raise(SIGALRM);

    struct epoll_event events[LEN(blocks) + 1];
    while (statusContinue) {
        int eventCount = epoll_wait(epollFD, events, LEN(events), -1);
        for (int i = 0; i < eventCount; i++) {
            unsigned short id = events[i].data.u32;
            if (id < LEN(blocks))
                updateBlock(id);
            else
                signalHandler();
        }

        if (eventCount != -1) writeStatus();
    }
}

void init() {
    epollFD = epoll_create(LEN(blocks));
    event.events = EPOLLIN;

    for (int i = 0; i < LEN(blocks); i++) {
        // Append each block's pipe to `epollFD`
        pipe(pipes[i]);
        event.data.u32 = i;
        epoll_ctl(epollFD, EPOLL_CTL_ADD, pipes[i][0], &event);

        // Calculate the max interval and tick size for the timer
        if (blocks[i].interval) {
            maxInterval = MAX(blocks[i].interval, maxInterval);
            timerTick = gcd(blocks[i].interval, timerTick);
        }
    }

    setupSignals();
}

int main(const int argc, const char *argv[]) {
    if (setupX()) {
        fprintf(stderr, "dwmblocks: Failed to open display\n");
        return 1;
    }

    writeStatus = setRoot;
    for (int i = 0; i < argc; i++)
        if (!strcmp("-d", argv[i])) writeStatus = debug;

    init();
    statusLoop();

    XCloseDisplay(dpy);
    close(epollFD);
    close(signalFD);
    for (int i = 0; i < LEN(pipes); i++) closePipe(pipes[i]);

    return 0;
}
