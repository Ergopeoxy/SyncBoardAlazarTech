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
//int main0(int, char**)
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