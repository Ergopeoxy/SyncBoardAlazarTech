//
//
//#include <string>
//#include <vector>
//#include <thread>
//#include <atomic>
//#include <sstream>
//#include <iostream>
//
//// --- Backend ---
//#include "AcquisitionControllerMultiThread.h" // Ensure this matches your header name
//#include "ProjectSetting.h"
//#include "IoBuffer.h"
//
//// --- ImGui ---
//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_win32.h"
//#include "imgui/imgui_impl_dx11.h"
//#include "implot/implot.h" 
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
//    // Hardware Defaults (Safe Known-Good Config)
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
//BOOL WINAPI ConsoleHandler(DWORD signal) {
//    if (signal == CTRL_CLOSE_EVENT) {
//        return TRUE;
//    }
//    return FALSE;
//}
//
//// --- Main ---
//int main1(int, char**)
//{
//    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//
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
//        ImGui::SetNextWindowPos(vp->WorkPos);
//        ImGui::SetNextWindowSize(vp->WorkSize);
//
//        // --- GUI ---
//        {
//            // Keeping your exact window flags to preserve the layout
//            ImGui::Begin("Alazar Dashboard", nullptr,
//                ImGuiWindowFlags_NoTitleBar |
//                ImGuiWindowFlags_NoResize |
//                ImGuiWindowFlags_NoMove |
//                ImGuiWindowFlags_NoCollapse |
//                ImGuiWindowFlags_NoBringToFrontOnFocus |
//                ImGuiWindowFlags_NoNavFocus
//            );
//
//            // --- CONTROLS (Left Side) ---
//            // 
//            // 
//            // 
//            
//            // --- CONTROLS (Left Side) ---
//            ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
//
//            // --------------------------------------------------------
//            // 1. DEFINE VARIABLES HERE (So they are visible everywhere)
//            // --------------------------------------------------------
//
//            // Hardware Vars
//            static double gui_clockHz = 25000000.0;
//            static int current_range = 4; // 1V default
//
//            // Buffer Logic Vars
//            static int mode = ADMA_NPT;
//            static int gui_samples = 384;
//            static int gui_recsPerBuf = 2048;
//            static int gui_totalBufs = 10000;
//            static int gui_timeout = 5000;
//            static bool save_data = true;
//
//            // --------------------------------------------------------
//            // 2. DRAW GUI
//            // --------------------------------------------------------
//
//            if (ImGui::Button("Discover Boards", ImVec2(-1, 0))) g_AcqController.DiscoverBoards();
//            ImGui::Separator();
//
//            // --- Configuration Header ---
//            if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
//            {
//                ImGui::Indent();
//                ImGui::Dummy(ImVec2(0, 5));
//
//                // A. Hardware Settings
//                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Hardware");
//
//                if (ImGui::InputDouble("Clock (Hz)", &gui_clockHz, 1000000.0, 10000000.0, "%.0f")) {
//                    g_boardConfig.sampleRateHz = gui_clockHz;
//                }
//
//                const char* ranges[] = { "+/- 40mV", "+/- 100mV", "+/- 200mV", "+/- 400mV", "+/- 1V", "+/- 2V", "+/- 4V" };
//                if (ImGui::Combo("Input Range", &current_range, ranges, IM_ARRAYSIZE(ranges))) {
//                    switch (current_range) {
//                    case 0: g_boardConfig.inputRangeId = INPUT_RANGE_PM_40_MV; g_boardConfig.inputRangeVolts = 0.04; break;
//                    case 1: g_boardConfig.inputRangeId = INPUT_RANGE_PM_100_MV; g_boardConfig.inputRangeVolts = 0.1; break;
//                    case 2: g_boardConfig.inputRangeId = INPUT_RANGE_PM_200_MV; g_boardConfig.inputRangeVolts = 0.2; break;
//                    case 3: g_boardConfig.inputRangeId = INPUT_RANGE_PM_400_MV; g_boardConfig.inputRangeVolts = 0.4; break;
//                    case 4: g_boardConfig.inputRangeId = INPUT_RANGE_PM_1_V;   g_boardConfig.inputRangeVolts = 1.0; break;
//                    case 5: g_boardConfig.inputRangeId = INPUT_RANGE_PM_2_V;   g_boardConfig.inputRangeVolts = 2.0; break;
//                    case 6: g_boardConfig.inputRangeId = INPUT_RANGE_PM_4_V;   g_boardConfig.inputRangeVolts = 4.0; break;
//                    }
//                }
//
//                int trigLvl = (int)g_boardConfig.triggerLevelCode;
//                if (ImGui::SliderInt("Trig Level", &trigLvl, 0, 255)) {
//                    g_boardConfig.triggerLevelCode = (U8)trigLvl;
//                }
//
//                if (ImGui::Button("Apply Hardware Config", ImVec2(-1, 0))) {
//                    g_AcqController.ConfigureAllBoards(g_boardConfig);
//                }
//
//                ImGui::Dummy(ImVec2(0, 10));
//
//                // B. Buffer Logic
//                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Buffers");
//
//                ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
//                ImGui::RadioButton("Stream", &mode, ADMA_CONTINUOUS_MODE);
//
//                ImGui::InputInt("Samples/Rec", &gui_samples);
//                ImGui::InputInt("Recs/Buffer", &gui_recsPerBuf);
//                ImGui::InputInt("Total Buffers", &gui_totalBufs);
//                ImGui::InputInt("Timeout (ms)", &gui_timeout);
//
//                double memMB = (double)gui_samples * gui_recsPerBuf * 2 * 4 / (1024.0 * 1024.0);
//                ImGui::TextDisabled("Buffer Size: %.2f MB", memMB);
//
//                ImGui::Checkbox("Save Data", &save_data);
//
//                ImGui::Unindent();
//            }
//            // ----------------------------------------
//
//            ImGui::Separator();
//
//            // --- Buttons ---
//            if (g_AcqController.IsAcquiring()) {
//                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
//                if (ImGui::Button("STOP ACQUISITION", ImVec2(-1, 40))) {
//                    // Stop logic handled by main loop check
//                }
//                ImGui::PopStyleColor();
//                ImGui::Text("Acquiring...");
//            }
//            else {
//                if (ImGui::Button("START ACQUISITION", ImVec2(-1, 40))) {
//                    AcquisitionConfig acq = {};
//                    acq.admaMode = mode; // <--- Now visible!
//                    acq.samplesPerRecord = gui_samples;
//                    acq.recordsPerBuffer = gui_recsPerBuf;
//                    acq.buffersPerAcquisition = gui_totalBufs;
//                    acq.saveData = save_data;
//                    acq.processData = true;
//                    acq.isSyncTest = false;
//                    acq.logInterval = 1000;
//
//                    g_boardConfig.triggerTimeoutMS = gui_timeout;
//
//                    if (acquisitionThread.joinable()) acquisitionThread.join();
//                    acquisitionThread = std::thread([acq]() { g_AcqController.RunAcquisition(acq); });
//                }
//
//                if (ImGui::Button("RUN SYNC TEST", ImVec2(-1, 30))) {
//                    if (!g_AcqController.IsAcquiring()) {
//                        AcquisitionConfig syncConf = {};
//                        syncConf.admaMode = ADMA_NPT;
//                        // Use GUI values for Sync Test too
//                        syncConf.samplesPerRecord = gui_samples;
//                        syncConf.recordsPerBuffer = gui_recsPerBuf;
//                        syncConf.buffersPerAcquisition = 100000;
//                        syncConf.saveData = false;
//                        syncConf.processData = true;
//                        syncConf.isSyncTest = true;
//                        syncConf.logInterval = 1000;
//
//                        if (acquisitionThread.joinable()) acquisitionThread.join();
//                        acquisitionThread = std::thread([syncConf]() {
//                            g_AcqController.ConfigureAllBoards(g_boardConfig);
//                            g_AcqController.RunAcquisition(syncConf);
//                            });
//                    }
//                }
//            }
//
//            ImGui::EndChild();
//
//
//            //ImGui::BeginChild("Controls", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
//
//            //if (ImGui::Button("Discover Boards", ImVec2(-1, 0))) g_AcqController.DiscoverBoards();
//            //ImGui::Separator();
//
//            //// --- ADDED: Collapsible Configuration ---
//            //if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
//            //{
//            //    ImGui::Indent();
//            //    ImGui::Dummy(ImVec2(0, 5));
//
//            //    // 1. Hardware Settings
//            //    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Hardware");
//            //    static double gui_clockHz = 25000000.0;
//            //    if (ImGui::InputDouble("Clock (Hz)", &gui_clockHz, 1000000.0, 10000000.0, "%.0f")) {
//            //        g_boardConfig.sampleRateHz = gui_clockHz;
//            //    }
//
//            //    // Trigger Level
//            //    int trigLvl = (int)g_boardConfig.triggerLevelCode;
//            //    if (ImGui::SliderInt("Trig Level", &trigLvl, 0, 255)) {
//            //        g_boardConfig.triggerLevelCode = (U8)trigLvl;
//            //    }
//
//            //    if (ImGui::Button("Apply Hardware Config", ImVec2(-1, 0))) {
//            //        g_AcqController.ConfigureAllBoards(g_boardConfig);
//            //    }
//
//            //    ImGui::Dummy(ImVec2(0, 10));
//
//            //    // 2. Buffer Settings (The numbers you asked for)
//            //    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Buffers");
//
//            //    static int mode = ADMA_NPT;
//            //    ImGui::RadioButton("NPT", &mode, ADMA_NPT); ImGui::SameLine();
//            //    ImGui::RadioButton("Stream", &mode, ADMA_CONTINUOUS_MODE);
//
//            //    // Initialize with SAFE values for 160kHz (from our calculation)
//            //    static int gui_samples = 384;     // Buffer: 384
//            //    static int gui_recsPerBuf = 2048; // To handle 160kHz rate safely
//            //    static int gui_totalBufs = 10000;
//            //    static int gui_timeout = 5000;
//
//            //    ImGui::InputInt("Samples/Rec", &gui_samples);
//            //    ImGui::InputInt("Recs/Buffer", &gui_recsPerBuf);
//            //    ImGui::InputInt("Total Buffers", &gui_totalBufs);
//            //    ImGui::InputInt("Timeout (ms)", &gui_timeout);
//
//            //    // Memory Calc
//            //    double memMB = (double)gui_samples * gui_recsPerBuf * 2 * 4 / (1024.0 * 1024.0);
//            //    ImGui::TextDisabled("Buffer Size: %.2f MB", memMB);
//
//            //    static bool save_data = true;
//            //    ImGui::Checkbox("Save Data", &save_data);
//
//            //    ImGui::Unindent();
//            //}
//            //// ----------------------------------------
//
//            //ImGui::Separator();
//
//            //// --- Buttons ---
//            //if (g_AcqController.IsAcquiring()) {
//            //    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
//            //    if (ImGui::Button("STOP ACQUISITION", ImVec2(-1, 40))) {
//            //        // Stop logic handled by main loop check
//            //    }
//            //    ImGui::PopStyleColor();
//            //    ImGui::Text("Acquiring...");
//            //}
//            //else {
//            //    if (ImGui::Button("START ACQUISITION", ImVec2(-1, 40))) {
//            //        AcquisitionConfig acq = {};
//            //        acq.admaMode = mode;
//
//            //        // USE GUI VALUES HERE
//            //        acq.samplesPerRecord = gui_samples;
//            //        acq.recordsPerBuffer = gui_recsPerBuf;
//            //        acq.buffersPerAcquisition = gui_totalBufs;
//
//            //        acq.saveData = save_data;
//            //        acq.processData = true;
//            //        acq.isSyncTest = false;
//            //        acq.logInterval = 1000;
//
//            //        // Update timeout globally
//            //        g_boardConfig.triggerTimeoutMS = gui_timeout;
//
//            //        if (acquisitionThread.joinable()) acquisitionThread.join();
//            //        acquisitionThread = std::thread([acq]() { g_AcqController.RunAcquisition(acq); });
//            //    }
//
//            //    if (ImGui::Button("RUN SYNC TEST", ImVec2(-1, 30))) {
//            //        if (!g_AcqController.IsAcquiring()) {
//            //            AcquisitionConfig syncConf = {};
//            //            syncConf.admaMode = ADMA_NPT;
//
//            //            // Use GUI values for Sync Test too (important for stability)
//            //            syncConf.samplesPerRecord = gui_samples;
//            //            syncConf.recordsPerBuffer = gui_recsPerBuf;
//            //            syncConf.buffersPerAcquisition = 100000; // Continuous
//
//            //            syncConf.saveData = false;
//            //            syncConf.processData = true;
//            //            syncConf.isSyncTest = true;
//            //            syncConf.logInterval = 1000;
//
//            //            if (acquisitionThread.joinable()) acquisitionThread.join();
//            //            acquisitionThread = std::thread([syncConf]() {
//            //                g_AcqController.ConfigureAllBoards(g_boardConfig);
//            //                g_AcqController.RunAcquisition(syncConf);
//            //                });
//            //        }
//            //    }
//            //}
//
//            //ImGui::EndChild();
//            //ImGui::SameLine();
//
//            // --- VISUALIZATION (Right Side) ---
//            ImGui::BeginChild("Viz", ImVec2(0, 0), true);
//
//            if (ImGui::BeginTabBar("Tabs")) {
//
//                // Tab 1: Live Scope
//                if (ImGui::BeginTabItem("Live Scope")) {
//                    static int selectedBoard = 1;
//                    ImGui::Text("Select Board:"); ImGui::SameLine();
//                    ImGui::RadioButton("Board 1", &selectedBoard, 1); ImGui::SameLine();
//                    ImGui::RadioButton("Board 2", &selectedBoard, 2);
//
//                    static std::vector<float> liveData;
//                    g_AcqController.GetLatestScopeData(liveData, (U32)selectedBoard);
//
//                    if (!liveData.empty() && ImPlot::BeginPlot("Live Signal")) {
//                        std::string lbl = "Board " + std::to_string(selectedBoard);
//                        ImPlot::PlotLine(lbl.c_str(), liveData.data(), (int)liveData.size());
//                        ImPlot::EndPlot();
//                    }
//                    else {
//                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for data...");
//                    }
//                    ImGui::EndTabItem();
//                }
//
//                // Tab 2: Sync Check
//                if (ImGui::BeginTabItem("Sync Check")) {
//                    static std::vector<float> dataB1, dataB2;
//                    static int lastLag = 0;
//
//                    g_AcqController.GetSyncSnapshot(dataB1, dataB2);
//                    lastLag = g_AcqController.GetLastSyncLag();
//
//                    ImGui::Text("Sync Status: "); ImGui::SameLine();
//                    if (lastLag == 0 && !dataB1.empty())
//                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "PERFECT (Lag: 0)");
//                    else if (dataB1.empty())
//                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "No Data");
//                    else
//                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFSET (Lag: %d)", lastLag);
//
//                    if (!dataB1.empty() && !dataB2.empty()) {
//                        if (ImPlot::BeginSubplots("Sync Analysis", 3, 1, ImVec2(-1, -1), ImPlotSubplotFlags_LinkAllX)) {
//                            if (ImPlot::BeginPlot("Board 1")) {
//                                ImPlot::PlotLine("B1", dataB1.data(), (int)dataB1.size());
//                                ImPlot::EndPlot();
//                            }
//                            if (ImPlot::BeginPlot("Board 2")) {
//                                ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 0, 1));
//                                ImPlot::PlotLine("B2", dataB2.data(), (int)dataB2.size());
//                                ImPlot::EndPlot();
//                            }
//                            if (ImPlot::BeginPlot("Overlay")) {
//                                ImPlot::PlotLine("B1", dataB1.data(), (int)dataB1.size());
//                                ImPlot::SetNextLineStyle(ImVec4(1, 0.5f, 0, 1));
//                                ImPlot::PlotLine("B2", dataB2.data(), (int)dataB2.size());
//                                ImPlot::EndPlot();
//                            }
//                            ImPlot::EndSubplots();
//                        }
//                    }
//                    ImGui::EndTabItem();
//                }
//
//                // Tab 3: Log
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
//        // --- RENDER ---
//        ImGui::Render();
//        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
//        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
//        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
//        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
//        g_pSwapChain->Present(1, 0);
//    }
//
//    // --- SAFE EXIT ---
//    if (g_AcqController.IsAcquiring() || acquisitionThread.joinable()) {
//        if (acquisitionThread.joinable()) {
//            std::cout << "Waiting for acquisition thread to stop..." << std::endl;
//            acquisitionThread.join();
//        }
//    }
//
//    ImGui_ImplDX11_Shutdown();
//    ImGui_ImplWin32_Shutdown();
//    ImPlot::DestroyContext();
//    ImGui::DestroyContext();
//    CleanupDeviceD3D();
//    ::DestroyWindow(hwnd);
//    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
//
//    return 0;
//}
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
//    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//    D3D_FEATURE_LEVEL featureLevel;
//    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
//    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
//    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
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
//        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
//        {
//            CleanupRenderTarget();
//            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
//            CreateRenderTarget();
//        }
//        return 0;
//    case WM_SYSCOMMAND:
//        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
//            return 0;
//        break;
//    case WM_DESTROY:
//        ::PostQuitMessage(0);
//        return 0;
//    }
//    return ::DefWindowProc(hWnd, msg, wParam, lParam);
//}