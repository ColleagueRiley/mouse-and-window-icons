# RGFW Under the Hood: Mouse and Window Icons
## Introduction
## Overview
1) window icons
2) mouse image icons
3) default mouse icons

## Window icons

### X11
```c
int longCount = 2 + width * height;

unsigned long* X11Icon = (unsigned long*) malloc(longCount * sizeof(unsigned long));
unsigned long* target = X11Icon;

*target++ = width;
*target++ = height;
```

```c
unsigned int i;

for (i = 0; i < width * height; i++) {
    *target++ = ((icon[i * 4 + 2])) // b
                ((icon[i * 4 + 1]) << 8) // g
                ((icon[i * 4 + 0]) << 16) // r
                ((icon[i * 4 + 3]) << 24); // a 
}
```

[`XInternAtom`](https://www.x.org/releases/X11R7.5/doc/man/man3/XInternAtom.3.html)

[`XChangeProperty`](https://linux.die.net/man/3/xchangeproperty)

```c
const Atom NET_WM_ICON = XInternAtom((Display*) display, "_NET_WM_ICON", False);

XChangeProperty((Display*) display, (Window) window,
    NET_WM_ICON,
    6, 32,
    PropModeReplace,
    (unsigned char*) X11Icon,
    longCount);
```

[`XFlush`](https://www.x.org/releases/X11R7.5/doc/man/man3/XSync.3.html)

```c
free(X11Icon);
XFlush((Display*) display);
```

### win32

```c
HICON loadHandleImage(unsigned char* src, unsigned int width, unsigned int height, BOOL icon) {
``` 

[`BITMAPV5HEADER`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header) stucture

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

[`GetDC`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc)

[`CreateDIBSection`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection)

[`ReleaseDC`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-releasedc)

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

[`CreateBitmap`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createbitmap)

```c
    HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);
```

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

[`ICONINFO`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-iconinfo)

```c
    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = mask;
    ii.hbmColor = color;
```

[`CreateIconIndirect`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createiconindirect)

```c
    HICON handle = CreateIconIndirect(&ii);
```

[`DeleteObject`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-deleteobject)

```c
    DeleteObject(color);
    DeleteObject(mask);

    return handle;
}
```

Now you can create a handle via the function we created. 

```c
HICON handle = loadHandleImage(buffer, width, height, TRUE);
```

[`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra)

```c
SetClassLongPtrA(hwnd, GCLP_HICON, (LPARAM) handle);
```

[`DestroyIcon`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroyicon)

```c
DestroyIcon(handle);
```

### cocoa

Make a bitmap representation, then copy the loaded image into it.

[`initWithBitmapData`](https://developer.apple.com/documentation/coreimage/ciimage/1437857-initwithbitmapdata)

[`bitmapData`](https://developer.apple.com/documentation/appkit/nsbitmapimagerep/1395421-bitmapdata)

```c
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, width, height, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, width * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), data, width * height * channels);
```

Add the representation.

[`NSImage_init`](https://developer.apple.com/documentation/appkit/nsimage/1519860-init)

[`addRepresentation`](https://developer.apple.com/documentation/appkit/nsimage/1519911-addrepresentation)

```c
void* dock_image = NSImage_initWithSize((NSSize){width, height});
NSImage_addRepresentation(dock_image, (void*) representation);
```

Finally, set the dock image to it.

[`setApplicationIconImage`](https://developer.apple.com/documentation/appkit/nsapplication/1428744-applicationiconimage)

```c
objc_msgSend_void_id(NSApp, sel_registerName("setApplicationIconImage:"), dock_image);
```

Free the garbage.
```c
release(dock_image);
release(representation);
```

## mouse icon image

### X11
[`XcursorImageCreate`](https://linux.die.net/man/3/xcursorimagecreate)
```c
XcursorImage* native = XcursorImageCreate(width, height);
native->xhot = 0;
native->yhot = 0;
```

```c
unsigned char* source = (unsigned char*) image;
XcursorPixel* target = native->pixels;
```

```c
unsigned int i;
for (i = 0; i < width * height; i++, target++, source += 4) {
    unsigned char alpha = source[3];

    *target = (alpha << 24) | (((source[0] * alpha) / 255) << 16) | (((source[1] * alpha) / 255) << 8) | (((source[2] * alpha) / 255) << 0);
}
```

[`XcursorImageLoadCursor`](https://man.archlinux.org/man/XcursorImageLoadCursor.3.en)

```c
Cursor cursor = XcursorImageLoadCursor((Display*) display, native);
```

[`XDefineCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XDefineCursor.3.html)

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

[`XFreeCursor`](https://linux.die.net/man/3/xfreecursor)
[`XcursorImageDestroy`](https://linux.die.net/man/3/xcursorimagedestroy)

```c
XFreeCursor((Display*) display, (Cursor) cursor);
XcursorImageDestroy(native);
```

### win32

```c
HCURSOR cursor = (HCURSOR) loadHandleImage(image, width, height, FALSE);
```

[`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra)
[`SetCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor)

```c
SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) cursor);
SetCursor(cursor);
```

[`DestroyCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroycursor)

```c
DestroyCursor(cursor);
```

### cocoa

Make a bitmap representation, then copy the loaded image into it.

[`initWithBitmapData`](https://developer.apple.com/documentation/coreimage/ciimage/1437857-initwithbitmapdata)

[`bitmapData`](https://developer.apple.com/documentation/appkit/nsbitmapimagerep/1395421-bitmapdata)

```c
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, width, height, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, width * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), image, width * height * channels);
```

Add the representation.

[`NSImage_init`](https://developer.apple.com/documentation/appkit/nsimage/1519860-init)

[`addRepresentation`](https://developer.apple.com/documentation/appkit/nsimage/1519911-addrepresentation)

```c
void* cursor_image = NSImage_initWithSize((NSSize){width, height});
NSImage_addRepresentation(cursor_image, representation);
```

Finally, set the cursor image.

[`initWithImage`](https://developer.apple.com/documentation/uikit/uiimageview/1621062-initwithimage)

[`set`](https://developer.apple.com/documentation/appkit/nscursor/1526148-set)

```c
void* cursor = NSCursor_initWithImage(cursor_image, (NSPoint){0.0, 0.0});

objc_msgSend_void(cursor, sel_registerName("set"));
```

Free the garbage.

```c
release(cursor_image);
release(representation);
```

## standard mouse icons
### X11

[`XCreateFontCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XCreateFontCursor.3.html)

```c
Cursor cursor = XCreateFontCursor((Display*) display, mouse);
```

[`XDefineCursor`](https://www.x.org/releases/X11R7.5/doc/man/man3/XDefineCursor.3.html)

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

[`XFreeCursor`](https://tronche.com/gui/x/xlib/pixmap-and-cursor/XFreeCursor.html)

```c
XFreeCursor((Display*) display, (Cursor) cursor);
```

### win32

[`MAKEINTRESOURCEA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcea)

```c
char* icon = MAKEINTRESOURCEA(mouse);
```

[`SetClassLongPtrA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclasslongptra)
[`SetCursor`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor)
[`LoadCursorA`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadcursora)

```c
SetClassLongPtrA(hwnd, GCLP_HCURSOR, (LPARAM) LoadCursorA(NULL, icon));
SetCursor(LoadCursorA(NULL, icon));
```

### cocoa

for example [`arrowCursor`](https://developer.apple.com/documentation/appkit/nscursor/1527160-arrowcursor?changes=_1&language=objc)

```c 
void* mouse = objc_msgSend_id(objc_getClass("NSCursor"), sel_registerName("arrowCursor"));
```

[`set`](https://developer.apple.com/documentation/appkit/nscursor/1526148-set)

```c
objc_msgSend_void(mouse, sel_registerName("set"));
```	
