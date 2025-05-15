#include <Windows.h>
#include <Windowsx.h>
#include <ShObjIdl.h>

#include <iostream>
#include <memory>
#include <string>

#include "scene.hpp"
#include "default_scene.hpp"
#include "world_scene.hpp"

#include "vk_renderer.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLM_IMPLEMENTATION
#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include <stb/stb_image.h>

#include <wrl.h>

using Microsoft::WRL::ComPtr;

/*
extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 615;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
*/

#define FILE_MENU_OPEN 10
#define DX_12_RADIO_BTN 101
#define VULKAN_RADIO_BTN 102

UINT_PTR r_btn_timer_id = 0;
#define R_BTN_TIMER_ID 201
#define R_BTN_INTERVAL 16
#define RENDER_TIMER_ID 202
#define RENDER_INTERVAL 16

HWND h_scene_wnd = nullptr;
HWND h_ctrl_pnl = nullptr;
HWND h_rndrr_wnd = nullptr;

std::string file_path;
std::unique_ptr<scene> s;
std::unique_ptr<renderer> r;

RENDERING_API r_api = RENDERING_API::VULKAN;

static HWND create_control_panel(const HINSTANCE h_instance)
{
    HMENU h_file_label = CreateMenu();
    HMENU h_file_menu = CreateMenu();
    AppendMenuA(h_file_menu, MF_STRING, FILE_MENU_OPEN, "Open");
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
        nullptr);
}

static HWND create_wnd(const HINSTANCE h_instance)
{
    RECT wnd_rect = {};
    wnd_rect.left = 0;
    wnd_rect.top = 0;
    wnd_rect.right = 1280;
    wnd_rect.bottom = 720;

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
        nullptr,
        nullptr,
        h_instance,
        nullptr);
}

/*
static LRESULT CALLBACK RendererWndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    HINSTANCE h_instance = GetModuleHandleA(nullptr);
    HWND dx12 = nullptr;
    HWND vk = nullptr;

    switch (msg)
    {
    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CREATE:
        vk = CreateWindowA("BUTTON", "Vulkan", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 10, 10, 100, 30, h_wnd, reinterpret_cast<HMENU>(VULKAN_RADIO_BTN), h_instance, nullptr);
        dx12 = CreateWindowA("BUTTON", "DX 12", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 10, 40, 100, 30, h_wnd, reinterpret_cast<HMENU>(DX_12_RADIO_BTN), h_instance, nullptr);

        Button_SetCheck(vk, 1);
        break;

    case WM_COMMAND:
        switch (w_param)
        {
        case DX_12_RADIO_BTN:
            std::cout << "setting dx12\n";
            r.reset();
            r = std::make_unique<dx12_renderer>(h_scene_wnd);

            if (!file_path.empty())
            {
                r->import_scene_data(file_path);
            }
            break;

        case VULKAN_RADIO_BTN:
            std::cout << "setting vulkan\n";
            r.reset();
            r = std::make_unique<vk_renderer>(h_scene_wnd);

            if (!file_path.empty())
            {
                r->import_scene_data(file_path);
            }

            break;
        }

    default:
        return DefWindowProcA(h_wnd, msg, w_param, l_param);
    }

    return 0;
}

static HWND create_rndrr_wnd(const HINSTANCE h_instance)
{
    HWND wnd = CreateWindowA(
        "Renderer",
        "Renderer",
        WS_OVERLAPPED,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        175, 150,
        nullptr,
        nullptr,
        h_instance,
        nullptr);

    return wnd;
}
*/

