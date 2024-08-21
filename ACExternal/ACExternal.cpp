#include <iostream>
#include <windows.h>
#include <numbers>
#include <print>
#include <thread>

#include <d2d1.h>
#include <d2d1_1.h>
#include <wil/com.h>

#include "mem.hpp"
#include "offset.h"
#include "Math.hpp"

#pragma comment(lib, "d2d1.lib")

struct EntityStruct {
    std::array<std::uint8_t, 4> pad_0000{};
    Vector3 HeadPos; //0x0004
    std::array<std::uint8_t, 24> pad_0001{};
    Vector3 FootPos; //0x0028
    Vector2 ViewAngle; //0x0034
    std::array<std::uint8_t, 176> pad_0002{};
    int32_t Health; //0x00EC
    std::array<std::uint8_t, 277> pad_0003{};
    char Name[16]; //0x0205
    std::array<std::uint8_t, 247> pad_0004{};
    int8_t Team; //0x030C
    std::array<std::uint8_t, 360> pad_0005{};
};

LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        std::cout << "Zaviram okno" << std::endl;
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(0, 0, 0)));

        EndPaint(hwnd, &ps);
        return 0;
    }
    break;

    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam); //default akce pokud se mi nic nechce delat
}

int main() {
    std::println("Hello World!");

    Memory mem("ac_client.exe");
    if (!mem.IsValid()) {
        std::println("Failed to get handle!");
        return 0;
    }

    std::println("Opened Handle!");

    uintptr_t baseAddress = mem.GetModuleBaseAddress("ac_client.exe");

    //Registrace třídy
    WNDCLASS windowClass{0};
    windowClass.lpfnWndProc = windowProc;
    windowClass.lpszClassName = "EspClass";
    windowClass.hInstance = GetModuleHandle(nullptr);

    RegisterClass(&windowClass);

    //Tvorba okna a nastavení viditelnosti
    HWND windowHandle = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        L"EspClass",
        L"EspWindow",
        WS_VISIBLE | WS_POPUP,
        0,0,
        GetSystemMetrics(SM_CXFULLSCREEN),
        GetSystemMetrics(SM_CYFULLSCREEN),
        nullptr,nullptr,nullptr,nullptr
    );

    std::println("Window Handle: {}", static_cast<PVOID>(windowHandle));

    SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0), NULL, LWA_COLORKEY);

    //Inicializace Direct2D
    wil::com_ptr<ID2D1Factory> pFactory = nullptr;
    wil::com_ptr<ID2D1HwndRenderTarget> pRenderTarget = nullptr;
    wil::com_ptr<ID2D1SolidColorBrush> pBrush = nullptr;

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

    RECT client_rect{};
    GetClientRect(windowHandle, &client_rect);

    pFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(
            windowHandle,
            D2D1::SizeU(
                client_rect.right - client_rect.left,
                client_rect.bottom - client_rect.top)
        ),
        &pRenderTarget
    );

    pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Red),
        &pBrush
    );

    MSG msg{};

    bool runLoop = true;

    auto LocalPlayerAddress{ mem.RPM<uintptr_t>(baseAddress + offset::LocalPlayer) };
    auto EntityListAddress{ mem.RPM<uintptr_t>(baseAddress + offset::EntityList) };

    while (runLoop) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                runLoop = false;
                break;
            }
        }

        if (GetAsyncKeyState(VK_END) & 0x8000)
            PostMessage(windowHandle, WM_CLOSE, 0, 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        auto LocalPlayer{ mem.RPM<EntityStruct>(LocalPlayerAddress) };
        auto playerCount{ mem.RPM<int>(baseAddress + offset::PlayerCount) };

        EntityStruct ClosestEntity {};
        double closestDist = DBL_MAX;
        bool entitiesOnScreen = false;

        //Start kresby
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        for (int i = 0; i <= playerCount; i++) {
            auto EntityAddress{ mem.RPM<uintptr_t>(EntityListAddress + (i * 0x4)) };
            auto Entity{ mem.RPM<EntityStruct>(EntityAddress) };

            if (LocalPlayer.Team == Entity.Team || Entity.Health <= 0)
                continue;

            Vector2 ScreenHead;
            Vector2 ScreenFoot;
            glMatrix matrix = mem.RPM<glMatrix>((long)mem.GetModuleBaseAddress("ac_client.exe") + offset::viewMatrix);
            if (WorldToScreen({ Entity.HeadPos }, ScreenHead, matrix) && WorldToScreen({ Entity.FootPos }, ScreenFoot, matrix)) {
                entitiesOnScreen = true;

                float height = ScreenFoot.y - ScreenHead.y;
                float width = height / 1.8f;
                //kresba ctverce
                pRenderTarget->DrawRectangle(D2D1::RectF(ScreenHead.x-width/2, ScreenHead.y,ScreenFoot.x+width / 2, ScreenFoot.y), pBrush.get());

                double distance = GetDistance(ScreenHead);
                if (distance < closestDist) {
                    closestDist = distance;
                    ClosestEntity = Entity;
                }
            }
        }

        //konec kresby
        pRenderTarget->EndDraw();

        if (!entitiesOnScreen || LocalPlayer.Health <= 0)
            continue;

        //Vypocet aimbotu
        Vector3 delta = ClosestEntity.HeadPos - LocalPlayer.HeadPos;

        float yaw = atan2f(delta.y, delta.x) * 180 / std::numbers::pi;
        float hyp = sqrt(delta.x * delta.x + delta.y * delta.y);
        float pitch = atan2f(delta.z, hyp) * 180 / std::numbers::pi;

        //mel bych zapsat cely struct
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
            mem.WPM<Vector2>(LocalPlayerAddress + offsetof(EntityStruct, ViewAngle), { yaw + 90, pitch });
        }
    }
    //cisteni po konci
    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
    return 0;
}
