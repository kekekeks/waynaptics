//
// Created by kekekeks on 18.05.2025.
//

#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include "synshared.h"

extern "C" {

int xf86FlushInput(int fd) {
    pollfd poll_fd;
    /* this needs to be big enough to flush an evdev event. */
    char c[256];

    //DebugF("FlushingSerial\n");
    if (tcflush(fd, TCIFLUSH) == 0)
        return 0;

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    while (poll(&poll_fd, 1, 0) > 0) {
        if (read(fd, &c, sizeof(c)) < 1)
            return 0;
    }
    return 0;
}

}