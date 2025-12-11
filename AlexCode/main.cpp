#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../implot/implot.h"
#include <tchar.h>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "parsguiconfig.h"
#include "dataThread.h"
#include "saveThread.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <iostream>

#include <vector>
#include <algorithm>
#include <queue>

#include <math.h>
#include <random>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device* g_pd3dDevice = NULL;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = NULL;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
static ID3D12CommandQueue* g_pd3dCommandQueue = NULL;
static ID3D12GraphicsCommandList* g_pd3dCommandList = NULL;
static ID3D12Fence* g_fence = NULL;
static HANDLE                       g_fenceEvent = NULL;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = NULL;
static HANDLE                       g_hSwapChainWaitableObject = NULL;
static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

// Processing Threads
bool runCh1ProcessingThread();
bool runCh2ProcessingThread();
bool runCh3ProcessingThread();
bool runCh4ProcessingThread();

// Line Threads
bool runLineDetectorThread();
bool runLineGeneratorThread();

// GUI Thread
bool runGUIThread();

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global variables
// General use variables
double	x_samples[NUMBER_OF_SAMPLES_PER_SEGMENT],
        x_segments[NUMBER_OF_SEGMENTS_PER_BUFFER],
        x_cb_segments[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER],
        x_line_samples[NUMBER_OF_POINTS_PER_LINE],
        x_time_samples[NUMBER_OF_PROCESSING_TIME_SAMPLES];

// raw td channel variables
int16_t     channel1_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
            channel2_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
            channel3_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT],
            channel4_V[NUMBER_OF_SEGMENTS_PER_BUFFER][NUMBER_OF_SAMPLES_PER_SEGMENT];
uint64_t	timestamps[NUMBER_OF_SEGMENTS_PER_BUFFER];

int time_index;
long buffer_index;

// 
std::mutex data_mutex;
std::condition_variable data_cv;
std::atomic<bool>	ch1_data_ready = false,
                    ch2_data_ready = false,
                    ch3_data_ready = false,
                    ch4_data_ready = false;
std::atomic<int> channel_read_count = 0;



// sig extraction variables
static double sig_dsc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], sig_nr_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], sig_esc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], sig_m1_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], sig_m2_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER];
static int sig_dsc_avg_region[2], sig_nr_avg_region[2], sig_esc_avg_region[2], sig_m1_avg_region[2], sig_m2_avg_region[2];

std::atomic<int> s_dsc_cb_w, s_nr_cb_w, s_esc_cb_w, s_m1_cb_w, s_m2_cb_w;

// data acqusition timing variables
double data_deinterlacing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
double avg_data_deinterlacing_time;

double data_acquisition_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
double avg_data_acquisition_time;

// td timing variables
static double channel1_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], channel2_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], channel3_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], channel4_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
static double channel1_avg_processing_time, channel2_avg_processing_time, channel3_avg_processing_time, channel4_avg_processing_time;

// sig timing variables
static double sig_dsc_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], sig_nr_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], sig_esc_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], sig_m1_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES], sig_m2_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
static double sig_dsc_avg_processing_time, sig_nr_avg_processing_time, sig_esc_avg_processing_time, sig_m1_avg_processing_time, sig_m2_avg_processing_time;

// save timing variables
double save_time[100], avg_save_time;


// sig gui variables
std::atomic<int> gui_s_cb_r;

// td gui channel variables
static double sig_channel1[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], sig_channel2[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], sig_channel3[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], sig_channel4[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT];
static int numberOfTDsToAverage_ch1 = 200, numberOfTDsToAverage_ch2 = NUMBER_OF_SEGMENTS_PER_BUFFER, num_TD_channel3 = 5, num_TD_channel4 = 5;
static bool ch1_axis_bool = true, ch2_axis_bool = false, ch3_axis_bool = false, ch4_axis_bool = false;
static bool td_ch1_sort_bool = false, td_ch1_sort_posneg_bool = false, td_ch1_rectify_bool = false;
static double percentile = 0.1;

// Signal Thread control variables
bool data_ready, buffers_ready;
bool gui_running = true;
bool sig_dsc_buffer_ready, sig_nr_buffer_ready, sig_esc_buffer_ready, sig_m1_buffer_ready, sig_m2_buffer_ready;
std::atomic<int> sig_ready_counter = 0;

// Line Thread control variables
std::mutex sig_mutex;
std::condition_variable sig_cv;
bool sig_ready = false;
double peak_detector_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
double peak_detector_avg_processing_time;

// Line compiler control variables
static std::mutex line_mutex;
static std::condition_variable line_cv;
static bool line_ready;
double line_generator_processing_time[NUMBER_OF_PROCESSING_TIME_SAMPLES];
double line_generator_avg_processing_time;


//
// Alazartech code
bool running;

// Board configuration parameters
double samplesPerSec = 0.0;
uint32_t preTriggerSamples, postTriggerSamples, recordsPerBuffer, buffersPerAcquisition, bytesPerBuffer;
uint16_t* BufferArray[2][BUFFER_COUNT] = { NULL };

 // Save thread variables
 std::atomic<bool> save_bool = false;
 std::atomic<bool>	ch_data_ready = false;

// Main code
int main(int, char**)
{
    std::thread gui_thread (runGUIThread);
    SetThreadPriority(gui_thread.native_handle(),1);
    //std::thread ch1_processing_thread(runCh1ProcessingThread);
    //SetThreadPriority(ch1_processing_thread.native_handle(), 5);
    //std::thread ch2_processing_thread(runCh2ProcessingThread);
    //SetThreadPriority(ch2_processing_thread.native_handle(), 5);
    //std::thread ch3_processing_thread(runCh3ProcessingThread);
    //SetThreadPriority(ch3_processing_thread.native_handle(), 5);
    //std::thread ch4_processing_thread(runCh4ProcessingThread);
    //SetThreadPriority(ch4_processing_thread.native_handle(), 5);
    //std::thread line_thread(runLineDetectorThread);
    //SetThreadPriority(runLineDetectorThread, 2);
    //std::thread line_compiler_thread(runLineGeneratorThread);
    //SetThreadPriority(runLineGeneratorThread, 2);

    std::thread save_thread(runSaveThread);
    SetThreadPriority(save_thread.native_handle(), 15);

    std::thread data_thread(runDataThread);
    SetThreadPriority(data_thread.native_handle(), 10);

    gui_thread.join();
    //ch1_processing_thread.join();
    //ch2_processing_thread.join();
    //ch3_processing_thread.join();
    //ch4_processing_thread.join();
    //line_thread.join();
    //line_compiler_thread.join();
    save_thread.join();
    data_thread.join();

    return 0;
}

// Functions for processing threads
template <typename A, typename B>
void zip(
    const std::vector<A>& a,
    const std::vector<B>& b,
    std::vector<std::pair<A, B>>& zipped)
{
    for (size_t i = 0; i < a.size(); ++i)
    {
        zipped.push_back(std::make_pair(a[i], b[i]));
    }
}

template <typename A, typename B>
void unzip(
    const std::vector<std::pair<A, B>>& zipped,
    std::vector<A>& a,
    std::vector<B>& b)
{
    for (size_t i = 0; i < a.size(); i++)
    {
        a[i] = zipped[i].first;
        b[i] = zipped[i].second;
    }
}

/////////////////////////////////////////////////////////
// Signal and time domain extraction 

