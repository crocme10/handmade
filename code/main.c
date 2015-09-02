#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <stdio.h>
#include <stdlib.h>		/* getenv(), etc. */
#include <unistd.h>		/* sleep(), etc.  */
#include <stdint.h>
#include <stdbool.h>

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

  XSelectInput (display, win, ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | ExposureMask
                | KeyPressMask | KeyReleaseMask | KeymapStateMask);

  return context;
}

static XImage *
create_image (Display * display, Visual * visual, uint8_t * image, int width, int height,
              int x_offset, int y_offset)
{
  int bytes_per_pixel = 4;
  uint32_t pitch = width * bytes_per_pixel;
  uint32_t *buffer = (uint32_t *) calloc (width * height, bytes_per_pixel);
  uint8_t *row = (uint8_t *)buffer;
  for (int j = 0; j < height; j++)
  {
    uint32_t *pixel = (uint32_t *)row;
    for (int i = 0; i < width; i++)
    {
      uint8_t blue = (i + x_offset);
      uint8_t green = (j + y_offset);
      *pixel++ = (green << 8) | blue;
    }
    row += pitch;
  }
  return XCreateImage (display, visual, 24 /* depth */, ZPixmap /* format */, 0 /* offset */,
                       (char *) buffer, width, height, 32 /* bitmap pad */, 0);
}

int
main_loop (Display * display, Window win, GC context, int width, int height)
{
  //Colormap colormap = DefaultColormap (display, DefaultScreen (display));
  Visual *visual = DefaultVisual (display, DefaultScreen (display));

  //fprintf ("%d depth\n", DefaultDepth (display, DefaultScreen (display)));
  if (visual->class != TrueColor)
  {
    fprintf (stderr, "Cannot handle not true color visual\n");
    return 1;
  }

  int x_offset = 0;
  int y_offset = 0;

  XEvent ev;
  while (1)
  {
    XNextEvent (display, &ev);
    switch (ev.type)
    {
    case KeymapNotify:
      {
        XRefreshKeyboardMapping(&ev.xmapping);
      } break;
    case KeyPress: break; /* ignore these */
    case KeyRelease:
      {
        char string[25];
        KeySym keysym;
        int len = XLookupString(&ev.xkey, string, 25, &keysym, NULL);
        bool redraw = false;
        switch (keysym)
        {
        case XK_Left:
          {
            x_offset += 5;
            redraw = true;
          } break;
        case XK_Right:
          {
            x_offset -= 5;
            redraw = true;
          } break;
        case XK_Up:
          {
            y_offset += 5;
            redraw = true;
          } break;
        case XK_Down:
          {
            y_offset -= 5;
            redraw = true;
          } break;
        default: break;
        }
        if (redraw)
        {
          XImage *image = create_image (display, visual, 0, width, height, x_offset, y_offset);
          XPutImage (display, win, context, image, 0, 0, 0, 0, width, height);
          XDestroyImage (image);
          XFlush (display);
        }
      } break;
      case Expose:
      {
        XImage *image = create_image (display, visual, 0, width, height, x_offset, y_offset);
        XPutImage (display, win, context, image, 0, 0, 0, 0, width, height);
        XDestroyImage (image);
        XFlush (display);
      } break;
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
      } break;
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
  //int screen_num = DefaultScreen (display);

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
