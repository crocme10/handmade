#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>		/* getenv(), etc. */
#include <unistd.h>		/* sleep(), etc.  */
#include <stdint.h>

/* 
 * function: create_window. Creates a window with a white background
 *           in the given size.
 * input:    display, size of the window (in pixels), and location of the window
 *           (in pixels).
 * output:   the window's ID.
 * notes:    window is created with a black border, 2 pixels wide.
 *           the window is automatically mapped after its creation.
 */
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

  /* make the window actually appear on the screen. */
  XMapWindow (display, win);

  /* flush all pending requests to the X server. */
  XFlush (display);

  return win;
}

GC
create_context (Display * display, Window win)
{
  int screen_num = DefaultScreen (display);

  unsigned long valuemask = 0;	/* which values in 'values' to */
  XGCValues values;		/* initial values for the GC.  */
  GC context = XCreateGC (display, win, valuemask, &values);

  XSetForeground (display, context, BlackPixel (display, screen_num));
  XSetBackground (display, context, WhitePixel (display, screen_num));

  XSelectInput (display, win, ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | ExposureMask);

  return context;
}

static XImage *
create_image (Display * display, Visual * visual, uint8_t * image, int width, int height)
{
  fprintf (stdout, "creating image %d x %d\n", width, height);
  int bytes_per_pixel = 4;
  int pitch = width;
  uint32_t *buffer = (uint32_t *) calloc (width * height, bytes_per_pixel);
  uint32_t *pixel = buffer; 
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height; j++)
    {
      uint8_t blue = i % 255;
      uint8_t green = j % 255;
      *pixel++ = (green << 8) | blue;
    }
  }
  return XCreateImage (display, visual, 24 /* depth */, ZPixmap /* format */, 0 /* offset */,
                       (char *) buffer, width, height, 32 /* bitmap pad */, 0);
}

int
main_loop (Display * display, Window win, GC context, int width, int height)
{
  Colormap colormap = DefaultColormap (display, DefaultScreen (display));
  Visual *visual = DefaultVisual (display, DefaultScreen (display));

  //fprintf ("%d depth\n", DefaultDepth (display, DefaultScreen (display)));
  if (visual->class != TrueColor)
  {
    fprintf (stderr, "Cannot handle not true color visual\n");
    return 1;
  }

  XEvent ev;
  while (1)
  {
    XNextEvent (display, &ev);
    switch (ev.type)
    {
      case Expose:
      {
	XImage *image = create_image (display, visual, 0, width, height);
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
  }
}


int
main (int argc, char **argv)
{
  char *display_name = getenv ("DISPLAY");	/* address of the X display.  */

  /* open connection with the X server. */
  Display *display = XOpenDisplay (display_name);
  if (display == NULL)
  {
    fprintf (stderr, "%s: cannot connect to X server '%s'\n", argv[0], display_name);
    exit (1);
  }

  /* get the geometry of the default screen for our display. */
  int screen_num = DefaultScreen (display);

  // unsigned int display_width = DisplayWidth (display, screen_num);
  // unsigned int display_height = DisplayHeight (display, screen_num);
  unsigned int display_width = 400;
  unsigned int display_height = 400;

  /* make the new window occupy 1/9 of the screen's size. */
  unsigned int width = display_width;
  unsigned int height = display_height;
  printf ("window width - '%d'; height - '%d'\n", width, height);

  /* create a simple window, as a direct child of the screen's */
  /* root window. Use the screen's white color as the background */
  /* color of the window. Place the new window's top-left corner */
  /* at the given 'x,y' coordinates.  */
  Window win = create_window (display, width, height, 0, 0);

  /* allocate a new GC (graphics context) for drawing in the window. */
  GC context = create_context (display, win);

  return main_loop (display, win, context, width, height);
}
