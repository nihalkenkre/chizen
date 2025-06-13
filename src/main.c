#include <Windows.h>
#include <Windowsx.h>
#include <ShObjIdl.h>
#include <strsafe.h>

// this is the 'one' source for the implementation to be defined in

#include "optix.hpp"
#include "scene.h"
#include "default_scene.h"
#include "world_scene.h"

#include "renderer.h"
#include "vk_renderer.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define STB_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include <stb/stb_image.h>

#define CGLM_IMPLEMENTATION
#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

#define FILE_OPEN_MENU 10
#define FILE_OPEN_HOT_KEY 11
#define RENDER_BUTTON_ID 301

UINT_PTR r_btn_timer_id = 0;
#define R_BTN_TIMER_ID 201
#define R_BTN_INTERVAL 2
#define RENDER_TIMER_ID 202
#define RENDER_INTERVAL 16

HWND h_scene_wnd = NULL;
HWND h_ctrl_pnl = NULL;
HWND h_render_settings_wnd = NULL;
HWND h_render_output_wnd = NULL;

static float render_aspect_ratio = 1.7778f;
static uint32_t render_height = 720;

HBITMAP h_bitmap = NULL;
uint8_t* pixels = NULL;

scene* s = NULL;
renderer* r = NULL;

static LRESULT CALLBACK RenderOutputWndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    HINSTANCE h_instance = GetModuleHandleA(NULL);

    switch (msg)
    {
    case WM_CLOSE:
        DeleteObject(h_bitmap);
        DestroyWindow(h_wnd);
        h_render_output_wnd = NULL;
        break;

    case WM_CREATE:
    {
        BITMAPINFO bmi = {
            .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
            .bmiHeader.biWidth = (LONG)(render_height * render_aspect_ratio),
            .bmiHeader.biHeight = -(LONG)(render_height),
            .bmiHeader.biPlanes = 1,
            .bmiHeader.biBitCount = 32,
            .bmiHeader.biCompression = BI_RGB,
        };

        HDC wnd_dc = GetDC(h_wnd);
        h_bitmap = CreateDIBSection(wnd_dc, &bmi, DIB_RGB_COLORS, (void**)(&pixels), NULL, 0);

        ReleaseDC(h_wnd, wnd_dc);
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h_wnd, &ps);

        HDC mem_dc = CreateCompatibleDC(hdc);
        SelectObject(mem_dc, h_bitmap);
        BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom, mem_dc, 0, 0, SRCCOPY);
        DeleteDC(mem_dc);

        EndPaint(h_wnd, &ps);
    }
    break;

    default:
        return DefWindowProcA(h_wnd, msg, w_param, l_param);
    }

    return 0;
}

static HWND create_control_panel(const HINSTANCE h_instance)
{
    HMENU h_file_label = CreateMenu();
    HMENU h_file_menu = CreateMenu();
    AppendMenuA(h_file_menu, MF_STRING, FILE_OPEN_MENU, "Open");
    AppendMenuA(h_file_label, MF_POPUP, (UINT_PTR)h_file_menu, "File");

    return CreateWindowA(
        "Chizen",
        "Chizen",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        100,
        h_scene_wnd,
        h_file_label,
        h_instance,
        NULL);
}

static HWND create_scene_wnd(const HINSTANCE h_instance)
{
    RECT wnd_rect = {
        .left = 0,
        .top = 0,
        .right = 1280,
        .bottom = 720,
    };

    UINT dw_style = WS_OVERLAPPED | WS_SIZEBOX;

    AdjustWindowRect(&wnd_rect, dw_style, FALSE);

    return CreateWindowA(
        "Chizen",
        "Chizen",
        WS_OVERLAPPED | WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wnd_rect.right - wnd_rect.left,
        wnd_rect.bottom - wnd_rect.top,
        NULL,
        NULL,
        h_instance,
        NULL);
}

static HWND create_render_settings_wnd(const HINSTANCE h_instance)
{
    return CreateWindowA(
        "Chizen",
        "Render Settings",
        WS_OVERLAPPED,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        100,
        NULL,
        NULL,
        h_instance,
        NULL);
}

static HWND create_render_output_wnd(const HINSTANCE h_instance)
{
    return CreateWindowA(
        "RenderOutput",
        "Render Output",
        WS_OVERLAPPED | WS_SIZEBOX | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        NULL,
        NULL,
        h_instance,
        0);
}

static void open_file(const HWND h_wnd)
{
    IFileDialog* file_open;
    if SUCCEEDED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileDialog, (LPVOID*)&file_open))
    {
        if SUCCEEDED(file_open->lpVtbl->Show(file_open, h_wnd))
        {
            IShellItem* open_file;
            if SUCCEEDED(file_open->lpVtbl->GetResult(file_open, &open_file))
            {
                PWSTR wfile_path;
                if SUCCEEDED(open_file->lpVtbl->GetDisplayName(open_file, SIGDN_FILESYSPATH, &wfile_path))
                {
                    char fp[MAX_PATH];
                    wcstombs(fp, wfile_path, MAX_PATH);

                    s->shutdown();
                    s = malloc(sizeof(scene));
                    world_scene_init(fp, s, r);
                }
                else
                {
                    printf("Could not get file path\n");
                }
            }
            else
            {
                printf("Could not get open file dialog result\n");
            }
        }
        else
        {
            printf("Could not show file open dialog\n");
        }
    }
    else
    {
        printf("Could not create open dialog instance\n");
    }
}

