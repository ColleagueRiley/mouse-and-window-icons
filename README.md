
## window icons 

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
    if (channels == 3)
        *target++ = ((icon[i * 3 + 0]) << 16) |
        ((icon[i * 3 + 1]) << 8) |
        ((icon[i * 3 + 2]) << 0) |
        (0xFF << 24);

    else if (channels == 4)
        *target++ = ((icon[i * 4 + 0]) << 16) |
        ((icon[i * 4 + 1]) << 8) |
        ((icon[i * 4 + 2]) << 0) |
        ((icon[i * 4 + 3]) << 24);
}
```

```c
const Atom NET_WM_ICON = XInternAtom((Display*) display, "_NET_WM_ICON", False);

XChangeProperty((Display*) display, (Window) window,
    NET_WM_ICON,
    6, 32,
    PropModeReplace,
    (unsigned char*) X11Icon,
    longCount);
```

```c
free(X11Icon);
XFlush((Display*) display);
```

### win32

```c
HICON handle = loadHandleImage(src, a, TRUE);
```

```c
SetClassLongPtrA(window, GCLP_HICON, (LPARAM) handle);
```

```c
DestroyIcon(handle);
```

### cocoa

Make a bitmap representation, then copy the loaded image into it.
```c
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, width, height, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, width * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), data, width * height * channels);
```

Add the representation.
```c
void* dock_image = NSImage_initWithSize((NSSize){width, height});
NSImage_addRepresentation(dock_image, (void*) representation);
```

Finally, set the dock image to it.

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
    unsigned char alpha = 0xFF;
    if (channels == 4)
        alpha = source[3];

    *target = (alpha << 24) | (((source[0] * alpha) / 255) << 16) | (((source[1] * alpha) / 255) << 8) | (((source[2] * alpha) / 255) << 0);
}
```

```c
Cursor cursor = XcursorImageLoadCursor((Display*) display, native);
```

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

```c
XFreeCursor((Display*) display, (Cursor) cursor);
XcursorImageDestroy(native);
```

### win32

```c
HICON loadHandleImage(unsigned char* src, RGFW_area a, BOOL icon) {
    unsigned int i;
    HDC dc;
    HICON handle;
    HBITMAP color, mask;
    BITMAPV5HEADER bi;
    ICONINFO ii;
    unsigned char* target = NULL;
    unsigned char* source = src;

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

```c
    dc = GetDC(NULL);
    color = CreateDIBSection(dc,
        (BITMAPINFO*) &bi,
        DIB_RGB_COLORS,
        (void**) &target,
        NULL,
        (DWORD) 0);
    ReleaseDC(NULL, dc);
```

```c
    mask = CreateBitmap(width, height, 1, 1, NULL);
```

```c
    for (i = 0; i < width * height; i++) {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }
```

```c
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = mask;
    ii.hbmColor = color;
```

```c
    handle = CreateIconIndirect(&ii);
```

```c
    DeleteObject(color);
    DeleteObject(mask);

    return handle;
}
```

```c
HCURSOR cursor = (HCURSOR) loadHandleImage(image, a, FALSE);
```

```c
SetClassLongPtrA(window, GCLP_HCURSOR, (LPARAM) cursor);
SetCursor(cursor);
```

```c
DestroyCursor(cursor);
```

### cocoa

Make a bitmap representation, then copy the loaded image into it.
```c
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, width, height, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, width * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), image, width * height * channels);
```

Add the representation.
```c
void* cursor_image = NSImage_initWithSize((NSSize){width, height});
NSImage_addRepresentation(cursor_image, representation);
```

Finally, set the cursor image.

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
```c
Cursor cursor = XCreateFontCursor((Display*) display, mouse);
```

```c
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);
```

```c
XFreeCursor((Display*) display, (Cursor) cursor);
```

### win32

```c
char* icon = MAKEINTRESOURCEA(mouse);
```

```c
SetClassLongPtrA(window, GCLP_HCURSOR, (LPARAM) LoadCursorA(NULL, icon));
SetCursor(LoadCursorA(NULL, icon));
```

### cocoa

```c
void* mouse = NSCursor_arrowStr(mouseStr);
```

```c
objc_msgSend_void(mouse, sel_registerName("set"));
```	
