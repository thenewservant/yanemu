/*
#include <windows.h>

int main()
{
    HDC hdc = GetDC(GetConsoleWindow());
    for (int x = 0; x < 1000; ++x)
        for (int y = 0; y < 256; ++y)
            SetPixel(hdc, x, y, RGB(x*y + x*x  -y *y, x, y));
} */

#include "graphics.h"

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

uint64_t n = 0;
uint8_t ratio = 10;
uint64_t width = 20;
uint64_t height = 20;
uint8_t* bytes2 = (uint8_t*)malloc(5*ratio*ratio*width * height * sizeof(uint8_t) * 3);


VOID onOpening(HDC hdc) {



    /*
    clock_gettime(CLOCK_REALTIME, &t);
    int64_t millitime = t.tv_sec * INT64_C(1000) + t.tv_nsec / 1000000;
    srand(millitime);




    RECT rect = {900, 400, 1000, 500};
    //DrawText(hdc, "Hello, World!", -1, &rect, DT_CENTER);
    char *temps = (char*) malloc(100);
    temps = itoa(millitime, temps,10);
    //DrawText(hdc, temps, -1, &rect, DT_CENTER);

    */
    
    // Create an array of bytes to hold the pixel data, and copy the pixel data into it.


    for (int i = 0; i < ratio*ratio*width * height * 3; i += 3) {
        bytes2[i] = 0; //blue
        bytes2[i + 1] = 255; //green
        bytes2[i + 2] = i;  //red
    }

    // do the same but make a "zoom" of the image, each pixel is a square of 5 pixels
    // an array of pixels appears 5 times bigger

    for (int i = 0; i < ratio*ratio*width * height * 3; i += 3) {
        for (int j = 0; j < ratio*ratio; j++) {
            bytes2[i + j * 3 * ratio * width] = 0; //blue
            bytes2[i + j * 3 * ratio * width + 1] = 255; //green
            bytes2[i + j * 3 * ratio * width + 2] = (127*(i%2));  //red
        }
    }



  

    

    //Do the exact same thing with a 24 bit RGB bitmap


    Bitmap bmp2(ratio*width, ratio*height, ratio * width * 3, PixelFormat24bppRGB, (BYTE*)bytes2);

    


    //Image image(L"draw.bmp");
    Graphics graphics(hdc);
    //graphics.DrawImage(&image, 100,100);
    graphics.DrawImage(&bmp2, 200, 200);
}

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    Pen      pen(Color(255, 0, 0, 255));
    graphics.DrawLine(&pen, 0, 0, 200, 100);

}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;

    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("GettingStarted");

    RegisterClass(&wndClass);

    hWnd = CreateWindow(
        TEXT("GettingStarted"),   // window class name
        TEXT("Getting Started"),  // window caption
        WS_OVERLAPPEDWINDOW,      // window style
        CW_USEDEFAULT,            // initial x position
        CW_USEDEFAULT,            // initial y position
        CW_USEDEFAULT,            // initial x size
        CW_USEDEFAULT,            // initial y size
        NULL,                     // parent window handle
        NULL,                     // window menu handle
        hInstance,                // program instance handle
        NULL);                    // creation parameters

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }




    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}  // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {
    case HSHELL_WINDOWCREATED:

        return 0;


    case WM_PAINT:

        hdc = BeginPaint(hWnd, &ps);

        onOpening(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }


} // WndProc
