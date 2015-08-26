/* From http://neuron-ai.tuke.sk/hudecm/Tutorials/C/special/xlib-programming/ */

#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h>		/* getenv(), etc. */
#include <unistd.h>		/* sleep(), etc.  */

int
main (int argc, char* argv[])
{
  /* Opening Connection to X Server */
  char *display_name = getenv ("DISPLAY");  /* address of the X display.      */

  Display* display = XOpenDisplay (display_name);
  if (display == NULL)
  {
    fprintf(stderr, "%s: cannot connect to X server '%s'\n", argv[0], display_name);
    exit(1);
  }

  /* get the geometry of the default screen for our display. */
  /* number of screen to place the window on.  */
  int screen_num = DefaultScreen (display);
  unsigned int display_width = DisplayWidth(display, screen_num);
  unsigned int display_height = DisplayHeight(display, screen_num);

  /* make the new window occupy 1/9 of the screen's size. */
  unsigned int width = (display_width / 3);
  unsigned int height = (display_height / 3);

  /* the window should be placed at the top-left corner of the screen. */
  unsigned int win_x = 0;
  unsigned int win_y = 0;

  /* the window's border shall be 2 pixels wide. */
  unsigned int win_border_width = 2;

  /* create a simple window, as a direct child of the screen's   */
  /* root window. Use the screen's white color as the background */
  /* color of the window. Place the new window's top-left corner */
  /* at the given 'x,y' coordinates.                             */
  /* pointer to the newly created window.      */
  Window win = XCreateSimpleWindow(display, RootWindow (display, screen_num),
                                   win_x, win_y, width, height, win_border_width,
                                   BlackPixel(display, screen_num),
                                   WhitePixel(display, screen_num));

  /* make the window actually appear on the screen. */
  XMapWindow (display, win);

  /* flush all pending requests to the X server, and wait until */
  /* they are processed by the X server.                        */
  XSync (display, False);

  /* make a delay for a short period. */
  sleep (4);

  /* close the connection to the X server. */
  XCloseDisplay (display);

  return 0;
}