bool runCh1ProcessingThread() {
    static double temp_sig_channel1[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT],temp_sig_dsc[NUMBER_OF_SEGMENTS_PER_BUFFER], temp_sig_nr[NUMBER_OF_SEGMENTS_PER_BUFFER];
    s_dsc_cb_w = 0;
    s_nr_cb_w = 0;
    
    sig_dsc_buffer_ready = false;
    sig_nr_buffer_ready = false;

    sig_dsc_avg_region[0] = 300; sig_dsc_avg_region[1] = 380;
    sig_nr_avg_region[0] = 380; sig_nr_avg_region[1] = 500;

    while (gui_running) {

        std::unique_lock<std::mutex> ul(data_mutex);
        data_cv.wait(ul, []() { return ch1_data_ready.load(); });

        channel_read_count++;

        ul.unlock();
        memset(temp_sig_channel1, 0, sizeof(temp_sig_channel1));
        memset(temp_sig_dsc, 0, sizeof(temp_sig_dsc));
        memset(temp_sig_nr, 0, sizeof(temp_sig_nr));

        /////////////////////////////////////////////////////////////////////////////////
        // extract signal dsc
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = sig_dsc_avg_region[0]; j < sig_dsc_avg_region[1]; j++) {
                temp_sig_dsc[i] += channel1_V[i][j];
            }
            temp_sig_dsc[i] /= (sig_dsc_avg_region[1]-sig_dsc_avg_region[0]);
        }
        
        memmove(&sig_dsc_circbuffer[s_dsc_cb_w * NUMBER_OF_SEGMENTS_PER_BUFFER], &temp_sig_dsc, sizeof(temp_sig_dsc));
        s_dsc_cb_w = (s_dsc_cb_w + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
        sig_dsc_buffer_ready = true;

        std::unique_lock < std::mutex> ul_dsc(sig_mutex);
        sig_ready_counter++;
        if (sig_ready_counter == 5) {
            if (s_dsc_cb_w == 0) {
                gui_s_cb_r = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER - 1;
            }
            else {
                gui_s_cb_r = (s_dsc_cb_w - 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

            }
            sig_ready_counter = 0;
            sig_ready = true;
            ul_dsc.unlock();
            sig_cv.notify_all();
        }
        else {
            ul_dsc.unlock();
        }

        auto end = std::chrono::high_resolution_clock::now();
        sig_dsc_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            sig_dsc_avg_processing_time += sig_dsc_processing_time[i];
        }
        sig_dsc_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        ///////////////////////////////////////////////////////////////////
        // extract signal nr
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = sig_nr_avg_region[0]; j < sig_nr_avg_region[1]; j++) {
                temp_sig_nr[i] += channel1_V[i][j];
            }
            temp_sig_nr[i] /= sig_nr_avg_region[1]- sig_nr_avg_region[0];
            temp_sig_nr[i] -= temp_sig_dsc[i];
            //temp_sig_nr[i] /= temp_sig_dsc[i];
        }

        memmove(&sig_nr_circbuffer[s_nr_cb_w * NUMBER_OF_SEGMENTS_PER_BUFFER], &temp_sig_nr, sizeof(temp_sig_nr));
        s_nr_cb_w = (s_nr_cb_w + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
        sig_nr_buffer_ready = true;


        std::unique_lock < std::mutex> ul_nr(sig_mutex);
        sig_ready_counter++;

        if (sig_ready_counter == 5) {
            if (s_nr_cb_w == 0) {
                gui_s_cb_r = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER - 1;
            }
            else {
                gui_s_cb_r = (s_nr_cb_w - 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

            }            
            
            sig_ready_counter = 0;
            sig_ready = true;
            ul_nr.unlock();
            sig_cv.notify_all();
        }
        else {
            ul_nr.unlock();
        }

        end = std::chrono::high_resolution_clock::now();
        sig_nr_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            sig_nr_avg_processing_time += sig_nr_processing_time[i];
        }
        sig_nr_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        ////////////////////////////////////////////////////////////////////////////
        // GUI time domain extraction
        start = std::chrono::high_resolution_clock::now();
        if (ch1_axis_bool) {
            if (td_ch1_sort_bool) {
                if (td_ch1_rectify_bool) {
                    double temp_sig_nr_abs[NUMBER_OF_SEGMENTS_PER_BUFFER];
                    for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
                        if (temp_sig_nr[i] < 0) {
                            temp_sig_nr_abs[i] = -1 * temp_sig_nr[i];

                        }
                        else {
                            temp_sig_nr_abs[i] = temp_sig_nr[i];
                        }
                    }
                    std::vector<double> nr_vector(temp_sig_nr_abs, temp_sig_nr_abs + NUMBER_OF_SEGMENTS_PER_BUFFER);
                    std::vector<double> index_vector(x_segments, x_segments + NUMBER_OF_SEGMENTS_PER_BUFFER);
                    std::vector< std::pair< double, double > > data_vector;

                    zip(nr_vector, index_vector, data_vector);
                    std::sort(data_vector.begin(), data_vector.end(), [&](const auto& a, const auto& b) { return a.first > b.first; });
                    unzip(data_vector, nr_vector, index_vector);

                    for (int i = 0; i < int(NUMBER_OF_SEGMENTS_PER_BUFFER * percentile); i++) {
                        for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                            if (temp_sig_nr[int(index_vector[i])] < 0) {
                                temp_sig_channel1[0][j] -= (channel1_V[int(index_vector[i])][j] - temp_sig_dsc[int(index_vector[i])]);
                            }
                            else {
                                temp_sig_channel1[0][j] += (channel1_V[int(index_vector[i])][j] - temp_sig_dsc[int(index_vector[i])]);
                            }
                        }
                    }
                    for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
                        temp_sig_channel1[0][i] /= (NUMBER_OF_SEGMENTS_PER_BUFFER * percentile);
                    }

                }
                else {
                    std::vector<double> nr_vector(temp_sig_nr, temp_sig_nr + NUMBER_OF_SEGMENTS_PER_BUFFER);
                    std::vector<double> index_vector(x_segments, x_segments + NUMBER_OF_SEGMENTS_PER_BUFFER);
                    std::vector< std::pair< double, double > > data_vector;

                    zip(nr_vector, index_vector, data_vector);
                    std::sort(data_vector.begin(), data_vector.end(), [&](const auto& a, const auto& b) { return a.first > b.first; });
                    unzip(data_vector, nr_vector, index_vector);


                    for (int i = 0; i < int(NUMBER_OF_SEGMENTS_PER_BUFFER * percentile); i++) {
                        for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                            temp_sig_channel1[0][j] += (channel1_V[int(index_vector[i])][j] - temp_sig_dsc[int(index_vector[i])]);
                        }
                    }
                    for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
                        temp_sig_channel1[0][i] /= (NUMBER_OF_SEGMENTS_PER_BUFFER * percentile);
                    }
                    if (td_ch1_sort_posneg_bool) {
                        for (int i = NUMBER_OF_SEGMENTS_PER_BUFFER - 1; i > int(NUMBER_OF_SEGMENTS_PER_BUFFER * (1 - percentile)); i--) {
                            for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                                temp_sig_channel1[1][j] += (channel1_V[int(index_vector[i])][j] - temp_sig_dsc[int(index_vector[i])]);
                            }
                        }
                        for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
                            temp_sig_channel1[1][i] /= (NUMBER_OF_SEGMENTS_PER_BUFFER * percentile);
                        }
                    }
                }

            }
            else {
                for (int N = 0; N < NUMBER_OF_SEGMENTS_PER_BUFFER / numberOfTDsToAverage_ch1; N++) {
                    for (int i = 0 + (numberOfTDsToAverage_ch1)*N; i < (numberOfTDsToAverage_ch1) * (N + 1); i++) {
                        for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                            temp_sig_channel1[N][j] += double(channel1_V[i][j]) - temp_sig_dsc[i];
                        }
                    }
                    for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                        temp_sig_channel1[N][j] /= double(numberOfTDsToAverage_ch1);
                    }
                }
            }
            memmove(&sig_channel1, &temp_sig_channel1, sizeof(temp_sig_channel1));
       }

        end = std::chrono::high_resolution_clock::now();
        channel1_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            channel1_avg_processing_time += channel1_processing_time[i];
        }
        channel1_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
       
        ch1_data_ready = false;
    }
    return 0;
}

bool runCh2ProcessingThread() {
    static double temp_sig_channel2[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], temp_sig_esc[NUMBER_OF_SEGMENTS_PER_BUFFER];
    s_esc_cb_w =  0;

    sig_esc_buffer_ready = false;

    sig_esc_avg_region[0] = 380; sig_esc_avg_region[1] = 450;

    while (gui_running) {
        std::unique_lock<std::mutex> ul(data_mutex);
        data_cv.wait(ul, []() { return ch2_data_ready.load(); });
        channel_read_count++;
        ul.unlock();

        memset(temp_sig_channel2, 0, sizeof(temp_sig_channel2));
        memset(temp_sig_esc, 0, sizeof(temp_sig_esc));

        ////////////////////////////////////////////////////////////////////////////
        // extract sig esc
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = sig_esc_avg_region[0]; j < sig_esc_avg_region[1]; j++) {
                temp_sig_esc[i] += channel2_V[i][j];
            }
            temp_sig_esc[i] /= (sig_esc_avg_region[1] - sig_esc_avg_region[0]);
        }

        memmove(&sig_esc_circbuffer[s_esc_cb_w * NUMBER_OF_SEGMENTS_PER_BUFFER], &temp_sig_esc, sizeof(temp_sig_esc));
        s_esc_cb_w = (s_esc_cb_w + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
        sig_esc_buffer_ready = true;


        std::unique_lock < std::mutex> ul_esc(sig_mutex);
        sig_ready_counter++;

        if (sig_ready_counter == 5) {
            if (s_esc_cb_w == 0) {
                gui_s_cb_r = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER - 1;
            }
            else {
                gui_s_cb_r = (s_esc_cb_w - 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

            }
            sig_ready_counter = 0;
            sig_ready = true;
            ul_esc.unlock();
            sig_cv.notify_all();
        }
        else {
            ul_esc.unlock();
        }

        auto end = std::chrono::high_resolution_clock::now();
        sig_esc_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            sig_esc_avg_processing_time += sig_esc_processing_time[i];
        }
        sig_esc_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        ///////////////////////////////////////////////////////////////////////////
        // GUI time domain extraction
        start = std::chrono::high_resolution_clock::now();
        if (ch2_axis_bool) {
            for (int N = 0; N < NUMBER_OF_SEGMENTS_PER_BUFFER / numberOfTDsToAverage_ch2; N++) {
                for (int i = 0 + (numberOfTDsToAverage_ch2)*N; i < (numberOfTDsToAverage_ch2) * (N + 1); i++) {
                    for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                        temp_sig_channel2[N][j] += channel2_V[i][j];
                    }
                }
                for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                    temp_sig_channel2[N][j] /= numberOfTDsToAverage_ch2;
                }
            }
            memmove(&sig_channel2, &temp_sig_channel2, sizeof(temp_sig_channel2));
        }

        end = std::chrono::high_resolution_clock::now();
        channel2_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            channel2_avg_processing_time += channel2_processing_time[i];
        }
        channel2_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        ch2_data_ready = false;
    }
    return 0;
}

bool runCh3ProcessingThread() {
    static double temp_sig_channel3[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], temp_sig_m1[NUMBER_OF_SEGMENTS_PER_BUFFER];
    s_m1_cb_w = 0;

    sig_m1_buffer_ready = false;

    sig_m1_avg_region[0] = 0; sig_m1_avg_region[1] = 256;

    while (gui_running) {
        std::unique_lock<std::mutex> ul(data_mutex);
        data_cv.wait(ul, []() { return ch3_data_ready.load(); });
        channel_read_count++;

        ul.unlock();

        memset(temp_sig_channel3, 0, sizeof(temp_sig_channel3));
        memset(temp_sig_m1, 0, sizeof(temp_sig_m1));

        /////////////////////////////////////////////////////////////////////////
        // extract m1
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = sig_m1_avg_region[0]; j < sig_m1_avg_region[1]; j++) {
                temp_sig_m1[i] += channel3_V[i][j];
            }
            temp_sig_m1[i] /= (sig_m1_avg_region[1] - sig_m1_avg_region[0]);
        }

        memmove(&sig_m1_circbuffer[(s_m1_cb_w) * NUMBER_OF_SEGMENTS_PER_BUFFER], &temp_sig_m1, sizeof(temp_sig_m1));
        s_m1_cb_w = (s_m1_cb_w + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
        sig_m1_buffer_ready = true;

        std::unique_lock < std::mutex> ul_m1(sig_mutex);
        sig_ready_counter++;
        if (sig_ready_counter == 5) {
            if (s_m1_cb_w == 0) {
                gui_s_cb_r = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER - 1;
            }
            else {
                gui_s_cb_r = (s_m1_cb_w - 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

            }

            sig_ready_counter = 0;
            sig_ready = true;
            ul_m1.unlock();
            sig_cv.notify_all();
        }
        else {
            ul_m1.unlock();
        }

        auto end = std::chrono::high_resolution_clock::now();
        sig_m1_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            sig_m1_avg_processing_time += sig_m1_processing_time[i];
        }
        sig_m1_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        /////////////////////////////////////////////////////////////////////////////////////
        // GUI time domain extraction
        start = std::chrono::high_resolution_clock::now();
        if (ch3_axis_bool) {
            for (int N = 0; N < num_TD_channel3; N++) {
                for (int i = 0 + (NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel3) * N; i < (NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel3) * (N + 1); i++) {
                    for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                        temp_sig_channel3[N][j] += channel3_V[i][j];
                    }
                }
                for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                    temp_sig_channel3[N][j] /= NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel3;
                }
            }
            memmove(&sig_channel3, &temp_sig_channel3, sizeof(temp_sig_channel3));
        }


        end = std::chrono::high_resolution_clock::now();
        channel3_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            channel3_avg_processing_time += channel3_processing_time[i];
        }
        channel3_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

        ch3_data_ready = false;
    }
    return 0;
}

