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
