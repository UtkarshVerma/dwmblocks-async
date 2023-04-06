#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include "bar.h"
#include "block.h"
#include "util.h"
#include "x11.h"

static unsigned short statusContinue = 1;
unsigned short debugMode = 0;
static int epollFD, signalFD;
static unsigned int timerTick = 0, maxInterval = 1;

void signalHandler() {
    struct signalfd_siginfo info;
    read(signalFD, &info, sizeof(info));
    unsigned int signal = info.ssi_signo;

    static unsigned int timer = 0;
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

    for (int j = 0; j < blockCount; j++) {
        const Block *block = blocks + j;
        if (block->signal == signal - SIGRTMIN) {
            char button[4];  // value can't be more than 255;
            sprintf(button, "%d", info.ssi_int & 0xff);
            execBlock(block, button);
            break;
        }
    }
}

void termHandler() {
    statusContinue = 0;
}

void setupSignals() {
    sigset_t handledSignals;
    sigemptyset(&handledSignals);
    sigaddset(&handledSignals, SIGUSR1);
    sigaddset(&handledSignals, SIGALRM);

    // Append all block signals to `handledSignals`
    for (int i = 0; i < blockCount; i++)
        if (blocks[i].signal > 0)
            sigaddset(&handledSignals, SIGRTMIN + blocks[i].signal);

    // Create a signal file descriptor for epoll to watch
    signalFD = signalfd(-1, &handledSignals, 0);

    // Block all realtime and handled signals
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++) sigaddset(&handledSignals, i);
    sigprocmask(SIG_BLOCK, &handledSignals, NULL);

    // Handle termination signals
    signal(SIGINT, termHandler);
    signal(SIGTERM, termHandler);

    // Avoid zombie subprocesses
    struct sigaction signalAction;
    signalAction.sa_handler = SIG_DFL;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &signalAction, 0);
}

void statusLoop() {
    // Update all blocks initially
    raise(SIGALRM);

    BarStatus status;
    initStatus(&status);
    struct epoll_event events[blockCount + 1];
    while (statusContinue) {
        int eventCount = epoll_wait(epollFD, events, LEN(events), 100);
        for (int i = 0; i < eventCount; i++) {
            unsigned short id = events[i].data.u32;
            if (id < blockCount) {
                updateBlock(blocks + id);
            } else {
                signalHandler();
            }
        }

        if (eventCount != -1) writeStatus(&status);
    }
    freeStatus(&status);
}

void init() {
    epollFD = epoll_create(blockCount);
    struct epoll_event event = {
        .events = EPOLLIN,
    };

    for (int i = 0; i < blockCount; i++) {
        // Append each block's pipe's read end to `epollFD`
        pipe(blocks[i].pipe);
        event.data.u32 = i;
        epoll_ctl(epollFD, EPOLL_CTL_ADD, blocks[i].pipe[0], &event);

        // Calculate the max interval and tick size for the timer
        if (blocks[i].interval) {
            maxInterval = MAX(blocks[i].interval, maxInterval);
            timerTick = gcd(blocks[i].interval, timerTick);
        }
    }

    setupSignals();

    // Watch signal file descriptor as well
    event.data.u32 = blockCount;
    epoll_ctl(epollFD, EPOLL_CTL_ADD, signalFD, &event);
}

int main(const int argc, const char *argv[]) {
    if (setupX()) {
        fprintf(stderr, "%s\n", "dwmblocks: Failed to open display");
        return 1;
    }

    for (int i = 0; i < argc; i++) {
        if (!strcmp("-d", argv[i])) {
            debugMode = 1;
            break;
        }
    }

    init();
    statusLoop();

    if (closeX())
        fprintf(stderr, "%s\n", "dwmblocks: Failed to close display");

    close(epollFD);
    close(signalFD);
    for (int i = 0; i < blockCount; i++) closePipe(blocks[i].pipe);

    return 0;
}
