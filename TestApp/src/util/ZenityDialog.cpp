#include "util/ZenityDialog.h"

ZenityDialog::ZenityDialog() {
    pid = -1;
    running = false;
}

ZenityDialog::~ZenityDialog() {
    close();
}

void ZenityDialog::show(const char* title, const char* message) {
    close();
    pid = fork();
    if (pid == 0) {
        execlp("zenity", "zenity", "--info", "--title", title, "--text", message, NULL);
        exit(1);
    }
    running = true;
}

void ZenityDialog::update(const char* title, const char* message) {
    if (!running) return;
    close();
    show("Dialog", message);
}

void ZenityDialog::close() {
    if (running && pid > 0) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        running = false;
        pid = -1;
    }
}