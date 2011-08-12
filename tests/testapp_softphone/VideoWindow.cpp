/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "VideoWindow.h"
#include <iostream>
#include <sstream>
#include <fstream>


using namespace std;

#ifdef WIN32
#include <TCHAR.H>
#include <strsafe.h>
#define WNDCLASSNAME L"TestWindowClass"
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

void ErrorExit(char * lpszFunction, DWORD MESSAGE) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (wchar_t *) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((wchar_t *)lpMsgBuf) + lstrlen((wchar_t *)lpszFunction) + 40));

    StringCchPrintf((STRSAFE_LPWSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf),
        L"%s failed with error %d: %s", 
        (wchar_t *)lpszFunction, dw, lpMsgBuf);

    MessageBox(NULL, (wchar_t *)lpDisplayBuf, L"Error", MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
#endif


VideoWindow::VideoWindow()
{
    _rWindow = NULL;
    _x = _y = 250;

    //cif video format 288 x 352
    _Width = 288;
    _Height = 352;
    _wndType = 0;
    _wndType = WT_NORMAL | WT_FRAME;
}

VideoWindow::~VideoWindow()
{
    if(NULL != _rWindow)
#ifdef WIN32
        DestroyWindow((HWND)_rWindow);
#else
        XDestroyWindow(_display, _window);
#endif
}

void VideoWindow::SetWindowDimensions(int x, int y, int width, int height)
{
    if (x != -1)
        _x = x;
    if (y != -1)
        _y = y;
    if (width != -1)
        _Width = width;
    if (height != -1)
        _Height = height;
}

void VideoWindow::SetWindowStyle(unsigned char wndType)
{
    _wndType = wndType;
}

void VideoWindow::SetParentWindow(void* pWnd)
{
    _parentWindow = pWnd;
}

//int VideoWindow::InitWindow(const char * windowName)
int VideoWindow::InitWindow()
{
#ifdef WIN32
	wchar_t * windowName = L"Test Window";
    _hInstance = GetModuleHandle(NULL);

    DWORD dwStyle = 0;
    if ((_wndType & WT_FRAME) == WT_FRAME)
        dwStyle = WS_CAPTION|WS_SIZEBOX|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SYSMENU|WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER;
    else  // need WS_CHILD for child window
        dwStyle = WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER;

    if ((_wndType & WT_NORMAL) == WT_NORMAL)
    {

        if (!InitWindowClass(_hInstance))
        {
            // InitWindowClass displays any errors
            return -1;
        }

        _rWindow = CreateWindowEx(0, WNDCLASSNAME, windowName, dwStyle,
                                  _x, _y, _Width, _Height,
                                  NULL, NULL, NULL, NULL);

        if(NULL == _rWindow)
        {
            ErrorExit("Error Creating Window",0);
            return -1;
        }

        DrawWindow(_rWindow);

        //MessagePump();
    }
    if ((_wndType & WT_CHILD) == WT_CHILD)
    {
        // create window that is a child of a pre-set parent window, like a browser HWND
        // create window that does not have a message pump
        _rWindow = CreateWindowEx(0, L"#32769", windowName, dwStyle,
                                  _x, _y, _Width, _Height,
                                  NULL, NULL, NULL, NULL);

        if(NULL == _rWindow)
        {
            ErrorExit("Error Creating Window",0);
            return -1;
        }

        DrawWindow(_rWindow);
    }
    if ((_wndType & WT_NOWINPROC) == WT_NOWINPROC)
    {
        // create window that does not have a message pump
        _rWindow = CreateWindowEx(0, L"#32769", windowName, dwStyle,
                                  _x, _y, _Width, _Height,
                                  (HWND)_parentWindow, NULL, NULL, NULL);

        if(NULL == _rWindow)
        {
            ErrorExit("Error Creating Window",0);
            return -1;
        }

        DrawWindow(_rWindow);
    }
#else
    _display = XOpenDisplay(NULL);
    _screen = DefaultScreen(_display);
    _rootwindow = RootWindow(_display,_screen);
    _window = XCreateSimpleWindow(_display, _rootwindow, 0, 0, 500, 500, 1, 0, 0);
    XMapWindow(_display, _window);
    XFlush(_display);
    _rWindow = (void *)_window; // not sure why we have 2 seperate values (NDM)
#endif

    return 0;
}

void VideoWindow::DrawWindow(void* rWindow)
{
#ifdef WIN32
    if(false == ::ShowWindow((HWND)_rWindow, SW_SHOWNORMAL))
    {
    }

    if(false == UpdateWindow((HWND)_rWindow))
    {
    }

    InvalidateRect((HWND)_rWindow,NULL,TRUE);
    UpdateWindow((HWND)_rWindow);
#endif
}

void VideoWindow::MoveWindow(int x, int y, int width, int height)
{
#ifdef WIN32
    SetWindowDimensions(x, y, width, height);
    ::MoveWindow((HWND)_rWindow, x, y, width, height, true);
    ::SetWindowPos((HWND)_rWindow, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
    ::ShowWindow((HWND)_rWindow, SW_SHOWNORMAL);

    InvalidateRect((HWND)_rWindow,NULL,TRUE);
    UpdateWindow((HWND)_rWindow);
#endif
}

void VideoWindow::CloseWindow()
{
#ifdef WIN32
    //::ShowWindow((HWND)_rWindow, SW_HIDE);
    DestroyWindow((HWND)_rWindow);
    UnRegisterWindowClass(_hInstance);
    _rWindow = NULL;
#else
    XDestroyWindow(_display, _window);
#endif
}


bool VideoWindow::isOpen()
{
    return true ? _rWindow == NULL : false;
}

void VideoWindow::ShowWindow(bool hide)
{
#ifdef WIN32
    if (hide)
        ::ShowWindow((HWND)_rWindow, SW_HIDE);
    else
        ::ShowWindow((HWND)_rWindow, SW_SHOWNORMAL | SW_SHOW);
#endif
}

void* VideoWindow::GetRenderingWindowHandle()
{
    return _rWindow;
}

#ifdef WIN32
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps; 
    HDC hdc;

    switch(uMsg )
    {
    //call function to create all controls.
    case WM_CREATE:
        break;
    case WM_PAINT: 
        hdc = BeginPaint(hWnd, &ps); 
        EndPaint(hWnd, &ps); 
        break;

    //used to close the window
    //case WM_CLOSE:
    //    DestroyWindow(hWnd);
    //    break;

    case WM_DESTROY:
        PostQuitMessage(0); // posts the message wm_quit to quit the window.
        break;

    default:
        return( DefWindowProc( hWnd, uMsg, wParam, lParam ));
    }
    return 0;
}

bool VideoWindow::InitWindowClass( HINSTANCE hInstanceExe )
{
    WNDCLASSEX wndClass;

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.hInstance = hInstanceExe;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hIcon = 0;
    wndClass.hbrBackground = CreateSolidBrush(RGB(192,192,192));
    wndClass.hCursor = LoadCursor(0, IDC_ARROW);
    wndClass.lpszClassName = WNDCLASSNAME;
    wndClass.lpszMenuName = NULL;
    wndClass.hIconSm = wndClass.hIcon;

    if (!RegisterClassEx(&wndClass))
    {
        return FALSE;
    }
    return TRUE;
}

bool VideoWindow::UnRegisterWindowClass(HINSTANCE hInstanceExe)
{
    if (! UnregisterClass(WNDCLASSNAME, hInstanceExe))
        return false;

    return true;
}

void VideoWindow::MessagePump()
{
    MSG msg;
    int retVal;

    while((retVal = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (retVal == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
#endif
