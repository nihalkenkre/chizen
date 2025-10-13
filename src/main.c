#include <Windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <Shlwapi.h>

#include "renderer.h"
#include "exr.h"
#include "error.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLM_IMPLEMENTATION
#include <cglm/include/cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

const float RENDER_HEIGHT = 720.f;
const float ASPECT_RATIO = 16.f / 9.f;

#define FILE_OPEN_MENU 10
#define RENDER_UPDATE_TIMER_ID 100
#define RENDER_UPDATE_INTERVAL_MSECS 500

static UINT_PTR render_timer = 0;
static PTP_WORK render_work = NULL;
static bool can_update = FALSE;
HANDLE render_thread = NULL;

BITMAPINFO bm_info = {0};
uint8_t *bm_pixels = NULL;

passes_info pi = 
{
	.passes[0] = {
		.layer = {
			.name = "FinalColor",
			.type = EXR_LAYER_TYPE_FINALCOLOR,
			.num_channels = 4,
		},
	},
	.passes_count = 1,
};

static char file_path[MAX_PATH] = {0};

static DWORD render_scene(LPVOID parameter)
{
	renderer_render_gltf(RENDER_HEIGHT * ASPECT_RATIO, RENDER_HEIGHT, file_path, &pi);

	char img_path[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
	PathRemoveFileSpecA(img_path);
	strcat(img_path, "\\test.exr");

	printf("writing exr...");
	write_exr(img_path, (size_t)(RENDER_HEIGHT * ASPECT_RATIO), (size_t)RENDER_HEIGHT, pi.passes, pi.passes_count);
	printf("done\n");
	KillTimer(*(HWND*)parameter, render_timer);

	
	free(parameter);
cpu_error:
gpu_error:

	return 0;
}

LRESULT CALLBACK WndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
	case WM_CLOSE:
		return DefWindowProcA(h_wnd, msg, w_param, l_param);

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
		switch (w_param)
		{
		case FILE_OPEN_MENU:
		{
			OPENFILENAMEA open_file_name = {
				.lStructSize = sizeof(OPENFILENAMEA),
				.hwndOwner = h_wnd,
				.lpstrFile = file_path,
				.nMaxFile = MAX_PATH,
				.lpstrFilter = ("GLTF Files(*.gltf;*.glb)\0*.gltf;*.glb\0\0"),
				.lpstrTitle = "Select GLTF file",
			};

			if (!GetOpenFileNameA(&open_file_name))
			{
				printf("could not get file name, ERR: %d\n", CommDlgExtendedError());
			}
			else 
			{
				bm_info.bmiHeader.biWidth = RENDER_HEIGHT * ASPECT_RATIO;
				bm_info.bmiHeader.biHeight = -RENDER_HEIGHT;
				bm_info.bmiHeader.biPlanes = 1;
				bm_info.bmiHeader.biBitCount = 32;
				bm_info.bmiHeader.biSize = sizeof(BITMAPINFO);

				bm_pixels = calloc(RENDER_HEIGHT * ASPECT_RATIO * RENDER_HEIGHT, sizeof(uint8_t) * 4);

				CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;

				for (size_t p = 0; p < pi.passes_count; ++p)
				{
					pi.passes[p].pixels = calloc((size_t)(RENDER_HEIGHT * ASPECT_RATIO * RENDER_HEIGHT * pi.passes[p].layer.num_channels), sizeof(float));
					if (pi.passes[p].pixels == NULL)
					{
						printf("calloc failed for passes[%lld].pixels\n", p);
						chi_result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
						goto cpu_error;
					}
				
					pi.passes[p].display_pixels = calloc((size_t)(RENDER_HEIGHT * ASPECT_RATIO * RENDER_HEIGHT * 4), sizeof(uint8_t));
					if (pi.passes[p].display_pixels == NULL)
					{
						printf("calloc failed for passes[%lld].display_pixels\n", p);
						chi_result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
						goto cpu_error;
					}
				}

				render_timer = SetTimer(h_wnd, RENDER_UPDATE_TIMER_ID, RENDER_UPDATE_INTERVAL_MSECS, NULL);
				can_update = TRUE;

				void* parameter = malloc(sizeof(HWND));			// cleared at the end of the thread function
				memcpy(parameter, &h_wnd, sizeof(HWND));

				if (render_thread != NULL)
					CloseHandle(render_thread);

				render_thread = CreateThread(NULL, 0, render_scene, parameter, 0, NULL);

				cpu_error:
				gpu_error:
				;						// place holder ; to pass compilation, have to find a better way to deal with errors.
			}
		}
		break;

		default:
			return DefWindowProcA(h_wnd, msg, w_param, l_param);
		}
		break;

	case WM_PAINT:
	{
		if (!can_update)
			break;

		PAINTSTRUCT ps;

		HDC paint_dc = BeginPaint(h_wnd, &ps);
		HDC hdc = GetDC(h_wnd);

		memcpy(bm_pixels, pi.passes[0].display_pixels, RENDER_HEIGHT * ASPECT_RATIO * RENDER_HEIGHT * 4);

		StretchDIBits(hdc, 0, 0, RENDER_HEIGHT * ASPECT_RATIO, RENDER_HEIGHT, 0, 0, RENDER_HEIGHT * ASPECT_RATIO, RENDER_HEIGHT, bm_pixels, &bm_info, DIB_RGB_COLORS, SRCCOPY);

		ReleaseDC(h_wnd, hdc);
		EndPaint(h_wnd, &ps);
	}
	break;

	case WM_TIMER:
		InvalidateRect(h_wnd, NULL, TRUE);
		UpdateWindow(h_wnd);
		break;

	default:
		return DefWindowProcA(h_wnd, msg, w_param, l_param);
	}

	return 0;
}

int main(int argc, char** argv)
{
	printf("Hello World\n");
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;
	const HINSTANCE h_instance = GetModuleHandleA(NULL);

	WNDCLASSA wc = {
		 .style = CS_HREDRAW | CS_VREDRAW,
		 .lpfnWndProc = WndProc,
		 .hInstance = h_instance,
		 .lpszClassName = "Chizen",
	};

	RegisterClassA(&wc);

	HMENU h_render_label = CreateMenu();
	HMENU h_file_menu = CreateMenu();
	AppendMenuA(h_file_menu, MF_STRING, FILE_OPEN_MENU, "Open");
	AppendMenuA(h_render_label, MF_POPUP, (UINT_PTR)h_file_menu, "Render");

	HWND h_wnd = CreateWindowA(
		"Chizen",
		"Chizen",
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
		NULL, h_render_label,
		h_instance, NULL);

	ShowWindow(h_wnd, SW_SHOW);
	UpdateWindow(h_wnd);

	MSG msg = { 0 };

	while (GetMessageA(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	KillTimer(h_wnd, render_timer);
	if (render_thread != NULL)
		CloseHandle(render_thread);
	
cpu_error:
gpu_error:
	for (size_t p = 0; p < pi.passes_count; ++p)
	{
		free(pi.passes[p].pixels);
		free(pi.passes[p].display_pixels);
	}

	free(bm_pixels);

	printf("Bye World\n");
	return 0;
}
