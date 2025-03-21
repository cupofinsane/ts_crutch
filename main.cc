#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>

#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <cstring>
#include <unistd.h>

#include <time.h>

static const char *const evval[3] = {
    "RELEASED",
    "PRESSED ",
    "REPEATED"
};


Window GetTypeStatsWindow(Display* dpy, Window root)
{
    Atom a = XInternAtom(dpy, "_NET_CLIENT_LIST", true);
    Atom actualType;
    int format;
    unsigned long numItems, bytesAfter;
    unsigned char *data = 0;
    int status = XGetWindowProperty(dpy, root, a, 0L, (~0L), false,
                                AnyPropertyType, &actualType, &format, &numItems,
                                &bytesAfter, &data);

    if (status==0  && numItems)
    {
        long *array = (long*) data;
        for (unsigned long k = 0; k < numItems; k++)
        {
            Window w = (Window) array[k];
            char* name;
	    XTextProperty prop;
	    status = XGetWMName(dpy, w, &prop);
	    if(!status)continue;
	    if(std::strncmp((char*)prop.value, "Typing statistics", 17)==0)
	      {
		XFree(data);
		return w;
	      }
	}
        XFree(data);
    }
    return 0;
}

int main()
{
      Display *dpy = XOpenDisplay(0);
      if(!dpy)
	{
	  std::cout << "ERROR: Couldn't open display server" << std::endl;
	  return 1;
	}

      Window r = DefaultRootWindow(dpy);
      Window tsWindow = GetTypeStatsWindow(dpy, r);
      if(!tsWindow)
	{
	  std::cout << "ERROR: Couldn't find Typing statistics window" << std::endl;
	  return 1;
	}

      std::cout << "Typing statistics window found. Handle: " << tsWindow  << std::endl;

	XEvent created_event;
	created_event.xkey.display = dpy;
	created_event.xkey.window = 0;
	created_event.xkey.root = r;
	created_event.xkey.subwindow = 0;
	
	//const char *dev = "/dev/input/by-id/usb-SONiX_USB_DEVICE-event-kbd";
	const char *dev = "/dev/input/event3";
	struct input_event ev;
	ssize_t n;
	int fd;

	fd = open(dev, O_RDONLY);
	if (fd == -1) {
	  std::cout << "Cannot open " << dev << ": " << strerror(errno) << ".\n";
	  return EXIT_FAILURE;
	}

	for(;;) {
          n = read(fd, &ev, sizeof ev);
	  if (n == (ssize_t)-1) {
	    if (errno == EINTR){
	      std::cout << "EINTR\n"; // never happened. what is it for?
	      continue;
	    }
            else
	      break;
	  } else
	    if (n != sizeof ev) {
	      errno = EIO;
	      break;
	    }

	  if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
	    {
	      std::cout  << evval[ev.value] << " " <<  (int)ev.code << std::endl;

	      if(ev.value == 1)created_event.xkey.type = 2;
	      else if(ev.value == 0)created_event.xkey.type = 3;
	      //TODO: REPEAT, ev.value == 2

	      timespec tp;
	      clock_gettime(CLOCK_MONOTONIC, &tp);
	      created_event.xkey.time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	      created_event.xkey.state = 0;
	      created_event.xkey.keycode = ev.code + 8;
	      created_event.xkey.same_screen = 1;

	      if(tsWindow)
		{
		  XSendEvent(dpy, tsWindow, false, NoEventMask, &created_event);
		  XFlush(dpy);
		}
	    }
	
	}      

	return 0;
}
