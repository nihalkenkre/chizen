#include <Windows.h>
#include <Windowsx.h>
#include <ShObjIdl.h>

#include <iostream>
#include <memory>
#include <string>

#include "scene.hpp"
#include "default_scene.hpp"
#include "world_scene.hpp"

#define FILE_MENU_OPEN 10

bool pause_h_wnd_msg_queue = false;

HWND h_scene_wnd = nullptr;
HWND h_ctrl_pnl = nullptr;

std::unique_ptr<scene> s;
std::unique_ptr<renderer> r;

HWND create_control_panel(const HINSTANCE h_instance)
{
    HWND hCtrlPnl = CreateWindowA(
        "Chizen",
        "Chizen",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        100,
        nullptr,
        nullptr,
        h_instance,
        nullptr);

    HMENU h_file_label = CreateMenu();
    HMENU h_file_menu = CreateMenu();
    AppendMenuA(h_file_menu, MF_STRING, FILE_MENU_OPEN, "Open");
    AppendMenuA(h_file_label, MF_POPUP, (UINT_PTR)h_file_menu, "File");
    SetMenu(hCtrlPnl, h_file_label);

    return hCtrlPnl;
}

HWND create_wnd(const HINSTANCE h_instance)
{
    return CreateWindowA(
        "Chizen",
        "Chizen",
        WS_OVERLAPPED | WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        h_instance,
        nullptr);
}

void open_file(const HWND h_wnd)
{
    IFileDialog* file_open;
    if SUCCEEDED (CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&file_open)))
    {
        if SUCCEEDED (file_open->Show(h_wnd))
        {
            IShellItem* open_file;
            if SUCCEEDED (file_open->GetResult(&open_file))
            {
                PWSTR wfile_path;
                if SUCCEEDED (open_file->GetDisplayName(SIGDN_FILESYSPATH, &wfile_path))
                {
                    char file_path[MAX_PATH];
                    wcstombs(file_path, wfile_path, MAX_PATH);

                    r.reset(nullptr);
                    r = std::make_unique<dx12_renderer>(h_scene_wnd, file_path);

                    s.reset(nullptr);
                    s = std::make_unique<world_scene>(file_path);
                }
                else
                {
                    std::cout << "Could not get file path\n";
                }
            }
            else
            {
                std::cout << "Could not get open file dialog result\n";
            }
        }
        else
        {
            std::cout << "Could not show file open dialog\n";
        }
    }
    else
    {
        std::cout << "Could not create open dialog instance\n";
    }
}

LRESULT CALLBACK WindowProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    POINT p = {};

    switch (msg)
    {
    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_COMMAND:
        switch (w_param)
        {
        case FILE_MENU_OPEN:

            pause_h_wnd_msg_queue = true;
            open_file(h_wnd);
            pause_h_wnd_msg_queue = false;

            break;
        default:
            break;
        }

        break;

    case WM_PAINT:
        s->render(r.get());
        break;

    case WM_MOUSEMOVE:
        p = {
            .x = GET_X_LPARAM(l_param),
            .y = GET_Y_LPARAM(l_param),
        };
        s->handle_mouse_move(p);
        break;

    case WM_SIZE:
        if (r.get() != nullptr && r->is_inited)
        {
            r->resize(LOWORD(l_param), HIWORD(l_param));
        }
        break;

    default:
        return DefWindowProcA(h_wnd, msg, w_param, l_param);
    }

    return 0;
}

int main(int argc, char **argv)
{
    const HINSTANCE h_instance = GetModuleHandleA(nullptr);

    std::cout << "Hi\n";

    WNDCLASSA wc = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = h_instance,
        .hCursor = LoadCursorA(h_instance, MAKEINTRESOURCEA(32512)),
        .lpszClassName = "Chizen",
    };

    if (!RegisterClassA(&wc))
    {
        return -1;
    }

    if FAILED (CoInitialize(nullptr))
    {
        printf("ERR: Could not initialize COM\n");
        return -1;
    }

    h_ctrl_pnl = create_control_panel(h_instance);
    h_scene_wnd = create_wnd(h_instance);

    ShowWindow(h_scene_wnd, 1);
    ShowWindow(h_ctrl_pnl, 1);

    r = std::make_unique<dx12_renderer>(h_scene_wnd, "");
    s = std::make_unique<default_scene>();

    MSG msg = {0};

    // while (TRUE)
    // {
    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY)
        {
            break;
        }

        if (!pause_h_wnd_msg_queue)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    // }

    CoUninitialize();

    std::cout << "Bye\n";

    return 0;
}