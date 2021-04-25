#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include "config.h"
#include "util.h"

int waitForChild(int pid) {
    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    return exitCode;
}

int _spawnArgs(const char* const args[], int* fds) {
    int pid = fork();
    if(pid == 0) {
        if(fds) {
            dup2(fds[1], STDOUT_FILENO);
            close(fds[0]);
        }
        close(STDIN_FILENO);
        execv(args[0], (char* const*)args);
        perror("exec failed; Aborting");
        exit(2);
    }
    else if(pid < 0){
        perror("error forking");
        exit(2);
    }
    if(fds)
        close(fds[1]);
    return pid;
}

int spawnArgs(const char* const args[]) {
    return waitForChild(spawnArgs(args));
}

int spawn(const char* command) {
    const char* const args[] = {SHELL, "-c", command, NULL};
    return spawnArgs(args);
}

int readCmd(const char* command, char*buffer, int bufferLen) {
    int fds[2];
    pipe(fds);
    const char* const args[] = {SHELL, "-c", command, NULL};
    int pid = _spawnArgs(args, fds);
    read(fds[0], buffer, bufferLen);
    return waitForChild(pid);
}

void quit() {
    exit(0);
}

static struct {
    struct pollfd pollFDs[NUM_FD_LISTENERS];
    void(*extraEventCallBacks[NUM_FD_LISTENERS])();
    int numberOfFDsToPoll;
} eventFDInfo = {0};
void addExtraEvent(int fd, void(*callBack)()) {
    int index = eventFDInfo.numberOfFDsToPoll++;
    eventFDInfo.pollFDs[index] = (struct pollfd) {fd, POLLIN};
    eventFDInfo.extraEventCallBacks[index] = callBack;
}
void removeExtraEvent(int index) {
    for(int i = index + 1; i < eventFDInfo.numberOfFDsToPoll; i++) {
        eventFDInfo.pollFDs[i - 1] = eventFDInfo.pollFDs[i];
        eventFDInfo.extraEventCallBacks[i - 1] = eventFDInfo.extraEventCallBacks[i];
    }
    eventFDInfo.numberOfFDsToPoll--;
}

int processEvents(int timeout) {
    int numEvents;
    assert(eventFDInfo.numberOfFDsToPoll);
    if((numEvents = poll(eventFDInfo.pollFDs, eventFDInfo.numberOfFDsToPoll, timeout))) {
        for(int i = eventFDInfo.numberOfFDsToPoll - 1; i >= 0; i--) {
            if(eventFDInfo.pollFDs[i].revents) {
                if(eventFDInfo.pollFDs[i].revents & eventFDInfo.pollFDs[i].events) {
                    eventFDInfo.extraEventCallBacks[i](eventFDInfo.pollFDs[i].fd, eventFDInfo.pollFDs[i].revents);
                }
                if(eventFDInfo.pollFDs[i].revents & (POLLERR | POLLNVAL | POLLHUP)) {
                    removeExtraEvent(i);
                }
            }
        }
    }
    return numEvents;
}
