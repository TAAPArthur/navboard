#ifndef NAVBOARD_UTIL_H
#define NAVBOARD_UTIL_H

int processEvents(int timeout);
void removeExtraEvent(int index);
void addExtraEvent(int fd, void(*callBack)());
int spawn(const char* command);
int spawnArgs(const char* const args[]);
int readCmd(const char* command, char*buffer, int bufferLen);
int waitForChild(int pid);
void quit();
#endif
