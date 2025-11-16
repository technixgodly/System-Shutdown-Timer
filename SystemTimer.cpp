#include <windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <powrprof.h>
#pragma comment(lib, "PowrProf.lib")

// Control IDs
#define ID_BUTTON_START  1
#define ID_CHECK_FORCE   2
#define ID_COMBO_ACTION  3

HWND hEditTime, hComboAction, hCheckForce, hLabelCountdown;
bool running = false;

void PerformAction(const std::wstring& action, bool force)
{
    if (action == L"Shutdown")
        system(force ? "shutdown /s /f /t 0" : "shutdown /s /t 0");
    else if (action == L"Reboot")
        system(force ? "shutdown /r /f /t 0" : "shutdown /r /t 0");
    else if (action == L"Sleep")
        SetSuspendState(FALSE, force, FALSE);
    else if (action == L"Hibernate")
        SetSuspendState(TRUE, force, FALSE);
}

void CountdownAndAct(int minutes, const std::wstring& action, bool force)
{
    running = true;
    for (int i = minutes * 60; i > 0 && running; --i) {
        int mins = i / 60;
        int secs = i % 60;
        std::wstring msg = L"Time remaining: " + std::to_wstring(mins) + L"m " + std::to_wstring(secs) + L"s";
        SetWindowTextW(hLabelCountdown, msg.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (running) PerformAction(action, force);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Minutes:", WS_VISIBLE | WS_CHILD, 15, 20, 60, 20, hwnd, NULL, NULL, NULL);
        hEditTime = CreateWindowW(L"EDIT", L"1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            85, 18, 50, 22, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Action:", WS_VISIBLE | WS_CHILD, 15, 55, 60, 20, hwnd, NULL, NULL, NULL);
        hComboAction = CreateWindowW(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            85, 52, 120, 100, hwnd, (HMENU)ID_COMBO_ACTION, NULL, NULL);
        SendMessageW(hComboAction, CB_ADDSTRING, 0, (LPARAM)L"Shutdown");
        SendMessageW(hComboAction, CB_ADDSTRING, 0, (LPARAM)L"Reboot");
        SendMessageW(hComboAction, CB_ADDSTRING, 0, (LPARAM)L"Sleep");
        SendMessageW(hComboAction, CB_ADDSTRING, 0, (LPARAM)L"Hibernate");
        SendMessageW(hComboAction, CB_SETCURSEL, 0, 0);

        hCheckForce = CreateWindowW(L"BUTTON", L"Force Close Apps", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            15, 85, 150, 20, hwnd, (HMENU)ID_CHECK_FORCE, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Start", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            15, 115, 80, 28, hwnd, (HMENU)ID_BUTTON_START, NULL, NULL);

        hLabelCountdown = CreateWindowW(L"STATIC", L"Waiting...", WS_VISIBLE | WS_CHILD,
            15, 155, 220, 25, hwnd, NULL, NULL, NULL);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BUTTON_START && !running) {
            wchar_t buf[32];
            GetWindowTextW(hEditTime, buf, 32);
            int minutes = _wtoi(buf);
            if (minutes <= 0) minutes = 1;

            LRESULT sel = SendMessageW(hComboAction, CB_GETCURSEL, 0, 0);
            wchar_t act[32];
            SendMessageW(hComboAction, CB_GETLBTEXT, sel, (LPARAM)act);

            BOOL force = (SendMessageW(hCheckForce, BM_GETCHECK, 0, 0) == BST_CHECKED);

            std::thread(CountdownAndAct, minutes, std::wstring(act), force).detach();
        }
        break;

    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ShutdownTimer";
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Failed to register window class.", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowW(CLASS_NAME, L"System Timer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 260, 240,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Failed to create window.", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
