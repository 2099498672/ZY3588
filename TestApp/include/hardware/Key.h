#ifndef __KEY_H__
#define __KEY_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <map>

#include "util/Log.h"

#define DEV_INPUT "/dev/input"
#define EVENT_DEV "event"

struct event_info {
    int fd;
    std::string device_path;
    bool is_open;
};

class Key {
public:
    int timeOut;
    int fd;
    std::map<int, std::string> keyMap;
    const char* KEY_EVENT_PATH;
    Key();
    Key(const char* eventPath, int timeOut);
    virtual ~Key();

    // Wait for key press or IR press v1.0
    int waitForKeyAndIrPress(const char* keyPath = "/dev/input/by-path/platform-adc-keys-event",            /*adc-keys*/
                             const char* irKeyPath = "/dev/input/by-path/platform-febd0030.pwm-event",    /*febd0030.pwm*/
                             int timeOut = 60);

    // Wait for key press v2.0
    std::map<std::string, std::string> keyEventNamesToPath;
    virtual int waitForKeyPress(int timeOut = 60);

    // wait for key press v3.0, not yet in use
    virtual void set_event_info_map(const std::map<std::string, event_info> info_map);           // set event_info_map
    virtual bool get_event_info();                                                               // open event devices based on event_info_map
    virtual std::map<std::string, event_info> get_event_info_map();                              // get event_info_map
    virtual void print_event_info_map();                                                         // print event_info_map
    virtual int wait_key_press(int time_out);                                                    // wait for key press based on event_info_map

    // virtual void set_key_value_info_map(const std::map<int, std::string> key_value_map);         // set key_value_info_map
    // virtual std::map<int, std::string> get_key_value_info_map();                                 // get key_value_info_map


private:
    const char* KEY_TAG = "KEY";

    // wait for key press v3.0, not yet in use
    std::map<std::string, event_info> event_info_map;
    std::map<int, std::string> key_value_info_map;
};

#endif