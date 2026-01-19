#ifndef __ZENITY_DIALOG_H__
#define __ZENITY_DIALOG_H__

#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

class ZenityDialog {
private:
    pid_t pid;
    bool running;

public:
    ZenityDialog();
    ~ZenityDialog();
    void show(const char* title, const char* message);
    void update(const char* title, const char* message);
    void close();
};

#endif