#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>		/* getenv(), etc. */
#include <unistd.h>		/* sleep(), etc.  */
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>

// http://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent

/* 
 * include the definition of the bitmap in our program. 
 */
#include "icon.bmp"

#ifndef timeradd
#define timeradd(_a, _b, _res)              \
      do {                          \
              (_res)->tv_usec = (_a)->tv_usec + (_b)->tv_usec;  \
              (_res)->tv_sec = (_a)->tv_sec + (_b)->tv_sec; \
              if ((_res)->tv_usec >= 1000000)           \
                {                           \
                            (_res)->tv_usec -= 1000000;         \
                            (_res)->tv_sec++;               \
                          }                         \
            } while (0)
#endif

#ifndef timersub
#define timersub(_a, _b, _res)              \
      do {                          \
              (_res)->tv_usec = (_a)->tv_usec - (_b)->tv_usec;  \
              (_res)->tv_sec = (_a)->tv_sec - (_b)->tv_sec; \
              if ((_res)->tv_usec < 0) {                \
                        (_res)->tv_usec += 1000000;         \
                        (_res)->tv_sec--;                   \
                      }                         \
            } while (0)
#endif

void
show_timeval (const char *str, struct timeval *tv)
{
	fprintf (stdout, "%s: %ld.%06ld\n", str, tv->tv_sec, tv->tv_usec);
}

int
set_window_hints (Display * display, Window win)
{

	/* This variable will store the newly created property.  */
	XTextProperty window_title_property;

	/* This is the string to be translated into a property.  */
	char *window_title = "Handmade Hero";

	/* translate the given string into an X property.  */
	int rc = XStringListToTextProperty (&window_title, 1, &window_title_property);

	/* check the success of the translation.  */
	if (rc == 0)
	{
		fprintf (stderr, "XStringListToTextProperty - out of memory\n");
		return (1);
	}

	/* assume that window_title_property is our XTextProperty, and is */
	/* defined to contain the desired window title.  */
	XSetWMName (display, win, &window_title_property);

	XTextProperty icon_name_property;
	char *icon_name = "Jelly";
	rc = XStringListToTextProperty (&icon_name, 1, &icon_name_property);
	/* this time we assume that icon_name_property is an initialized */
	/* XTextProperty variable containing the desired icon name.  */
	XSetWMIconName (display, win, &icon_name_property);

	/* pointer to the size hints structure.  */
	XSizeHints *win_size_hints = XAllocSizeHints ();
	if (!win_size_hints)
	{
		fprintf (stderr, "XAllocSizeHints - out of memory\n");
		return (1);
	}

	/* initialize the structure appropriately.  */
	/* first, specify which size hints we want to fill in.  */
	/* in our case - setting the minimal size as well as the initial size.  */
	/* in our case - setting the state hint as well as the icon position hint.  */
	win_size_hints->flags = PSize | PMinSize;
	win_size_hints->min_width = 300;
	win_size_hints->min_height = 200;
	win_size_hints->base_width = 400;
	win_size_hints->base_height = 250;
	XSetWMNormalHints (display, win, win_size_hints);
	XFree (win_size_hints);

	/* load the given bitmap data and create an X pixmap containing it.  */
	Pixmap icon_pixmap = XCreateBitmapFromData (display, win, icon_bitmap_bits,
						    icon_bitmap_width,
						    icon_bitmap_height);
	if (!icon_pixmap)
	{
		fprintf (stderr, "XCreateBitmapFromData - error creating pixmap\n");
		return (1);
	}

	XWMHints *win_hints = XAllocWMHints ();
	if (!win_hints)
	{
		fprintf (stderr, "XAllocSizeHints - out of memory\n");
		return (1);
	}
	win_hints->flags = StateHint | IconPositionHint | IconPixmapHint;
	win_hints->initial_state = IconicState;
	win_hints->icon_x = 0;
	win_hints->icon_y = 0;
	win_hints->icon_pixmap = icon_pixmap;

	/* pass the hints to the window manager.  */
	XSetWMHints (display, win, win_hints);

	/* finally, we can free the WM hints structure.  */
	XFree (win_hints);

	return 0;
}

