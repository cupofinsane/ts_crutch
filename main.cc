// This is a crutch to run Typing Statistics on Linux
// Copyright © 2025 cupofinsane
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See the COPYING file for more details.

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <string>
#include <unistd.h>
#include <unordered_set>

#include <poll.h>
#include <time.h>

static const char *const evval[3] = {"RELEASED", "PRESSED ", "REPEATED"};

enum ReadResult
{
    NOTHING,
    OK,
    RECOVERABLE_ERROR,
    ERROR
};

ReadResult ReadPolledResults(pollfd *fds, int N, input_event *ev)
{
    int recoverableError = 0;
    for (int i = 0; i < N; ++i)
    {
        if (fds[i].revents & POLLERR)
        {
            std::cout << "ReadPolledResults, error in revents\n";
            return ReadResult::ERROR;
        }
        if (fds[i].revents & POLLIN)
        {
            ssize_t n = read(fds[i].fd, ev, sizeof *ev);
            if (n == (ssize_t)-1)
            {
                if (errno == EINTR)
                {
                    std::cout << "EINTR\n"; // never happened. what is it for?
                    recoverableError++;
                    continue;


                }
                else
                    return ReadResult::ERROR;
            }
            else if (n != sizeof *ev)
            {
                errno = EIO;
                return ReadResult::ERROR;
            }
            fds[i].revents = 0;
            return ReadResult::OK;
        }
    }
    if (recoverableError)
        return ReadResult::RECOVERABLE_ERROR;
    return ReadResult::NOTHING;
}

ReadResult ReadKeyboardInput(pollfd *fds, int N, input_event *ev)
{
    ReadResult read_res = ReadPolledResults(fds, N, ev);
    if (read_res != ReadResult::NOTHING)
        return read_res;

    auto res = poll(fds, N, -1);
    if (res == -1)
    {
        std::cout << "ReadKeyboardInput, error in poll: " << strerror(errno) << std::endl;
        return ReadResult::ERROR;
    }
    return ReadPolledResults(fds, N, ev);
}

Window GetTypeStatsWindow(Display *dpy, Window root)
{
    Atom a = XInternAtom(dpy, "_NET_CLIENT_LIST", true);
    Atom actualType;
    int format;
    unsigned long numItems, bytesAfter;
    unsigned char *data = 0;
    int status = XGetWindowProperty(dpy, root, a, 0L, (~0L), false, AnyPropertyType, &actualType, &format, &numItems, &bytesAfter, &data);

    if (status == 0 && numItems)
    {
        long *array = (long *)data;
        for (unsigned long k = 0; k < numItems; k++)
        {
            Window w = (Window)array[k];
            char *name;
            XTextProperty prop;
            status = XGetWMName(dpy, w, &prop);
            if (!status)
                continue;
            if (std::strncmp((char *)prop.value, "Typing statistics", 17) == 0)
            {
                XFree(data);
                return w;
            }
        }
        XFree(data);
    }
    return 0;
}

std::unordered_set<std::string> GetAllKeyboards()
{
    std::unordered_set<std::string> result;
    const int BUFSIZE = 1024;
    char fullNameBuf[BUFSIZE] = "/dev/input/by-path/";
    int fullNameBufLen = std::strlen(fullNameBuf);

    DIR *dir = opendir(fullNameBuf);
    if (dir == NULL)
    {
        std::cout << "Could not open /dev/input/by-path" << std::endl;
        return result;
    }

    dirent *entry;
    char symLinkName[BUFSIZE];
    while ((entry = readdir(dir)) != NULL)
    {
        const char *fileName = entry->d_name;

        if (std::strncmp(fileName + std::strlen(fileName) - 3, "kbd", 3) == 0)
        {
            fullNameBuf[fullNameBufLen] = 0;
            strcat(fullNameBuf, fileName);

            int n = readlink(fullNameBuf, symLinkName, BUFSIZE);
            if (n == -1)
            {
                std::cout << "Couldn't read symlink for keyboard " << fullNameBuf << ". " << strerror(errno) << std::endl;
                continue;
            }
            symLinkName[n] = 0;
            fullNameBuf[fullNameBufLen] = 0;
            strcat(fullNameBuf, symLinkName);
            result.insert(fullNameBuf);
        }
    }
    if (result.empty())
        std::cout << "No keyboards found" << std::endl;
    std::cout << "Found " << result.size() << " keyboard(s)" << std::endl;
    return result;
}

int main()
{
    Display *dpy = XOpenDisplay(0);
    if (!dpy)
    {
        std::cout << "ERROR: Couldn't open display server" << std::endl;
        return 1;
    }

    Window r = DefaultRootWindow(dpy);
    Window tsWindow = GetTypeStatsWindow(dpy, r);
    if (!tsWindow)
    {
        std::cout << "ERROR: Couldn't find Typing statistics window" << std::endl;
        return 1;
    }

    std::cout << "Typing statistics window found. Handle: " << tsWindow << std::endl;

    XEvent created_event;
    created_event.xkey.display = dpy;
    
    created_event.xkey.window = 0;
    created_event.xkey.root = r;
    created_event.xkey.subwindow = 0;

    auto kbds = GetAllKeyboards();
    auto fds = new pollfd[kbds.size()];
    int i = 0;
    for (auto &kbd : kbds)
    {
        int fd = open(kbd.c_str(), O_RDONLY);
        if (fd == -1)
        {
            std::cout << "Cannot open " << kbd << ": " << strerror(errno) << ".\n";
            return EXIT_FAILURE;
        }
        fds[i].fd = fd;
        fds[i].events = POLLIN;
        ++i;
    }
    struct input_event ev;

    for (;;)
    {
        auto r = ReadKeyboardInput(fds, kbds.size(), &ev);
        if (r == ReadResult::RECOVERABLE_ERROR)
            continue;
        if (r == ReadResult::ERROR)
            break;

        if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
        {
            std::cout << evval[ev.value] << " " << (int)ev.code << std::endl;

            if (ev.value == 1)
                created_event.xkey.type = 2;
            else if (ev.value == 0)
                created_event.xkey.type = 3;
            // TODO: REPEAT, ev.value == 2

            timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            created_event.xkey.time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
            created_event.xkey.state = 0;
            created_event.xkey.keycode = ev.code + 8;
            created_event.xkey.same_screen = 1;

            if (tsWindow)
            {
                XSendEvent(dpy, tsWindow, false, NoEventMask, &created_event);
                XFlush(dpy);
            }
        }
    }

    delete fds;

    return 0;
}