bool runCh4ProcessingThread() {
    static double temp_sig_channel4[NUMBER_OF_TD_SIGNALS][NUMBER_OF_SAMPLES_PER_SEGMENT], temp_sig_m2[NUMBER_OF_SEGMENTS_PER_BUFFER];
    s_m2_cb_w = 0;
    sig_m2_buffer_ready = false;

    sig_m2_avg_region[0] = 0; sig_m2_avg_region[1] = 256;


    while (gui_running) {
        std::unique_lock<std::mutex> ul(data_mutex);
        data_cv.wait(ul, []() { return ch4_data_ready.load(); });
        channel_read_count++;

        ul.unlock();

        memset(temp_sig_channel4, 0, sizeof(temp_sig_channel4));
        memset(temp_sig_m2, 0, sizeof(temp_sig_m2));

        /////////////////////////////////////////////////////////////
        // extract m2
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = sig_m2_avg_region[0]; j < sig_m2_avg_region[1]; j++) {
                temp_sig_m2[i] += channel4_V[i][j];
            }
            temp_sig_m2[i] /= (sig_m2_avg_region[1]- sig_m2_avg_region[0]);
        }


        memmove(&sig_m2_circbuffer[s_m2_cb_w * NUMBER_OF_SEGMENTS_PER_BUFFER], &temp_sig_m2, sizeof(temp_sig_m2));
        s_m2_cb_w = (s_m2_cb_w + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

        std::unique_lock < std::mutex> ul_m2(sig_mutex);
        sig_ready_counter++;

        if (sig_ready_counter == 5) {
            if (s_m1_cb_w == 0) {
                gui_s_cb_r = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER - 1;
            }
            else {
                gui_s_cb_r = (s_m1_cb_w - 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;

            }

            sig_ready_counter = 0;
            sig_ready = true;
            ul_m2.unlock();
            sig_cv.notify_all();
        }
        else {
            ul_m2.unlock();
        }

        auto end = std::chrono::high_resolution_clock::now();
        sig_m2_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            sig_m2_avg_processing_time += sig_m2_processing_time[i];
        }
        sig_m2_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
        sig_m2_buffer_ready = true;


        // Average buffer into number of sections set by user
        start = std::chrono::high_resolution_clock::now();
        if (ch4_axis_bool) {
            for (int N = 0; N < num_TD_channel4; N++) {
                for (int i = 0 + (NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel4) * N; i < (NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel4) * (N + 1); i++) {
                    for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                        temp_sig_channel4[N][j] += channel4_V[i][j];
                    }
                }
                for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                    temp_sig_channel4[N][j] /= NUMBER_OF_SEGMENTS_PER_BUFFER / num_TD_channel4;
                }
            }
            memmove(&sig_channel4, &temp_sig_channel4, sizeof(temp_sig_channel4));
        }

        end = std::chrono::high_resolution_clock::now();
        channel4_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            channel4_avg_processing_time += channel4_processing_time[i];
        }
        channel4_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;


        ch4_data_ready = false;
    }
    return 0;
}


///////////////////////////////////////////////////////////
//// Line detector thread
const int NUMBER_OF_LINE_PARAMS = 100;
struct line {
    INT64 start[NUMBER_OF_LINE_PARAMS], end[NUMBER_OF_LINE_PARAMS];
    int leftright[NUMBER_OF_LINE_PARAMS];
    int updown[NUMBER_OF_LINE_PARAMS];
    double mean_m2[NUMBER_OF_LINE_PARAMS];
};

static line line_params_circbuffer;

// global variables for leftright detection
static bool top_peak_found, bot_peak_found;
static INT64 top_peak_position, bot_peak_position;
static int leftright;
static std::atomic<int> s_m1_cb_lp_r = 0; 
static std::atomic<int> lp_cb_lr_w = 0;



// global variables for updown detection
static std::atomic<int> lp_cb_ud_r = 0;
static bool run_updown_detect = false;
static int run_updown_detect_counter = 0;
static int updown_detect_index_lag = 5;

// global variables for GUI reading
static std::atomic<int> gui_lp_r = 0;

