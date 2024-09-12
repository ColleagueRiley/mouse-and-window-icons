# RGFW Under the Hood: Mouse and Window Icons
## Introduction
Changing the mouse and window icons can be annoying with low-level APIs. That's because you must load data structures specific to the API and load the bitmap using a format the API supports. It can also be unclear which functions must be used to update the icon. This tutorial aims to streamline the process and explain how to load mouse and window icons. 

This tutorial is based on my experience in making RGFW and its source code. The repository can be found [here](https://github.com/ColleagueRiley/RGFW), if you would like to reference it.

## Overview
1) window icons
2) mouse image icons
3) default mouse icons

## Window icons

### X11
First, allocate the array, it will hold our icon data converted for X11's BGR format.

The format will be in ints, which should be 32 bits on Linux. 
The first two elements will be used for the size. So the array length will be the width * height + 2.

```c
int longCount = 2 + width * height;

unsigned long* X11Icon = (unsigned long*) malloc(longCount * sizeof(unsigned long));
unsigned long* target = X11Icon;

*target++ = width;
*target++ = height;
```


Now we can convert the icon to X11's format. This means manually casting the icon array into a 32-bit int in the BGRA format.

```c
unsigned int i;

for (i = 0; i < width * height; i++) {
    *target++ = ((icon[i * 4 + 2])) // b
                ((icon[i * 4 + 1]) << 8) // g
                ((icon[i * 4 + 0]) << 16) // r
                ((icon[i * 4 + 3]) << 24); // a 
}
```

Next, we'll load the X11 atom, `NET_WM_ICON`, using [`XInternAtom`](https://www.x.org/releases/X11R7.5/doc/man/man3/XInternAtom.3.html). This atom is used for the Window's Window Manager icon property. 

Then we'll use the [`XChangeProperty`](https://linux.die.net/man/3/xchangeproperty) function to change the icon property to the icon data.

```c
const Atom NET_WM_ICON = XInternAtom((Display*) display, "_NET_WM_ICON", False);

XChangeProperty((Display*) display, (Window) window,
    NET_WM_ICON,
    6, 32,
    PropModeReplace,
    (unsigned char*) X11Icon,
    longCount);
```

