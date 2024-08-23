#include <print>
#include <array>
#include <wil/com.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>

#pragma comment(lib, "d3d11")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DESTROY:
        std::println("Ending window");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

class GuiRenderer {
    public:
        GuiRenderer() = default;
        ~GuiRenderer() { 
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        }

        bool initWindow() {
            // Register class
            windowClass.lpfnWndProc = WndProc;
            windowClass.lpszClassName = "EspClass";
            windowClass.hInstance = GetModuleHandle(nullptr);

            RegisterClass(&windowClass);

            windowHandle = wil::unique_hwnd(CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
                L"EspClass",
                L"EspWindow",
                WS_VISIBLE | WS_POPUP,
                0, 0,
                GetSystemMetrics(SM_CXFULLSCREEN),
                GetSystemMetrics(SM_CYFULLSCREEN),
                nullptr, nullptr, nullptr, nullptr
            ));

            std::println("Window Handle: {}", static_cast<PVOID>(windowHandle.get()));

            SetLayeredWindowAttributes(windowHandle.get(), RGB(0, 0, 0), NULL, LWA_COLORKEY);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            ImGui::StyleColorsDark();

            if (!CreateDeviceD3D(windowHandle.get()))
                return false;

            ImGui_ImplWin32_Init(windowHandle.get());
            ImGui_ImplDX11_Init(D3DDevice.get(), D3DDeviceContext.get());

            return true;
        }

        void startDraw() {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void endDraw() {
            ImGui::Render();

            D3DDeviceContext->OMSetRenderTargets(1, mainRenderTargetView.addressof(), nullptr);
            D3DDeviceContext->ClearRenderTargetView(mainRenderTargetView.get(), targetViewColor.data());
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            swapChain->Present(1, 0);
        }

        void drawEsp(Vector2 screenHead, Vector2 screenFoot, float width) {
            drawList = ImGui::GetBackgroundDrawList();
            drawList->AddRect(ImVec2(screenHead.x-width/2,screenHead.y), ImVec2(screenFoot.x + width / 2, screenFoot.y), ImColor(255, 0, 0));
        }
        void drawText(Vector2 pos, std::string_view text) {
            drawList = ImGui::GetBackgroundDrawList();
            drawList->AddText(ImVec2(pos.x, pos.y), ImColor(255, 255, 255), text.data());
        }
        void drawHpBar(Vector2 screenFoot, float width, float height, int hp) {
            drawList = ImGui::GetBackgroundDrawList();
            drawList->AddRectFilled(ImVec2(screenFoot.x-width/2-2, screenFoot.y-(height*(static_cast<float>(hp)/static_cast<float>(100)))),ImVec2(screenFoot.x-width/2-5, screenFoot.y),ImColor(0,255,0));
        }

        wil::unique_hwnd windowHandle = nullptr;
        ImDrawList* drawList = nullptr;
    private:
        wil::com_ptr<ID3D11Device> D3DDevice = nullptr;
        wil::com_ptr<ID3D11DeviceContext> D3DDeviceContext = nullptr;
        wil::com_ptr<IDXGISwapChain> swapChain = nullptr;
        wil::com_ptr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;

        WNDCLASS windowClass{ 0 };
        std::array<float, 4> targetViewColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        void CreateRenderTarget() {
            ID3D11Texture2D* pBackBuffer;
            swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            D3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
            pBackBuffer->Release();
        }

        bool CreateDeviceD3D(HWND hWnd) {
            DXGI_SWAP_CHAIN_DESC sd;
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = 2;
            sd.BufferDesc.Width = 0;
            sd.BufferDesc.Height = 0;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferDesc.RefreshRate.Numerator = 60;
            sd.BufferDesc.RefreshRate.Denominator = 1;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = hWnd;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            UINT createDeviceFlags = 0;
            D3D_FEATURE_LEVEL featureLevel{};
            std::array<D3D_FEATURE_LEVEL, 2> featureLevelArray = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
            HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray.data(), 2, D3D11_SDK_VERSION, &sd, &swapChain, &D3DDevice, &featureLevel, &D3DDeviceContext);
            if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
                res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray.data(), 2, D3D11_SDK_VERSION, &sd, &swapChain, &D3DDevice, &featureLevel, &D3DDeviceContext);
            if (res != S_OK)
                return false;

            CreateRenderTarget();
            return true;
        }
};