Window
create_window (Display * display, int width, int height, int x, int y)
{
	int screen_num = DefaultScreen (display);
	int win_border_width = 2;

	Window win = XCreateSimpleWindow (display, RootWindow (display, screen_num),
					  x, y, width, height,
					  win_border_width,
					  BlackPixel (display, screen_num),
					  WhitePixel (display, screen_num));

	/* 
	 * make the window actually appear on the screen. 
	 */
	XMapWindow (display, win);

	/* 
	 * flush all pending requests to the X server. 
	 */
	XFlush (display);

	return win;
}

GC
create_context (Display * display, Window win)
{
	int screen_num = DefaultScreen (display);

	unsigned long valuemask = 0;	/* which values in 'values' to */
	XGCValues values;	/* initial values for the GC.  */
	GC context = XCreateGC (display, win, valuemask, &values);

	XSetForeground (display, context, BlackPixel (display, screen_num));
	XSetBackground (display, context, WhitePixel (display, screen_num));

	XSelectInput (display, win,
		      ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | KeymapStateMask);

	return context;
}

static XImage *
create_image (Display * display, Visual * visual, uint8_t * image, int width, int height, int x_offset, int y_offset)
{
	int bytes_per_pixel = 4;
	uint32_t pitch = width * bytes_per_pixel;
	uint32_t *buffer = (uint32_t *) calloc (width * height, bytes_per_pixel);
	uint8_t *row = (uint8_t *) buffer;
	for (int j = 0; j < height; j++)
	{
		uint32_t *pixel = (uint32_t *) row;
		for (int i = 0; i < width; i++)
		{
			uint8_t blue = (i + x_offset);
			uint8_t green = (j + y_offset);
			*pixel++ = (green << 8) | blue;
		}
		row += pitch;
	}
	return XCreateImage (display, visual, 24 /* depth */ ,
			     ZPixmap /* format */ , 0 /* offset */ ,
			     (char *) buffer, width, height, 32 /* bitmap pad */ ,
			     0);
}

