//////#include <iostream>
//////#include <string>
//////#include <vector>
//////#include "AcquisitionController.h"
//////#include "ProjectSetting.h" // <-- All settings are now in here
//////////
////////#include "imgui/imgui.h"
////////#include "imgui/imgui_impl_win32.h"
////////#include "imgui/imgui_impl_dx11.h"
////////#include <d3d11.h>
////////#include <Windows.h>
////////
////////
////////// --- User Settings (These are now hard-coded from the settings file) ---
////////const U32 INPUT_RANGE_ID = INPUT_RANGE_PM_1_V;
////////const double INPUT_RANGE_VOLTS = 1.0;
////////const U32 COUPLING_ID = DC_COUPLING;
////////const U32 IMPEDANCE_ID = IMPEDANCE_50_OHM;
//////////const U32 SAMPLE_RATE_ID = SAMPLE_RATE_125MSPS;
//////////const double SAMPLES_PER_SEC = 125000000.0;
//////////const U32 SAMPLE_RATE_ID = SAMPLE_RATE_10MSPS;
//////////const double SAMPLES_PER_SEC = 10000000.0;
////////// Tell AlazarSetCaptureClock to use the external clock port
////////const U32 SAMPLE_RATE_ID = SAMPLE_RATE_USER_DEF;
////////const double SAMPLES_PER_SEC = 125000000.0; // <-- The *expected* frequency of your external clock
////////
//////////const U32 TRIGGER_SOURCE_ID = TRIG_CHAN_A;
////////const U32 TRIGGER_SOURCE_ID = TRIG_EXTERNAL;
////////const U32 TRIGGER_SLOPE_ID = TRIGGER_SLOPE_POSITIVE;
////////const U8 TRIGGER_LEVEL_CODE = 140;
////////const U32 TRIGGER_TIMEOUT_MS = 5000;
////////
////////// Create one global instance of our controller
////////AcquisitionController g_AcqController;
////////
////////// --- GUI Screen Functions ---
////////
////////void ShowBoardInfo()
////////{
////////    std::cout << "\n--- Board Information ---\n";
////////    std::vector<std::string> info = g_AcqController.GetBoardInfoList();
////////    for (const std::string& s : info) {
////////        std::cout << s << std::endl;
////////    }
////////    std::cout << "Press Enter to continue...";
////////    std::cin.ignore(10000, '\n');
////////    std::cin.get();
////////}
////////
////////void ConfigureBoards()
////////{
////////    // Create a config struct from the hard-coded values
////////    BoardConfig config = {};
////////    config.sampleRateId = SAMPLE_RATE_ID;
////////    config.sampleRateHz = SAMPLES_PER_SEC;
////////    config.inputRangeId = INPUT_RANGE_ID;
////////    config.inputRangeVolts = INPUT_RANGE_VOLTS;
////////    config.couplingId = COUPLING_ID;
////////    config.impedanceId = IMPEDANCE_ID;
////////    config.triggerSourceId = TRIGGER_SOURCE_ID;
////////    config.triggerSlopeId = TRIGGER_SLOPE_ID;
////////    config.triggerLevelCode = TRIGGER_LEVEL_CODE;
////////    config.triggerTimeoutMS = TRIGGER_TIMEOUT_MS;
////////
////////    std::cout << "\n--- Board Configuration ---\n";
////////    printf("1. Sample Rate: 125 MS/s\n");
////////    printf("2. Input Range: +/- 1V\n");
////////    printf("3. Trigger Source: Channel A\n");
////////    printf("4. Trigger Level: 140\n");
////////    printf("(Configuration is hard-coded in this example)\n");
////////
////////    if (g_AcqController.ConfigureAllBoards(config)) {
////////        printf("All boards configured successfully.\n");
////////    }
////////    else {
////////        printf("ERROR: Board configuration FAILED.\n");
////////    }
////////
////////    std::cout << "Press Enter to continue...";
////////    std::cin.ignore(10000, '\n');
////////    std::cin.get();
////////}
////////
////////U32 SelectAcquisitionMode()
////////{
////////    char choice = 0;
////////    while (true)
////////    {
////////        std::cout << "\n--- Select Acquisition Mode ---\n";
////////        std::cout << "  [1] NPT (No Pre-Trigger): High-speed, triggered records.\n";
////////        std::cout << "  [2] Traditional (TR): Oscilloscope-style with pre-trigger.\n";
////////        std::cout << "  [3] Continuous (CS): Start immediately, stream gapless data.\n";
////////        std::cout << "  [4] Triggered Stream (TS): Wait for trigger, stream gapless data.\n";
////////        std::cout << "Enter choice: ";
////////        std::cin >> choice;
////////
////////        switch (choice)
////////        {
////////        case '1': printf("Mode set to: NPT\n"); return ADMA_NPT;
////////        case '2': printf("Mode set to: Traditional\n"); return ADMA_TRADITIONAL_MODE;
////////        case '3': printf("Mode set to: Continuous\n"); return ADMA_CONTINUOUS_MODE;
////////        case '4': printf("Mode set to: Triggered Streaming\n"); return ADMA_TRIGGERED_STREAMING;
////////        default:
////////            printf("Invalid choice. Please try again.\n");
////////            std::cin.ignore(10000, '\n');
////////        }
////////    }
////////}
////////
////////void RunAcquisitionTask()
////////{
////////    U32 mode = SelectAcquisitionMode();
////////
////////    AcquisitionConfig acqConfig = {};
////////    acqConfig.admaMode = mode;
////////    acqConfig.saveData = true;
////////    acqConfig.processData = true;
////////
////////    // Use defines from ProjectSettings.h
////////    if (mode == ADMA_CONTINUOUS_MODE || mode == ADMA_TRIGGERED_STREAMING)
////////    {
////////        acqConfig.samplesPerRecord = SAMPLES_PER_RECORD_STREAMING; // 1,024,000
////////        acqConfig.recordsPerBuffer = 16;
////////        acqConfig.buffersPerAcquisition = BUFFERS_PER_ACQUISITION;
////////    }
////////    else // NPT or Traditional
////////    {
////////        acqConfig.samplesPerRecord = SAMPLES_PER_RECORD_NPT_TR; // 4096
////////        acqConfig.recordsPerBuffer = RECORDS_PER_BUFFER;
////////        acqConfig.buffersPerAcquisition = BUFFERS_PER_ACQUISITION;
////////    }
////////
////////    std::cout << "\n--- Run Acquisition ---\n";
////////    printf("Settings:\n");
////////    printf("  Mode:              %u\n", acqConfig.admaMode);
////////    printf("  Samples/Ch/Record: %u\n", acqConfig.samplesPerRecord);
////////    printf("  Records/Buffer:    %u\n", acqConfig.recordsPerBuffer);
////////    printf("  Buffers/Acq:       %u\n", acqConfig.buffersPerAcquisition);
////////    printf("  Save Data:         %s\n", acqConfig.saveData ? "Yes" : "No");
////////    printf("  Process & Plot:    %s\n", acqConfig.processData ? "Yes" : "No");
////////
////////    std::string choice;
////////    std::cout << "\nStart acquisition? (y/n): ";
////////    std::cin >> choice;
////////
////////    if (choice == "y" || choice == "Y") {
////////        g_AcqController.RunAcquisition(acqConfig);
////////    }
////////    else {
////////        printf("Acquisition cancelled.\n");
////////    }
////////
////////    std::cout << "Press Enter to continue...";
////////    std::cin.ignore(10000, '\n');
////////    std::cin.get();
////////}
////////
////////// --- Main Menu Loop ---
////////int main()
////////{
////////    if (!g_AcqController.DiscoverBoards()) {
////////        printf("No boards found. Exiting.\n");
////////        printf("Press Enter to continue...");
////////        std::cin.get();
////////        return 1;
////////    }
////////
////////    char choice = 0;
////////    while (choice != 'q' && choice != 'Q')
////////    {
////////        std::cout << "\n===== Alazar Control GUI =====\n";
////////        std::cout << "  [1] Show Board Info\n";
////////        std::cout << "  [2] Configure All Boards\n";
////////        std::cout << "  [3] Run Acquisition Task\n";
////////        std::cout << "  [Q] Quit\n";
////////        std::cout << "==============================\n";
////////        std::cout << "Enter choice: ";
////////        std::cin >> choice;
////////
////////        switch (choice)
////////        {
////////        case '1':
////////            ShowBoardInfo();
////////            break;
////////        case '2':
////////            ConfigureBoards();
////////            break;
////////        case '3':
////////            RunAcquisitionTask();
////////            break;
////////        case 'q':
////////        case 'Q':
////////            printf("Exiting...\n");
////////            break;
////////        default:
////////            printf("Invalid choice.\n");
////////            std::cin.ignore(10000, '\n');
////////            break;
////////        }
////////    }
////////    return 0;
//////////}
////////
////////#include "imgui/imgui.h"
////////
////////#include <d3d11.h>
////////#include <tchar.h>
////////
////////#include "imgui/imgui_impl_win32.h"
////////#include "imgui/imgui_impl_dx11.h"
////////
////////// Data
////////static ID3D11Device* g_pd3dDevice = nullptr;
////////static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
////////static IDXGISwapChain* g_pSwapChain = nullptr;
////////static bool                     g_SwapChainOccluded = false;
////////static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
////////static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
////////
////////// Forward declarations of helper functions
////////bool CreateDeviceD3D(HWND hWnd);
////////void CleanupDeviceD3D();
////////void CreateRenderTarget();
////////void CleanupRenderTarget();
////////LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
////////
////////// Main code
////////int main(int, char**)
////////{
////////    // Make process DPI aware and obtain main monitor scale
////////    ImGui_ImplWin32_EnableDpiAwareness();
////////    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
////////
////////    // Create application window
////////    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
////////    ::RegisterClassExW(&wc);
////////    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
////////
////////    // Initialize Direct3D
////////    if (!CreateDeviceD3D(hwnd))
////////    {
////////        CleanupDeviceD3D();
////////        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
////////        return 1;
////////    }
////////
////////    // Show the window
////////    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
////////    ::UpdateWindow(hwnd);
////////
////////    // Setup Dear ImGui context
////////    IMGUI_CHECKVERSION();
////////    ImGui::CreateContext();
////////    ImGuiIO& io = ImGui::GetIO(); (void)io;
////////    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
////////    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
////////
////////    // Setup Dear ImGui style
////////    ImGui::StyleColorsDark();
////////    //ImGui::StyleColorsLight();
////////
////////    // Setup scaling
////////    ImGuiStyle& style = ImGui::GetStyle();
////////    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
////////    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
////////
////////    // Setup Platform/Renderer backends
////////    ImGui_ImplWin32_Init(hwnd);
////////    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
////////
////////    // Load Fonts
////////    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
////////    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
////////    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
////////    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
////////    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
////////    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
////////    //style.FontSizeBase = 20.0f;
////////    //io.Fonts->AddFontDefault();
////////    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
////////    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
////////    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
////////    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
////////    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
////////    //IM_ASSERT(font != nullptr);
////////
////////    // Our state
////////    bool show_demo_window = true;
////////    bool show_another_window = false;
////////    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
////////
////////    // Main loop
////////    bool done = false;
////////    while (!done)
////////    {
////////        // Poll and handle messages (inputs, window resize, etc.)
////////        // See the WndProc() function below for our to dispatch events to the Win32 backend.
////////        MSG msg;
////////        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
////////        {
////////            ::TranslateMessage(&msg);
////////            ::DispatchMessage(&msg);
////////            if (msg.message == WM_QUIT)
////////                done = true;
////////        }
////////        if (done)
////////            break;
////////
////////        // Handle window being minimized or screen locked
////////        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
////////        {
////////            ::Sleep(10);
////////            continue;
////////        }
////////        g_SwapChainOccluded = false;
////////
////////        // Handle window resize (we don't resize directly in the WM_SIZE handler)
////////        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
////////        {
////////            CleanupRenderTarget();
////////            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
////////            g_ResizeWidth = g_ResizeHeight = 0;
////////            CreateRenderTarget();
////////        }
////////
////////        // Start the Dear ImGui frame
////////        ImGui_ImplDX11_NewFrame();
////////        ImGui_ImplWin32_NewFrame();
////////        ImGui::NewFrame();
////////
////////        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
////////        if (show_demo_window)
////////            ImGui::ShowDemoWindow(&show_demo_window);
////////
////////        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
////////        {
////////            static float f = 0.0f;
////////            static int counter = 0;
////////
////////            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
////////
////////            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
////////            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
////////            ImGui::Checkbox("Another Window", &show_another_window);
////////
////////            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
////////            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
////////
////////            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
////////                counter++;
////////            ImGui::SameLine();
////////            ImGui::Text("counter = %d", counter);
////////
////////            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
////////            ImGui::End();
////////        }
////////
////////        // 3. Show another simple window.
////////        if (show_another_window)
////////        {
////////            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
////////            ImGui::Text("Hello from another window!");
////////            if (ImGui::Button("Close Me"))
////////                show_another_window = false;
////////            ImGui::End();
////////        }
////////
////////        // Rendering
////////        ImGui::Render();
////////        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
////////        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
////////        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
////////        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
////////
////////        // Present
////////        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
////////        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
////////        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
////////    }
////////
////////    // Cleanup
////////    ImGui_ImplDX11_Shutdown();
////////    ImGui_ImplWin32_Shutdown();
////////    ImGui::DestroyContext();
////////
////////    CleanupDeviceD3D();
////////    ::DestroyWindow(hwnd);
////////    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
////////
////////    return 0;
////////}
////////
////////// Helper functions
////////
////////bool CreateDeviceD3D(HWND hWnd)
////////{
////////    // Setup swap chain
////////    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
////////    DXGI_SWAP_CHAIN_DESC sd;
////////    ZeroMemory(&sd, sizeof(sd));
////////    sd.BufferCount = 2;
////////    sd.BufferDesc.Width = 0;
////////    sd.BufferDesc.Height = 0;
////////    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
////////    sd.BufferDesc.RefreshRate.Numerator = 60;
////////    sd.BufferDesc.RefreshRate.Denominator = 1;
////////    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
////////    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
////////    sd.OutputWindow = hWnd;
////////    sd.SampleDesc.Count = 1;
////////    sd.SampleDesc.Quality = 0;
////////    sd.Windowed = TRUE;
////////    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
////////
////////    UINT createDeviceFlags = 0;
////////    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
////////    D3D_FEATURE_LEVEL featureLevel;
////////    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
////////    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
////////    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
////////        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
////////    if (res != S_OK)
////////        return false;
////////
////////    CreateRenderTarget();
////////    return true;
////////}
////////
////////void CleanupDeviceD3D()
////////{
////////    CleanupRenderTarget();
////////    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
////////    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
////////    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
////////}
////////
////////void CreateRenderTarget()
////////{
////////    ID3D11Texture2D* pBackBuffer;
////////    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
////////    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
////////    pBackBuffer->Release();
////////}
////////
////////void CleanupRenderTarget()
////////{
////////    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
////////}
////////
////////// Forward declare message handler from imgui_impl_win32.cpp
////////extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
////////
////////// Win32 message handler
////////// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
////////// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
////////// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
////////// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
////////LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
////////{
////////    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
////////        return true;
////////
////////    switch (msg)
////////    {
////////    case WM_SIZE:
////////        if (wParam == SIZE_MINIMIZED)
////////            return 0;
////////        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
////////        g_ResizeHeight = (UINT)HIWORD(lParam);
////////        return 0;
////////    case WM_SYSCOMMAND:
////////        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
////////            return 0;
////////        break;
////////    case WM_DESTROY:
////////        ::PostQuitMessage(0);
////////        return 0;
////////    }
////////    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
////////}
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////
//////// main.cpp
////////
//////// This is the main GUI entry point, modified to use int main()
//////// for a console application. It merges the official ImGui DX11
//////// example with the Alazar AcquisitionController.
////////
//////
//////#include <string>
//////#include <vector>
//////#include <thread>         // <-- Required for non-blocking acquisition
//////#include <atomic>         // <-- Required for thread-safe flags
//////#include <sstream>        // For building strings
//////#include <iostream>       // For std::cout
//////
//////// --- Your Backend (NO CHANGES NEEDED) ---
//////#include "AcquisitionController.h"
//////#include "ProjectSetting.h" // (Or "ProjectSettings.h")
//////#include "IoBuffer.h"
//////
//////// --- ImGui & Renderer Headers ---
//////#include "imgui/imgui.h"
//////#include "imgui/imgui_impl_win32.h"
//////#include "imgui/imgui_impl_dx11.h"
//////#include <d3d11.h>
//////#include <tchar.h> // From your sample
//////
//////// --- Global "Brain" and GUI State ---
//////AcquisitionController   g_AcqController;              // Your existing "backend"
//////std::atomic<bool>     g_isAcquiring(false);       // Thread-safe flag
//////std::vector<std::string> g_boardInfoStrings;            // Holds the text from "Discover Boards"
//////std::vector<std::string> g_logMessages;               // A simple log for the GUI
//////std::thread           g_acquisitionThread;          // The thread for our long-running acquisition
//////
//////// --- Default settings (these are now your "form" variables) ---
//////static BoardConfig g_boardConfig = {};
//////static AcquisitionConfig g_acqConfig = {};
//////
//////
//////// --- DirectX Boilerplate Globals (from your sample) ---
//////static ID3D11Device* g_pd3dDevice = nullptr;
//////static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
//////static IDXGISwapChain* g_pSwapChain = nullptr;
//////static bool             g_SwapChainOccluded = false;
//////static UINT             g_ResizeWidth = 0, g_ResizeHeight = 0;
//////static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
//////
//////// --- Boilerplate Function Declarations ---
//////bool CreateDeviceD3D(HWND hWnd);
//////void CleanupDeviceD3D();
//////void CreateRenderTarget();
//////void CleanupRenderTarget();
//////LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//////
//////// --- Simple Log Helper ---
//////void Log(std::string message)
//////{
//////    // A real GUI would use a thread-safe message queue, but this is simple
//////    std::cout << message << std::endl; // Still print to console for debugging
//////    g_logMessages.push_back(message);
//////    if (g_logMessages.size() > 100) // Keep the log from getting too big
//////    {
//////        g_logMessages.erase(g_logMessages.begin());
//////    }
//////}
//////
//////// --- Initialize GUI settings with your hard-coded defaults ---
//////void InitializeGuiDefaults()
//////{
//////    // Board Config (from your old main.cpp)
//////    g_boardConfig.sampleRateId = SAMPLE_RATE_USER_DEF;
//////    g_boardConfig.sampleRateHz = 125000000.0;
//////    g_boardConfig.inputRangeId = INPUT_RANGE_PM_1_V;
//////    g_boardConfig.inputRangeVolts = 1.0;
//////    g_boardConfig.couplingId = DC_COUPLING;
//////    g_boardConfig.impedanceId = IMPEDANCE_50_OHM;
//////    g_boardConfig.triggerSourceId = TRIG_EXTERNAL;
//////    g_boardConfig.triggerSlopeId = TRIGGER_SLOPE_POSITIVE;
//////    g_boardConfig.triggerLevelCode = 140;
//////    g_boardConfig.triggerTimeoutMS = 10000;
//////
//////    // Acquisition Config (from your old main.cpp)
//////    g_acqConfig.admaMode = ADMA_NPT;
//////    g_acqConfig.samplesPerRecord = SAMPLES_PER_RECORD_NPT_TR;
//////    g_acqConfig.recordsPerBuffer = RECORDS_PER_BUFFER;
//////    g_acqConfig.buffersPerAcquisition = BUFFERS_PER_ACQUISITION;
//////    g_acqConfig.saveData = true;
//////    g_acqConfig.processData = true;
//////}
//////
//////// --- Main application entry point ---
//////// (Changed from WinMain to main)
//////int main(int, char**)
//////{
//////    // --- 1. Create Application Window ---
//////    // (This logic is from your working sample)
//////    ImGui_ImplWin32_EnableDpiAwareness();
//////    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
//////
//////    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
//////    ::RegisterClassExW(&wc);
//////    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"AlazarTech Control Panel", WS_OVERLAPPEDWINDOW, 100, 100,
//////        (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
//////
//////    // --- 2. Initialize DirectX ---
//////    if (!CreateDeviceD3D(hwnd))
//////    {
//////        CleanupDeviceD3D();
//////        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//////        return 1;
//////    }
//////
//////    // --- 3. Show The Window ---
//////    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
//////    ::UpdateWindow(hwnd);
//////
//////    // --- 4. Initialize ImGui ---
//////    IMGUI_CHECKVERSION();
//////    ImGui::CreateContext();
//////    ImGuiIO& io = ImGui::GetIO(); (void)io;
//////    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//////    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//////
//////    ImGui::StyleColorsDark();
//////
//////    // Setup scaling (from your sample)
//////    ImGuiStyle& style = ImGui::GetStyle();
//////    style.ScaleAllSizes(main_scale);
//////    style.FontScaleDpi = main_scale;
//////
//////    // Setup Platform/Renderer backends
//////    ImGui_ImplWin32_Init(hwnd);
//////    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
//////
//////    // --- 5. Initialize your GUI variables & Discover Boards ---
//////    InitializeGuiDefaults();
//////    Log("Discovering boards on startup...");
//////    if (g_AcqController.DiscoverBoards()) {
//////        g_boardInfoStrings = g_AcqController.GetBoardInfoList();
//////        Log("Board discovery successful.");
//////    }
//////    else {
//////        Log("ERROR: No boards found.");
//////    }
//////
//////    // --- 6. Main Application Loop ---
//////    bool done = false;
//////    while (!done)
//////    {
//////        // Poll and handle messages (inputs, window resize, etc.)
//////        MSG msg;
//////        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
//////        {
//////            ::TranslateMessage(&msg);
//////            ::DispatchMessage(&msg);
//////            if (msg.message == WM_QUIT)
//////                done = true;
//////        }
//////        if (done)
//////            break;
//////
//////        // Handle window occlusion (from your sample)
//////        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
//////        {
//////            ::Sleep(10);
//////            continue;
//////        }
//////        g_SwapChainOccluded = false;
//////
//////        // Handle window resize (from your sample)
//////        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
//////        {
//////            CleanupRenderTarget();
//////            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
//////            g_ResizeWidth = g_ResizeHeight = 0;
//////            CreateRenderTarget();
//////        }
//////
//////        // Start the Dear ImGui frame
//////        ImGui_ImplDX11_NewFrame();
//////        ImGui_ImplWin32_NewFrame();
//////        ImGui::NewFrame();
//////
//////        // --- 7. YOUR GUI CODE GOES HERE ---
//////        {
//////            // Create one main window covering the whole application
//////            const ImGuiViewport* viewport = ImGui::GetMainViewport();
//////            ImGui::SetNextWindowPos(viewport->WorkPos);
//////            ImGui::SetNextWindowSize(viewport->WorkSize);
//////
//////            ImGui::Begin("MainControl", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
//////
//////            // --- Menu Bar ---
//////            if (ImGui::BeginMenuBar()) {
//////                if (ImGui::BeginMenu("File")) {
//////                    if (ImGui::MenuItem("Exit")) {
//////                        done = true;
//////                    }
//////                    ImGui::EndMenu();
//////                }
//////                ImGui::EndMenuBar();
//////            }
//////
//////            // --- Left Column: Controls ---
//////            ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);
//////            {
//////                // --- Section [1]: Discover Boards ---
//////                ImGui::Text("Board Discovery");
//////                if (ImGui::Button("Re-Discover Boards", ImVec2(-1, 0))) {
//////                    Log("Discovering boards...");
//////                    if (g_AcqController.DiscoverBoards()) {
//////                        g_boardInfoStrings = g_AcqController.GetBoardInfoList();
//////                        Log("Board discovery successful.");
//////                    }
//////                    else {
//////                        Log("ERROR: No boards found.");
//////                    }
//////                }
//////
//////                ImGui::Separator();
//////
//////                // --- Section [2]: Configure Boards ---
//////                ImGui::Text("Board Configuration");
//////                // (Here you would add ImGui widgets to edit g_boardConfig)
//////                ImGui::Text("  Sample Rate: External, 125 MS/s");
//////                ImGui::Text("  Input Range: +/- 1V");
//////                ImGui::Text("  Trigger: External, 140");
//////
//////                if (ImGui::Button("Apply Configuration", ImVec2(-1, 0))) {
//////                    Log("Applying configuration to all boards...");
//////                    if (g_AcqController.ConfigureAllBoards(g_boardConfig)) {
//////                        Log("Configuration successful.");
//////                    }
//////                    else {
//////                        Log("ERROR: Board configuration FAILED.");
//////                    }
//////                }
//////
//////                ImGui::Separator();
//////
//////                // --- Section [3]: Run Acquisition (Your Form) ---
//////                ImGui::Text("Acquisition Task");
//////
//////                // (Use static variables to hold the GUI's state)
//////                static int mode = ADMA_NPT;
//////                ImGui::Text("Acquisition Mode:");
//////                ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
//////                ImGui::RadioButton("CS", &mode, ADMA_CONTINUOUS_MODE); ImGui::SameLine();
//////                ImGui::RadioButton("TS", &mode, ADMA_TRIGGERED_STREAMING);
//////
//////                static int samples_per_rec = SAMPLES_PER_RECORD_NPT_TR;
//////                static int records_per_buf = RECORDS_PER_BUFFER;
//////                static int buffers_per_acq = BUFFERS_PER_ACQUISITION;
//////
//////                // When user selects a mode, update the defaults
//////                if (mode == ADMA_CONTINUOUS_MODE || mode == ADMA_TRIGGERED_STREAMING) {
//////                    samples_per_rec = SAMPLES_PER_RECORD_STREAMING;
//////                    records_per_buf = 1;
//////                }
//////                else {
//////                    samples_per_rec = SAMPLES_PER_RECORD_NPT_TR;
//////                    records_per_buf = RECORDS_PER_BUFFER;
//////                }
//////
//////                ImGui::InputInt("Samples/Channel/Record", &samples_per_rec);
//////                ImGui::InputInt("Records/Buffer", &records_per_buf);
//////                ImGui::InputInt("Buffers to Acquire", &buffers_per_acq);
//////
//////                static bool save_data = true;
//////                static bool process_data = true;
//////                ImGui::Checkbox("Save Data to File", &save_data);
//////                ImGui::Checkbox("Process & Plot (first record)", &process_data);
//////
//////                ImGui::Separator();
//////
//////                // --- Dynamic Start/Stop Button ---
//////                if (g_isAcquiring) {
//////                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
//////                    ImGui::Button("Acquiring...", ImVec2(-1, 40));
//////                    ImGui::PopStyleVar();
//////                }
//////                else {
//////                    if (ImGui::Button("Start Acquisition", ImVec2(-1, 40))) {
//////                        // Launch the acquisition on a *new thread* to keep the GUI responsive
//////                        g_isAcquiring = true;
//////                        Log("Starting acquisition...");
//////
//////                        // Pack the GUI settings into the config struct
//////                        AcquisitionConfig acqConfig = {};
//////                        acqConfig.admaMode = mode;
//////                        acqConfig.saveData = save_data;
//////                        acqConfig.processData = process_data;
//////                        acqConfig.buffersPerAcquisition = buffers_per_acq;
//////                        acqConfig.recordsPerBuffer = records_per_buf;
//////                        acqConfig.samplesPerRecord = samples_per_rec;
//////
//////                        // Launch the thread
//////                        g_acquisitionThread = std::thread([acqConfig]() {
//////                            if (g_AcqController.RunAcquisition(acqConfig)) {
//////                                Log("Acquisition finished successfully.");
//////                            }
//////                            else {
//////                                Log("ERROR: Acquisition failed. Check console log.");
//////                            }
//////                            g_isAcquiring = false; // Set the flag to false when done
//////                            });
//////                        g_acquisitionThread.detach(); // Let the thread run independently
//////                    }
//////                }
//////            }
//////            ImGui::EndChild(); // End "Controls" child
//////            ImGui::SameLine();
//////
//////            // --- Right Column: Output ---
//////            ImGui::BeginChild("Output", ImVec2(0, 0), true);
//////            {
//////                ImGui::Text("Board Information");
//////                ImGui::BeginChild("BoardInfoChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
//////                if (g_boardInfoStrings.empty()) {
//////                    ImGui::Text("Click 'Discover Boards' to populate.");
//////                }
//////                for (const std::string& s : g_boardInfoStrings) {
//////                    ImGui::TextUnformatted(s.c_str());
//////                }
//////                ImGui::EndChild();
//////
//////                ImGui::Separator();
//////
//////                ImGui::Text("Log Output");
//////                ImGui::BeginChild("LogChild", ImVec2(0, 0), true);
//////                for (const std::string& s : g_logMessages) {
//////                    ImGui::TextUnformatted(s.c_str());
//////                }
//////                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) // Auto-scroll
//////                    ImGui::SetScrollHereY(1.0f);
//////                ImGui::EndChild();
//////            }
//////            ImGui::EndChild(); // End "Output" child
//////            ImGui::End(); // End "MainControl" window
//////        }
//////
//////        // --- 8. Render the Frame ---
//////        ImGui::Render();
//////        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
//////        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
//////        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
//////        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
//////        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//////
//////        // Present
//////        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
//////        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
//////    }
//////
//////    // --- 9. Cleanup ---
//////    if (g_isAcquiring) {
//////        Log("Waiting for acquisition thread to finish...");
//////        if (g_acquisitionThread.joinable()) {
//////            g_acquisitionThread.join();
//////        }
//////    }
//////
//////    ImGui_ImplDX11_Shutdown();
//////    ImGui_ImplWin32_Shutdown();
//////    ImGui::DestroyContext();
//////
//////    CleanupDeviceD3D();
//////    ::DestroyWindow(hwnd);
//////    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//////
//////    return 0;
//////}
//////
//////// ------------------------------------------------------------------
//////// --- BOILERPLATE: Win32 & DirectX Setup Functions ---
//////// (This is standard code copied from your ImGui sample)
//////// ------------------------------------------------------------------
//////
//////bool CreateDeviceD3D(HWND hWnd)
//////{
//////    DXGI_SWAP_CHAIN_DESC sd;
//////    ZeroMemory(&sd, sizeof(sd));
//////    sd.BufferCount = 2;
//////    sd.BufferDesc.Width = 0;
//////    sd.BufferDesc.Height = 0;
//////    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//////    sd.BufferDesc.RefreshRate.Numerator = 60;
//////    sd.BufferDesc.RefreshRate.Denominator = 1;
//////    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//////    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//////    sd.OutputWindow = hWnd;
//////    sd.SampleDesc.Count = 1;
//////    sd.SampleDesc.Quality = 0;
//////    sd.Windowed = TRUE;
//////    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//////
//////    UINT createDeviceFlags = 0;
//////    D3D_FEATURE_LEVEL featureLevel;
//////    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
//////    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//////    if (res == DXGI_ERROR_UNSUPPORTED)
//////        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//////    if (res != S_OK)
//////        return false;
//////
//////    CreateRenderTarget();
//////    return true;
//////}
//////
//////void CleanupDeviceD3D()
//////{
//////    CleanupRenderTarget();
//////    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
//////    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
//////    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
//////}
//////
//////void CreateRenderTarget()
//////{
//////    ID3D11Texture2D* pBackBuffer;
//////    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
//////    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
//////    pBackBuffer->Release();
//////}
//////
//////void CleanupRenderTarget()
//////{
//////    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
//////}
//////
//////// Forward declare message handler from imgui_impl_win32.cpp
//////extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//////
//////// Win32 message handler
//////LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
//////{
//////    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
//////        return true;
//////
//////    switch (msg)
//////    {
//////    case WM_SIZE:
//////        if (wParam == SIZE_MINIMIZED)
//////            return 0;
//////        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
//////        g_ResizeHeight = (UINT)HIWORD(lParam);
//////        return 0;
//////    case WM_SYSCOMMAND:
//////        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
//////            return 0;
//////        break;
//////    case WM_DESTROY:
//////        ::PostQuitMessage(0);
//////        return 0;
//////    }
//////    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
//////}
////
////
////#include "AcquisitionController.h"
////#include <stdio.h>
////#include <iostream>
////#include <sstream>
////
////// --- Windows/Linux Compatibility Headers ---
////#ifdef _WIN32
////#include <conio.h>
////#include <direct.h>
////#define GetCurrentDir _getcwd
////#else
////    // ... (Linux helper function prototypes) ...
////#endif
////
////
////AcquisitionController::AcquisitionController() :
////    m_isAcquiring(false)
////{
////}
////
////AcquisitionController::~AcquisitionController()
////{
////    if (m_isAcquiring) StopAcquisition();
////    for (AlazarDigitizer* board : m_boards) delete board;
////    m_boards.clear();
////}
////
////// --- Internal Log Function ---
////void AcquisitionController::Log(std::string message)
////{
////    std::cout << message << std::endl;
////    m_logMessages.push_back(message);
////    if (m_logMessages.size() > 100)
////    {
////        m_logMessages.erase(m_logMessages.begin());
////    }
////}
////
////bool AcquisitionController::IsAcquiring()
////{
////    return m_isAcquiring;
////}
////
////std::vector<std::string> AcquisitionController::GetLogMessages()
////{
////    // In a real app, you'd add a std::mutex here
////    return m_logMessages;
////}
////
////
////bool AcquisitionController::DiscoverBoards()
////{
////    Log("Finding all AlazarTech boards...");
////    U32 systemCount = AlazarNumOfSystems();
////    if (systemCount < 1) {
////        Log("Error: No AlazarTech systems found.");
////        return false;
////    }
////
////    for (AlazarDigitizer* board : m_boards) delete board;
////    m_boards.clear();
////
////    U32 totalBoardCount = 0;
////    for (U32 systemId = 1; systemId <= systemCount; systemId++) {
////        U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
////        for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
////            if (totalBoardCount >= MAX_BOARDS) break;
////            HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
////            if (handle == NULL) continue;
////
////            AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, boardId);
////            if (pBoard->QueryBoardInfo()) {
////                m_boards.push_back(pBoard);
////                totalBoardCount++;
////            }
////            else {
////                delete pBoard;
////            }
////        }
////    }
////    std::stringstream ss;
////    ss << "Found and initialized " << m_boards.size() << " board(s).";
////    Log(ss.str());
////    return m_boards.size() > 0;
////}
////
////std::vector<std::string> AcquisitionController::GetBoardInfoList()
////{
////    std::vector<std::string> infoList;
////    if (m_boards.empty()) {
////        infoList.push_back("No boards found. Call DiscoverBoards().");
////        return infoList;
////    }
////    for (AlazarDigitizer* board : m_boards) {
////        infoList.push_back(board->GetInfoString());
////    }
////    return infoList;
////}
////
////bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
////{
////    if (m_boards.empty()) {
////        Log("Error: No boards to configure. Call DiscoverBoards() first.");
////        return false;
////    }
////    m_currentConfig = config;
////    bool allSuccess = true;
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->ConfigureBoard(config)) {
////            allSuccess = false;
////        }
////    }
////    return allSuccess;
////}
////
////bool AcquisitionController::RunAcquisition(const AcquisitionConfig& config)
////{
////    if (m_isAcquiring) {
////        Log("Error: Acquisition already in progress.");
////        return false;
////    }
////    m_isAcquiring = true;
////    m_currentAcqConfig = config;
////
////    const int channelCount = 4;
////    U32 bytesPerSample = 2;
////    U32 samplesPerChannel = config.samplesPerRecord;
////    U32 recordsPerBuffer;
////
////    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) {
////        recordsPerBuffer = 1;
////    }
////    else {
////        recordsPerBuffer = config.recordsPerBuffer;
////    }
////    U32 samplesPerRecordAllChannels = samplesPerChannel * channelCount;
////    U32 bytesPerRecord = samplesPerRecordAllChannels * bytesPerSample;
////    U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer;
////
////    if (config.saveData) {
////        if (!OpenDataFiles(m_boards.size())) {
////            m_isAcquiring = false;
////            return false;
////        }
////    }
////
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->AllocateBuffers(bytesPerBuffer)) {
////            StopAcquisition();
////            return false;
////        }
////    }
////
////    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->PrepareForAcquisition(config, channelMask)) {
////            StopAcquisition();
////            return false;
////        }
////    }
////
////    for (AlazarDigitizer* board : m_boards) {
////        for (U32 i = 0; i < BUFFER_COUNT; i++) {
////            if (!board->PostBuffer(i)) {
////                StopAcquisition();
////                return false;
////            }
////        }
////    }
////
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->StartCapture()) {
////            StopAcquisition();
////            return false;
////        }
////    }
////
////    std::stringstream ss;
////    ss << "Capturing " << config.buffersPerAcquisition << " buffers per board... Press any key to abort console.";
////    Log(ss.str());
////
////    U32 buffersCompleted = 0;
////    bool success = true;
////
////    while (buffersCompleted < config.buffersPerAcquisition && success)
////    {
////        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
////        for (U32 boardIndex = 0; boardIndex < m_boards.size(); boardIndex++)
////        {
////            AlazarDigitizer* board = m_boards[boardIndex];
////
////            if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
////                success = false;
////                break;
////            }
////
////            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
////            U16* pBuffer = (U16*)pIoBuffer->pBuffer;
////
////            if (!ProcessBufferData(board, pBuffer, config)) {
////                success = false;
////                break;
////            }
////            if (!board->PostBuffer(bufferIndex)) {
////                success = false;
////                break;
////            }
////        }
////
////        if (!success) break;
////
////        buffersCompleted++;
////        if (buffersCompleted % 10 == 0) {
////            std::stringstream ss_prog;
////            ss_prog << "Completed " << buffersCompleted << " of " << config.buffersPerAcquisition << " buffers per board";
////            Log(ss_prog.str());
////        }
////
////        if (_kbhit()) {
////            Log("Abort request from console key press...");
////            break;
////        }
////    }
////
////    Log("Capture complete.");
////
////    StopAcquisition();
////    return success;
////}
////
////void AcquisitionController::StopAcquisition()
////{
////    Log("\n--- Task 6: Cleaning Up ---");
////    for (AlazarDigitizer* board : m_boards) {
////        board->AbortAcquisition();
////        board->FreeBuffers();
////    }
////    CloseDataFiles();
////    m_isAcquiring = false;
////}
////
////bool AcquisitionController::OpenDataFiles(U32 boardCount)
////{
////    char currentDir[MAX_PATH];
////    if (!GetCurrentDir(currentDir, MAX_PATH)) {
////        Log("Error: Could not get current working directory.");
////        return false;
////    }
////    m_savePath = std::string(currentDir);
////
////    std::stringstream ss_log;
////    ss_log << "Saving data to absolute path: " << m_savePath;
////    Log(ss_log.str());
////
////    m_fileStreams.resize(boardCount * 4);
////    char channels[] = { 'A', 'B', 'C', 'D' };
////
////    for (U32 boardIndex = 0; boardIndex < boardCount; boardIndex++)
////    {
////        for (int ch = 0; ch < 4; ch++)
////        {
////            std::stringstream ss;
////            ss << m_savePath << "\\board" << (m_boards[boardIndex]->GetBoardId()) << "_ch" << channels[ch] << ".bin";
////            int file_index = boardIndex * 4 + ch;
////            m_fileStreams[file_index].open(ss.str(), std::ios::binary);
////            if (!m_fileStreams[file_index].is_open()) {
////                Log("Error: Unable to create data file " + ss.str());
////                return false;
////            }
////        }
////    }
////    return true;
////}
////
////void AcquisitionController::CloseDataFiles()
////{
////    for (auto& fileStream : m_fileStreams) {
////        if (fileStream.is_open()) fileStream.close();
////    }
////    m_fileStreams.clear();
////}
////
////bool AcquisitionController::ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config)
////{
////    U32 samplesPerChannel = config.samplesPerRecord;
////    U32 recordsPerBuffer;
////    U32 channelCount = 4;
////
////    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) {
////        recordsPerBuffer = 1;
////        // In streaming, samplesPerRecord is the *total* samples (A+B+C+D) in the buffer.
////        // We need samples *per channel*
////        samplesPerChannel = config.samplesPerRecord / channelCount;
////    }
////    else {
////        recordsPerBuffer = config.recordsPerBuffer;
////    }
////
////    std::vector<U16> chanA_full(samplesPerChannel * recordsPerBuffer);
////    std::vector<U16> chanB_full(samplesPerChannel * recordsPerBuffer);
////    std::vector<U16> chanC_full(samplesPerChannel * recordsPerBuffer);
////    std::vector<U16> chanD_full(samplesPerChannel * recordsPerBuffer);
////
////    for (U32 r = 0; r < recordsPerBuffer; r++)
////    {
////        for (U32 s = 0; s < samplesPerChannel; s++)
////        {
////            U32 interleaved_index = (r * (samplesPerChannel * channelCount)) + (s * channelCount);
////            U32 flat_index = r * samplesPerChannel + s;
////            chanA_full[flat_index] = buffer[interleaved_index + 0];
////            chanB_full[flat_index] = buffer[interleaved_index + 1];
////            chanC_full[flat_index] = buffer[interleaved_index + 2];
////            chanD_full[flat_index] = buffer[interleaved_index + 3];
////        }
////    }
////
////    if (config.saveData)
////    {
////        // Find the index of this board in our vector
////        U32 boardIndex = 0;
////        for (U32 i = 0; i < m_boards.size(); ++i) {
////            if (m_boards[i]->GetHandle() == board->GetHandle()) {
////                boardIndex = i;
////                break;
////            }
////        }
////
////        int f_idx = boardIndex * 4;
////        m_fileStreams[f_idx + 0].write((char*)chanA
//
//
//#include <string>
//#include <vector>
//#include <thread>
//#include <atomic>
//#include <sstream>
//#include <iostream>
//
//// --- Your Backend ---
//#include "AcquisitionController.h"
//#include "ProjectSetting.h"
//#include "IoBuffer.h"
//
//// --- ImGui & Renderer Headers ---
//// These are the "flat" includes that require the
//// "Additional Include Directory" setting.
//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_win32.h"
//#include "imgui/imgui_impl_dx11.h"
//#include <d3d11.h>
//#include <tchar.h> // From your sample
//
//// --- Global "Brain" ---
//AcquisitionController g_AcqController; // Your existing "backend"
//
//// --- Default settings (these are now your "form" variables) ---
//static BoardConfig g_boardConfig = {};
//
//// --- DirectX Boilerplate Globals ---
//static ID3D11Device* g_pd3dDevice = nullptr;
//static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
//static IDXGISwapChain* g_pSwapChain = nullptr;
//static bool             g_SwapChainOccluded = false;
//static UINT             g_ResizeWidth = 0, g_ResizeHeight = 0;
//static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
//
//// --- Boilerplate Function Declarations ---
//bool CreateDeviceD3D(HWND hWnd);
//void CleanupDeviceD3D();
//void CreateRenderTarget();
//void CleanupRenderTarget();
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//// --- Initialize GUI settings with your hard-coded defaults ---
//void InitializeGuiDefaults()
//{
//    // Board Config
//    g_boardConfig.sampleRateId = SAMPLE_RATE_USER_DEF;
//    //g_boardConfig.sampleRateHz = 125000000.0;
//    g_boardConfig.sampleRateHz = 25000000.0;
//    g_boardConfig.inputRangeId = INPUT_RANGE_PM_1_V;
//    g_boardConfig.inputRangeVolts = 1.0;
//    g_boardConfig.couplingId = DC_COUPLING;
//    g_boardConfig.impedanceId = IMPEDANCE_50_OHM;
//    g_boardConfig.triggerSourceId = TRIG_EXTERNAL;
//    g_boardConfig.triggerSlopeId = TRIGGER_SLOPE_POSITIVE;
//    g_boardConfig.triggerLevelCode = 140;
//    g_boardConfig.triggerTimeoutMS = 5000;
//}
//
//// --- Main application entry point ---
//int main_no_multi(int, char**)
//{
//    // --- 1. Create Application Window ---
//    ImGui_ImplWin32_EnableDpiAwareness();
//    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
//    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
//    ::RegisterClassExW(&wc);
//    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"AlazarTech Control Panel", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
//
//    // --- 2. Initialize DirectX ---
//    if (!CreateDeviceD3D(hwnd))
//    {
//        CleanupDeviceD3D();
//        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//        return 1;
//    }
//
//    // --- 3. Show The Window ---
//    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
//    ::UpdateWindow(hwnd);
//
//    // --- 4. Initialize ImGui ---
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO(); (void)io;
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
//    ImGui::StyleColorsDark();
//    ImGuiStyle& style = ImGui::GetStyle();
//    style.ScaleAllSizes(main_scale);
//    style.FontScaleDpi = main_scale;
//    ImGui_ImplWin32_Init(hwnd);
//    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
//
//    // --- 5. Initialize your GUI variables & Discover Boards ---
//    InitializeGuiDefaults();
//    g_AcqController.DiscoverBoards(); // Call this once on startup
//
//    // --- 6. Main Application Loop ---
//    bool done = false;
//    std::thread acquisitionThread; // The thread object
//
//    while (!done)
//    {
//        // Poll and handle messages
//        MSG msg;
//        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
//        {
//            ::TranslateMessage(&msg);
//            ::DispatchMessage(&msg);
//            if (msg.message == WM_QUIT)
//                done = true;
//        }
//        if (done)
//            break;
//
//        // Handle window occlusion
//        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
//        {
//            ::Sleep(10);
//            continue;
//        }
//        g_SwapChainOccluded = false;
//
//        // Handle window resize
//        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
//        {
//            CleanupRenderTarget();
//            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
//            g_ResizeWidth = g_ResizeHeight = 0;
//            CreateRenderTarget();
//        }
//
//        // Start the Dear ImGui frame
//        ImGui_ImplDX11_NewFrame();
//        ImGui_ImplWin32_NewFrame();
//        ImGui::NewFrame();
//
//        // --- 7. GUI CODE GOES HERE ---
//        {
//            const ImGuiViewport* viewport = ImGui::GetMainViewport();
//            ImGui::SetNextWindowPos(viewport->WorkPos);
//            ImGui::SetNextWindowSize(viewport->WorkSize);
//
//            ImGui::Begin("MainControl", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
//
//            if (ImGui::BeginMenuBar()) {
//                if (ImGui::BeginMenu("File")) {
//                    if (ImGui::MenuItem("Exit")) { done = true; }
//                    ImGui::EndMenu();
//                }
//                ImGui::EndMenuBar();
//            }
//
//            // --- Left Column: Controls ---
//            ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);
//            {
//                if (ImGui::Button("Re-Discover Boards", ImVec2(-1, 0))) {
//                    if (!g_AcqController.IsAcquiring())
//                        g_AcqController.DiscoverBoards();
//                }
//
//                ImGui::Separator();
//                ImGui::Text("Board Configuration");
//                // ... (Your config widgets would go here) ...
//                if (ImGui::Button("Apply Configuration", ImVec2(-1, 0))) {
//                    if (!g_AcqController.IsAcquiring())
//                        g_AcqController.ConfigureAllBoards(g_boardConfig);
//                }
//
//                ImGui::Separator();
//                ImGui::Text("Acquisition Task");
//
//                static int mode = ADMA_NPT;
//                ImGui::Text("Acquisition Mode:");
//                ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
//                ImGui::RadioButton("CS", &mode, ADMA_CONTINUOUS_MODE); ImGui::SameLine();
//                ImGui::RadioButton("TS", &mode, ADMA_TRIGGERED_STREAMING);
//
//                static int samples_per_rec = SAMPLES_PER_RECORD_NPT_TR;
//                static int records_per_buf = RECORDS_PER_BUFFER;
//                static int buffers_per_acq = BUFFERS_PER_ACQUISITION;
//
//                if (mode == ADMA_CONTINUOUS_MODE || mode == ADMA_TRIGGERED_STREAMING) {
//                    samples_per_rec = SAMPLES_PER_RECORD_STREAMING;
//                    records_per_buf = 1;
//                }
//                else {
//                    samples_per_rec = SAMPLES_PER_RECORD_NPT_TR;
//                    records_per_buf = RECORDS_PER_BUFFER;
//                }
//
//                ImGui::InputInt("Samples/Channel/Record", &samples_per_rec);
//                ImGui::InputInt("Records/Buffer", &records_per_buf);
//                ImGui::InputInt("Buffers to Acquire", &buffers_per_acq);
//
//                static bool save_data = true;
//                static bool process_data = true;
//                ImGui::Checkbox("Save Data to File", &save_data);
//                ImGui::Checkbox("Process & Plot (first record)", &process_data);
//
//                ImGui::Separator();
//
//                // --- Dynamic Start/Stop Button ---
//                if (g_AcqController.IsAcquiring())
//                {
//                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
//                    ImGui::Button("Acquiring...", ImVec2(-1, 40));
//                    ImGui::PopStyleVar();
//                }
//                else
//                {
//                    if (ImGui::Button("Start Acquisition", ImVec2(-1, 40)))
//                    {
//                        AcquisitionConfig acqConfig = {};
//                        acqConfig.admaMode = mode;
//                        acqConfig.saveData = save_data;
//                        acqConfig.processData = process_data;
//                        acqConfig.buffersPerAcquisition = buffers_per_acq;
//                        acqConfig.recordsPerBuffer = records_per_buf;
//                        acqConfig.samplesPerRecord = samples_per_rec;
//
//                        if (acquisitionThread.joinable()) acquisitionThread.join();
//                        acquisitionThread = std::thread([acqConfig]() {
//                            g_AcqController.RunAcquisition(acqConfig);
//                            });
//                        acquisitionThread.detach();
//                    }
//                }
//
//                ImGui::Separator();
//                if (ImGui::Button("Run Sync Test", ImVec2(-1, 40)))
//                {
//                    /*if (!g_AcqController.IsAcquiring())
//                    {
//                        if (acquisitionThread.joinable()) acquisitionThread.join();
//                        acquisitionThread = std::thread([]() {
//                            g_AcqController.RunSyncTest(g_boardConfig);
//                            });
//                        acquisitionThread.detach();
//                    }*/
//                    if (!g_AcqController.IsAcquiring())
//                    {
//                        // 1. Define the defaults you asked for
//                        // (In the future, you can add ImGui inputs here to let the user edit them)
//                        BoardConfig syncTestConfig;
//                        syncTestConfig.sampleRateId = SAMPLE_RATE_USER_DEF;  // External Clock
//                        syncTestConfig.sampleRateHz = 10000000.0;            // 25 MHz
//                        syncTestConfig.inputRangeId = INPUT_RANGE_PM_1_V;
//                        syncTestConfig.inputRangeVolts = 1.0;
//                        syncTestConfig.couplingId = DC_COUPLING;
//                        syncTestConfig.impedanceId = IMPEDANCE_50_OHM;
//                        syncTestConfig.triggerSourceId = TRIG_EXTERNAL;      // SyncBox Trigger
//                        syncTestConfig.triggerSlopeId = TRIGGER_SLOPE_POSITIVE;
//                        syncTestConfig.triggerLevelCode = 140;               // ~0.5V
//                        syncTestConfig.triggerTimeoutMS = 10000;             // 10 Seconds
//
//                        if (acquisitionThread.joinable()) acquisitionThread.join();
//
//                        // 2. Launch thread passing this config
//                        acquisitionThread = std::thread([syncTestConfig]() { // Capture by value
//                            g_AcqController.RunSyncTest(syncTestConfig);
//                            });
//                        acquisitionThread.detach();
//                    }
//
//
//
//
//                }
//            }
//            ImGui::EndChild(); // End "Controls" child
//            ImGui::SameLine();
//
//            // --- Right Column: Output ---
//            ImGui::BeginChild("Output", ImVec2(0, 0), true);
//            {
//                ImGui::Text("Board Information");
//                ImGui::BeginChild("BoardInfoChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
//
//                std::vector<std::string> info = g_AcqController.GetBoardInfoList();
//                if (info.empty()) {
//                    ImGui::Text("Click 'Discover Boards' to populate.");
//                }
//                for (const std::string& s : info) {
//                    ImGui::TextUnformatted(s.c_str());
//                }
//                ImGui::EndChild();
//
//                ImGui::Separator();
//
//                ImGui::Text("Log Output");
//                ImGui::BeginChild("LogChild", ImVec2(0, 0), true);
//
//                std::vector<std::string> logs = g_AcqController.GetLogMessages();
//                for (const std::string& s : logs) {
//                    ImGui::TextUnformatted(s.c_str());
//                }
//                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) // Auto-scroll
//                    ImGui::SetScrollHereY(1.0f);
//                ImGui::EndChild();
//            }
//            ImGui::EndChild(); // End "Output" child
//            ImGui::End(); // End "MainControl" window
//        }
//
//        // --- 8. Render the Frame ---
//        ImGui::Render();
//        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
//        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
//        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
//        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
//        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//
//        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
//        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
//    }
//
//    // --- 9. Cleanup ---
//    if (g_AcqController.IsAcquiring()) {
//        std::cout << "Waiting for acquisition thread to finish..." << std::endl;
//        if (acquisitionThread.joinable()) {
//            acquisitionThread.join();
//        }
//    }
//
//    ImGui_ImplDX11_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImGui::DestroyContext();
//
//    CleanupDeviceD3D();
//    ::DestroyWindow(hwnd);
//    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//
//    return 0;
//}
//
//// ------------------------------------------------------------------
//// --- BOILERPLATE: Win32 & DirectX Setup Functions ---
//// (This is standard code copied from your ImGui sample)
//// ------------------------------------------------------------------
//
//bool CreateDeviceD3D(HWND hWnd)
//{
//    DXGI_SWAP_CHAIN_DESC sd;
//    ZeroMemory(&sd, sizeof(sd));
//    sd.BufferCount = 2;
//    sd.BufferDesc.Width = 0;
//    sd.BufferDesc.Height = 0;
//    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    sd.BufferDesc.RefreshRate.Numerator = 60;
//    sd.BufferDesc.RefreshRate.Denominator = 1;
//    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//    sd.OutputWindow = hWnd;
//    sd.SampleDesc.Count = 1;
//    sd.SampleDesc.Quality = 0;
//    sd.Windowed = TRUE;
//    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//
//    UINT createDeviceFlags = 0;
//    D3D_FEATURE_LEVEL featureLevel;
//    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
//    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//    if (res == DXGI_ERROR_UNSUPPORTED)
//        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//    if (res != S_OK)
//        return false;
//
//    CreateRenderTarget();
//    return true;
//}
//
//void CleanupDeviceD3D()
//{
//    CleanupRenderTarget();
//    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
//    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
//    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
//}
//
//void CreateRenderTarget()
//{
//    ID3D11Texture2D* pBackBuffer;
//    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
//    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
//    pBackBuffer->Release();
//}
//
//void CleanupRenderTarget()
//{
//    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
//}
//
//// Forward declare message handler from imgui_impl_win32.cpp
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//// Win32 message handler
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
//        return true;
//
//    switch (msg)
//    {
//    case WM_SIZE:
//        if (wParam == SIZE_MINIMIZED)
//            return 0;
//        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
//        g_ResizeHeight = (UINT)HIWORD(lParam);
//        return 0;
//    case WM_SYSCOMMAND:
//        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
//            return 0;
//        break;
//    case WM_DESTROY:
//        ::PostQuitMessage(0);
//        return 0;
//    }
//    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
//}////////////////////////////////////////////////////////////////