Use [`XFlush`](https://www.x.org/releases/X11R7.5/doc/man/man3/XSync.3.html) to update the X11 server on the change.

```c
free(X11Icon);
XFlush((Display*) display);
```

### win32

I'll start by creating a `loadHandleImage` function. This function will be used for mouse and window icons to load a bitmap image into a win32 icon handle.

```c
HICON loadHandleImage(unsigned char* src, unsigned int width, unsigned int height, BOOL icon) {
``` 

We'll start by creating a [`BITMAPV5HEADER`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header) structure with the proper format to match how our data is stored.

```c
    BITMAPV5HEADER bi; 
    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = width;
    bi.bV5Height = -((LONG) height);
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00ff0000;
    bi.bV5GreenMask = 0x0000ff00;
    bi.bV5BlueMask = 0x000000ff;
    bi.bV5AlphaMask = 0xff000000;
```

Next, we'll create a color section for the icon.

First get the Drawing Context with [`GetDC`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc), next create the section with  [`CreateDIBSection`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection).

Make sure to release the Drawing Context with [`ReleaseDC`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-releasedc).

```c
    unsigned char* target = NULL;
      
    HDC dc = GetDC(NULL);
    HBITMAP color = CreateDIBSection(dc,
        (BITMAPINFO*) &bi,
        DIB_RGB_COLORS,
        (void**) &target,
        NULL,
        (DWORD) 0);
    ReleaseDC(NULL, dc);
```

Then a win32 bitmap can be created with [`CreateBitmap`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createbitmap)

```c
    HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);
```

Now we'll load our image data into the bitmap.

```c
    unsigned char* source = src; 
    unsigned int i;
    for (i = 0; i < width * height; i++) {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }
```

Now we can create a [`ICONINFO`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-iconinfo) structure for updating the window icon.

```c
    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = mask;
    ii.hbmColor = color;
```

Then we'll create the icon handle with [`CreateIconIndirect`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createiconindirect). 

```c
    HICON handle = CreateIconIndirect(&ii);
```

Finally, we can free the object color and mask data and return the icon handle.

[`DeleteObject`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-deleteobject)

```c
    DeleteObject(color);
    DeleteObject(mask);

    return handle;
}
```

#### part 2

Now we can use our function to create an icon handle  

```c
HICON handle = loadHandleImage(buffer, width, height, TRUE);
```

Then we'll set the handle as the window icon via [`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra)

```c
SetClassLongPtrA(hwnd, GCLP_HICON, (LPARAM) handle);
```

Make sure to free the icon handle now that we're done with it using [`DestroyIcon`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroyicon).

```c
DestroyIcon(handle);
```

### cocoa

Make a bitmap representation with [`initWithBitmapData`](https://developer.apple.com/documentation/coreimage/ciimage/1437857-initwithbitmapdata) then copy the icon data to it using [`bitmapData`](https://developer.apple.com/documentation/appkit/nsbitmapimagerep/1395421-bitmapdata).

I will also be using the NSString "NSCalibratedRGBColorSpace" for the colorSpaceName. This means it must be converted from a c-string.


```c
func = sel_registerName("initWithBitmapDataPlanes:pixelsWide:pixelsHigh:bitsPerSample:samplesPerPixel:hasAlpha:isPlanar:colorSpaceName:bitmapFormat:bytesPerRow:bitsPerPixel:");

char* NSCalibratedRGBColorSpace = ((id(*)(id, SEL, const char*))objc_msgSend) ((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "NSCalibratedRGBColorSpace"); 

void* representation = ((id(*)(id, SEL, unsigned char**, NSInteger, NSInteger, NSInteger, NSInteger, bool, bool, const char*, NSBitmapFormat, NSInteger, NSInteger))objc_msgSend)
        (NSAlloc((id)objc_getClass("NSBitmapImageRep")), func, NULL, width, height, 8, 4, true, false, NSCalibratedRGBColorSpace, 1 << 1, width * channels, 8 * channels);
```

Next, create the image with the matching size via [`NSImage_init`](https://developer.apple.com/documentation/appkit/nsimage/1519860-init) and add the representation using [`addRepresentation`](https://developer.apple.com/documentation/appkit/nsimage/1519911-addrepresentation).

```c
void* dock_image = ((id(*)(id, SEL, NSSize))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSImage")), sel_registerName("initWithSize:"), (NSSize){width, height});
	objc_msgSend_void_id(dock_image, sel_registerName("addRepresentation:"), representation);
```

Finally, set the dock image to it using [`setApplicationIconImage`](https://developer.apple.com/documentation/appkit/nsapplication/1428744-applicationiconimage).

```c
objc_msgSend_void_id(NSApp, sel_registerName("setApplicationIconImage:"), dock_image);
```

Free the leftover data with `NSRelease`.
```c
NSRelease(dock_image);
NSRelease(representation);
```

## mouse icon image

### X11
First, create an X11 cursor image using [`XcursorImageCreate`](https://linux.die.net/man/3/xcursorimagecreate).
```c
XcursorImage* native = XcursorImageCreate(width, height);
native->xhot = 0;
native->yhot = 0;
```

Then we'll make a pointer to the pixel data and a pointer to the XCursor image's pointer data.

```c
unsigned char* source = (unsigned char*) image;
XcursorPixel* target = native->pixels;
```

Now we can convert the pixel data to the X11 format.

The X11 format uses BGRA with ints, we must reorganize the array, apply the opacity, and cast the data type manually.

```c
unsigned int i;
for (i = 0; i < width * height; i++, target++, source += 4) {
    unsigned char alpha = source[3];

    *target = (((source[2] * alpha) / 255)) | // b 
                (((source[1] * alpha) / 255) << 8)  | // g 
                ((source[0] * alpha) / 255) << 16)) | // r 
                (alpha << 24); // r
}
```

Next, we'll create a cursor with the cursor image using [`XcursorImageLoadCursor`](https://man.archlinux.org/man/XcursorImageLoadCursor.3.en).

```c
Cursor cursor = XcursorImageLoadCursor((Display*) display, native);
```

Then [`XDefineCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XDefineCursor.3.html) sets the new cursor.

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

Finally, we can free the cursor and cursor image with [`XFreeCursor`](https://linux.die.net/man/3/xfreecursor) and [`XcursorImageDestroy`](https://linux.die.net/man/3/xcursorimagedestroy).

```c
XFreeCursor((Display*) display, (Cursor) cursor);
XcursorImageDestroy(native);
```

### win32

We'll use the function I defined earlier to create a cursor icon handle. 

```c
HCURSOR cursor = (HCURSOR) loadHandleImage(image, width, height, FALSE);
```

Then we can use  [`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra) and [`SetCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor) to change the cursor. 

```c
SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) cursor);
SetCursor(cursor);
```

Free the cursor via [`DestroyCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroycursor).

```c
DestroyCursor(cursor);
```

### cocoa

Make a bitmap representation with initWithBitmapData then copy the icon data to it using bitmapData.

```c
representation = ((id(*)(id, SEL, unsigned char**, NSInteger, NSInteger, NSInteger, NSInteger, bool, bool, const char*, NSBitmapFormat, NSInteger, NSInteger))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSBitmapImageRep")), func, NULL, width, height, 8, channels, true, false, NSCalibratedRGBColorSpace, 1 << 1, width * channels, 8 * channels);

	memcpy(((unsigned char* (*)(id, SEL))objc_msgSend)
			(representation, sel_registerName("bitmapData")), 
			icon, width * height * channels);
```

Next, create the image with the matching size via [`NSImage_init`](https://developer.apple.com/documentation/appkit/nsimage/1519860-init) and 
Add the representation with [`addRepresentation`](https://developer.apple.com/documentation/appkit/nsimage/1519911-addrepresentation).

```c
void* cursor_image = ((id(*)(id, SEL, NSSize))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSImage")), sel_registerName("initWithSize:"), (NSSize){width, height});
	objc_msgSend_void_id(cursor_image, sel_registerName("addRepresentation:"), representation);


```

Finally, create the cursor icon using  [`initWithImage`](https://developer.apple.com/documentation/uikit/uiimageview/1621062-initwithimage) and set the cursor using [`set`](https://developer.apple.com/documentation/appkit/nscursor/1526148-set).

```c
void* cursor = ((id(*)(id, SEL, id, NSPoint))objc_msgSend)
						(NSAlloc(objc_getClass("NSCursor")), sel_registerName("initWithImage:hotSpot:"), 
						 cursor_image, (NSPoint){0.0, 0.0});

objc_msgSend_void(cursor, sel_registerName("set"));
```

Make sure to free the list over data with `NSRelease`

```c
NSRelease(cursor_image);
NSRelease(representation);
```

## standard mouse icons
### X11

First, create a cursor for the standard cursor you want to use with [`XCreateFontCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XCreateFontCursor.3.html).

Change `mouse` to be an actual cursor macro (they can be found in `/usr/include/X11/cursorfont.h`)

```c
Cursor cursor = XCreateFontCursor((Display*) display, mouse);
```

Then you can update the cursor with [`XDefineCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XDefineCursor.3.html).

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

Free the cursor data with [`XFreeCursor`](https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreeCursor.html).

```c
XFreeCursor((Display*) display, (Cursor) cursor);
```

### win32

First, create a cursor for the standard cursor you want to use with [`MAKEINTRESOURCEA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcea)

```c
char* icon = MAKEINTRESOURCEA(mouse);
```

Then you can use  [`LoadCursorA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadcursora) to load the cursor.

Then use [`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra) and [`SetCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor) to update the current cursor.

```c
SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) LoadCursorA(NULL, icon));
SetCursor(LoadCursorA(NULL, icon));
```

### cocoa

First, find the cursor object you would like to use.

For example, [`arrowCursor`](https://developer.apple.com/documentation/appkit/nscursor/1527160-arrowcursor?changes=_1&language=objc)

```c 
void* mouse = objc_msgSend_id(objc_getClass("NSCursor"), sel_registerName("arrowCursor"));
```

Then you can use [`set`](https://developer.apple.com/documentation/appkit/nscursor/1526148-set) to set it as the current cursor.

```c
objc_msgSend_void(mouse, sel_registerName("set"));
```


## full examples

### x11
```c
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
```

### winapi
```c
// This can be compiled with 
// gcc win32.c -lgdi32

#include <windows.h>

unsigned char icon[4 * 3 * 3] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};


HICON loadHandleImage(unsigned char* src, unsigned int width, unsigned int height, BOOL icon) {
    BITMAPV5HEADER bi; 
    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = width;
    bi.bV5Height = -((LONG) height);
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00ff0000;
    bi.bV5GreenMask = 0x0000ff00;
    bi.bV5BlueMask = 0x000000ff;
    bi.bV5AlphaMask = 0xff000000;
    
	unsigned char* target = NULL;
      
    HDC dc = GetDC(NULL);
    HBITMAP color = CreateDIBSection(dc,
        (BITMAPINFO*) &bi,
        DIB_RGB_COLORS,
        (void**) &target,
        NULL,
        (DWORD) 0);
    ReleaseDC(NULL, dc);

    HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);

    unsigned char* source = src; 
    unsigned int i;
    for (i = 0; i < width * height; i++) {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = mask;
    ii.hbmColor = color;

    HICON handle = CreateIconIndirect(&ii);

    DeleteObject(color);
    DeleteObject(mask);

    return handle;
}

int main() {
	WNDCLASS wc = {0};
	wc.lpfnWndProc   = DefWindowProc; // Default window procedure
	wc.hInstance     = GetModuleHandle(NULL);
	wc.lpszClassName = "SampleWindowClass";
	
	RegisterClass(&wc);
	
	HWND hwnd = CreateWindowA(wc.lpszClassName, "Sample Window", WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX,
			500, 500, 500, 500,
			NULL, NULL, wc.hInstance, NULL);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// window icon 
	HICON handle = loadHandleImage(icon, 3, 3, TRUE);
	SetClassLongPtrA(hwnd, GCLP_HICON, (LPARAM) handle);
	DestroyIcon(handle);
	
	// mouse icon image
	HCURSOR cursor = (HCURSOR) loadHandleImage(icon, 3, 3, FALSE);

	SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) cursor);
	SetCursor(cursor);

	DestroyCursor(cursor);

	MSG msg;
	
	BOOL running = TRUE;
	
	while (running) {
		if (PeekMessageA(&msg, hwnd, 0u, 0u, PM_REMOVE)) {
			if (msg.message == WM_KEYUP) {
				char* icon = MAKEINTRESOURCEA(IDC_IBEAM);
				
				SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) LoadCursorA(NULL, icon));
				SetCursor(LoadCursorA(NULL, icon));	
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		running = IsWindow(hwnd);
	}
}
```

### cocoa
```c
// compile with:
// gcc cocoa.c -lm -framework Foundation -framework AppKit -framework CoreVideo

#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreVideo/CVDisplayLink.h>
#include <ApplicationServices/ApplicationServices.h>

#ifdef __arm64__
/* ARM just uses objc_msgSend */
#define abi_objc_msgSend_stret objc_msgSend
#define abi_objc_msgSend_fpret objc_msgSend
#else /* __i386__ */
/* x86 just uses abi_objc_msgSend_fpret and (NSColor *)objc_msgSend_id respectively */
#define abi_objc_msgSend_stret objc_msgSend_stret
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#endif

typedef CGRect NSRect;
typedef CGPoint NSPoint;
typedef CGSize NSSize;

typedef void NSEvent;
typedef void NSString;
typedef void NSWindow;	
typedef void NSApplication;

typedef unsigned long NSUInteger;
typedef long NSInteger;

#define NS_ENUM(type, name) type name; enum 

typedef NS_ENUM(NSUInteger, NSWindowStyleMask) {
	NSWindowStyleMaskBorderless = 0,
	NSWindowStyleMaskTitled = 1 << 0,
	NSWindowStyleMaskClosable = 1 << 1,
	NSWindowStyleMaskMiniaturizable = 1 << 2,
	NSWindowStyleMaskResizable = 1 << 3,
	NSWindowStyleMaskTexturedBackground = 1 << 8, /* deprecated */
	NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
	NSWindowStyleMaskFullScreen = 1 << 14,
	NSWindowStyleMaskFullSizeContentView = 1 << 15,
	NSWindowStyleMaskUtilityWindow = 1 << 4,
	NSWindowStyleMaskDocModalWindow = 1 << 6,
	NSWindowStyleMaskNonactivatingPanel = 1 << 7,
	NSWindowStyleMaskHUDWindow = 1 << 13
};

typedef NS_ENUM(NSUInteger, NSBackingStoreType) {
	NSBackingStoreRetained = 0,
	NSBackingStoreNonretained = 1,
	NSBackingStoreBuffered = 2
};

typedef NS_ENUM(NSUInteger, NSEventType) {        /* various types of events */
	NSEventTypeLeftMouseDown             = 1,
	NSEventTypeLeftMouseUp               = 2,
	NSEventTypeRightMouseDown            = 3,
	NSEventTypeRightMouseUp              = 4,
	NSEventTypeMouseMoved                = 5,
	NSEventTypeLeftMouseDragged          = 6,
	NSEventTypeRightMouseDragged         = 7,
	NSEventTypeMouseEntered              = 8,
	NSEventTypeMouseExited               = 9,
	NSEventTypeKeyDown                   = 10,
	NSEventTypeKeyUp                     = 11,
	NSEventTypeFlagsChanged              = 12,
	NSEventTypeAppKitDefined             = 13,
	NSEventTypeSystemDefined             = 14,
	NSEventTypeApplicationDefined        = 15,
	NSEventTypePeriodic                  = 16,
	NSEventTypeCursorUpdate              = 17,
	NSEventTypeScrollWheel               = 22,
	NSEventTypeTabletPoint               = 23,
	NSEventTypeTabletProximity           = 24,
	NSEventTypeOtherMouseDown            = 25,
	NSEventTypeOtherMouseUp              = 26,
	NSEventTypeOtherMouseDragged         = 27,
	/* The following event types are available on some hardware on 10.5.2 and later */
	NSEventTypeGesture API_AVAILABLE(macos(10.5))       = 29,
	NSEventTypeMagnify API_AVAILABLE(macos(10.5))       = 30,
	NSEventTypeSwipe   API_AVAILABLE(macos(10.5))       = 31,
	NSEventTypeRotate  API_AVAILABLE(macos(10.5))       = 18,
	NSEventTypeBeginGesture API_AVAILABLE(macos(10.5))  = 19,
	NSEventTypeEndGesture API_AVAILABLE(macos(10.5))    = 20,

	NSEventTypeSmartMagnify API_AVAILABLE(macos(10.8)) = 32,
	NSEventTypeQuickLook API_AVAILABLE(macos(10.8)) = 33,

	NSEventTypePressure API_AVAILABLE(macos(10.10.3)) = 34,
	NSEventTypeDirectTouch API_AVAILABLE(macos(10.10)) = 37,

	NSEventTypeChangeMode API_AVAILABLE(macos(10.15)) = 38,
};

typedef NS_ENUM(unsigned long long, NSEventMask) { /* masks for the types of events */
	NSEventMaskLeftMouseDown         = 1ULL << NSEventTypeLeftMouseDown,
	NSEventMaskLeftMouseUp           = 1ULL << NSEventTypeLeftMouseUp,
	NSEventMaskRightMouseDown        = 1ULL << NSEventTypeRightMouseDown,
	NSEventMaskRightMouseUp          = 1ULL << NSEventTypeRightMouseUp,
	NSEventMaskMouseMoved            = 1ULL << NSEventTypeMouseMoved,
	NSEventMaskLeftMouseDragged      = 1ULL << NSEventTypeLeftMouseDragged,
	NSEventMaskRightMouseDragged     = 1ULL << NSEventTypeRightMouseDragged,
	NSEventMaskMouseEntered          = 1ULL << NSEventTypeMouseEntered,
	NSEventMaskMouseExited           = 1ULL << NSEventTypeMouseExited,
	NSEventMaskKeyDown               = 1ULL << NSEventTypeKeyDown,
	NSEventMaskKeyUp                 = 1ULL << NSEventTypeKeyUp,
	NSEventMaskFlagsChanged          = 1ULL << NSEventTypeFlagsChanged,
	NSEventMaskAppKitDefined         = 1ULL << NSEventTypeAppKitDefined,
	NSEventMaskSystemDefined         = 1ULL << NSEventTypeSystemDefined,
	NSEventMaskApplicationDefined    = 1ULL << NSEventTypeApplicationDefined,
	NSEventMaskPeriodic              = 1ULL << NSEventTypePeriodic,
	NSEventMaskCursorUpdate          = 1ULL << NSEventTypeCursorUpdate,
	NSEventMaskScrollWheel           = 1ULL << NSEventTypeScrollWheel,
	NSEventMaskTabletPoint           = 1ULL << NSEventTypeTabletPoint,
	NSEventMaskTabletProximity       = 1ULL << NSEventTypeTabletProximity,
	NSEventMaskOtherMouseDown        = 1ULL << NSEventTypeOtherMouseDown,
	NSEventMaskOtherMouseUp          = 1ULL << NSEventTypeOtherMouseUp,
	NSEventMaskOtherMouseDragged     = 1ULL << NSEventTypeOtherMouseDragged,
};
/* The following event masks are available on some hardware on 10.5.2 and later */
#define NSEventMaskGesture API_AVAILABLE(macos(10.5))          (1ULL << NSEventTypeGesture)
#define NSEventMaskMagnify API_AVAILABLE(macos(10.5))          (1ULL << NSEventTypeMagnify)
#define NSEventMaskSwipe API_AVAILABLE(macos(10.5))            (1ULL << NSEventTypeSwipe)
#define NSEventMaskRotate API_AVAILABLE(macos(10.5))           (1ULL << NSEventTypeRotate)
#define NSEventMaskBeginGesture API_AVAILABLE(macos(10.5))     (1ULL << NSEventTypeBeginGesture)
#define NSEventMaskEndGesture API_AVAILABLE(macos(10.5))       (1ULL << NSEventTypeEndGesture)

/* Note: You can only use these event masks on 64 bit. In other words, you cannot setup a local, nor global, event monitor for these event types on 32 bit. Also, you cannot search the event queue for them (nextEventMatchingMask:...) on 32 bit. */
#define NSEventMaskSmartMagnify API_AVAILABLE(macos(10.8)) (1ULL << NSEventTypeSmartMagnify)
#define NSEventMaskPressure API_AVAILABLE(macos(10.10.3)) (1ULL << NSEventTypePressure)
#define NSEventMaskDirectTouch API_AVAILABLE(macos(10.12.2)) (1ULL << NSEventTypeDirectTouch)
#define NSEventMaskChangeMode API_AVAILABLE(macos(10.15)) (1ULL << NSEventTypeChangeMode)
#define NSEventMaskAny              NSUIntegerMax

typedef NS_ENUM(NSUInteger, NSEventModifierFlags) {
	NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
	NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
	NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
	NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
	NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
	NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
	NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
	NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
};

#define objc_msgSend_id				((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_id_id			((id (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_id_rect		((id (*)(id, SEL, NSRect))objc_msgSend)
#define objc_msgSend_uint			((NSUInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_int			((NSInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_SEL			((SEL (*)(id, SEL))objc_msgSend)
#define objc_msgSend_float			((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)
#define objc_msgSend_bool			((BOOL (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void			((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_double			((double (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id		((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_uint		((void (*)(id, SEL, NSUInteger))objc_msgSend)
#define objc_msgSend_void_int		((void (*)(id, SEL, NSInteger))objc_msgSend)
#define objc_msgSend_void_bool		((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_void_float		((void (*)(id, SEL, CGFloat))objc_msgSend)
#define objc_msgSend_void_double	((void (*)(id, SEL, double))objc_msgSend)
#define objc_msgSend_void_SEL		((void (*)(id, SEL, SEL))objc_msgSend)
#define objc_msgSend_id_char_const	((id (*)(id, SEL, char const *))objc_msgSend)

typedef enum NSApplicationActivationPolicy {
	NSApplicationActivationPolicyRegular,
	NSApplicationActivationPolicyAccessory,
	NSApplicationActivationPolicyProhibited
} NSApplicationActivationPolicy;

typedef enum NSBitmapFormat {
	NSBitmapFormatAlphaFirst = 1 << 0,       // 0 means is alpha last (RGBA, CMYKA, etc.)
	NSBitmapFormatAlphaNonpremultiplied = 1 << 1,       // 0 means is premultiplied
	NSBitmapFormatFloatingPointSamples = 1 << 2,  // 0 is integer

	NSBitmapFormatSixteenBitLittleEndian API_AVAILABLE(macos(10.10)) = (1 << 8),
	NSBitmapFormatThirtyTwoBitLittleEndian API_AVAILABLE(macos(10.10)) = (1 << 9),
	NSBitmapFormatSixteenBitBigEndian API_AVAILABLE(macos(10.10)) = (1 << 10),
	NSBitmapFormatThirtyTwoBitBigEndian API_AVAILABLE(macos(10.10)) = (1 << 11)
} NSBitmapFormat;


#define NSAlloc(nsclass) objc_msgSend_id((id)nsclass, sel_registerName("alloc"))
#define NSRelease(nsclass) objc_msgSend_id((id)nsclass, sel_registerName("release"))

unsigned char icon[4 * 3 * 3] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};

bool running = true;

unsigned int onClose(void* self) {
	NSWindow* win = NULL;
	object_getInstanceVariable(self, "NSWindow", (void*)&win);
	if (win == NULL)
		return true;

	running = false;

	return true;
}

#include <string.h>

int main(int argc, char* argv[]) {
	class_addMethod(objc_getClass("NSObject"), sel_registerName("windowShouldClose:"), (IMP) onClose, 0);

	NSApplication* NSApp = objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
	objc_msgSend_void_int(NSApp, sel_registerName("setActivationPolicy:"), NSApplicationActivationPolicyRegular);

	NSBackingStoreType macArgs = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSBackingStoreBuffered | NSWindowStyleMaskTitled | NSWindowStyleMaskResizable;

	SEL func = sel_registerName("initWithContentRect:styleMask:backing:defer:");
	
	NSWindow* window = ((id (*)(id, SEL, NSRect, NSWindowStyleMask, NSBackingStoreType, bool))objc_msgSend)
			(NSAlloc(objc_getClass("NSWindow")), func, 
						(NSRect){{200, 200}, {200, 200}}, 
						macArgs, macArgs, false);

	objc_msgSend_void_bool(NSApp, sel_registerName("activateIgnoringOtherApps:"), true);
	((id(*)(id, SEL, SEL))objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), NULL);
	objc_msgSend_void_bool(window, sel_registerName("setIsVisible:"), true);

	objc_msgSend_void(NSApp, sel_registerName("finishLaunching"));
	// window icons
	func = sel_registerName("initWithBitmapDataPlanes:pixelsWide:pixelsHigh:bitsPerSample:samplesPerPixel:hasAlpha:isPlanar:colorSpaceName:bitmapFormat:bytesPerRow:bitsPerPixel:");
	char* NSCalibratedRGBColorSpace = ((id(*)(id, SEL, const char*))objc_msgSend) ((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "NSCalibratedRGBColorSpace"); 
	void* representation = ((id(*)(id, SEL, unsigned char**, NSInteger, NSInteger, NSInteger, NSInteger, bool, bool, const char*, NSBitmapFormat, NSInteger, NSInteger))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSBitmapImageRep")), func, NULL, 3, 3, 8, 4, true, false, NSCalibratedRGBColorSpace, 1 << 1, 3 * 4, 8 * 4);

	memcpy(((unsigned char* (*)(id, SEL))objc_msgSend)
			(representation, sel_registerName("bitmapData")), 
			icon, 3 * 3 * 4);
	
	void* dock_image = ((id(*)(id, SEL, NSSize))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSImage")), sel_registerName("initWithSize:"), (NSSize){3, 3});
	objc_msgSend_void_id(dock_image, sel_registerName("addRepresentation:"), representation);

	objc_msgSend_void_id(NSApp, sel_registerName("setApplicationIconImage:"), dock_image);

	NSRelease(dock_image);
	NSRelease(representation);

	// mouse icon image
	representation = ((id(*)(id, SEL, unsigned char**, NSInteger, NSInteger, NSInteger, NSInteger, bool, bool, const char*, NSBitmapFormat, NSInteger, NSInteger))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSBitmapImageRep")), func, NULL, 3, 3, 8, 4, true, false, NSCalibratedRGBColorSpace, 1 << 1, 3 * 4, 8 * 4);

	memcpy(((unsigned char* (*)(id, SEL))objc_msgSend)
			(representation, sel_registerName("bitmapData")), 
			icon, 3 * 3 * 4);
	
	void* cursor_image = ((id(*)(id, SEL, NSSize))objc_msgSend)
			(NSAlloc((id)objc_getClass("NSImage")), sel_registerName("initWithSize:"), (NSSize){3, 3});
	objc_msgSend_void_id(cursor_image, sel_registerName("addRepresentation:"), representation);

	void* cursor = ((id(*)(id, SEL, id, NSPoint))objc_msgSend)
						(NSAlloc(objc_getClass("NSCursor")), sel_registerName("initWithImage:hotSpot:"), 
						 cursor_image, (NSPoint){0.0, 0.0});

	objc_msgSend_void(cursor, sel_registerName("set"));
	
	NSRelease(cursor_image);
	NSRelease(representation);


	while (running) {
		id pool = objc_msgSend_id(NSAlloc(objc_getClass("NSAutoreleasePool")), sel_registerName("init"));

		NSEvent* e = (NSEvent*) ((id(*)(id, SEL, NSEventMask, void*, NSString*, bool))objc_msgSend) (NSApp, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"), ULONG_MAX, NULL, ((id(*)(id, SEL, const char*))objc_msgSend) ((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "kCFRunLoopDefaultMode"), true);
	
		unsigned int type = objc_msgSend_uint(e, sel_registerName("type"));  
		
		if (type == NSEventTypeLeftMouseUp) {
			// standard mouse icons
			void* mouse = objc_msgSend_id(objc_getClass("NSCursor"), sel_registerName("IBeamCursor"));
			objc_msgSend_void(mouse, sel_registerName("set"));
		}

		if (type == NSEventTypeRightMouseUp)
			objc_msgSend_void(cursor, sel_registerName("set"));

		objc_msgSend_void_id(NSApp, sel_registerName("sendEvent:"), e);
		((void(*)(id, SEL))objc_msgSend)(NSApp, sel_registerName("updateWindows"));
  	
		NSRelease(pool);
	}
}
```
