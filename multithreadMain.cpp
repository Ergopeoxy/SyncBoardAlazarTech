//#include <string>
//#include <vector>
//#include <thread>
//#include <atomic>
//#include <sstream>
//#include <iostream>
//
//// --- Backend ---
//#include "AcquisitionControllerMultithread.h"
//#include "ProjectSetting.h"
//#include "IoBuffer.h"
//
//// --- ImGui ---
//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_win32.h"
//#include "imgui/imgui_impl_dx11.h"
//#include "implot/implot.h" // Assuming you added ImPlot
//#include <d3d11.h>
//#include <tchar.h>
//
//// --- Global ---
//AcquisitionController g_AcqController;
//static BoardConfig g_boardConfig = {};
//
//// --- DirectX Globals ---
//static ID3D11Device* g_pd3dDevice = nullptr;
//static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
//static IDXGISwapChain* g_pSwapChain = nullptr;
//static bool g_SwapChainOccluded = false;
//static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
//static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
//
//// --- Forward Decls ---
//bool CreateDeviceD3D(HWND hWnd);
//void CleanupDeviceD3D();
//void CreateRenderTarget();
//void CleanupRenderTarget();
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//// --- Helpers ---
//void InitializeGuiDefaults()
//{
//    g_boardConfig.sampleRateId = SAMPLE_RATE_USER_DEF;
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
//
//BOOL WINAPI ConsoleHandler(DWORD signal) {
//    if (signal == CTRL_CLOSE_EVENT) {
//        // Last ditch effort to close handles if user kills console
//        // Note: You can't do much here safely, but it prevents immediate hard crash
//        // g_AcqController.~AcquisitionController(); // Risky but sometimes necessary
//        return TRUE;
//    }
//    return FALSE;
//}
//
//
//// --- Main ---
//int main(int, char**)
//{
//    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//    // Setup Window
//    ImGui_ImplWin32_EnableDpiAwareness();
//    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
//    ::RegisterClassExW(&wc);
//    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Alazar Control", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
//
//    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
//    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
//    ::UpdateWindow(hwnd);
//
//    // Setup ImGui
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImPlot::CreateContext();
//    ImGui::StyleColorsDark();
//    ImGui_ImplWin32_Init(hwnd);
//    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
//
//    InitializeGuiDefaults();
//    g_AcqController.DiscoverBoards();
//
//    bool done = false;
//    std::thread acquisitionThread;
//
//    while (!done)
//    {
//        MSG msg;
//        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
//            ::TranslateMessage(&msg);
//            ::DispatchMessage(&msg);
//            if (msg.message == WM_QUIT) done = true;
//        }
//        if (done) break;
//
//        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
//            ::Sleep(10); continue;
//        }
//        g_SwapChainOccluded = false;
//
//        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
//            CleanupRenderTarget();
//            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
//            g_ResizeWidth = g_ResizeHeight = 0;
//            CreateRenderTarget();
//        }
//
//        ImGui_ImplDX11_NewFrame();
//        ImGui_ImplWin32_NewFrame();
//        ImGui::NewFrame();
//
//        ImGuiViewport* vp = ImGui::GetMainViewport();
//        //WorkPos is the place where we are allowed to drawd
//        ImGui::SetNextWindowPos(vp->WorkPos);
//        //WorkSize seems like the pixel size of the workable area
//        ImGui::SetNextWindowSize(vp->WorkSize);
//
//        // --- GUI ---
//        {
//            ImGui::Begin("Alazar Dashboard", nullptr,
//                ImGuiWindowFlags_NoTitleBar |
//                ImGuiWindowFlags_NoResize |
//                ImGuiWindowFlags_NoMove |
//                ImGuiWindowFlags_NoCollapse |
//                ImGuiWindowFlags_NoBringToFrontOnFocus |
//                ImGuiWindowFlags_NoNavFocus
//            );
//
//            /*   if (ImGui::BeginMenuBar()) {
//                   if (ImGui::BeginMenu("File")) {
//                       if (ImGui::MenuItem("Exit")) done = true;
//                       ImGui::EndMenu();
//                   }
//                   ImGui::EndMenuBar();
//               }*/
//
//               // --- CONTROLS ---
//            ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
//
//            if (ImGui::Button("Discover Boards")) g_AcqController.DiscoverBoards();
//            ImGui::Separator();
//
//            if (ImGui::Button("Apply Config")) g_AcqController.ConfigureAllBoards(g_boardConfig);
//            ImGui::Separator();
//
//            static int mode = ADMA_NPT;
//            ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
//            ImGui::RadioButton("CS", &mode, ADMA_CONTINUOUS_MODE);
//
//            static int buffers_to_cap = 1000;
//            ImGui::InputInt("Buffers", &buffers_to_cap);
//
//            static bool save_data = true;
//            ImGui::Checkbox("Save Data (Disk)", &save_data);
//
//            ImGui::Separator();
//
//            if (g_AcqController.IsAcquiring()) {
//                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
//                ImGui::Button("Acquiring...", ImVec2(-1, 40));
//                ImGui::PopStyleVar();
//            }
//            else {
//                if (ImGui::Button("START ACQUISITION", ImVec2(-1, 40))) {
//                    AcquisitionConfig acq = {};
//                    acq.admaMode = mode;
//                    acq.buffersPerAcquisition = buffers_to_cap;
//                    acq.recordsPerBuffer = 10;
//                    acq.samplesPerRecord = 4096;
//                    acq.saveData = save_data;
//                    acq.processData = true; // For GUI plot
//
//                    if (acquisitionThread.joinable()) acquisitionThread.join();
//                    acquisitionThread = std::thread([acq]() { g_AcqController.RunAcquisition(acq); });
//                }
//            }
//
//            if (ImGui::Button("Sync Test", ImVec2(-1, 30))) {
//                if (!g_AcqController.IsAcquiring()) {
//                    BoardConfig sConf = g_boardConfig;
//                    sConf.sampleRateHz = 10000000.0; // 10MHz for Sync Test
//                    if (acquisitionThread.joinable()) acquisitionThread.join();
//                    acquisitionThread = std::thread([sConf]() { g_AcqController.RunSyncTest(sConf); });
//                }
//            }
//
//            ImGui::EndChild();
//            ImGui::SameLine();
//
//            // --- VISUALIZATION ---
//            ImGui::BeginChild("Viz", ImVec2(0, 0), true);
//
//            if (ImGui::BeginTabBar("Tabs")) {
//                //if (ImGui::BeginTabItem("Live Scope")) {
//                //    static std::vector<float> liveData;
//                //    // safely fetch latest data
//                //    g_AcqController.GetLatestScopeData(liveData);
//
//                //    if (!liveData.empty() && ImPlot::BeginPlot("Live Signal")) {
//                //        ImPlot::PlotLine("Ch A", liveData.data(), liveData.size());
//                //        ImPlot::EndPlot();
//                //    }
//                //    ImGui::EndTabItem();
//                //}
//
//                if (ImGui::BeginTabItem("Live Scope")) {
//
//                    //// --- NEW: Board Selector ---
//                    //static int selectedBoard = 1;
//                    //ImGui::Text("Select Board:"); ImGui::SameLine();
//                    //ImGui::RadioButton("Board 1", &selectedBoard, 1); ImGui::SameLine();
//                    //ImGui::RadioButton("Board 2", &selectedBoard, 2);
//                    //// ---------------------------
//
//                    //static std::vector<float> liveData;
//
//                    //// Pass the selected ID to the controller
//                    //g_AcqController.GetLatestScopeData(liveData, (U32)selectedBoard);
//
//                    //if (!liveData.empty() && ImPlot::BeginPlot("Live Signal")) {
//                    //    // Change label dynamically
//                    //    std::string label = "Board " + std::to_string(selectedBoard) + " - Ch A";
//                    //    ImPlot::PlotLine(label.c_str(), liveData.data(), liveData.size());
//                    //    ImPlot::EndPlot();
//                    //}
//                    //ImGui::EndTabItem();
//
//
//
//                    static int selectedBoard = 1; // Default to 1
//
//                    ImGui::Text("Select Board:");
//                    ImGui::SameLine();
//                    // If you only have one board connected, this second button won't show anything
//                    ImGui::RadioButton("Board 1", &selectedBoard, 1);
//                    ImGui::SameLine();
//                    ImGui::RadioButton("Board 2", &selectedBoard, 2);
//
//                    static std::vector<float> liveData;
//
//                    // This will now correctly request ID 1 when the first button is selected
//
//                    g_AcqController.GetLatestScopeData(liveData, (U32)selectedBoard);
//
//                    if (!liveData.empty() && ImPlot::BeginPlot("Live Signal")) {
//                        std::string label = "Board " + std::to_string(selectedBoard) + " - Ch A";
//                        ImPlot::PlotLine(label.c_str(), liveData.data(), liveData.size());
//                        ImPlot::EndPlot();
//                    }
//                    else {
//                        // Helpful debug text if graph is empty
//                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for data from Board %d...", selectedBoard);
//                    }
//                    ImGui::EndTabItem();
//                }
//
//                //if (ImGui::BeginTabItem("Sync Check"))
//                //{
//                //    // 1. Retrieve Data (Thread Safe)
//                //    static std::vector<float> b1, b2;
//                //    static int lag = 0;
//
//                //    // Only fetch if not acquiring to prevent tearing
//                //    if (!g_AcqController.IsAcquiring()) {
//                //        g_AcqController.GetSyncSnapshot(b1, b2);
//                //        lag = g_AcqController.GetLastSyncLag(); // Assuming you added this getter
//                //    }
//
//                //    // 2. Display Status
//                //    ImGui::Text("Sync Status:"); ImGui::SameLine();
//                //    if (lag == 0) ImGui::TextColored(ImVec4(0, 1, 0, 1), "PERFECT (Lag 0)");
//                //    else ImGui::TextColored(ImVec4(1, 0, 0, 1), "FAILED (Lag %d)", lag);
//
//                //    // 3. Draw Graphs (Only if data exists)
//                //    if (!b1.empty() && !b2.empty())
//                //    {
//                //        // Create a stack of 3 plots sharing the X-axis
//                //        if (ImPlot::BeginSubplots("Sync Analysis", 3, 1, ImVec2(-1, -1), ImPlotSubplotFlags_LinkAllX))
//                //        {
//                //            // --- Graph 1: Board 1 Only ---
//                //            if (ImPlot::BeginPlot("Board 1 (Ch A)")) {
//                //                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//                //                ImPlot::PlotLine("Signal B1", b1.data(), (int)b1.size());
//                //                ImPlot::EndPlot();
//                //            }
//
//                //            // --- Graph 2: Board 2 Only ---
//                //            if (ImPlot::BeginPlot("Board 2 (Ch A)")) {
//                //                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//                //                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); // Orange color
//                //                ImPlot::PlotLine("Signal B2", b2.data(), (int)b2.size());
//                //                ImPlot::EndPlot();
//                //            }
//
//                //            // --- Graph 3: Overlay (The Verification) ---
//                //            if (ImPlot::BeginPlot("Overlay Comparison")) {
//                //                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//
//                //                // Draw Board 1 (Blue)
//                //                ImPlot::PlotLine("Board 1", b1.data(), (int)b1.size());
//
//                //                // Draw Board 2 (Orange)
//                //                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
//                //                ImPlot::PlotLine("Board 2", b2.data(), (int)b2.size());
//
//                //                ImPlot::EndPlot();
//                //            }
//
//                //            ImPlot::EndSubplots();
//                //        }
//                //    }
//                //    else {
//                //        ImGui::Text("No data available. Click 'Run Sync Test' to capture.");
//                //    }
//
//                //    ImGui::EndTabItem();
//                //}
//
//
//                if (ImGui::BeginTabItem("Sync Check"))
//                {
//                    //std::cout << "called the synccehck";
//                    // --- 1. Retrieve Data ---
//                    static std::vector<float> dataB1, dataB2;
//                    static int lastLag = 0;
//
//                    // Only update local copy if acquisition is NOT running (to prevent tearing/crashing)
//                    /*if (!g_AcqController.IsAcquiring()) {
//                        g_AcqController.GetSyncSnapshot(dataB1, dataB2);
//                        lastLag = g_AcqController.GetLastSyncLag();
//                    }*/
//                    g_AcqController.GetSyncSnapshot(dataB1, dataB2);
//                    lastLag = g_AcqController.GetLastSyncLag();
//                    // --- 2. Status Header ---
//                    ImGui::Text("Sync Status: "); ImGui::SameLine();
//                    if (lastLag == 0 && !dataB1.empty())
//                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "PERFECT SYNC (Lag: 0 samples)");
//                    else if (dataB1.empty())
//                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "No Data (Run Test First)");
//                    else
//                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFSET DETECTED (Lag: %d samples)", lastLag);
//
//                    // --- 3. The Visual Proof (ImPlot) ---
//                    if (!dataB1.empty() && !dataB2.empty())
//                    {
//                        // Create a stack of 3 plots that share the X-Axis (Zooming one zooms all)
//                        // This is crucial for inspecting the edges of the signal.
//                        if (ImPlot::BeginSubplots("Synchronization Analysis", 3, 1, ImVec2(-1, -1), ImPlotSubplotFlags_LinkAllX))
//                        {
//                            // [Plot 1] Board 1 Alone
//                            if (ImPlot::BeginPlot("Board 1 (Signal A)")) {
//                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//                                ImPlot::PlotLine("B1 Data", dataB1.data(), (int)dataB1.size());
//                                ImPlot::EndPlot();
//                            }
//
//                            // [Plot 2] Board 2 Alone
//                            if (ImPlot::BeginPlot("Board 2 (Signal A)")) {
//                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//                                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); // Orange
//                                ImPlot::PlotLine("B2 Data", dataB2.data(), (int)dataB2.size());
//                                ImPlot::EndPlot();
//                            }
//
//                            // [Plot 3] The Overlay (Visual Proof)
//                            if (ImPlot::BeginPlot("Overlay Comparison (Zoom In Here!)")) {
//                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
//
//                                // Draw Board 1 (Blue)
//                                ImPlot::PlotLine("Board 1", dataB1.data(), (int)dataB1.size());
//
//                                // Draw Board 2 (Orange)
//                                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
//                                ImPlot::PlotLine("Board 2", dataB2.data(), (int)dataB2.size());
//
//                                ImPlot::EndPlot();
//                            }
//
//                            ImPlot::EndSubplots();
//                        }
//                    }
//
//                    ImGui::EndTabItem();
//                }
//                if (ImGui::BeginTabItem("Log")) {
//                    auto logs = g_AcqController.GetLogMessages();
//                    for (auto& s : logs) ImGui::TextUnformatted(s.c_str());
//                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
//                    ImGui::EndTabItem();
//                }
//                ImGui::EndTabBar();
//            }
//            ImGui::EndChild();
//
//            ImGui::End();
//        }
//
//        ImGui::Render();
//        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
//        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
//        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
//        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//        g_pSwapChain->Present(1, 0);
//    }
//
//    // --- SAFE EXIT SEQUENCE ---
//
//    // 1. Stop the Acquisition Thread
//    if (g_AcqController.IsAcquiring() || acquisitionThread.joinable()) {
//        // Set flag to false so the loop inside the thread knows to stop
//        // (You might need a public setter or just call StopAcquisition if exposed, 
//        //  but calling a new method ForceStop() is cleaner).
//
//        // Ideally, calling StopAcquisition() inside the controller handles this,
//        // but we need to ensure the thread *joins* so we don't kill it mid-operation.
//
//        // Trigger a stop from the main thread to be sure
//        // (Assuming you have a method for this, or rely on the destructor)
//        // g_AcqController.StopAcquisition(); // If public
//
//        // Wait for the thread to finish its current loop and exit
//        if (acquisitionThread.joinable()) {
//            std::cout << "Waiting for acquisition thread to stop..." << std::endl;
//            acquisitionThread.join();
//        }
//    }
//
//    // 2. ImGui Cleanup
//    ImGui_ImplDX11_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImPlot::DestroyContext();
//    ImGui::DestroyContext();
//
//    // 3. DirectX Cleanup
//    CleanupDeviceD3D();
//    ::DestroyWindow(hwnd);
//    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//
//    return 0;
//
//
//
//
//
//
//    //if (g_AcqController.IsAcquiring()) {
//    //    if (acquisitionThread.joinable()) acquisitionThread.join();
//    //}
//
//    //ImGui_ImplDX11_Shutdown();
//    //ImGui_ImplWin32_Shutdown();
//    //ImPlot::DestroyContext();
//    //ImGui::DestroyContext();
//    //CleanupDeviceD3D();
//    //::DestroyWindow(hwnd);
//    //::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//    //return 0;
//}
//
//// --- BOILERPLATE DX11 FUNCTIONS ---
//// (These remain identical to your previous working main.cpp)
//bool CreateDeviceD3D(HWND hWnd) {
//    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
//    sd.BufferCount = 2; sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0;
//    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
//    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//    sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
//    sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//    UINT createDeviceFlags = 0;
//    D3D_FEATURE_LEVEL featureLevel;
//    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
//    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
//    CreateRenderTarget(); return true;
//}
//void CleanupDeviceD3D() {
//    CleanupRenderTarget();
//    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
//    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
//    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
//}
//void CreateRenderTarget() {
//    ID3D11Texture2D* pBackBuffer;
//    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
//    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
//    pBackBuffer->Release();
//}
//void CleanupRenderTarget() {
//    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
//}
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
//    switch (msg) {
//    case WM_SIZE:
//        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
//            CleanupRenderTarget();
//            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
//            CreateRenderTarget();
//        }
//        return 0;
//    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
//    case WM_DESTROY: ::PostQuitMessage(0); return 0;
//    }
//    return ::DefWindowProc(hWnd, msg, wParam, lParam);
//}//////////////////////////////
//
//
//
//
//
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <sstream>
#include <iostream>
#include <algorithm> // for min/max element