bool runLineDetectorThread() {
    // line param struct setup
    memset(line_params_circbuffer.leftright, -1, sizeof(line_params_circbuffer.leftright));
    memset(line_params_circbuffer.updown, -1, sizeof(line_params_circbuffer.updown));
    memset(line_params_circbuffer.mean_m2, 0, sizeof(line_params_circbuffer.mean_m2));

    // local variables for leftright detection
    top_peak_found = false;
    bot_peak_found = false;
    const int LR_SEARCH_AREA = 10;
    static double temp_ld_sig_m1[2 * LR_SEARCH_AREA + NUMBER_OF_SEGMENTS_PER_BUFFER];

    // local variables for updown detection
    static double temp_sig_m2_array[10000 + 1]; // set to a size much larger than the maximum length of a line in segments (ie. expecting ~1024 as max so set to 10000 cause why not memory is cheap)

    // local variables for notifying
    static bool new_lines = false;

    static bool first_buffer = true;

    while (gui_running) {
        std::unique_lock<std::mutex> ul(sig_mutex);
        sig_cv.wait(ul, []() { return sig_ready; });
        sig_ready = false;
        ul.unlock();

        auto start = std::chrono::high_resolution_clock::now();

        // find line segements and fill leftright buffer

        memmove(&temp_ld_sig_m1, &temp_ld_sig_m1[NUMBER_OF_SEGMENTS_PER_BUFFER], sizeof(double) * (2 * LR_SEARCH_AREA));
        memmove(&temp_ld_sig_m1[2 * LR_SEARCH_AREA], &sig_m1_circbuffer[(s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER)], sizeof(temp_ld_sig_m1) - sizeof(double) * (2 * LR_SEARCH_AREA));

        bool peak_detected;
        for (int i = LR_SEARCH_AREA; i < NUMBER_OF_SEGMENTS_PER_BUFFER + LR_SEARCH_AREA; i++) {
            if (first_buffer) {
                i += (2 * LR_SEARCH_AREA);
                first_buffer = false;
            }
            peak_detected = true;
            if (!top_peak_found) {
                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
                    if (temp_ld_sig_m1[i] < temp_ld_sig_m1[i + j]) {
                        peak_detected = false;
                        break;
                    }
                }

                if (peak_detected) {

                    top_peak_position = i + s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
                    if (top_peak_position < 0) {
                        top_peak_position += (NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }

                    top_peak_found = true;
                    leftright = 0;

                    if (bot_peak_found) {
                        line_params_circbuffer.start[lp_cb_lr_w] = bot_peak_position;
                        line_params_circbuffer.end[lp_cb_lr_w] = top_peak_position;
                        line_params_circbuffer.leftright[lp_cb_lr_w] = leftright;

                        lp_cb_lr_w = (lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
                    }
                    bot_peak_found = false;
                }
            }
            else {
                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
                    if (temp_ld_sig_m1[i] > temp_ld_sig_m1[i + j]) {
                        peak_detected = false;
                        break;
                    }
                }
                if (peak_detected) {

                    bot_peak_position = i + s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
                    if (bot_peak_position < 0) {
                        bot_peak_position += (NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }
                    bot_peak_found = true;
                    leftright = 1;

                    line_params_circbuffer.start[lp_cb_lr_w] = top_peak_position;
                    line_params_circbuffer.end[lp_cb_lr_w] = bot_peak_position;
                    line_params_circbuffer.leftright[lp_cb_lr_w] = leftright;

                    lp_cb_lr_w = (lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;

                    top_peak_found = false;
                }
            }

        }
        s_m1_cb_lp_r = (s_m1_cb_lp_r + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;


        // calculate average of m2 and fill updown buffer
        int temp_lp_lr_write_index = lp_cb_lr_w;
        if (lp_cb_ud_r > temp_lp_lr_write_index) {
            temp_lp_lr_write_index += NUMBER_OF_LINE_PARAMS;
        }
        
        if (lp_cb_ud_r < temp_lp_lr_write_index) {
            new_lines = true;
        }
        

        for (int i = lp_cb_ud_r; i < temp_lp_lr_write_index; i++) {
            int i_mod = i % NUMBER_OF_LINE_PARAMS;
            // grab data
            memset(temp_sig_m2_array, 0, sizeof(temp_sig_m2_array));
            if (line_params_circbuffer.start[i_mod] > line_params_circbuffer.end[i_mod]) {
                temp_sig_m2_array[0] = (line_params_circbuffer.end[i_mod] + NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER) - line_params_circbuffer.start[i_mod];
                UINT64 temp_segs_to_end = NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - line_params_circbuffer.start[i_mod];
                memmove(&temp_sig_m2_array[1], &sig_m2_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * temp_segs_to_end);
                memmove(&temp_sig_m2_array[1 + temp_segs_to_end], &sig_m2_circbuffer, sizeof(double) * (line_params_circbuffer.end[i_mod]));
            }
            else {
                temp_sig_m2_array[0] = line_params_circbuffer.end[i_mod] - line_params_circbuffer.start[i_mod];
                memmove(&temp_sig_m2_array[1], &sig_m2_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * (line_params_circbuffer.end[i_mod] - line_params_circbuffer.start[i_mod]));
            }

            // calculate average
            double temp_sig_m2_average = 0;
            for (int j = 1; j < temp_sig_m2_array[0] + 1; j++) {
                temp_sig_m2_average += temp_sig_m2_array[j];
            }
            temp_sig_m2_average /= temp_sig_m2_array[0];
            line_params_circbuffer.mean_m2[i_mod] = temp_sig_m2_average;
            
            // fill updown (TODO: FIX WRONG ON FIRST LINE)
            if (line_params_circbuffer.mean_m2[i_mod] > line_params_circbuffer.mean_m2[(i - 1) % NUMBER_OF_LINE_PARAMS]) {
                line_params_circbuffer.updown[i_mod] = 0;
            }
            else if (line_params_circbuffer.mean_m2[i_mod] < line_params_circbuffer.mean_m2[(i - 1) % NUMBER_OF_LINE_PARAMS]) {
                line_params_circbuffer.updown[i_mod] = 1;
            }

            // increment read pointer
            lp_cb_ud_r = (lp_cb_ud_r + 1) % NUMBER_OF_LINE_PARAMS;
            
        }

        if (new_lines) {
            gui_lp_r = (lp_cb_ud_r - 1) % NUMBER_OF_LINE_PARAMS;
            std::unique_lock<std::mutex> ul(line_mutex);
            line_ready = true;
            new_lines = false;
            ul.unlock();
            line_cv.notify_all();
        }

        auto end = std::chrono::high_resolution_clock::now();
                
        peak_detector_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        peak_detector_avg_processing_time = 0;
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            peak_detector_avg_processing_time += peak_detector_processing_time[i];
        }
        peak_detector_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
    }
    return 0;
}

////////////////////////////////////////////////////////
// Line generator thread

static double line_cb_dsc[2000][NUMBER_OF_POINTS_PER_LINE];
static double line_cb_nr[2000][NUMBER_OF_POINTS_PER_LINE];
static double line_cb_esc[2000][NUMBER_OF_POINTS_PER_LINE];

static std::atomic<long> line_cb_dsc_w = 0;
static std::atomic<long> lp_line_r = 0;

static std::atomic<long> line_cb_dsc_r = 1999;

static bool line_dsc_bool = true, line_esc_bool = false, line_nr_bool = false;

// data shapes for images
static double img_nr[NUMBER_OF_POINTS_PER_LINE][NUMBER_OF_POINTS_PER_LINE];
static double img_dsc[NUMBER_OF_POINTS_PER_LINE][NUMBER_OF_POINTS_PER_LINE];
static double img_esc[NUMBER_OF_POINTS_PER_LINE][NUMBER_OF_POINTS_PER_LINE];

static bool img_dsc_bool = true, img_esc_bool = false, img_nr_bool = false;

static bool img_nr_log = false, img_nr_abs = false;

bool runLineGeneratorThread() {
    static std::atomic<long> temp_lp_w;

    while (gui_running) {
        std::unique_lock<std::mutex> ul(line_mutex);
        line_cv.wait(ul, []() { return line_ready; });
        line_ready = false;
        ul.unlock();

        auto start = std::chrono::high_resolution_clock::now();

        temp_lp_w = lp_cb_lr_w;
        
        if (temp_lp_w < lp_line_r) {
            temp_lp_w += NUMBER_OF_LINE_PARAMS;
        }

        for (int i = lp_line_r; i < temp_lp_w; i++) {
            int i_mod = (i % NUMBER_OF_LINE_PARAMS);
            
            // set up constant width copy
            int copy_length = 0;
            if (line_params_circbuffer.start[i_mod] > line_params_circbuffer.end[i_mod]) {
                if (line_params_circbuffer.end[i_mod] + NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - line_params_circbuffer.start[i_mod] > NUMBER_OF_POINTS_PER_LINE) {
                    copy_length = NUMBER_OF_POINTS_PER_LINE;
                }
                else {
                    copy_length = line_params_circbuffer.end[i_mod] + NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - line_params_circbuffer.start[i_mod];
                }
            }
            else {
                if (line_params_circbuffer.end[i_mod] - line_params_circbuffer.start[i_mod] > NUMBER_OF_POINTS_PER_LINE) {
                    copy_length = NUMBER_OF_POINTS_PER_LINE;
                }
                else {
                    copy_length = line_params_circbuffer.end[i_mod] - line_params_circbuffer.start[i_mod];
                }
            }

            // copy or reverse copy for length
            if (line_params_circbuffer.start[i_mod] > line_params_circbuffer.end[i_mod]) {
                //long number_of_samples_before_end = (NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - line_params_circbuffer.start[i_mod]);
                //if (line_params_circbuffer.leftright[i_mod] == 0) {
                //    memmove(&line_cb_dsc[line_cb_dsc_w], &sig_dsc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * number_of_samples_before_end);
                //    memmove(&line_cb_dsc[line_cb_dsc_w + number_of_samples_before_end], &sig_dsc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length - number_of_samples_before_end );

                //    memmove(&line_cb_nr[line_cb_dsc_w], &sig_nr_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * number_of_samples_before_end);
                //    memmove(&line_cb_nr[line_cb_dsc_w + number_of_samples_before_end], &sig_nr_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length - number_of_samples_before_end);

                //    memmove(&line_cb_esc[line_cb_dsc_w], &sig_esc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * number_of_samples_before_end);
                //    memmove(&line_cb_esc[line_cb_dsc_w + number_of_samples_before_end], &sig_esc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length - number_of_samples_before_end);
                //}
                //else {
                //    std::reverse_copy(&sig_dsc_circbuffer[0], &sig_dsc_circbuffer[line_params_circbuffer.end[i_mod]], line_cb_dsc[line_cb_dsc_w]);
                //    std::reverse_copy(&sig_dsc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - (500 - line_params_circbuffer.end[i_mod])], &sig_dsc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], &line_cb_dsc[line_cb_dsc_w][copy_length - number_of_samples_before_end]);
                //
                //    std::reverse_copy(&sig_nr_circbuffer[0], &sig_nr_circbuffer[line_params_circbuffer.end[i_mod]], line_cb_dsc[line_cb_dsc_w]);
                //    std::reverse_copy(&sig_nr_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - (500 - line_params_circbuffer.end[i_mod])], &sig_nr_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], &line_cb_nr[line_cb_dsc_w][copy_length - number_of_samples_before_end]);

                //    std::reverse_copy(&sig_esc_circbuffer[0], &sig_esc_circbuffer[line_params_circbuffer.end[i_mod]], line_cb_dsc[line_cb_dsc_w]);
                //    std::reverse_copy(&sig_esc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER - (500 - line_params_circbuffer.end[i_mod])], &sig_esc_circbuffer[NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER], &line_cb_esc[line_cb_dsc_w][copy_length - number_of_samples_before_end]);

                //}
            }
            else {
                if (line_params_circbuffer.leftright[i_mod] == 0) {
                    memmove(&line_cb_dsc[line_cb_dsc_w], &sig_dsc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length);

                    memmove(&line_cb_nr[line_cb_dsc_w], &sig_nr_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length);

                    memmove(&line_cb_esc[line_cb_dsc_w], &sig_esc_circbuffer[line_params_circbuffer.start[i_mod]], sizeof(double) * copy_length);

                }
                else {
                    std::reverse_copy(&sig_dsc_circbuffer[line_params_circbuffer.start[i_mod]], &sig_dsc_circbuffer[line_params_circbuffer.start[i_mod] + copy_length], line_cb_dsc[line_cb_dsc_w]);

                    std::reverse_copy(&sig_nr_circbuffer[line_params_circbuffer.start[i_mod]], &sig_nr_circbuffer[line_params_circbuffer.start[i_mod] + copy_length], line_cb_nr[line_cb_dsc_w]);

                    std::reverse_copy(&sig_esc_circbuffer[line_params_circbuffer.start[i_mod]], &sig_esc_circbuffer[line_params_circbuffer.start[i_mod] + copy_length], line_cb_esc[line_cb_dsc_w]);

                }
            }
            
            static double m2_max = 38500;
            static double m2_min = 28500;

            static double y = 0;
            y = (line_params_circbuffer.mean_m2[i_mod] - m2_min) / (m2_max - m2_min);
            y = ceil(y * NUMBER_OF_POINTS_PER_LINE) - 1;

            if ((y < 0) || (y > NUMBER_OF_POINTS_PER_LINE-1)) std::cout << "y is broken: " << y << "\n";

            if (y < 0) { y = 0; }
            if (y > NUMBER_OF_POINTS_PER_LINE-1) { y = NUMBER_OF_POINTS_PER_LINE-1; }

            if (img_nr_abs) {
                static double temp_nr[NUMBER_OF_POINTS_PER_LINE];
                // take abs()
                for (int i = 0; i < NUMBER_OF_POINTS_PER_LINE; i++) {
                    if (line_cb_nr[line_cb_dsc_w][i] < 0) {
                        temp_nr[i] = line_cb_nr[line_cb_dsc_w][i] * -1;
                    }
                    else {
                        temp_nr[i] = line_cb_nr[line_cb_dsc_w][i];
                    }
                }
                if (img_nr_log) {
                    // take log10()
                    for (int i = 0; i < NUMBER_OF_POINTS_PER_LINE; i++) {
                        temp_nr[i] = std::log10(temp_nr[i]);
                    }
                }
                memmove(&img_nr[(int)y][0], &temp_nr, sizeof(img_nr[0]));
            }
            else {
                memmove(&img_nr[(int)y][0], &line_cb_nr[line_cb_dsc_w], sizeof(img_nr[0]));

            }
            memmove(&img_dsc[(int)y][0], &line_cb_dsc[line_cb_dsc_w], sizeof(img_dsc[0]));
            memmove(&img_esc[(int)y][0], &line_cb_esc[line_cb_dsc_w], sizeof(img_esc[0]));


            line_cb_dsc_w = (line_cb_dsc_w + 1) % (sizeof(line_cb_dsc) / sizeof(line_cb_dsc[0]));
            line_cb_dsc_r = (line_cb_dsc_r + 1) % (sizeof(line_cb_dsc) / sizeof(line_cb_dsc[0]));
        }
        lp_line_r = temp_lp_w % NUMBER_OF_LINE_PARAMS;

        auto end = std::chrono::high_resolution_clock::now();

        line_generator_processing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        line_generator_avg_processing_time = 0;
        for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
            line_generator_avg_processing_time += line_generator_processing_time[i];
        }
        line_generator_avg_processing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
    }

    return 0;
}

// Functions for GUI
bool runGUIThread() {
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);


    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    bool show_time_domain_window = true;
    bool show_time_vs_signals_window = true;
    bool show_signal_vs_line_window = false;
    bool show_images = false;
    bool show_save_window = true;

    //debug bools
    bool show_debug = true;

    int numberSaveSegments = 100000;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    bool seg_dsc_bool = true;
    bool seg_nr_bool = false;
    bool seg_esc_bool = false;
    bool seg_m1_bool = false;
    bool seg_m2_bool = false;

    // declaration of new colour

    const ImVec4 custom_hot[255] = { {0.010417, 0.000000, 0.000000, 1},
{0.020833, 0.000000, 0.000000, 1},
{0.031250, 0.000000, 0.000000, 1},
{0.041667, 0.000000, 0.000000, 1},
{0.052083, 0.000000, 0.000000, 1},
{0.062500, 0.000000, 0.000000, 1},
{0.072917, 0.000000, 0.000000, 1},
{0.083333, 0.000000, 0.000000, 1},
{0.093750, 0.000000, 0.000000, 1},
{0.104167, 0.000000, 0.000000, 1},
{0.114583, 0.000000, 0.000000, 1},
{0.125000, 0.000000, 0.000000, 1},
{0.135417, 0.000000, 0.000000, 1},
{0.145833, 0.000000, 0.000000, 1},
{0.156250, 0.000000, 0.000000, 1},
{0.166667, 0.000000, 0.000000, 1},
{0.177083, 0.000000, 0.000000, 1},
{0.187500, 0.000000, 0.000000, 1},
{0.197917, 0.000000, 0.000000, 1},
{0.208333, 0.000000, 0.000000, 1},
{0.218750, 0.000000, 0.000000, 1},
{0.229167, 0.000000, 0.000000, 1},
{0.239583, 0.000000, 0.000000, 1},
{0.250000, 0.000000, 0.000000, 1},
{0.260417, 0.000000, 0.000000, 1},
{0.270833, 0.000000, 0.000000, 1},
{0.281250, 0.000000, 0.000000, 1},
{0.291667, 0.000000, 0.000000, 1},
{0.302083, 0.000000, 0.000000, 1},
{0.312500, 0.000000, 0.000000, 1},
{0.322917, 0.000000, 0.000000, 1},
{0.333333, 0.000000, 0.000000, 1},
{0.343750, 0.000000, 0.000000, 1},
{0.354167, 0.000000, 0.000000, 1},
{0.364583, 0.000000, 0.000000, 1},
{0.375000, 0.000000, 0.000000, 1},
{0.385417, 0.000000, 0.000000, 1},
{0.395833, 0.000000, 0.000000, 1},
{0.406250, 0.000000, 0.000000, 1},
{0.416667, 0.000000, 0.000000, 1},
{0.427083, 0.000000, 0.000000, 1},
{0.437500, 0.000000, 0.000000, 1},
{0.447917, 0.000000, 0.000000, 1},
{0.458333, 0.000000, 0.000000, 1},
{0.468750, 0.000000, 0.000000, 1},
{0.479167, 0.000000, 0.000000, 1},
{0.489583, 0.000000, 0.000000, 1},
{0.500000, 0.000000, 0.000000, 1},
{0.510417, 0.000000, 0.000000, 1},
{0.520833, 0.000000, 0.000000, 1},
{0.531250, 0.000000, 0.000000, 1},
{0.541667, 0.000000, 0.000000, 1},
{0.552083, 0.000000, 0.000000, 1},
{0.562500, 0.000000, 0.000000, 1},
{0.572917, 0.000000, 0.000000, 1},
{0.583333, 0.000000, 0.000000, 1},
{0.593750, 0.000000, 0.000000, 1},
{0.604167, 0.000000, 0.000000, 1},
{0.614583, 0.000000, 0.000000, 1},
{0.625000, 0.000000, 0.000000, 1},
{0.635417, 0.000000, 0.000000, 1},
{0.645833, 0.000000, 0.000000, 1},
{0.656250, 0.000000, 0.000000, 1},
{0.666667, 0.000000, 0.000000, 1},
{0.677083, 0.000000, 0.000000, 1},
{0.687500, 0.000000, 0.000000, 1},
{0.697917, 0.000000, 0.000000, 1},
{0.708333, 0.000000, 0.000000, 1},
{0.718750, 0.000000, 0.000000, 1},
{0.729167, 0.000000, 0.000000, 1},
{0.739583, 0.000000, 0.000000, 1},
{0.750000, 0.000000, 0.000000, 1},
{0.760417, 0.000000, 0.000000, 1},
{0.770833, 0.000000, 0.000000, 1},
{0.781250, 0.000000, 0.000000, 1},
{0.791667, 0.000000, 0.000000, 1},
{0.802083, 0.000000, 0.000000, 1},
{0.812500, 0.000000, 0.000000, 1},
{0.822917, 0.000000, 0.000000, 1},
{0.833333, 0.000000, 0.000000, 1},
{0.843750, 0.000000, 0.000000, 1},
{0.854167, 0.000000, 0.000000, 1},
{0.864583, 0.000000, 0.000000, 1},
{0.875000, 0.000000, 0.000000, 1},
{0.885417, 0.000000, 0.000000, 1},
{0.895833, 0.000000, 0.000000, 1},
{0.906250, 0.000000, 0.000000, 1},
{0.916667, 0.000000, 0.000000, 1},
{0.927083, 0.000000, 0.000000, 1},
{0.937500, 0.000000, 0.000000, 1},
{0.947917, 0.000000, 0.000000, 1},
{0.958333, 0.000000, 0.000000, 1},
{0.968750, 0.000000, 0.000000, 1},
{0.979167, 0.000000, 0.000000, 1},
{0.989583, 0.000000, 0.000000, 1},
{1.000000, 0.000000, 0.000000, 1},
{1.000000, 0.010417, 0.000000, 1},
{1.000000, 0.020833, 0.000000, 1},
{1.000000, 0.031250, 0.000000, 1},
{1.000000, 0.041667, 0.000000, 1},
{1.000000, 0.052083, 0.000000, 1},
{1.000000, 0.062500, 0.000000, 1},
{1.000000, 0.072917, 0.000000, 1},
{1.000000, 0.083333, 0.000000, 1},
{1.000000, 0.093750, 0.000000, 1},
{1.000000, 0.104167, 0.000000, 1},
{1.000000, 0.114583, 0.000000, 1},
{1.000000, 0.125000, 0.000000, 1},
{1.000000, 0.135417, 0.000000, 1},
{1.000000, 0.145833, 0.000000, 1},
{1.000000, 0.156250, 0.000000, 1},
{1.000000, 0.166667, 0.000000, 1},
{1.000000, 0.177083, 0.000000, 1},
{1.000000, 0.187500, 0.000000, 1},
{1.000000, 0.197917, 0.000000, 1},
{1.000000, 0.208333, 0.000000, 1},
{1.000000, 0.218750, 0.000000, 1},
{1.000000, 0.229167, 0.000000, 1},
{1.000000, 0.239583, 0.000000, 1},
{1.000000, 0.250000, 0.000000, 1},
{1.000000, 0.260417, 0.000000, 1},
{1.000000, 0.270833, 0.000000, 1},
{1.000000, 0.281250, 0.000000, 1},
{1.000000, 0.291667, 0.000000, 1},
{1.000000, 0.302083, 0.000000, 1},
{1.000000, 0.312500, 0.000000, 1},
{1.000000, 0.322917, 0.000000, 1},
{1.000000, 0.333333, 0.000000, 1},
{1.000000, 0.343750, 0.000000, 1},
{1.000000, 0.354167, 0.000000, 1},
{1.000000, 0.364583, 0.000000, 1},
{1.000000, 0.375000, 0.000000, 1},
{1.000000, 0.385417, 0.000000, 1},
{1.000000, 0.395833, 0.000000, 1},
{1.000000, 0.406250, 0.000000, 1},
{1.000000, 0.416667, 0.000000, 1},
{1.000000, 0.427083, 0.000000, 1},
{1.000000, 0.437500, 0.000000, 1},
{1.000000, 0.447917, 0.000000, 1},
{1.000000, 0.458333, 0.000000, 1},
{1.000000, 0.468750, 0.000000, 1},
{1.000000, 0.479167, 0.000000, 1},
{1.000000, 0.489583, 0.000000, 1},
{1.000000, 0.500000, 0.000000, 1},
{1.000000, 0.510417, 0.000000, 1},
{1.000000, 0.520833, 0.000000, 1},
{1.000000, 0.531250, 0.000000, 1},
{1.000000, 0.541667, 0.000000, 1},
{1.000000, 0.552083, 0.000000, 1},
{1.000000, 0.562500, 0.000000, 1},
{1.000000, 0.572917, 0.000000, 1},
{1.000000, 0.583333, 0.000000, 1},
{1.000000, 0.593750, 0.000000, 1},
{1.000000, 0.604167, 0.000000, 1},
{1.000000, 0.614583, 0.000000, 1},
{1.000000, 0.625000, 0.000000, 1},
{1.000000, 0.635417, 0.000000, 1},
{1.000000, 0.645833, 0.000000, 1},
{1.000000, 0.656250, 0.000000, 1},
{1.000000, 0.666667, 0.000000, 1},
{1.000000, 0.677083, 0.000000, 1},
{1.000000, 0.687500, 0.000000, 1},
{1.000000, 0.697917, 0.000000, 1},
{1.000000, 0.708333, 0.000000, 1},
{1.000000, 0.718750, 0.000000, 1},
{1.000000, 0.729167, 0.000000, 1},
{1.000000, 0.739583, 0.000000, 1},
{1.000000, 0.750000, 0.000000, 1},
{1.000000, 0.760417, 0.000000, 1},
{1.000000, 0.770833, 0.000000, 1},
{1.000000, 0.781250, 0.000000, 1},
{1.000000, 0.791667, 0.000000, 1},
{1.000000, 0.802083, 0.000000, 1},
{1.000000, 0.812500, 0.000000, 1},
{1.000000, 0.822917, 0.000000, 1},
{1.000000, 0.833333, 0.000000, 1},
{1.000000, 0.843750, 0.000000, 1},
{1.000000, 0.854167, 0.000000, 1},
{1.000000, 0.864583, 0.000000, 1},
{1.000000, 0.875000, 0.000000, 1},
{1.000000, 0.885417, 0.000000, 1},
{1.000000, 0.895833, 0.000000, 1},
{1.000000, 0.906250, 0.000000, 1},
{1.000000, 0.916667, 0.000000, 1},
{1.000000, 0.927083, 0.000000, 1},
{1.000000, 0.937500, 0.000000, 1},
{1.000000, 0.947917, 0.000000, 1},
{1.000000, 0.958333, 0.000000, 1},
{1.000000, 0.968750, 0.000000, 1},
{1.000000, 0.979167, 0.000000, 1},
{1.000000, 0.989583, 0.000000, 1},
{1.000000, 1.000000, 0.000000, 1},
{1.000000, 1.000000, 0.015625, 1},
{1.000000, 1.000000, 0.031250, 1},
{1.000000, 1.000000, 0.046875, 1},
{1.000000, 1.000000, 0.062500, 1},
{1.000000, 1.000000, 0.078125, 1},
{1.000000, 1.000000, 0.093750, 1},
{1.000000, 1.000000, 0.109375, 1},
{1.000000, 1.000000, 0.125000, 1},
{1.000000, 1.000000, 0.140625, 1},
{1.000000, 1.000000, 0.156250, 1},
{1.000000, 1.000000, 0.171875, 1},
{1.000000, 1.000000, 0.187500, 1},
{1.000000, 1.000000, 0.203125, 1},
{1.000000, 1.000000, 0.218750, 1},
{1.000000, 1.000000, 0.234375, 1},
{1.000000, 1.000000, 0.250000, 1},
{1.000000, 1.000000, 0.265625, 1},
{1.000000, 1.000000, 0.281250, 1},
{1.000000, 1.000000, 0.296875, 1},
{1.000000, 1.000000, 0.312500, 1},
{1.000000, 1.000000, 0.328125, 1},
{1.000000, 1.000000, 0.343750, 1},
{1.000000, 1.000000, 0.359375, 1},
{1.000000, 1.000000, 0.375000, 1},
{1.000000, 1.000000, 0.390625, 1},
{1.000000, 1.000000, 0.406250, 1},
{1.000000, 1.000000, 0.421875, 1},
{1.000000, 1.000000, 0.437500, 1},
{1.000000, 1.000000, 0.453125, 1},
{1.000000, 1.000000, 0.468750, 1},
{1.000000, 1.000000, 0.484375, 1},
{1.000000, 1.000000, 0.500000, 1},
{1.000000, 1.000000, 0.515625, 1},
{1.000000, 1.000000, 0.531250, 1},
{1.000000, 1.000000, 0.546875, 1},
{1.000000, 1.000000, 0.562500, 1},
{1.000000, 1.000000, 0.578125, 1},
{1.000000, 1.000000, 0.593750, 1},
{1.000000, 1.000000, 0.609375, 1},
{1.000000, 1.000000, 0.625000, 1},
{1.000000, 1.000000, 0.640625, 1},
{1.000000, 1.000000, 0.656250, 1},
{1.000000, 1.000000, 0.671875, 1},
{1.000000, 1.000000, 0.687500, 1},
{1.000000, 1.000000, 0.703125, 1},
{1.000000, 1.000000, 0.718750, 1},
{1.000000, 1.000000, 0.734375, 1},
{1.000000, 1.000000, 0.750000, 1},
{1.000000, 1.000000, 0.765625, 1},
{1.000000, 1.000000, 0.781250, 1},
{1.000000, 1.000000, 0.796875, 1},
{1.000000, 1.000000, 0.812500, 1},
{1.000000, 1.000000, 0.828125, 1},
{1.000000, 1.000000, 0.843750, 1},
{1.000000, 1.000000, 0.859375, 1},
{1.000000, 1.000000, 0.875000, 1},
{1.000000, 1.000000, 0.890625, 1},
{1.000000, 1.000000, 0.906250, 1},
{1.000000, 1.000000, 0.921875, 1},
{1.000000, 1.000000, 0.937500, 1},
{1.000000, 1.000000, 0.953125, 1},
{1.000000, 1.000000, 0.968750, 1},
{1.000000, 1.000000, 0.984375, 1} };

    ImPlotColormap custom_map_hot = ImPlot::AddColormap("Custom Hot", custom_hot, 255, true);


    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        ImPlot::ShowDemoWindow();

        // Show application statistics
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Application Statistics");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Show time domain window
        if (show_time_domain_window) {

            if (ImGui::Begin("Time Domain Real Time")) {
                if (ImGui::CollapsingHeader("Time Domain Configuration"))
                {
                    if (ImGui::TreeNode("Channel 1##2"))
                    {
                        // setting number of time domains to average
                        int temp_num_TD_channel1 = numberOfTDsToAverage_ch1;
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("# Segments to Average", &temp_num_TD_channel1, 1, 2);
                        if (temp_num_TD_channel1 > NUMBER_OF_SEGMENTS_PER_BUFFER) {
                            temp_num_TD_channel1 = NUMBER_OF_SEGMENTS_PER_BUFFER;
                        }
                        else if (temp_num_TD_channel1 < NUMBER_OF_SEGMENTS_PER_BUFFER / NUMBER_OF_TD_SIGNALS) {
                            temp_num_TD_channel1 = NUMBER_OF_SEGMENTS_PER_BUFFER / NUMBER_OF_TD_SIGNALS;
                        }
                        numberOfTDsToAverage_ch1 = temp_num_TD_channel1;


                        double temp_percentile = percentile;

                        ImGui::SameLine();
                        ImGui::Checkbox("Sort based on NR", &td_ch1_sort_bool);
                        ImGui::SameLine();
                        ImGui::Checkbox("Show positive and negative", &td_ch1_sort_posneg_bool);
                        ImGui::SameLine();
                        ImGui::Checkbox("Rectify negative", &td_ch1_rectify_bool);
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputDouble("Percentile", &temp_percentile, 0.05, 0.1);
                        if (temp_percentile > 1) {
                            temp_percentile = 1;
                        }
                        else if (temp_percentile < 0) {
                            temp_percentile = 0;
                        }
                        percentile = temp_percentile;
                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Channel 2##2"))
                    {
                        // setting number of time domains to average
                        int temp_num_TD_channel2 = numberOfTDsToAverage_ch2;

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("# Segments to Average", &temp_num_TD_channel2, 1, 2);
                        if (temp_num_TD_channel2 > NUMBER_OF_SEGMENTS_PER_BUFFER) {
                            temp_num_TD_channel2 = NUMBER_OF_SEGMENTS_PER_BUFFER;
                        }
                        else if (temp_num_TD_channel2 < NUMBER_OF_SEGMENTS_PER_BUFFER / NUMBER_OF_TD_SIGNALS) {
                            temp_num_TD_channel2 = NUMBER_OF_SEGMENTS_PER_BUFFER / NUMBER_OF_TD_SIGNALS;
                        }
                        numberOfTDsToAverage_ch2 = temp_num_TD_channel2;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Channel 3##2"))
                    {
                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Channel 4##2"))
                    {
                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                }
                
                //Checkboxes for activating channels
                ImGui::Checkbox("Channel 1", &ch1_axis_bool);
                ImGui::SameLine();
                ImGui::Checkbox("Channel 2", &ch2_axis_bool);
                ImGui::SameLine();
                ImGui::Checkbox("Channel 3", &ch3_axis_bool);
                ImGui::SameLine();
                ImGui::Checkbox("Channel 4", &ch4_axis_bool);

                ImGui::Text("Average processing time for");
                if (ch1_axis_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| Ch1: %.2f ms", channel1_avg_processing_time * 1e-6);
                }
                if (ch2_axis_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| Ch2: %.2f ms", channel2_avg_processing_time * 1e-6);
                }
                if (ch3_axis_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| Ch3: %.2f ms", channel3_avg_processing_time * 1e-6);
                }
                if (ch4_axis_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| Ch4: %.2f ms", channel4_avg_processing_time * 1e-6);
                }

                if (ImPlot::BeginPlot("Time Domains", ImVec2(-1, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1,"Time (samples)");
                    ImPlot::SetupAxis(ImAxis_Y1, "Empty");

                    if (ch1_axis_bool) {
                        ImPlot::SetupAxis(ImAxis_Y1, "Channel 1");
                    }
                    if (ch2_axis_bool) {
                        ImPlot::SetupAxis(ImAxis_Y2, "Channel 2");
                    }
                    if (ch3_axis_bool) {
                        ImPlot::SetupAxis(ImAxis_Y3, "Channel 3");
                    }
                    if (ch4_axis_bool) {
                        ImPlot::SetupAxis(ImAxis_Y4, "Channel 4");
                    }

                    if (ch1_axis_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                        if (td_ch1_sort_bool) {
                            ImPlot::PlotLine("Raw Signal Channel 1", x_samples, sig_channel1[0], NUMBER_OF_SAMPLES_PER_SEGMENT);
                            if (td_ch1_sort_posneg_bool && !td_ch1_rectify_bool)
                                ImPlot::PlotLine("Raw Signal Channel 1", x_samples, sig_channel1[1], NUMBER_OF_SAMPLES_PER_SEGMENT);

                        }
                        else {
                            for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER / numberOfTDsToAverage_ch1; i++) {
                                ImPlot::PlotLine("Raw Signal Channel 1", x_samples, sig_channel1[i], NUMBER_OF_SAMPLES_PER_SEGMENT);
                            }
                        }

                    }
                    if (ch2_axis_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER/numberOfTDsToAverage_ch2; i++) {
                            ImPlot::PlotLine("Raw Signal Channel 2", x_samples, sig_channel2[i], NUMBER_OF_SAMPLES_PER_SEGMENT);
                        }
                    }
                    if (ch3_axis_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                        for (int i = 0; i < num_TD_channel3; i++) {
                            ImPlot::PlotLine("Raw Signal Channel 3", x_samples, sig_channel3[i], NUMBER_OF_SAMPLES_PER_SEGMENT);
                        }
                    }
                    if (ch4_axis_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y4);
                        for (int i = 0; i < num_TD_channel4; i++) {
                            ImPlot::PlotLine("Raw Signal Channel 4", x_samples, sig_channel4[i], NUMBER_OF_SAMPLES_PER_SEGMENT);
                        }
                    }
                    ImPlot::EndPlot();
                }
            }
            ImGui::End();
        }
        
        if (show_time_vs_signals_window) {

            if (ImGui::Begin("Signals vs. Segments")) {

                if (ImGui::CollapsingHeader("Signal Extraction Configuration"))
                {
                    if (ImGui::TreeNode("Detection Scattering (DSc)"))
                    {
                        // setting number of time domains to average
                        int temp_dsc_avg_start_region = sig_dsc_avg_region[0];
                        int temp_dsc_avg_end_region = sig_dsc_avg_region[1];

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("Start of avg region", &temp_dsc_avg_start_region, 1, 2);
                        if (temp_dsc_avg_start_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1) {
                            temp_dsc_avg_start_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1;
                        }
                        else if (temp_dsc_avg_start_region < 0) {
                            temp_dsc_avg_start_region = 0;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("End of avg region", &temp_dsc_avg_end_region, 1, 2);
                        if (temp_dsc_avg_end_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1) {
                            temp_dsc_avg_end_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1;
                        }
                        else if (temp_dsc_avg_end_region < temp_dsc_avg_start_region) {
                            temp_dsc_avg_end_region = temp_dsc_avg_start_region+10;
                        }

                        sig_dsc_avg_region[1] = temp_dsc_avg_end_region;
                        sig_dsc_avg_region[0] = temp_dsc_avg_start_region;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Non-radiative (NR)"))
                    {
                        // setting number of time domains to average
                        int temp_nr_avg_start_region = sig_nr_avg_region[0];
                        int temp_nr_avg_end_region = sig_nr_avg_region[1];

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("Start of avg region", &temp_nr_avg_start_region, 1, 2);
                        if (temp_nr_avg_start_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1) {
                            temp_nr_avg_start_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1;
                        }
                        else if (temp_nr_avg_start_region < 0) {
                            temp_nr_avg_start_region = 0;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("End of avg region", &temp_nr_avg_end_region, 1, 2);
                        if (temp_nr_avg_end_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1) {
                            temp_nr_avg_end_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1;
                        }
                        else if (temp_nr_avg_end_region <= temp_nr_avg_start_region) {
                            temp_nr_avg_end_region = temp_nr_avg_start_region + 1;
                        }

                        sig_nr_avg_region[1] = temp_nr_avg_end_region;
                        sig_nr_avg_region[0] = temp_nr_avg_start_region;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Excitation Scattering (ESc)"))
                    {
                        // setting number of time domains to average
                        int temp_esc_avg_start_region = sig_esc_avg_region[0];
                        int temp_esc_avg_end_region = sig_esc_avg_region[1];

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("Start of avg region", &temp_esc_avg_start_region, 1, 2);
                        if (temp_esc_avg_start_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1) {
                            temp_esc_avg_start_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1;
                        }
                        else if (temp_esc_avg_start_region < 0) {
                            temp_esc_avg_start_region = 0;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("End of avg region", &temp_esc_avg_end_region, 1, 2);
                        if (temp_esc_avg_end_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1) {
                            temp_esc_avg_end_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1;
                        }
                        else if (temp_esc_avg_end_region <= temp_esc_avg_start_region) {
                            temp_esc_avg_end_region = temp_esc_avg_start_region + 1;
                        }

                        sig_esc_avg_region[1] = temp_esc_avg_end_region;
                        sig_esc_avg_region[0] = temp_esc_avg_start_region;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Mirror 1"))
                    {
                        // setting number of time domains to average
                        int temp_m1_avg_start_region = sig_m1_avg_region[0];
                        int temp_m1_avg_end_region = sig_m1_avg_region[1];

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("Start of avg region", &temp_m1_avg_start_region, 1, 2);
                        if (temp_m1_avg_start_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1) {
                            temp_m1_avg_start_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1;
                        }
                        else if (temp_m1_avg_start_region < 0) {
                            temp_m1_avg_start_region = 0;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("End of avg region", &temp_m1_avg_end_region, 1, 2);
                        if (temp_m1_avg_end_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1) {
                            temp_m1_avg_end_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1;
                        }
                        else if (temp_m1_avg_end_region <= temp_m1_avg_start_region) {
                            temp_m1_avg_end_region = temp_m1_avg_start_region + 1;
                        }

                        sig_m1_avg_region[1] = temp_m1_avg_end_region;
                        sig_m1_avg_region[0] = temp_m1_avg_start_region;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Mirror 2"))
                    {
                        // setting number of time domains to average
                        int temp_m2_avg_start_region = sig_m2_avg_region[0];
                        int temp_m2_avg_end_region = sig_m2_avg_region[1];

                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("Start of avg region", &temp_m2_avg_start_region, 1, 2);
                        if (temp_m2_avg_start_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1) {
                            temp_m2_avg_start_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1 - 1;
                        }
                        else if (temp_m2_avg_start_region < 0) {
                            temp_m2_avg_start_region = 0;
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(100);
                        ImGui::InputInt("End of avg region", &temp_m2_avg_end_region, 1, 2);
                        if (temp_m2_avg_end_region > NUMBER_OF_SAMPLES_PER_SEGMENT - 1) {
                            temp_m2_avg_end_region = NUMBER_OF_SAMPLES_PER_SEGMENT - 1;
                        }
                        else if (temp_m2_avg_end_region <= temp_m2_avg_start_region) {
                            temp_m2_avg_end_region = temp_m2_avg_start_region + 1;
                        }

                        sig_m2_avg_region[1] = temp_m2_avg_end_region;
                        sig_m2_avg_region[0] = temp_m2_avg_start_region;

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                }

                static bool sig_dsc_bool = true;
                static bool sig_nr_bool = false;
                static bool sig_esc_bool = false;
                static bool sig_m1_bool = false;
                static bool sig_m2_bool = false;

                //Checkboxes for activating channels
                ImGui::Checkbox("DSc", &sig_dsc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("NR", &sig_nr_bool);
                ImGui::SameLine();
                ImGui::Checkbox("ESc", &sig_esc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("M1", &sig_m1_bool);
                ImGui::SameLine();
                ImGui::Checkbox("M2", &sig_m2_bool);
                
                //
                ImGui::Text("Average processing time for");
                if (sig_dsc_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| DSc Sig: %.2f ms", sig_dsc_avg_processing_time * 1e-6);
                }
                if (sig_nr_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| NR Sig: %.2f ms", sig_nr_avg_processing_time * 1e-6);
                }
                if (sig_esc_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| ESc Sig: %.2f ms", sig_esc_avg_processing_time * 1e-6);
                }
                if (sig_m1_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| m1 Sig: %.2f ms", sig_m1_avg_processing_time * 1e-6);
                }
                if (sig_m2_bool) {
                    ImGui::SameLine();
                    ImGui::Text("| m2 Sig: %.2f ms", sig_m1_avg_processing_time * 1e-6);
                }

                if (ImPlot::BeginPlot("Time Domains", ImVec2(-1, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Buffer (segments)");
                    ImPlot::SetupAxis(ImAxis_Y1, "Empty");

                    if (sig_dsc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y1, "Detection");
                        ImPlot::SetupAxisLimits(ImAxis_Y1, 35000, 40000);
                    }
                    if (sig_nr_bool) {
                        ImPlot::SetupAxis(ImAxis_Y2, "Non-radiative");
                    }
                    if (sig_esc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y3, "Excitation");
                    }
                    if (sig_m1_bool) {
                        ImPlot::SetupAxis(ImAxis_Y4, "Mirror 1");
                    }
                    if (sig_m2_bool) {
                        ImPlot::SetupAxis(ImAxis_Y5, "Mirror 2");

                    }

                    if (sig_dsc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                        ImPlot::PlotLine("Detection", x_segments, &sig_dsc_circbuffer[gui_s_cb_r * NUMBER_OF_SEGMENTS_PER_BUFFER], NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }
                    if (sig_nr_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        ImPlot::PlotLine("Non-radiative", x_segments, &sig_nr_circbuffer[gui_s_cb_r * NUMBER_OF_SEGMENTS_PER_BUFFER], NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }
                    if (sig_esc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                        ImPlot::PlotLine("Excitation", x_segments, &sig_esc_circbuffer[gui_s_cb_r * NUMBER_OF_SEGMENTS_PER_BUFFER], NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }
                    if (sig_m1_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y4);
                        ImPlot::PlotLine("Mirror 1", x_segments, &sig_m1_circbuffer[gui_s_cb_r * NUMBER_OF_SEGMENTS_PER_BUFFER], NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }
                    if (sig_m2_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y5);
                        ImPlot::PlotLine("Mirror 2", x_segments, &sig_m2_circbuffer[gui_s_cb_r * NUMBER_OF_SEGMENTS_PER_BUFFER], NUMBER_OF_SEGMENTS_PER_BUFFER);
                    }

                    ImPlot::EndPlot();
                }
            }
            ImGui::End();
        }

        if (show_debug) {

            if (ImGui::Begin("Debug")) {
                // Dropdown for timing
                if (ImPlot::BeginPlot("Timing Plot", ImVec2(-1, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Buffers");
                    ImPlot::SetupAxis(ImAxis_Y1, "Data Period");
                    ImPlot::SetupAxis(ImAxis_Y2, "Deinterlacing");
                    ImPlot::SetupAxis(ImAxis_Y3, "Saving");

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                    ImPlot::PlotLine("Data Period", x_time_samples, data_acquisition_time, NUMBER_OF_PROCESSING_TIME_SAMPLES);
                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                    ImPlot::PlotLine("Deinterlacing", x_time_samples, data_deinterlacing_time, NUMBER_OF_PROCESSING_TIME_SAMPLES);
                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                    ImPlot::PlotLine("Saving", x_time_samples, save_time, NUMBER_OF_PROCESSING_TIME_SAMPLES);

                    ImPlot::EndPlot();
                }

                // Dropdown for segments
                //Checkboxes for activating channels
                ImGui::Checkbox("DSc", &seg_dsc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("NR", &seg_nr_bool);
                ImGui::SameLine();
                ImGui::Checkbox("ESc", &seg_esc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("M1", &seg_m1_bool);
                ImGui::SameLine();
                ImGui::Checkbox("M2", &seg_m2_bool);

                if (ImPlot::BeginPlot("Segment Buffers", ImVec2(-1, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Segments");
                    ImPlot::SetupAxis(ImAxis_Y1, "Empty");

                    if (seg_dsc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y1, "Detection");
                    }
                    if (seg_nr_bool) {
                        ImPlot::SetupAxis(ImAxis_Y2, "Non-radiative");
                    }
                    if (seg_esc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y3, "Excitation");
                    }
                    if (seg_m1_bool) {
                        ImPlot::SetupAxis(ImAxis_Y4, "Mirror 1");
                    }
                    if (seg_m2_bool) {
                        ImPlot::SetupAxis(ImAxis_Y5, "Mirror 2");

                    }

                    if (seg_dsc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                        ImPlot::PlotLine("Detection", x_cb_segments, sig_dsc_circbuffer, NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER);
                    }
                    if (seg_nr_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        ImPlot::PlotLine("Non-radiative", x_cb_segments, sig_nr_circbuffer, NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER);
                    }
                    if (seg_esc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                        ImPlot::PlotLine("Excitation", x_cb_segments, sig_esc_circbuffer, NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER);
                    }
                    if (seg_m1_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y4);
                        ImPlot::PlotLine("Mirror 1", x_cb_segments, sig_m1_circbuffer, NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER);
                    }
                    if (seg_m2_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y5);
                        ImPlot::PlotLine("Mirror 2", x_cb_segments, sig_m2_circbuffer, NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER);
                    }
                    ImPlot::EndPlot();
                }
            }
            ImGui::End();
        }

        if (show_signal_vs_line_window) {
            if (ImGui::Begin("Signals vs. Lines")) {
                ImGui::Text("Average time to find line segments : %.2f ms", peak_detector_avg_processing_time * 1e-6);
                ImGui::Text("Average time to generate lines : %.2f ms", line_generator_avg_processing_time * 1e-6);
\
                ImGui::Text("Line bounds: %d | %d | %d | %d | %.0f", 
                    line_params_circbuffer.start[gui_lp_r],
                    line_params_circbuffer.end[gui_lp_r],
                    line_params_circbuffer.leftright[gui_lp_r],
                    line_params_circbuffer.updown[gui_lp_r],
                    line_params_circbuffer.mean_m2[gui_lp_r]);

                //Checkboxes for activating channels
                ImGui::Checkbox("DSc", &line_dsc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("NR", &line_nr_bool);
                ImGui::SameLine();
                ImGui::Checkbox("ESc", &line_esc_bool);

                if (ImPlot::BeginPlot("Signals", ImVec2(-1, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Buffer (segments)");
                    ImPlot::SetupAxis(ImAxis_Y1, "Empty");

                    if (line_dsc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y1, "Detection");
                    }
                    if (line_nr_bool) {
                        ImPlot::SetupAxis(ImAxis_Y2, "Non-radiative");
                    }
                    if (line_esc_bool) {
                        ImPlot::SetupAxis(ImAxis_Y3, "Excitation");
                    }

                    if (line_dsc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                        ImPlot::PlotLine("Detection", x_line_samples, line_cb_dsc[line_cb_dsc_r], NUMBER_OF_POINTS_PER_LINE);
                    }
                    if (line_nr_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        ImPlot::PlotLine("Non-radiative", x_line_samples, line_cb_nr[line_cb_dsc_r], NUMBER_OF_POINTS_PER_LINE);
                    }
                    if (line_esc_bool) {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                        ImPlot::PlotLine("Excitation", x_line_samples, line_cb_esc[line_cb_dsc_r], NUMBER_OF_POINTS_PER_LINE);
                    }



                    ImPlot::EndPlot();
                }

            }
            ImGui::End();
        }

        if (show_images) {
            if (ImGui::Begin("Images")) {

                static float nr_scale_min = 1;
                static float nr_scale_max = 3;

                static float dsc_scale_min = 30000;
                static float dsc_scale_max = 40000;

                static float esc_scale_min =40000;
                static float esc_scale_max = 50000;

                if (ImGui::CollapsingHeader("Image Display Configuration"))
                {
                    if (ImGui::TreeNode("Detection Scattering (DSc)"))
                    {
                        ImGui::SetNextItemWidth(200);
                        ImGui::DragFloatRange2("Min / Max DSc", &dsc_scale_min, &dsc_scale_max, 100.0f);
                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Non-radiative (nr)"))
                    {
                        ImGui::Checkbox("Abs", &img_nr_abs);
                        if (img_nr_abs) {
                            ImGui::SameLine();
                            ImGui::Checkbox("Log Scale", &img_nr_log);
                        }
                        ImGui::SetNextItemWidth(200);
                        ImGui::DragFloatRange2("Min / Max NR", &nr_scale_min, &nr_scale_max, 0.1f);


                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (ImGui::TreeNode("Excitation Scattering (ESc)"))
                    {
                        ImGui::SetNextItemWidth(200);
                        ImGui::DragFloatRange2("Min / Max ESc", &esc_scale_min, &esc_scale_max, 100.0f);
                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                }

                ImGui::Checkbox("DSc", &img_dsc_bool);
                ImGui::SameLine();
                ImGui::Checkbox("NR", &img_nr_bool);
                ImGui::SameLine();
                ImGui::Checkbox("ESc", &img_esc_bool);


                if (img_dsc_bool) {
                    ImPlot::PushColormap(ImPlotColormap_Greys);

                    if (ImPlot::BeginPlot("##Heatmap4", ImVec2(NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE))) {
                        ImPlot::SetupAxesLimits(0, 1, 0, 1);
                        ImPlot::PlotHeatmap("Detection", &img_dsc[0][0], NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE, dsc_scale_min, dsc_scale_max, NULL);
                        ImPlot::EndPlot();
                    }
                    ImGui::SameLine();
                    ImPlot::ColormapScale("##HeatScale4", dsc_scale_min, dsc_scale_max, ImVec2(60, NUMBER_OF_POINTS_PER_LINE));
                    ImPlot::PopColormap(1);

                    ImGui::SameLine();
                }
                
                if (img_nr_bool) {
                    ImPlot::PushColormap(custom_map_hot);
                    if (ImPlot::BeginPlot("##Heatmap3", ImVec2(NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE))) {
                        ImPlot::SetupAxesLimits(0, 1, 0, 1);
                        ImPlot::PlotHeatmap("Non-radiative", &img_nr[0][0], NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE, nr_scale_min, nr_scale_max, NULL);
                        ImPlot::EndPlot();
                    }
                    ImGui::SameLine();
                    ImPlot::ColormapScale("##HeatScale3", nr_scale_min, nr_scale_max, ImVec2(60, NUMBER_OF_POINTS_PER_LINE));
                    ImPlot::PopColormap(1);
                    ImGui::SameLine();
                }

                if (img_esc_bool) {
                    ImPlot::PushColormap(ImPlotColormap_Greys);

                    if (ImPlot::BeginPlot("##Heatmap5", ImVec2(NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE))) {
                        ImPlot::SetupAxesLimits(0, 1, 0, 1);
                        ImPlot::PlotHeatmap("Excitation", &img_esc[0][0], NUMBER_OF_POINTS_PER_LINE, NUMBER_OF_POINTS_PER_LINE, esc_scale_min, esc_scale_max, NULL);
                        ImPlot::EndPlot();
                    }
                    ImGui::SameLine();
                    ImPlot::ColormapScale("##HeatScale5", esc_scale_min, esc_scale_max, ImVec2(60, NUMBER_OF_POINTS_PER_LINE));
                    ImPlot::PopColormap(1);

                    ImGui::SameLine();
                }


            }
            ImGui::End();
        }

        if (show_save_window) {
            if (ImGui::Begin("Save Window")) {
                ImGui::Text("Number of Segments: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(200);
                ImGui::InputInt("", &numberSaveSegments,50000,100000);
                ImGui::SameLine();

                if (ImGui::Button("Start Save") && !save_bool) {
                    startSave(numberSaveSegments);
                }
                ImGui::SameLine();
                
                if (ImGui::Button("Stop Save") && save_bool) {
                    stopSave();
                }
                
                if (save_bool) {
                    ImGui::SameLine();
                    ImGui::Text("( %.0f %% / 100 %%)", double(saveCount)*double(chunk[0])/double(numberSaveSegments) * 100.0f);
                }
                ImGui::Text("Save time : %.2f ms", avg_save_time * 1e-6);


            }
            ImGui::End();

        }

        if (1) {
            if (ImGui::Begin("Debug Metrics")) {
                 ImGui::Text("Data acquisition time: %.2f ms", avg_data_acquisition_time * 1e-6);
                 ImGui::SameLine();
                 ImGui::Text("| Deinterlacing time: %.2f ms", avg_data_deinterlacing_time * 1e-6);
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, NULL);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    WaitForLastSubmittedFrame();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    gui_running = false;
    running = false;

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0 ;
}



// Helper functions from imGUI

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = NULL;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != NULL)
    {
        ID3D12InfoQueue* pInfoQueue = NULL;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, NULL, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_fenceEvent == NULL)
        return false;

    {
        IDXGIFactory4* dxgiFactory = NULL;
        IDXGISwapChain1* swapChain1 = NULL;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, NULL); g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_hSwapChainWaitableObject != NULL) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = NULL; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = NULL; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = NULL; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = NULL; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = NULL; }
    if (g_fence) { g_fence->Release(); g_fence = NULL; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = NULL;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = NULL; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, NULL };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
