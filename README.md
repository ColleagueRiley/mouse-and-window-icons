# RGFW Under the Hood: Mouse and Window Icons
## Introduction
## Overview
1) window icons
2) mouse image icons
3) default mouse icons

## Window icons

### X11
First, allocate an array to send to X11. Our RGB icon must be converted to match X11's BGR format.

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

Next, we'll need to load an X11 atom with  [`XInternAtom`](https://www.x.org/releases/X11R7.5/doc/man/man3/XInternAtom.3.html).

We're loading the `NET_WM_ICON` atom, this atom is used for the Window's Window Manager icon property. 

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

Make sure to use [`XFlush`](https://www.x.org/releases/X11R7.5/doc/man/man3/XSync.3.html) to update the X11 server on the change.

```c
free(X11Icon);
XFlush((Display*) display);
```

### win32

I'll start by creating a `loadHandleImage` function. This function will be used for mouse and window icons to load a bitmap image into a win32 icon handle.

```c
HICON loadHandleImage(unsigned char* src, unsigned int width, unsigned int height, BOOL icon) {
``` 

We'll start by creating a [`BITMAPV5HEADER`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header) stucture with the proper format to match how our data is stored.

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

Next we'll create color section for the icon.

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

Now we can crete a [`ICONINFO`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-iconinfo) structure for updating the window icon.

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

Finally we can free the object color, mask data and return the icon handle.

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

Then [`XDefineCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XDefineCursor.3.html) can be used to set the new cursor.

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

Finally, we can free the cursor and cursor image with [`XFreeCursor`](https://linux.die.net/man/3/xfreecursor) and [`XcursorImageDestroy`](https://linux.die.net/man/3/xcursorimagedestroy).

```c
XFreeCursor((Display*) display, (Cursor) cursor);
XcursorImageDestroy(native);
```

### win32

We'll use the function I defined earlier for creating a cursor icon handle. 

```c
HCURSOR cursor = (HCURSOR) loadHandleImage(image, width, height, FALSE);
```

Then we can use  [`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra) and [`SetCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor) to change the cursor. 

```c
SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) cursor);
SetCursor(cursor);
```

Make sure to free the cursor via [`DestroyCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroycursor).

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

Make sure to free the cursor data with [`XFreeCursor`](https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreeCursor.html).

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

First, find the cursor object you want to use.

For example, [`arrowCursor`](https://developer.apple.com/documentation/appkit/nscursor/1527160-arrowcursor?changes=_1&language=objc)

```c 
void* mouse = objc_msgSend_id(objc_getClass("NSCursor"), sel_registerName("arrowCursor"));
```

Then you can use [`set`](https://developer.apple.com/documentation/appkit/nscursor/1526148-set) to set it as the current cursor.

```c
objc_msgSend_void(mouse, sel_registerName("set"));
```	