int
main_loop (Display * display, Window win, GC context, int width, int height, int frequency)
{
	Visual *visual = DefaultVisual (display, DefaultScreen (display));

	if (visual->class != TrueColor)
	{
		fprintf (stderr, "Cannot handle not true color visual\n");
		return 1;
	}

	int x_offset = 0;
	int y_offset = 0;

	struct timeval interval_tv;	// this is how long the frame should last. This is a constant
	interval_tv.tv_sec = 0;
	interval_tv.tv_usec = 1000000 / frequency;

	struct timeval begin_tv;	// this is the beginning of the frame
	gettimeofday (&begin_tv, 0);

    int padding = 500;
	struct timeval timeout_tv;	// this is the duration select has to wait. this duration can be updated within the loop
	timeout_tv.tv_sec = interval_tv.tv_sec;
	timeout_tv.tv_usec = interval_tv.tv_usec - padding; // 50 usec

	struct timeval end_tv;	// this is the expected end of the frame
	timeradd (&begin_tv, &interval_tv, &end_tv);	/* end = begin + interval */

	XEvent ev;
	int x11_fd = ConnectionNumber (display);
	fd_set in_fds;
	uint64_t frame_index = 0;
	while (1)
	{
		FD_ZERO (&in_fds);
		FD_SET (x11_fd, &in_fds);

        show_timeval ("timeout", &timeout_tv);
		int rc = select (x11_fd + 1, &in_fds, 0, 0, &timeout_tv);
		if (rc < 0)
		{
			fprintf (stdout, "select return <0\n");
			return 1;
		}
		if (rc == 0)	/* timeout */
		{
			/* we consider the frame is over... increase the frame count */
			frame_index++;

			/* see if we overshot the frame: compare now with the time we initially expected to finish the frame */
			struct timeval now;
			gettimeofday (&now, 0);
			if (timercmp (&now, &end_tv, >))
			{
				show_timeval ("begin", &begin_tv);
				show_timeval ("end", &end_tv);
				show_timeval ("now", &now);
				struct timeval diff;
				timersub (&now, &end_tv, &diff);
				show_timeval ("overshot", &diff);
				return 1;
			}
			else
			{
				//show_timeval ("begin", &begin_tv);
				//show_timeval ("end", &now);
				/* we did not overshoot, so measure the actual frame duration */
				struct timeval diff;
				timersub (&now, &begin_tv, &diff);
				fprintf (stdout, "frame %ld duration: %ld.%06ld | FPS: %lf\n",
                         frame_index, diff.tv_sec, diff.tv_usec, 1000000.0f / (float)diff.tv_usec);
				/* reset the select timeout to a whole frame length. */
				timeout_tv.tv_sec = interval_tv.tv_sec;
				timeout_tv.tv_usec = interval_tv.tv_usec - padding;
				/* reset the beginning of the frame timestamp */
				begin_tv.tv_sec = now.tv_sec;
				begin_tv.tv_usec = now.tv_usec;
				/* reset the expected end of the frame */
				timeradd (&begin_tv, &interval_tv, &end_tv);
			}
		}
		else		/* rc > 0, an event was received */
		{
			fprintf (stdout, "event fired!\n");
			while (XPending (display))
			{
				XNextEvent (display, &ev);
				switch (ev.type)
				{
				    case KeymapNotify:
				    {
					    XRefreshKeyboardMapping (&ev.xmapping);
				    }
					    break;
				    case KeyPress:
					    break;	/* ignore these */
				    case KeyRelease:
				    {
					    char string[25];
					    KeySym keysym;
					    int len = XLookupString (&ev.xkey, string, 25, &keysym,
								     NULL);
					    bool redraw = false;
					    switch (keysym)
					    {
						case XK_Left:
						{
							x_offset += 5;
							redraw = true;
						}
							break;
						case XK_Right:
						{
							x_offset -= 5;
							redraw = true;
						}
							break;
						case XK_Up:
						{
							y_offset += 5;
							redraw = true;
						}
							break;
						case XK_Down:
						{
							y_offset -= 5;
							redraw = true;
						}
							break;
						default:
							break;
					    }
					    if (redraw)
					    {
						    XImage *image = create_image (display, visual, 0,
										  width, height,
										  x_offset,
										  y_offset);
						    XPutImage (display, win, context, image, 0, 0, 0, 0, width, height);
						    XDestroyImage (image);
						    XFlush (display);
					    }
				    }
					    break;
				    case Expose:
				    {
					    fprintf (stdout, "Expose\n");
					    XImage *image = create_image (display, visual, 0, width,
									  height, x_offset,
									  y_offset);
					    XPutImage (display, win, context, image, 0, 0, 0, 0, width, height);
					    XDestroyImage (image);
					    XFlush (display);
				    }
					    break;
				    case ConfigureNotify:
				    {
					    fprintf (stdout, "ConfigureNotify\n");
					    if (width != ev.xconfigure.width || height != ev.xconfigure.height)
					    {
						    width = ev.xconfigure.width;
						    height = ev.xconfigure.height;
						    XClearWindow (display, ev.xany.window);
						    printf ("Size changed to: %d by %d\n", width, height);
					    }
				    }
					    break;
				    case ButtonPress:
				    {
					    XCloseDisplay (display);
					    return 0;
				    }
				}
			}	/* pending events */

			/* we're done processing events, so readjust the timeout */
            fprintf (stdout, "adjusting timeout\n");
			struct timeval now;
			gettimeofday (&now, 0);
			timersub (&end_tv, &now, &timeout_tv);
			timeout_tv.tv_usec -= 50;
		}		/* event was received */
	}
}

int
main (int argc, char **argv)
{
	char *display_name = getenv ("DISPLAY");	/* address of the X display.  */

	/* open connection with the X server.  */
	Display *display = XOpenDisplay (display_name);
	if (display == NULL)
	{
		fprintf (stderr, "%s: cannot connect to X server '%s'\n", argv[0], display_name);
		exit (1);
	}

	unsigned int display_width = 800;
	unsigned int display_height = 600;

	/* make the new window occupy the whole screen's size.  */
	unsigned int width = display_width;
	unsigned int height = display_height;
	printf ("window width - '%d'; height - '%d'\n", width, height);

	/* create a simple window, as a direct child of the screen's root window. Use the screen's white color as the background color of the
	   window. Place the new window's top-left corner at the given 'x,y' coordinates.  */
	Window win = create_window (display, width, height, 0, 0);

	/* allocate a new GC (graphics context) for drawing in the window.  */
	GC context = create_context (display, win);

	set_window_hints (display, win);

	int monitor_refresh_hz = 60;
	int game_update_hz = monitor_refresh_hz / 2;
	float target_seconds_per_frame = 1.0f / (float) game_update_hz;
	return main_loop (display, win, context, width, height, game_update_hz);
}
