
## window icons 

### X11
```c
i32 longCount = 2 + a.w * a.h;

u64* X11Icon = (u64*) malloc(longCount * sizeof(u64));
u64* target = X11Icon;

*target++ = a.w;
*target++ = a.h;

u32 i;

for (i = 0; i < a.w * a.h; i++) {
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

static Atom NET_WM_ICON = 0;
if (NET_WM_ICON == 0)
    NET_WM_ICON = XInternAtom((Display*) display, "_NET_WM_ICON", False);

XChangeProperty((Display*) display, (Window) window,
    NET_WM_ICON,
    6, 32,
    PropModeReplace,
    (u8*) X11Icon,
    longCount);

free(X11Icon);

XFlush((Display*) display);
```

### win32

```c
HICON handle = loadHandleImage(src, a, TRUE);

SetClassLongPtrA(window, GCLP_HICON, (LPARAM) handle);

DestroyIcon(handle);
```

### cocoa

```c
/* code by EimaMei  */
// Make a bitmap representation, then copy the loaded image into it.
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, area.w, area.h, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, area.w * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), data, area.w * area.h * channels);

// Add ze representation.
void* dock_image = NSImage_initWithSize((NSSize){area.w, area.h});
NSImage_addRepresentation(dock_image, (void*) representation);

// Finally, set the dock image to it.
objc_msgSend_void_id(NSApp, sel_registerName("setApplicationIconImage:"), dock_image);
// Free the garbage.
release(dock_image);
release(representation);
```



## mouse icon image

### X11
```c
XcursorImage* native = XcursorImageCreate(a.w, a.h);
native->xhot = 0;
native->yhot = 0;

u8* source = (u8*) image;
XcursorPixel* target = native->pixels;

u32 i;
for (i = 0; i < a.w * a.h; i++, target++, source += 4) {
    u8 alpha = 0xFF;
    if (channels == 4)
        alpha = source[3];

    *target = (alpha << 24) | (((source[0] * alpha) / 255) << 16) | (((source[1] * alpha) / 255) << 8) | (((source[2] * alpha) / 255) << 0);
}

Cursor cursor = XcursorImageLoadCursor((Display*) display, native);
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);

XFreeCursor((Display*) display, (Cursor) cursor);
XcursorImageDestroy(native);
```

### win32

```c
HICON loadHandleImage(u8* src, RGFW_area a, BOOL icon) {
    u32 i;
    HDC dc;
    HICON handle;
    HBITMAP color, mask;
    BITMAPV5HEADER bi;
    ICONINFO ii;
    u8* target = NULL;
    u8* source = src;

    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = a.w;
    bi.bV5Height = -((LONG) a.h);
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

    mask = CreateBitmap(a.w, a.h, 1, 1, NULL);

    for (i = 0; i < a.w * a.h; i++) {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }

    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = icon;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = mask;
    ii.hbmColor = color;

    handle = CreateIconIndirect(&ii);

    DeleteObject(color);
    DeleteObject(mask);

    return handle;
}
```

```c
HCURSOR cursor = (HCURSOR) loadHandleImage(image, a, FALSE);
SetClassLongPtrA(window, GCLP_HCURSOR, (LPARAM) cursor);
SetCursor(cursor);
DestroyCursor(cursor);
```

### cocoa

```c
if (image == NULL) {
    objc_msgSend_void(NSCursor_arrowStr("arrowCursor"), sel_registerName("set"));
    return;
}

/* NOTE(EimaMei): Code by yours truly. */
// Make a bitmap representation, then copy the loaded image into it.
void* representation = NSBitmapImageRep_initWithBitmapData(NULL, a.w, a.h, 8, channels, (channels == 4), false, "NSCalibratedRGBColorSpace", 1 << 1, a.w * channels, 8 * channels);
memcpy(NSBitmapImageRep_bitmapData(representation), image, a.w * a.h * channels);

// Add ze representation.
void* cursor_image = NSImage_initWithSize((NSSize){a.w, a.h});
NSImage_addRepresentation(cursor_image, representation);

// Finally, set the cursor image.
void* cursor = NSCursor_initWithImage(cursor_image, (NSPoint){0.0, 0.0});

objc_msgSend_void(cursor, sel_registerName("set"));

// Free the garbage.
release(cursor_image);
release(representation);
```

## standard mouse icons
### X11
```c
Cursor cursor = XCreateFontCursor((Display*) display, mouse);
XDefineCursor((Display*) display, (Window) window, (Cursor) cursor);

XFreeCursor((Display*) display, (Cursor) cursor);
```

### win32

```c
char* icon = MAKEINTRESOURCEA(mouse);

SetClassLongPtrA(window, GCLP_HCURSOR, (LPARAM) LoadCursorA(NULL, icon));
SetCursor(LoadCursorA(NULL, icon));
```

### cocoa

```
if (stdMouses > ((sizeof(RGFW_mouseIconSrc)) / (sizeof(char*))))
    return;

char* mouseStr = RGFW_mouseIconSrc[stdMouses];
void* mouse = NSCursor_arrowStr(mouseStr);

if (mouse == NULL)
    return;

RGFW_UNUSED(win);
CGDisplayShowCursor(kCGDirectMainDisplay);
objc_msgSend_void(mouse, sel_registerName("set"));
```	