static void open_file(const HWND h_wnd)
{
    ComPtr<IFileDialog> file_open;
    if SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&file_open)))
    {
        if SUCCEEDED(file_open->Show(h_wnd))
        {
            ComPtr<IShellItem> open_file;
            if SUCCEEDED(file_open->GetResult(&open_file))
            {
                PWSTR wfile_path;
                if SUCCEEDED(open_file->GetDisplayName(SIGDN_FILESYSPATH, &wfile_path))
                {
                    char fp[MAX_PATH];
                    wcstombs(fp, wfile_path, MAX_PATH);

                    file_path = std::string(fp);

                    s.reset(nullptr);
                    s = std::make_unique<world_scene>(file_path, r.get());
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

static void CALLBACK timer_cb(HWND h_wnd, UINT msg, UINT timer_id, DWORD current_system_time)
{
    std::cout << timer_id << ' ' << current_system_time << '\n';
}

static LRESULT CALLBACK WindowProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    POINT p = {};
    PAINTSTRUCT ps = {};

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

            open_file(h_wnd);

            break;
        default:
            break;
        }

        break;

    case WM_PAINT:
        BeginPaint(h_wnd, &ps);
        s->render(r.get());
        EndPaint(h_wnd, &ps);

        break;

    case WM_MOUSEMOVE:
        p.x = GET_X_LPARAM(l_param);
        p.y = GET_Y_LPARAM(l_param);

        s->handle_mouse_move(p);
        break;

    case WM_KEYDOWN:
        switch (w_param)
        {
        case 87:   // w
            s->handle_w_down();
            break;

        case 65:    // a
            s->handle_a_down();
            break;

        case 83:   // s
            s->handle_s_down();
            break;

        case 68:   // d
            s->handle_d_down();
            break;

        case 81:    // q
            s->handle_q_down();
            break;

        case 69:    // e
            s->handle_e_down();
            break;

        default:
            break;
        }
        break;

    case WM_KEYUP:
        switch (w_param)
        {
        case 87:   // w
            s->handle_w_up();
            break;

        case 65:    // a
            s->handle_a_up();
            break;

        case 83:   // s
            s->handle_s_up();
            break;

        case 68:   // d
            s->handle_d_up();
            break;

        case 81:    // q
            s->handle_q_up();
            break;

        case 69:    // e
            s->handle_e_up();
            break;

        default:
            break;
        }
        break;

    case WM_LBUTTONDOWN:
        s->handle_mouse_l_btn_down();
        break;

    case WM_LBUTTONUP:
        s->handle_mouse_l_btn_up();
        break;

    case WM_MBUTTONDOWN:
        s->handle_mouse_m_btn_down();
        break;

    case WM_MBUTTONUP:
        s->handle_mouse_m_btn_up();
        break;

    case WM_RBUTTONDOWN:
        s->handle_mouse_r_btn_down();

        r_btn_timer_id = SetTimer(h_wnd, R_BTN_TIMER_ID, R_BTN_INTERVAL, nullptr);

        if (r_btn_timer_id == 0)
        {
            std::cerr << "Failed to create r btn down timer\n";
        }

        break;

    case WM_RBUTTONUP:
        s->handle_mouse_r_btn_up();

        if (!KillTimer(h_wnd, r_btn_timer_id))
        {
            std::cerr << "Failed to kill r btn down timer\n";
        }

        break;

    case WM_SIZE:
        if (r.get() != nullptr)
        {
            UINT width = static_cast<UINT>(GET_X_LPARAM(l_param));
            UINT height = static_cast<UINT>(GET_Y_LPARAM(l_param));

            r->resize(width, height);
        }
        break;

    case WM_TIMER:
        switch (w_param)
        {
        case RENDER_TIMER_ID:
            InvalidateRect(h_wnd, &r->wnd_rect, FALSE);
            break;

        case R_BTN_TIMER_ID:
            s->handle_mouse_r_btn_repeat();
            break;

        default:
            break;
        }

        break;

    default:
        return DefWindowProcA(h_wnd, msg, w_param, l_param);
    }

    return 0;
}

int main(int argc, char** argv)
{
    const HINSTANCE h_instance = GetModuleHandleA(nullptr);

    std::cout << "Hi\n";

    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = h_instance;
    wc.hCursor = LoadCursorA(h_instance, MAKEINTRESOURCEA(32512));
    wc.lpszClassName = "Chizen";

    if (!RegisterClassA(&wc))
    {
        DWORD err = GetLastError();
        std::cout << err << '\n';
        return err;
    }

    if FAILED(CoInitialize(nullptr))
    {
        printf("ERR: Could not initialize COM\n");
        return -1;
    }

    h_ctrl_pnl = create_control_panel(h_instance);
    h_scene_wnd = create_wnd(h_instance);

    ShowWindow(h_scene_wnd, SW_SHOW);
    ShowWindow(h_ctrl_pnl, SW_SHOW);

    /*
        WNDCLASSA rc = {
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = RendererWndProc,
                .hInstance = h_instance,
                .hCursor = LoadCursorA(h_instance, MAKEINTRESOURCEA(32512)),
                .lpszClassName = "Renderer",
            };

            if (!RegisterClassA(&rc))
            {
                DWORD err = GetLastError();
                std::cout << err << '\n';
                return err;
            }

            h_rndrr_wnd = create_rndrr_wnd(h_instance);

            ShowWindow(h_rndrr_wnd, SW_SHOW);
    */

    VK_CHECK("volk initialize", volkInitialize());
    r = std::make_unique<vk_renderer>(h_scene_wnd);
    s = std::make_unique<default_scene>();

    UpdateWindow(h_scene_wnd);

    MSG msg = { 0 };

    UINT_PTR t = SetTimer(h_scene_wnd, RENDER_TIMER_ID, RENDER_INTERVAL, nullptr);

    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        DispatchMessageA(&msg);
    }

    CoUninitialize();

    KillTimer(h_scene_wnd, t);

    std::cout << "Bye\n";

    return 0;
}