// --- Backend ---
#include "AcquisitionControllerLineDetect.h"
#include "IoBuffer.h"

// --- ImGui ---
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "implot/implot.h" 
#include <d3d11.h>
#include <tchar.h>

// --- Global ---
AcquisitionController g_AcqController;
static BoardConfig g_boardConfig = {};
// --- ADD THIS AT THE TOP OF YOUR FILE (Global Scope) ---
static const ImVec4 custom_hot[255] = {
    {0.010417f, 0.0f, 0.0f, 1}, {0.020833f, 0.0f, 0.0f, 1}, {0.031250f, 0.0f, 0.0f, 1}, {0.041667f, 0.0f, 0.0f, 1},
    // ... (This array is massive, I will condense it to a linear interpolation for you below) ... 
    {1.0f, 1.0f, 0.0f, 1}, {1.0f, 1.0f, 1.0f, 1}
};






// --- DirectX Globals ---
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// --- Forward Decls ---
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Helpers ---
void InitializeGuiDefaults()
{
    g_boardConfig.sampleRateId = SAMPLE_RATE_USER_DEF;
    g_boardConfig.sampleRateHz = 25000000.0;
    g_boardConfig.inputRangeId = INPUT_RANGE_PM_1_V;
    g_boardConfig.inputRangeVolts = 1.0;
    g_boardConfig.couplingId = DC_COUPLING;
    g_boardConfig.impedanceId = IMPEDANCE_50_OHM;
    g_boardConfig.triggerSourceId = TRIG_EXTERNAL;
    g_boardConfig.triggerSlopeId = TRIGGER_SLOPE_POSITIVE;
    g_boardConfig.triggerLevelCode = 140;
    g_boardConfig.triggerTimeoutMS = 5000;
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_CLOSE_EVENT) {
        return TRUE;
    }
    return FALSE;
}


