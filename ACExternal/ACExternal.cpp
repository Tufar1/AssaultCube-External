#include <iostream>
#include <windows.h>
#include <numbers>
#include <print>
#include <thread>

#include <wil/com.h>

#include "mem.hpp"
#include "offset.h"
#include "Math.hpp"
#include "Gui.hpp"

struct EntityStruct {
    std::array<std::uint8_t, 4> pad_0000{};
    Vector3 HeadPos; //0x0004
    std::array<std::uint8_t, 24> pad_0001{};
    Vector3 FootPos; //0x0028
    Vector2 ViewAngle; //0x0034
    std::array<std::uint8_t, 176> pad_0002{};
    int32_t Health; //0x00EC
    std::array<std::uint8_t, 277> pad_0003{};
    std::array<char, 16> Name; //0x0205
    std::array<std::uint8_t, 247> pad_0004{};
    int8_t Team; //0x030C
    std::array<std::uint8_t, 360> pad_0005{};
};

int main() {
    std::println("Hello World!");

    Memory mem("ac_client.exe");
    if (!mem.IsValid()) {
        std::println("Failed to get handle!");
        return 0;
    }

    std::println("Opened Handle!");

    uintptr_t baseAddress = mem.GetModuleBaseAddress("ac_client.exe");

    Math math{};
    math.init();
    
    GuiRenderer esp{};
    esp.initWindow();

    MSG msg{};

    bool runLoop = true;

    while (runLoop) {
        if (GetAsyncKeyState(VK_END) & 0x8000)
            PostMessage(esp.windowHandle.get(), WM_CLOSE, 0, 0);

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                runLoop = false;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        auto LocalPlayerAddress{ mem.RPM<uintptr_t>(baseAddress + offset::LocalPlayer) };
        auto EntityListAddress{ mem.RPM<uintptr_t>(baseAddress + offset::EntityList) };

        auto localPlayer{ mem.RPM<EntityStruct>(LocalPlayerAddress) };
        auto playerCount{ mem.RPM<int>(baseAddress + offset::PlayerCount) };

        EntityStruct closestEntity{};
        double closestDist = DBL_MAX;
        bool entitiesOnScreen = false;

        esp.startDraw();
        for (int i = 1; i < playerCount; i++) {
            auto entityAddress{ mem.RPM<uintptr_t>(EntityListAddress + (i * 0x4)) };
            auto Entity{ mem.RPM<EntityStruct>(entityAddress) };

            if (localPlayer.Team == Entity.Team || Entity.Health <= 0)
                continue;

            Vector2 screenHead{};
            Vector2 screenFoot{};
            glMatrix matrix{ mem.RPM<glMatrix>(baseAddress + offset::viewMatrix) };
            if (math.WorldToScreen({ Entity.HeadPos }, screenHead, matrix) && math.WorldToScreen({ Entity.FootPos }, screenFoot, matrix)) {
                entitiesOnScreen = true;

                float height = screenFoot.y - screenHead.y;
                float width = height / 1.8f;

                int worldDistance = sqrt(
                    pow(Entity.HeadPos.x - localPlayer.HeadPos.x, 2) +
                    pow(Entity.HeadPos.y - localPlayer.HeadPos.y, 2) +
                    pow(Entity.HeadPos.z - localPlayer.HeadPos.z, 2)
                );

                //kresba ctverce
                esp.drawEsp(screenHead, screenFoot, width);
                esp.drawText({ screenHead.x-width / 2, screenHead.y - 15 }, Entity.Name.data());
                esp.drawText(screenFoot, std::to_string(worldDistance) + "m");
                esp.drawHpBar(screenFoot, width, height, Entity.Health);


                double distance = math.GetDistance(screenHead);
                if (distance < closestDist) {
                    closestDist = distance;
                    closestEntity = Entity;
                }
            }
        }
        esp.endDraw();

        if (!entitiesOnScreen || localPlayer.Health <= 0)
            continue;

        //Vypocet aimbotu
        Vector3 delta = closestEntity.HeadPos -localPlayer.HeadPos;

        float yaw = atan2f(delta.y, delta.x) * 180 / std::numbers::pi;
        float hyp = sqrt(delta.x * delta.x + delta.y * delta.y);
        float pitch = atan2f(delta.z, hyp) * 180 / std::numbers::pi;

        //mel bych zapsat cely struct
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
            mem.WPM<Vector2>(LocalPlayerAddress + offsetof(EntityStruct, ViewAngle), {yaw + 90, pitch} );
        }
    }

    return 0;
}
