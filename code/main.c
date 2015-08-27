/* first include the standard headers that we're likely to need */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static XrmOptionDescRec xrmTable[] = {
  {"-bg", "*background", XrmoptionSepArg, NULL},
  {"-fg", "*foreground", XrmoptionSepArg, NULL},
  {"-bc", "*bordercolour", XrmoptionSepArg, NULL},
  {"-font", "*font", XrmoptionSepArg, NULL},
};

static XrmDatabase
setupDB (Display * display, XrmOptionDescRec * xrmTable,
         int nCommandLineResources, const char *progname, int *argc, char **argv)
{
  char filename[256];

  XrmInitialize ();
  XrmDatabase db = XrmGetDatabase (display);
  XrmParseCommand (&db, xrmTable, nCommandLineResources, progname, argc, argv);
  sprintf (filename, "%.240s.resources", progname);
  if (XrmCombineFileDatabase (filename, &db, False))
  {
    printf ("read %s ", filename);
  }
  else
  {
    printf ("didn't read %s ", filename);
  }
  return db;
}

/* This function may not return safe values */
static char *
getResource (Display * display, XrmDatabase db, char *name, char *cl, char *def)
{
  XrmValue value;
  char *type;
  if (XrmGetResource (db, name, cl, &type, &value))
  {
    return strdup (value.addr);
  }
  return strdup (def);
}

/* Probably leaking resources */
static unsigned long
getColour (Display * display, XrmDatabase db, char *name, char *cl, char *def)
{
  XColor col1, col2;
  Colormap colormap = DefaultColormap (display, DefaultScreen (display));
  char *type;

  XrmValue value;
  char *resource = getResource (display, db, name, cl, def);
  XAllocNamedColor (display, colormap, resource, &col1, &col2);
  return col2.pixel;
}

static GC
create_context (Display * display, int argc, char **argv, int *width, int *height)
{
  XrmInitialize ();
  XrmDatabase db = setupDB (display,  xrmTable, sizeof (xrmTable) / sizeof (xrmTable[0]),
                            "main", &argc, argv);
  XrmParseCommand (&db, xrmTable, sizeof (xrmTable) / sizeof (xrmTable[0]), "main", &argc, argv);

  unsigned long background = getColour (display, db, "main.background", "main.Background", "DarkGreen");
  unsigned long border = getColour (display, db, "main.border", "main.Border", "LightGreen");

  int w = 200;
  int h = 200;

  Window win = XCreateSimpleWindow (display, DefaultRootWindow (display),	/* display, parent */
      0, 0, w, h, 2, border, background);

  XGCValues values;
  //values.foreground = getColour (display, db, "main.foreground", "main.Foreground", "Black");
  values.line_width = 1;
  values.line_style = LineSolid;

  XmbSetWMProperties (display, win, "main", "main", argv, argc, NULL, NULL, NULL);

  GC context = XCreateGC (display, win, GCLineWidth | GCLineStyle, &values);

  /* tell the display server what kind of events we would like to see */
  XSelectInput (display, win, ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | ExposureMask);

  /* okay, put the window on the screen, please */
  XMapWindow (display, win);

  *width = w;
  *height = h;

  return context;
}

static XImage *
createImage (Display *display, GC context, int width, int height)
{
  Colormap colormap = DefaultColormap (display, DefaultScreen (display));
  // Window win = DefaultRootWindow (display);
  char *buffer = (char *) malloc (width * height * sizeof (char));
  XImage *image = XCreateImage (display, DefaultVisual (display, DefaultScreen (display)),
                                8, ZPixmap, 0, (char *)buffer, width, height, 8, 0);
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      XColor color;
      color.red = 0;
      color.green = 65535;
      color.blue = 0;
      //color.flags = DoRed | DoGreen | DoBlue;
      XAllocColor (display, colormap, &color); 
      XSetForeground (display, context, color.pixel);
      XPutPixel (image, x, y, color.pixel);
      //buffer [x*y+x] = color.pixel;
    }
  }
  return image;
}

int
main_loop (Display * display, GC context, int width, int height)
{
  XEvent ev;
  //XImage *image = NULL;
  Colormap colormap = DefaultColormap (display, DefaultScreen (display));
  Window win = DefaultRootWindow (display);
  while (1)
  {
    XNextEvent (display, &ev);
    switch (ev.type)
    {
    case Expose:
      {
        for (int y = 0; y < height; y++)
        {
          for (int x = 0; x < width; x++)
          {
            XColor color;
            color.red = 0;
            color.green = 255;
            color.blue = 255;
            //color.flags = DoRed | DoGreen | DoBlue;
            XAllocColor (display, colormap, &color); 
            XSetForeground (display, context, color.pixel);
            XDrawPoint (display, win, context, x, y);
            //buffer [x*y+x] = color.pixel;
          }
        }
      }
      //{
      //  if (image)
      //    XDestroyImage (image);
      //  image = createImage (display, context, width, height);
      //  XPutImage (display, ev.xany.window, context, image, 0, 0, 0, 0, width, height);
      //} break;
    case ConfigureNotify:
      {
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

  /* First connect to the display server */
  Display *display = XOpenDisplay (NULL);
  if (!display)
  {
    fprintf (stderr, "unable to connect to display\n");
    return 7;
  }

  int width, height;
  GC context = create_context (display, argc, argv, &width, &height);
  return main_loop (display, context, width, height);
}
