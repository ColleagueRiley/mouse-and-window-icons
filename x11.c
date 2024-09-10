// this can be compiled with:
// gcc x11.c -lX11 -lXcursor 

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/cursorfont.h>

#include <stdio.h>
#include <stdlib.h>

unsigned char icon[4 * 3 * 3] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};

int main(void) {
 
    Display* display = XOpenDisplay(NULL); 
    Window window = XCreateSimpleWindow(display, RootWindow(display, DefaultScreen(display)), 10, 10, 200, 200, 1,
                                 BlackPixel(display, DefaultScreen(display)), WhitePixel(display, DefaultScreen(display)));
    
	XSelectInput(display, window, ExposureMask | KeyPressMask | ButtonPressMask);
    XMapWindow(display, window);

	// window icon
	int longCount = 2 + 3 * 3;

	unsigned long* X11Icon = (unsigned long*) malloc(longCount * sizeof(unsigned long));
	unsigned long* target = X11Icon;

	*target++ = 3;
	*target++ = 3;

	unsigned int i;

	for (i = 0; i < 3 * 3; i++) {
		*target++ = ((icon[i * 4 + 2])) | // g
			((icon[i * 4 + 1]) << 8) | // b
			((icon[i * 4 + 0]) << 16) | // r 
			((icon[i * 4 + 3]) << 24); // a
	}

	const Atom NET_WM_ICON = XInternAtom((Display*) display, "_NET_WM_ICON", False);

	XChangeProperty((Display*) display, (Window) window,
		NET_WM_ICON,
		6, 32,
		PropModeReplace,
		(unsigned char*) X11Icon,
		longCount);

	free(X11Icon);
	XFlush((Display*) display);

	// mouse icon image
	XcursorImage* native = XcursorImageCreate(3, 3);
	native->xhot = 0;
	native->yhot = 0;

	unsigned char* source = (unsigned char*) icon;
	XcursorPixel* icon_target = native->pixels;

	for (i = 0; i < 3 * 3; i++, icon_target++, source += 4) {
		unsigned char alpha = 0xFF;
		alpha = source[3];

		*icon_target = (alpha << 24) | (((source[0] * alpha) / 255) << 16) | (((source[1] * alpha) / 255) << 8) | (((source[2] * alpha) / 255) << 0);
	}

	Cursor cursor = XcursorImageLoadCursor((Display*) display, native);

	XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);

	XFreeCursor((Display*) display, (Cursor) cursor);
	XcursorImageDestroy(native);


	

	XEvent event;
    
	for (;;) {
        XNextEvent(display, &event);
		
		if (event.type == ButtonPress) {
			Cursor cursor = XCreateFontCursor((Display*) display, XC_watch);
			XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
			XFreeCursor((Display*) display, (Cursor) cursor);
		}
        if (event.type == KeyPress)
            break;
    }
 
    XCloseDisplay(display);
 }