void SetupCustomColormap() {
    static bool initialized = false;
    if (initialized) return;

    // Define the key colors (Black -> Red -> Yellow -> White)
    static const ImVec4 colors[] = {
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f), // Black  (at 0%)
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // Red    (at 33%)
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f), // Yellow (at 66%)
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f)  // White  (at 100%)
    };

    // CORRECT FUNCTION CALL:
    // Arguments: (Name, ColorArray, Count, Qualitative)
    // Set 'Qualitative' to FALSE to enable smooth linear interpolation (Gradient)
    ImPlot::AddColormap("Custom Hot", colors, 4, false);

    initialized = true;
}
void DrawImagesWindow() {
    // Ensure Colormap is ready
    SetupCustomColormap();

    // --- STATE VARIABLES (Static, just like Code B) ---
    static float dsc_scale_min = 0; // Adjusted for your data range (0-55000)
    static float dsc_scale_max = 55000;

    // Toggles for channels (Code B style)
    static bool img_dsc_bool = true;
    static bool img_nr_bool = false; // "Non-Radiative" (Placeholder for your other channels)

    // Data Containers
    static std::vector<std::vector<double>> image2D;
    static std::vector<double> flatImage;

    // Fetch Data from your Controller
    g_AcqController.GetLatestImage(image2D);

    // Flatten logic (Required for ImPlot)
    if (!image2D.empty()) {
        int rows = (int)image2D.size();
        int cols = (int)image2D[0].size();
        if (flatImage.size() != rows * cols) flatImage.resize(rows * cols);
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                flatImage[y * cols + x] = image2D[y][x];
            }
        }

        // --- THE GUI LAYOUT (Exact Replica of Code B) ---
        if (ImGui::Begin("Images")) { // Floating Window

            // 1. CONFIG HEADER
            if (ImGui::CollapsingHeader("Image Display Configuration")) {
                if (ImGui::TreeNode("Detection Scattering (DSc)")) {
                    ImGui::SetNextItemWidth(200);
                    // Code B uses DragFloatRange2 for Min/Max sliders
                    ImGui::DragFloatRange2("Min / Max DSc", &dsc_scale_min, &dsc_scale_max, 100.0f);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
            }

            // 2. CHECKBOXES
            ImGui::Checkbox("DSc", &img_dsc_bool);
            ImGui::SameLine();
            ImGui::Checkbox("NR", &img_nr_bool); // Placeholder for future channel

            // 3. HEATMAP RENDER
            if (img_dsc_bool) {
                // Code B uses specific colormaps per channel
                // We will use the Custom "Hot" map we defined
                ImPlot::PushColormap("Custom Hot");

                // The Plot
                if (ImPlot::BeginPlot("##HeatmapDSc", ImVec2(512, 512))) { // Code B uses ImVec2(PointsPerLine, PointsPerLine)
                    ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks);
                    ImPlot::SetupAxesLimits(0, 1, 0, 1); // Normalized Coordinates 0-1

                    // Note: Code B uses &img_dsc[0][0]. We use flatImage.data()
                    ImPlot::PlotHeatmap("Detection", flatImage.data(), rows, cols, (double)dsc_scale_min, (double)dsc_scale_max);

                    ImPlot::EndPlot();
                }

                // The Scale Bar (Side-by-Side)
                ImGui::SameLine();
                ImPlot::ColormapScale("##HeatScaleDSc", (double)dsc_scale_min, (double)dsc_scale_max, ImVec2(60, 512));

                ImPlot::PopColormap();
            }
        }
        ImGui::End();
    }
}
// --- Main ---
int main(int, char**)
{
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Setup Window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Alazar Control", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    InitializeGuiDefaults();
    g_AcqController.DiscoverBoards();

    bool done = false;
    std::thread acquisitionThread;
    static int syncSrcB1 = 0; // 0=ChA, 1=ChB, 2=ChC, 3=ChD
    static int syncSrcB2 = 0;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10); continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);

        // --- GUI ---
        {




            ImGui::Begin("Alazar Dashboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

            // --- CONTROLS ---
            ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, 0), true);

            if (ImGui::Button("Discover Boards")) g_AcqController.DiscoverBoards();
            ImGui::Separator();

            // Create actual variables with default values
            static int g_SegmentsPerBuffer = 4096;
            static int g_SegBuffersInCirc = 1000;
            static int g_LineParams = 5000;
            static int g_PointsPerLine = 1024;
            static int g_ProcQueueMax = 50;

            // Also variables for your specific config inputs
            static int g_SamplesPerRecord = 384;
            static int g_RecordsPerBuffer = 200;
            static int g_BuffersPerAcq = 0;
            
            ImGui::Text("Add configs");

            // The second argument MUST be &variable_name
            ImGui::InputInt("Samples Per Record", &g_SamplesPerRecord);
            ImGui::InputInt("Records Per Buffer", &g_RecordsPerBuffer);
            //ImGui::InputInt("Buffers Per Acq", &g_BuffersPerAcq);

            ImGui::Separator();
            ImGui::Text("Advanced Settings");

            ImGui::InputInt("Segments Per Buffer", &g_SegmentsPerBuffer);
            ImGui::InputInt("Circ Buffer Count", &g_SegBuffersInCirc);
            ImGui::InputInt("Line Params", &g_LineParams);
            ImGui::InputInt("Points Per Line", &g_PointsPerLine);
            ImGui::InputInt("Proc Queue Max", &g_ProcQueueMax);

            //#define NUMBER_OF_SEGMENTS_PER_BUFFER 4096 
            //#define NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER 1000
            //#define CIRC_BUFFER_SIZE (NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER)
            //#define NUMBER_OF_LINE_PARAMS 5000
            //#define NUMBER_OF_POINTS_PER_LINE 1024 
            //#define PROCESSING_QUEUE_MAX_SIZE 50

         

            ImGui::Separator();

            if (ImGui::Button("Apply Config")) g_AcqController.ConfigureAllBoards(g_boardConfig);
            ImGui::Separator();

            static int mode = ADMA_NPT;
            ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
            ImGui::RadioButton("CS", &mode, ADMA_CONTINUOUS_MODE);

            static int buffers_to_cap = 1000;
            ImGui::InputInt("Buffers per acquisition (set to -1 for infinite acq)", &buffers_to_cap);

            static bool save_data = true;
            ImGui::Checkbox("Save Data (Disk)", &save_data);

            ImGui::Separator();

            

            if (g_AcqController.IsAcquiring()) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::Button("Acquiring...", ImVec2(-1, 40));
                ImGui::PopStyleVar();
            }
            else {
                if (ImGui::Button("START ACQUISITION", ImVec2(-1, 40))) {
                    AcquisitionConfig acq = {};
                    acq.admaMode = mode;
                    acq.buffersPerAcquisition = buffers_to_cap;
                    //acq.recordsPerBuffer = 300; //10
                    //acq.samplesPerRecord = 384; //4096
                    acq.recordsPerBuffer = g_RecordsPerBuffer;
                    acq.samplesPerRecord = g_SamplesPerRecord;
                    acq.saveData = save_data;
                    acq.processData = true;

                    if (acquisitionThread.joinable()) acquisitionThread.join();
                    acquisitionThread = std::thread([acq]() { g_AcqController.RunAcquisition(acq); });
                }
            }

            if (ImGui::Button("Sync Test", ImVec2(-1, 30))) {
                if (!g_AcqController.IsAcquiring()) {
                    BoardConfig sConf = g_boardConfig;
                    sConf.sampleRateHz = 10000000.0; // 10MHz for Sync Test

                    if (acquisitionThread.joinable()) acquisitionThread.join();

                    // --- CAPTURE SELECTION ---
                    // Capture the current GUI selection by value [=] so the thread gets the right numbers
                    int s1 = syncSrcB1; // 0=A, 1=B, etc.
                    int s2 = syncSrcB2;

                    // --- LAUNCH THREAD WITH ARGUMENTS ---
                    acquisitionThread = std::thread([sConf, s1, s2]() {
                        g_AcqController.RunSyncTest(sConf, s1, s2);
                        });
                }
            }

            ImGui::EndChild();
            ImGui::SameLine();

            // --- VISUALIZATION ---
            ImGui::BeginChild("Viz", ImVec2(0, 0), true);

            if (ImGui::BeginTabBar("Tabs")) {

                // --- TAB 1: 1D Live Scope (Signals) ---
                //if (ImGui::BeginTabItem("Live Signals")) {
                //    static int selectedBoard = 1;

                //    // Channel Selection for Debugging Peaks
                //    static bool showChA = true;
                //    static bool showChB = false; // M1 (Peak Detect)
                //    static bool showChC = false; // M2 (Vertical)

                //    ImGui::Text("Board:"); ImGui::SameLine();
                //    ImGui::RadioButton("1", &selectedBoard, 1); ImGui::SameLine();
                //    ImGui::RadioButton("2", &selectedBoard, 2);
                //    //ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

                //    ImGui::Checkbox("Img (ChA)", &showChA); ImGui::SameLine();
                //    ImGui::Checkbox("Fast M1 (ChB)", &showChB); ImGui::SameLine();
                //    ImGui::Checkbox("Slow M2 (ChC)", &showChC);

                //    static std::vector<float> liveData;

                //    // NOTE: You'd need to update GetLatestScopeData to accept a channel index 
                //    // or return a struct with all channels to make ChB/ChC work fully. 
                //    // For now, assuming ChA is returned.
                //    g_AcqController.GetLatestScopeData(liveData, (U32)selectedBoard);

                //    if (!liveData.empty() && ImPlot::BeginPlot("Live Signal")) {
                //        ImPlot::SetupAxes("Samples", "Volts");
                //        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImGuiCond_Once);

                //        if (showChA) ImPlot::PlotLine("Image", liveData.data(), liveData.size());

                //        // Placeholder: If you want to see M1/M2, you need to expose them in the Controller
                //        // For now, this visualizes where you would look for peaks.

                //        ImPlot::EndPlot();
                //    }
                //    ImGui::EndTabItem();
                //}
                // --- TAB 1: 1D Live Scope (Signals) ---
                //static std::vector<double> peaks;
                //g_AcqController.GetLatestPeaks(peaks);
                //if (ImGui::BeginTabItem("Live Signals")) {

                //    static int phaseOffset = 0;
                //    // Drag integer from -50 to +50 samples
                //    if (ImGui::DragInt("Phase Adjust (Samples)", &phaseOffset, 1, -50, 50)) {
                //        g_AcqController.SetPhaseAdjust(phaseOffset);
                //    }


                //    static int selectedBoard = 1;

                //    static bool showChA = true;
                //    static bool showChB = false;
                //    static bool showChC = false;

                //    ImGui::Text("Board:"); ImGui::SameLine();
                //    ImGui::RadioButton("1", &selectedBoard, 1); ImGui::SameLine();
                //    ImGui::RadioButton("2", &selectedBoard, 2);
                //    //ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

                //    ImGui::Checkbox("Img (ChA)", &showChA); ImGui::SameLine();
                //    ImGui::Checkbox("Fast M1 (ChB)", &showChB); ImGui::SameLine();
                //    ImGui::Checkbox("Slow M2 (ChC)", &showChC);

                //    if (ImPlot::BeginPlot("Live Signal", ImVec2(-1, -1))) {
                //        ImPlot::SetupAxes("Samples", "Volts");
                //        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImGuiCond_Once);

                //        // 1. Plot Channel A (Image)
                //        if (showChA) {
                //            static std::vector<float> dataA;
                //            g_AcqController.GetLatestScopeData(dataA, (U32)selectedBoard, 0); // 0 = ChA
                //            if (!dataA.empty()) ImPlot::PlotLine("Image (ChA)", dataA.data(), (int)dataA.size());
                //        }

                //        // 2. Plot Channel B (M1) - This is likely your Sine Wave
                //        if (showChB) {
                //            static std::vector<float> dataB;
                //            g_AcqController.GetLatestScopeData(dataB, (U32)selectedBoard, 1); // 1 = ChB
                //            if (!dataB.empty()) {
                //                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); // Orange
                //                ImPlot::PlotLine("M1 (ChB)", dataB.data(), (int)dataB.size());
                //            }
                //        }

                //        // 3. Plot Channel C (M2) - This is likely your Ramp
                //        if (showChC) {
                //            static std::vector<float> dataC;
                //            g_AcqController.GetLatestScopeData(dataC, (U32)selectedBoard, 2); // 2 = ChC
                //            if (!dataC.empty()) {
                //                ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                //                ImPlot::PlotLine("M2 (ChC)", dataC.data(), (int)dataC.size());
                //            }
                //        }

                //        // Draw Peaks as Vertical Lines
                //        ImPlot::SetNextLineStyle(ImVec4(1, 0, 0, 1)); // Red
                //        for (double p : peaks) {
                //            // Draw a vertical line at x = p
                //            double xs[] = { p, p };
                //            double ys[] = { -2, 2 };
                //            ImPlot::PlotLine("Peak", xs, ys, 2);
                //        }


                //        ImPlot::EndPlot();
                //    }
                //    ImGui::EndTabItem();
                //}
                

                // --- TAB 1: Live Signals ---
           // --- TAB 1: Live Signals ---
            if (ImGui::BeginTabItem("Live Signals")) {
               





                // ==============================
                // 1. GENERAL CONTROLS
                // ==============================
                static int selectedBoard = 1;
                static int phaseOffset = 0;

                // Channel Visibility Toggles
                static bool showChA = true;
                static bool showChB = false; // M1
                static bool showChC = false; // M2
                static bool showChD = false;

                // Board Selection
                ImGui::Text("Board:"); ImGui::SameLine();
                ImGui::RadioButton("1", &selectedBoard, 1); ImGui::SameLine();
                ImGui::RadioButton("2", &selectedBoard, 2);

                ImGui::SameLine(); ImGui::Text(" | "); ImGui::SameLine();

                // Channel Toggles
                ImGui::Checkbox("Img (ChA)", &showChA); ImGui::SameLine();
                ImGui::Checkbox("Fast M1 (ChB)", &showChB); ImGui::SameLine();
                ImGui::Checkbox("Slow M2 (ChC)", &showChC); ImGui::SameLine();
                ImGui::Checkbox("Aux (ChD)", &showChD); ImGui::SameLine();
                // Phase Adjustment Slider
                ImGui::SameLine(); ImGui::Text(" | "); ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::DragInt("Phase Adjust", &phaseOffset, 1, -50, 50)) {
                    g_AcqController.SetPhaseAdjust(phaseOffset);
                }

                ImGui::Separator();

                // ==============================
                // 2. ALGORITHM TUNING & DEBUG
                // ==============================
                static double tHigh, tLow;
                static int tDist;

                // Fetch current values from backend
                g_AcqController.GetAlgoParams(&tHigh, &tLow, &tDist);

                bool changed = false;
                ImGui::Text("Algorithm Tuning:"); ImGui::SameLine();
                // --- FIX: Define limits as variables first ---
                double min_v = 0.0;
                double max_v = 65535.0;
                int min_dist_limit = 1;
                int max_dist_limit = 500;

                ImGui::PushItemWidth(120);

                // Pass the ADDRESS (&) of the limits, not the value
                if (ImGui::DragScalar("High Thresh", ImGuiDataType_Double, &tHigh, 100.0f, &min_v, &max_v)) changed = true;
                ImGui::SameLine();
                if (ImGui::DragScalar("Low Thresh", ImGuiDataType_Double, &tLow, 100.0f, &min_v, &max_v))  changed = true;
                ImGui::SameLine();
                if (ImGui::DragInt("Min Dist", &tDist, 1, min_dist_limit, max_dist_limit)) changed = true;

                ImGui::PopItemWidth();

                if (changed) g_AcqController.SetAlgoParams(tHigh, tLow, tDist);
               

                // ==============================
                // 3. DATA FETCHING (SYNCHRONIZED)
                // ==============================
                static std::vector<float> snapshotWave;
                static std::vector<double> snapshotPeaks;

                // This gets Waveform + Peaks from the SAME processing pass to prevent flickering
                g_AcqController.GetLatestSnapshot(snapshotWave, snapshotPeaks);

                // ==============================
                // 4. PLOTTING
                // Note :
                // ImVec4 stands for ImGui Vector 4. It holds four floating-point numbers typically used to represent RGBA (Red, Green, Blue, Alpha).

               /* example (1.0f, 0.0f, 1.0f, 1.0f) :

                    Red(1.0f) : 100 % (Full Red)

                    Green(0.0f) : 0 % (No Green)

                    Blue(1.0f) : 100 % (Full Blue)

                    Alpha(1.0f) : 100 % (Fully Opaque / No Transparency*/
                // ==============================
                if (ImPlot::BeginPlot("Live Signal", ImVec2(-1, -1))) {
                    ImPlot::SetupAxes("Samples", "Volts");
                    ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImGuiCond_Once);

                    // A. Plot the Waveform (Channel A)
                    if (showChA && !snapshotWave.empty()) {
                        ImPlot::PlotLine("Image (ChA)", snapshotWave.data(), (int)snapshotWave.size());
                    }

                    // B. Plot Other Channels (Non-Sync, just for checking M1/M2 health)
                    if (showChB) {
                        static std::vector<float> dataB;
                        g_AcqController.GetLatestScopeData(dataB, (U32)selectedBoard, 1);
                        if (!dataB.empty()) {
                            ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); // Orange
                            ImPlot::PlotLine("M1 (ChB)", dataB.data(), (int)dataB.size());
                        }
                    }

                    if (showChC) {
                        static std::vector<float> dataC;
                        g_AcqController.GetLatestScopeData(dataC, (U32)selectedBoard, 2);
                        if (!dataC.empty()) {
                            ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                            ImPlot::PlotLine("M2 (ChC)", dataC.data(), (int)dataC.size());
                        }
                    }

                    if (showChD) {
                        static std::vector<float> dataD;
                        // Fetch Index 3 (0=A, 1=B, 2=C, 3=D)
                        g_AcqController.GetLatestScopeData(dataD, (U32)selectedBoard, 3);
                        if (!dataD.empty()) {
                            ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.0f, 1.0f, 1.0f)); // Magenta
                            ImPlot::PlotLine("Aux (ChD)", dataD.data(), (int)dataD.size());
                        }
                    }

                    // C. Draw Threshold Lines (Visual Debug)
                    // Convert Raw U16 to Voltage Float for the plot
                    double plotThreshHigh = (tHigh - 32768.0) / 32768.0;
                    double plotThreshLow = (tLow - 32768.0) / 32768.0;

                    ImPlot::SetNextLineStyle(ImVec4(0, 1, 1, 0.5f)); // Cyan, semi-transparent
                    double lineHigh[] = { plotThreshHigh, plotThreshHigh };
                    ImPlot::PlotInfLines("High Thresh", lineHigh, 1, ImPlotInfLinesFlags_Horizontal);

                    ImPlot::SetNextLineStyle(ImVec4(1, 0, 1, 0.5f)); // Magenta
                    double lineLow[] = { plotThreshLow, plotThreshLow };
                    ImPlot::PlotInfLines("Low Thresh", lineLow, 1, ImPlotInfLinesFlags_Horizontal);

                    // D. Plot Detected Peaks (Red Vertical Lines)
                    // Uses the snapshot data so it moves perfectly with the wave
                    if (!snapshotPeaks.empty()) {
                        ImPlot::SetNextLineStyle(ImVec4(1, 0, 0, 1)); // Red Color
                        for (double p : snapshotPeaks) {
                            double xs[] = { p, p };
                            double ys[] = { -2, 2 };
                            ImPlot::PlotLine("##Peak", xs, ys, 2);
                        }
                    }

                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }






            //// --- TAB 2: 2D Line Visualization (Heatmap) ---
            //if (ImGui::BeginTabItem("2D Image")) {


            //    // --- DEBUGGING THE GENERATOR ---
            //    ImGui::Separator();
            //    ImGui::Text("--- Generator Debug ---");

            //    // 1. Check if lines are actually being detected
            //    ImGui::Text("Total Lines Processed: %d", g_AcqController.GetTotalLinesProcessed()); // You might need to add this getter

            //    // 2. Check the raw values driving the Y-Axis
            //    // (You will need to expose these variables or just log them to console for now)
            //    static double last_m2_mean = 0;
            //    // If you can, expose the latest 'mean_m2' from the controller
            //    // ImGui::Text("Last Channel C Value: %.2f", last_m2_mean); 

            //    ImGui::Text("Expected Range: [28500.0, 38500.0]");
            //    ImGui::Separator();

            //    static std::vector<std::vector<double>> image2D;
            //    static std::vector<double> flatImage;

            //    // Fetch from Backend
            //    g_AcqController.GetLatestImage(image2D);

            //    if (!image2D.empty()) {
            //        int rows = (int)image2D.size();
            //        int cols = (int)image2D[0].size();
            //        size_t total = rows * cols;

            //        // --- 1. Flatten Data ---
            //        if (flatImage.size() != total) flatImage.resize(total);

            //        // Track data range for debugging
            //        double dataMin = 1e9;
            //        double dataMax = -1e9;
            //        bool isAllZero = true;

            //        for (int y = 0; y < rows; y++) {
            //            for (int x = 0; x < cols; x++) {
            //                double val = image2D[y][x];
            //                flatImage[y * cols + x] = val; // Row-Major flatten

            //                // Capture Stats
            //                if (val < dataMin) dataMin = val;
            //                if (val > dataMax) dataMax = val;
            //                if (val != 0.0) isAllZero = false;
            //            }
            //        }

            //        // --- 2. Controls & Debug Info ---
            //        ImGui::Text("Image Dimensions: %d x %d", cols, rows);

            //        // ALERT if data is empty
            //        if (isAllZero) {
            //            ImGui::TextColored(ImVec4(1, 0, 0, 1), "WARNING: All data is ZERO. Check Peak Detection!");
            //        }
            //        else {
            //            ImGui::Text("Data Range: [%.4f, %.4f]", dataMin, dataMax);
            //        }

            //        static double scaleMin = 0;
            //        static double scaleMax = 65535;

            //        // Auto-Scale Button (The Fix)
            //        if (ImGui::Button("Auto Scale")) {
            //            scaleMin = dataMin;
            //            scaleMax = dataMax;
            //            // Prevent crash if min == max (flat image)
            //            if (abs(scaleMax - scaleMin) < 0.0001) scaleMax = scaleMin + 1.0;
            //        }

            //        ImGui::SameLine();
            //        ImGui::DragScalar("Scale Min", ImGuiDataType_Double, &scaleMin, 0.1f);
            //        ImGui::SameLine();
            //        ImGui::DragScalar("Scale Max", ImGuiDataType_Double, &scaleMax, 0.1f);

            //        // --- 3. Plotting ---
            //        if (ImPlot::BeginPlot("##ScanImage", ImVec2(-1, -1))) {
            //            // Use 'ImGuiCond_Always' to force the axes to match the image size exactly
            //            ImPlot::SetupAxes(NULL, NULL, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks);
            //            ImPlot::SetupAxisLimits(ImAxis_X1, 0, cols, ImGuiCond_Always);
            //            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, rows, ImGuiCond_Always);

            //            // Select a visible Colormap (Viridis is good for scientific data)
            //            ImPlot::PushColormap(ImPlotColormap_Viridis);

            //            // PlotHeatmap arguments: (ID, values, rows, cols, min, max)
            //            // Note: If image looks rotated 90 degrees, swap 'rows' and 'cols' here.
            //            ImPlot::PlotHeatmap("##Heatmap", flatImage.data(), rows, cols, scaleMin, scaleMax);

            //            ImPlot::PopColormap();
            //            ImPlot::EndPlot();
            //        }
            //    }
            //    else {
            //        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for Image Buffer...");
            //    }
            //    ImGui::EndTabItem();
            //}

                // --- TAB 2: 2D Point Cloud (Scatter Plot) ---
            //if (ImGui::BeginTabItem("2D Image")) {

            //    static std::vector<std::vector<double>> image2D;

            //    // Arrays for the Scatter Plot
            //    static std::vector<double> scatterX;
            //    static std::vector<double> scatterY;

            //    // Fetch the latest full grid
            //    g_AcqController.GetLatestImage(image2D);

            //    if (!image2D.empty()) {
            //        int rows = (int)image2D.size();
            //        int cols = (int)image2D[0].size();

            //        // --- 1. Filter Data (Convert Grid to Dots) ---
            //        scatterX.clear();
            //        scatterY.clear();

            //        // Threshold: Only show dots brighter than this value
            //        // You can make this adjustable with a slider
            //        static double show_threshold = 0.001;

            //        // Optimization: Reserve memory to prevent stutter
            //        scatterX.reserve(10000);
            //        scatterY.reserve(10000);

            //        for (int y = 0; y < rows; y++) {
            //            for (int x = 0; x < cols; x++) {
            //                double val = image2D[y][x];

            //                // If this pixel has signal, add it to our "Dot List"
            //                if (val > show_threshold) {
            //                    scatterX.push_back((double)x);
            //                    scatterY.push_back((double)y);
            //                }
            //            }
            //        }

            //        // --- 2. Controls ---
            //        ImGui::Text("Visible Points: %zu", scatterX.size());

            //        // Slider to filter noise
            //        ImGui::DragScalar("Threshold", ImGuiDataType_Double, &show_threshold, 0.001f, 0, 0, "%.4f");

            //        // --- 3. Plotting (Scatter) ---
            //        if (ImPlot::BeginPlot("##ScatterPlot", ImVec2(-1, -1))) {
            //            // Force axes to 1024x1024
            //            ImPlot::SetupAxisLimits(ImAxis_X1, 0, cols, ImGuiCond_Always);
            //            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, rows, ImGuiCond_Always);

            //            // Optional: Make dots smaller or larger
            //            // ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f); 

            //            // Draw the dots
            //            // Note: If you have >100,000 points, this might get slow!
            //            if (scatterX.size() > 0) {
            //                ImPlot::PlotScatter("Signal", scatterX.data(), scatterY.data(), (int)scatterX.size());
            //            }

            //            ImPlot::EndPlot();
            //        }
            //    }
            //    else {
            //        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for Data...");
            //    }
            //    ImGui::EndTabItem();
            //}
            // 
            // 