static LRESULT CALLBACK WindowProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_CREATE:
    {
        CREATESTRUCTA* create_info = (CREATESTRUCTA*)(l_param);
        if (strcmp(create_info->lpszName, "Render Settings") != 0)
            break;

        CreateWindowA("BUTTON", "Render", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 10, 75, 30, h_wnd, NULL, GetModuleHandleA(NULL), 0);
    }
    break;

    case WM_COMMAND:
        switch (w_param)
        {
        case FILE_OPEN_MENU:
            open_file(h_wnd);
            break;

        case BN_CLICKED:
            if (h_wnd == h_render_settings_wnd && h_render_output_wnd == NULL)
            {
                h_render_output_wnd = create_render_output_wnd(GetModuleHandleA(NULL));
                ShowWindow(h_render_output_wnd, SW_SHOW);
            }

            optix_render((uint32_t)(render_height * render_aspect_ratio), render_height, pixels);
            //r->render_offline(render_width, render_height, pixels);
            InvalidateRect(h_render_output_wnd, NULL, TRUE);
            UpdateWindow(h_render_output_wnd);

            break;

        default:
            break;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps = { 0 };
        BeginPaint(h_wnd, &ps);
        s->render(r);
        EndPaint(h_wnd, &ps);
    }
    break;

    case WM_MOUSEMOVE:
    {
        POINT p = {};
        p.x = GET_X_LPARAM(l_param);
        p.y = GET_Y_LPARAM(l_param);

        s->handle_mouse_move(p);
    }
    break;

    case WM_KEYUP:
        break;

    case WM_LBUTTONDOWN:
        break;

    case WM_SIZE:
        if (r != NULL)
        {
            r->resize(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
        }
        break;

    case WM_TIMER:
        switch (w_param)
        {
        case RENDER_TIMER_ID:
            InvalidateRect(h_wnd, NULL, FALSE);
            break;
        }
        break;

    case WM_HOTKEY:
        switch (w_param)
        {
        case FILE_OPEN_HOT_KEY:
            open_file(h_wnd);
            break;

        default:
            break;
        }

    default:
        return DefWindowProcA(h_wnd, msg, w_param, l_param);
    }

    return 0;
}

int main(int argc, char** argv)
{
    const HINSTANCE h_instance = GetModuleHandleA(NULL);

    HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE h_stderr = GetStdHandle(STD_ERROR_HANDLE);

    WNDCLASSA wc = {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WindowProc,
        .hInstance = h_instance,
        .hCursor = LoadCursorA(h_instance, MAKEINTRESOURCEA(32512)),
        .lpszClassName = "Chizen",
    };

    if (!RegisterClassA(&wc))
    {
        DWORD err = GetLastError();
        printf("RegisterClassA failed with %d\n", err);
        return err;
    }

    if (FAILED(CoInitialize(NULL)))
    {
        DWORD err = GetLastError();
        printf("CoInitialize failed with %d\n", err);
        return err;
    }

    h_ctrl_pnl = create_control_panel(h_instance);
    h_scene_wnd = create_scene_wnd(h_instance);
    h_render_settings_wnd = create_render_settings_wnd(h_instance);

    ShowWindow(h_scene_wnd, SW_SHOW);
    ShowWindow(h_ctrl_pnl, SW_SHOW);
    ShowWindow(h_render_settings_wnd, SW_SHOW);

    s = malloc(sizeof(scene));
    r = malloc(sizeof(renderer));

    default_scene_init(s);
    vk_renderer_init(r, h_scene_wnd);

    UpdateWindow(h_scene_wnd);
    UpdateWindow(h_render_settings_wnd);
    UpdateWindow(h_ctrl_pnl);

    WNDCLASSA render_output_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = RenderOutputWndProc,
        .hInstance = h_instance,
        .hCursor = LoadCursorA(h_instance, MAKEINTRESOURCEA(32512)),
        .lpszClassName = "RenderOutput",
    };

    if (!RegisterClassA(&render_output_class))
    {
        DWORD err = GetLastError();
        printf("RegisterClassA failed with %d\n", err);
        return err;
    }

    RegisterHotKey(h_scene_wnd, FILE_OPEN_HOT_KEY, MOD_CONTROL, 79);
    MSG msg = { 0 };
    UINT_PTR t = SetTimer(h_scene_wnd, RENDER_TIMER_ID, RENDER_INTERVAL, NULL);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        DispatchMessageA(&msg);
    }

    CoUninitialize();
    KillTimer(h_scene_wnd, t);

    s->shutdown();
    r->shutdown();

    free(r);
    free(s);

    return 0;
}