// --- TAB: XY Scan Path (The Maze) ---
if (ImGui::BeginTabItem("XY Scan Path"))
{
    // --- 1. CONTROLS ---
    static int xyBoardID = 1;
    static int xAxisChannel = 1; // Default ChB
    static int yAxisChannel = 2; // Default ChC
    static bool showX = true;
    static bool showY = true;

    // NEW: Trace Length Control
    static int traceLength = 4096; // Default to a standard record size
    static bool limitTrace = false;

    // Board Selection
    ImGui::Text("Source Board:"); ImGui::SameLine();
    ImGui::RadioButton("1##xy", &xyBoardID, 1); ImGui::SameLine();
    ImGui::RadioButton("2##xy", &xyBoardID, 2);

    ImGui::Separator();

    // Axis Assignment
    ImGui::Text("X-Axis:"); ImGui::SameLine();
    ImGui::RadioButton("A##x", &xAxisChannel, 0); ImGui::SameLine();
    ImGui::RadioButton("B##x", &xAxisChannel, 1); ImGui::SameLine();
    ImGui::RadioButton("C##x", &xAxisChannel, 2); ImGui::SameLine();
    ImGui::RadioButton("D##x", &xAxisChannel, 3);
    ImGui::SameLine(); ImGui::Checkbox("On##x", &showX);

    ImGui::Text("Y-Axis:"); ImGui::SameLine();
    ImGui::RadioButton("A##y", &yAxisChannel, 0); ImGui::SameLine();
    ImGui::RadioButton("B##y", &yAxisChannel, 1); ImGui::SameLine();
    ImGui::RadioButton("C##y", &yAxisChannel, 2); ImGui::SameLine();
    ImGui::RadioButton("D##y", &yAxisChannel, 3);
    ImGui::SameLine(); ImGui::Checkbox("On##y", &showY);

    ImGui::Separator();

    // --- NEW CONTROL: LIMIT CYCLES ---
    ImGui::Checkbox("Limit Trace Length", &limitTrace);
    if (limitTrace) {
        ImGui::SameLine();
        // Allow user to drag from 10 samples (tiny) to 50,000 (huge)
        ImGui::DragInt("##samples", &traceLength, 10.0f, 10, 100000, "%d pts");
    }

    // --- 2. FETCH DATA ---
    static std::vector<float> dataX;
    static std::vector<float> dataY;

    g_AcqController.GetLatestScopeData(dataX, (U32)xyBoardID, xAxisChannel);
    g_AcqController.GetLatestScopeData(dataY, (U32)xyBoardID, yAxisChannel);

    // --- 3. PLOT ---
    if (ImPlot::BeginPlot("Lissajous Scan Path", ImVec2(-1, -1))) {

        char xLabel[32], yLabel[32];
        sprintf_s(xLabel, "X-Axis (Ch %c)", 'A' + xAxisChannel);
        sprintf_s(yLabel, "Y-Axis (Ch %c)", 'A' + yAxisChannel);
        ImPlot::SetupAxes(xLabel, yLabel);
        ImPlot::SetupAxisLimits(ImAxis_X1, -1.2, 1.2);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.2, 1.2);

        if (!dataX.empty() && !dataY.empty() && dataX.size() == dataY.size()) {

            // Apply Toggles
            if (!showX) std::fill(dataX.begin(), dataX.end(), 0.0f);
            if (!showY) std::fill(dataY.begin(), dataY.end(), 0.0f);

            // CALCULATE COUNT
            int count = (int)dataX.size();
            if (limitTrace) {
                // Clamp the slider value to the actual available data size
                if (traceLength > count) traceLength = count;
                if (traceLength < 2) traceLength = 2;
                count = traceLength;
            }

            ImPlot::SetNextLineStyle(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);

            // Plot using 'count'
            ImPlot::PlotLine("Scan Trace", dataX.data(), dataY.data(), count);
        }
        else {
            ImPlot::Annotation(0, 0, ImVec4(1, 1, 0, 1), ImVec2(0, -10), false, "Waiting for data...");
        }

        ImPlot::EndPlot();
    }
    ImGui::EndTabItem();
}
                //// --- TAB 3: Sync Check ---
                //if (ImGui::BeginTabItem("Sync Check"))
                //{
                //    static std::vector<float> dataB1, dataB2;
                //    static int lastLag = 0;

                //    // Fetch
                //    g_AcqController.GetSyncSnapshot(dataB1, dataB2);
                //    lastLag = g_AcqController.GetLastSyncLag();

                //    ImGui::Text("Sync Status: "); ImGui::SameLine();
                //    if (lastLag == 0 && !dataB1.empty()) ImGui::TextColored(ImVec4(0, 1, 0, 1), "PERFECT SYNC (Lag: 0)");
                //    else if (dataB1.empty()) ImGui::TextColored(ImVec4(1, 1, 0, 1), "No Data (Run Test First)");
                //    else ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFSET DETECTED (Lag: %d)", lastLag);

                //    if (!dataB1.empty() && !dataB2.empty())
                //    {
                //        if (ImPlot::BeginSubplots("Sync Analysis", 2, 1, ImVec2(-1, -1), ImPlotSubplotFlags_LinkAllX))
                //        {
                //            if (ImPlot::BeginPlot("Overlay")) {
                //                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
                //                ImPlot::PlotLine("Board 1", dataB1.data(), (int)dataB1.size());
                //                ImPlot::SetNextLineStyle(ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                //                ImPlot::PlotLine("Board 2", dataB2.data(), (int)dataB2.size());
                //                ImPlot::EndPlot();
                //            }
                //            // Cross Correlation Plot could go here if exposed
                //            ImPlot::EndSubplots();
                //        }
                //    }
                //    ImGui::EndTabItem();
                //}


                // --- TAB 3: Sync Check (Multi-Channel) ---
                    
                    if (ImGui::BeginTabItem("Sync Check"))
                    {
                        // --- 1. GUI CONTROLS ---
                        //static bool showCh[4] = { true, false, false, false }; // Default: Show Ch A only

                        //ImGui::Text("Channels:"); ImGui::SameLine();
                        //ImGui::Checkbox("Ch A", &showCh[0]); ImGui::SameLine();
                        //ImGui::Checkbox("Ch B", &showCh[1]); ImGui::SameLine();
                        //ImGui::Checkbox("Ch C", &showCh[2]); ImGui::SameLine();
                        //ImGui::Checkbox("Ch D", &showCh[3]);

                        ImGui::Text("Sync Lag Calculation Source:");

                        // Board 1 Selectors
                        ImGui::Text("B1:"); ImGui::SameLine();
                        ImGui::RadioButton("A##1", &syncSrcB1, 0); ImGui::SameLine();
                        ImGui::RadioButton("B##1", &syncSrcB1, 1); ImGui::SameLine();
                        ImGui::RadioButton("C##1", &syncSrcB1, 2); ImGui::SameLine();
                        ImGui::RadioButton("D##1", &syncSrcB1, 3);

                        // Board 2 Selectors
                        ImGui::Text("B2:"); ImGui::SameLine();
                        ImGui::RadioButton("A##2", &syncSrcB2, 0); ImGui::SameLine();
                        ImGui::RadioButton("B##2", &syncSrcB2, 1); ImGui::SameLine();
                        ImGui::RadioButton("C##2", &syncSrcB2, 2); ImGui::SameLine();
                        ImGui::RadioButton("D##2", &syncSrcB2, 3);

                        if (ImGui::Button("Run Sync Test", ImVec2(-1, 30))) {
                            if (!g_AcqController.IsAcquiring()) {
                                BoardConfig sConf = g_boardConfig;
                                sConf.sampleRateHz = 10000000.0;

                                if (acquisitionThread.joinable()) acquisitionThread.join();

                                // Capture current selection by value [=]
                                int s1 = syncSrcB1;
                                int s2 = syncSrcB2;

                                acquisitionThread = std::thread([sConf, s1, s2]() {
                                    // Pass the selected channels to the backend logic we wrote earlier
                                    g_AcqController.RunSyncTest(sConf, s1, s2);
                                    });
                            }
                        }


                        ImGui::Separator();

                        // --- 1. VISUALIZATION CONTROLS ---
                        static bool showCh[4] = { true, true, false, false }; // Default view

                        ImGui::Text("Visualize:"); ImGui::SameLine();
                        ImGui::Checkbox("Ch A", &showCh[0]); ImGui::SameLine();
                        ImGui::Checkbox("Ch B", &showCh[1]); ImGui::SameLine();
                        ImGui::Checkbox("Ch C", &showCh[2]); ImGui::SameLine();
                        ImGui::Checkbox("Ch D", &showCh[3]);

                        ImGui::Separator();

                        // --- 2. FETCH DATA & STATUS ---
                        int lastLag = g_AcqController.GetLastSyncLag();

                        // Display which channels were used for the calculation
                        ImGui::Text("Lag (B1-Ch%c vs B2-Ch%c): ", 'A' + syncSrcB1, 'A' + syncSrcB2);
                        ImGui::SameLine();

                        if (lastLag == 0) ImGui::TextColored(ImVec4(0, 1, 0, 1), "PERFECT (0)");
                        else ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFSET (%d)", lastLag);

                        // --- 3. PLOTTING ---
                        if (ImPlot::BeginSubplots("Sync Analysis", 3, 1, ImVec2(-1, -1), ImPlotSubplotFlags_LinkAllX))
                        {
                            // Standard Colors: A=Cyan, B=Orange, C=Green, D=Magenta
                            ImVec4 col[] = {
                                ImVec4(0,1,1,1), ImVec4(1,0.6f,0,1), ImVec4(0,1,0,1), ImVec4(1,0,1,1)
                            };

                            // --- PLOT BOARD 1 ---
                            if (ImPlot::BeginPlot("Board 1")) {
                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
                                for (int c = 0; c < 4; c++) {
                                    if (!showCh[c]) continue;
                                    static std::vector<float> d;
                                    g_AcqController.GetSyncSnapshotMulti(1, c, d);
                                    if (!d.empty()) {
                                        ImPlot::SetNextLineStyle(col[c]);
                                        std::string label = "Ch " + std::string(1, 'A' + c);
                                        ImPlot::PlotLine(label.c_str(), d.data(), (int)d.size());
                                    }
                                }
                                ImPlot::EndPlot();
                            }

                            // --- PLOT BOARD 2 ---
                            if (ImPlot::BeginPlot("Board 2")) {
                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
                                for (int c = 0; c < 4; c++) {
                                    if (!showCh[c]) continue;
                                    static std::vector<float> d;
                                    g_AcqController.GetSyncSnapshotMulti(2, c, d);
                                    if (!d.empty()) {
                                        ImPlot::SetNextLineStyle(col[c]);
                                        std::string label = "Ch " + std::string(1, 'A' + c);
                                        ImPlot::PlotLine(label.c_str(), d.data(), (int)d.size());
                                    }
                                }
                                ImPlot::EndPlot();
                            }

                            // --- PLOT OVERLAY (Comparison) ---
                            if (ImPlot::BeginPlot("Overlay")) {
                                ImPlot::SetupAxis(ImAxis_Y1, "Volts", ImPlotAxisFlags_AutoFit);
                                for (int c = 0; c < 4; c++) {
                                    if (!showCh[c]) continue;

                                    static std::vector<float> d1, d2;
                                    g_AcqController.GetSyncSnapshotMulti(1, c, d1);
                                    g_AcqController.GetSyncSnapshotMulti(2, c, d2);

                                    if (!d1.empty()) {
                                        ImPlot::SetNextLineStyle(col[c], 2.0f);
                                        std::string l = "B1-" + std::string(1, 'A' + c);
                                        ImPlot::PlotLine(l.c_str(), d1.data(), (int)d1.size());
                                    }
                                    if (!d2.empty()) {
                                        // Make Board 2 lines semi-transparent so overlaps are visible
                                        ImVec4 dim = col[c]; dim.w = 0.5f;
                                        ImPlot::SetNextLineStyle(dim, 2.0f);
                                        std::string l = "B2-" + std::string(1, 'A' + c);
                                        ImPlot::PlotLine(l.c_str(), d2.data(), (int)d2.size());
                                    }
                                }
                                ImPlot::EndPlot();
                            }
                            ImPlot::EndSubplots();
                        }
                        ImGui::EndTabItem();
                    }

                // --- TAB 4: Logs ---
                if (ImGui::BeginTabItem("Log")) {
                    auto logs = g_AcqController.GetLogMessages();
                    ImGui::BeginChild("LogScroll");
                    for (auto& s : logs) ImGui::TextUnformatted(s.c_str());
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();
            ImGui::End();
            // 2. Draw the Floating Image Window (Separate)
            DrawImagesWindow();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // --- SAFE EXIT ---
    if (g_AcqController.IsAcquiring() || acquisitionThread.joinable()) {
        std::cout << "Stopping Threads..." << std::endl;
        g_AcqController.StopAcquisition();
        if (acquisitionThread.joinable()) acquisitionThread.join();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// --- BOILERPLATE DX11 FUNCTIONS ---
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2; sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
    CreateRenderTarget(); return true;
}
void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}
void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
    case WM_DESTROY: ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}////////////////////////////////////////////////////////////////////