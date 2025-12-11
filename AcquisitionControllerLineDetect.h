#pragma once

// --- Standard Includes ---
#include <fstream> 
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <sstream>
#include <cmath>
#include <map>

// --- Project Includes ---
#include "AlazarDigitizer.h"
#include "ProjectSetting.h" // Assumes BoardConfig/AcquisitionConfig are here
#include "ATSsync.h"        // KEEPING YOUR ORIGINAL HEADER

// --- Constants for Peak Detection Algorithm ---

/*
1. The DMA Circular Buffer (Raw Ring)This is the memory allocated for the Alazar card to dump data into continuously.
Total Segments: 4,096,000 (4096 segments/buffer $\times$ 1000 buffers)Size per Segment:
8 KB (1024 samples $\times$ 4 channels $\times$ 2 bytes)
Total Size:
31.25 GB
Impact: 
This 31 GB is allocated immediately when you start acquisition.
If you have only 32 GB of RAM, your computer will likely freeze or crash instantly.



2. The Save Queue (Disk Buffer)
This memory is used only if your hard drive is slower than the laser (which it usually is).
It acts as a buffer to prevent data loss.

Max Capacity: 500 Chunks

Size per Chunk: 32 MB

Total Size: 15.63 GB

Impact: If your SSD writes slowly and this queue fills up to 100%, it will eat up an additional 15 GB of
RAM on top of the DMA buffer.

3. The Processing Queue
Max Capacity: 50 Chunks

Total Size: 1.56 GB






*/

#define NUMBER_OF_SEGMENTS_PER_BUFFER 4096 
#define NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER 1000
#define CIRC_BUFFER_SIZE (NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER)
#define NUMBER_OF_LINE_PARAMS 5000
#define NUMBER_OF_POINTS_PER_LINE 1024 
#define PROCESSING_QUEUE_MAX_SIZE 50

// --- Data Structures ---

struct DataChunk
{
    std::vector<U16> chA;
    std::vector<U16> chB;
    std::vector<U16> chC;
    std::vector<U16> chD;
    U32 boardId;
    uint64_t timestamp;
};

struct LineParams {
    int start[NUMBER_OF_LINE_PARAMS];
    int end[NUMBER_OF_LINE_PARAMS];
    int leftright[NUMBER_OF_LINE_PARAMS];
    int updown[NUMBER_OF_LINE_PARAMS];
    double mean_m2[NUMBER_OF_LINE_PARAMS];
};

/**
 * @class AcquisitionController
 * @brief Multithreaded Controller: Acquisition -> Saving + Peak Detection -> Image Gen
 */
class AcquisitionController
{
public:
    AcquisitionController();
    ~AcquisitionController();

    // --- Hardware Discovery ---
    bool DiscoverBoards();
    std::vector<std::string> GetBoardInfoList();

    // --- Configuration ---
    bool ConfigureAllBoards(const BoardConfig& config);

    // --- Acquisition Functions ---
    bool RunAcquisition(const AcquisitionConfig& config);
    bool RunSyncTest(const BoardConfig& config, int board1_ch, int board2_ch); // Restored original logic
    void StopAcquisition();
    bool IsAcquiring();

    // --- GUI Helpers ---
    std::vector<std::string> GetLogMessages();

    // Sync Test Plotting
    void GetSyncSnapshot(std::vector<float>& chA_board1, std::vector<float>& chA_board2);
    void GetSyncSnapshotMulti(int boardID, int channelID, std::vector<float>& outData);
    int GetLastSyncLag() const { return m_lastCalculatedLag; }

    //// Live Scope Plotting
    //void GetLatestScopeData(std::vector<float>& chA_data, U32 boardId);
    // Live Scope Plotting (Updated to support Channel Selection)
    // channelIndex: 0=A, 1=B, 2=C
    void GetLatestScopeData(std::vector<float>& data, U32 boardId, int channelIndex);
    // Live Image Plotting (New)
    void GetLatestImage(std::vector<std::vector<double>>& outImage);
    void GetLatestPeaks(std::vector<double>& peaks);
    int m_phaseAdjust = 0;
    void SetPhaseAdjust(int offset) { m_phaseAdjust = offset; }

    // Snapshot storage for the GUI
    std::vector<float> m_guiSnapshotWaveform;
    std::vector<double> m_guiSnapshotPeaks;

    // Update the Getter Function
    // We get BOTH at the same time to ensure they match
    void GetLatestSnapshot(std::vector<float>& waveform, std::vector<double>& peaks);
    // --- Tuning Variables ---
    std::atomic<double> m_algoThreshHigh = { 40000.0 }; // Top Peak Minimum Voltage
    std::atomic<double> m_algoThreshLow = { 25000.0 };  // Bottom Peak Maximum Voltage
    std::atomic<int>    m_algoMinDist = { 50 };         // Minimum samples between peaks

    // Setters for GUI
    void SetAlgoParams(double high, double low, int dist) {
        m_algoThreshHigh = high;
        m_algoThreshLow = low;
        m_algoMinDist = dist;
    }
    void GetAlgoParams(double* high, double* low, int* dist) {
        *high = m_algoThreshHigh;
        *low = m_algoThreshLow;
        *dist = m_algoMinDist;
    }
    int GetTotalLinesProcessed() const { return m_totalLinesProcessed; }
private:
    // --- Internal Helpers ---
    bool ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config);
    bool OpenDataFiles(U32 boardCount);
    void CloseDataFiles();
    void Log(std::string message);
    static int FindLagByXCorr(const std::vector<float>& x, const std::vector<float>& y, int maxLag);

    // --- Hardware Members ---
    std::vector<AlazarDigitizer*> m_boards;
    sb_device_t* m_syncBoardHandle = nullptr; // Keeping your original type

    // --- State & Config ---
    BoardConfig m_currentConfig;
    AcquisitionConfig m_currentAcqConfig;
    std::string m_savePath;
    std::vector<std::ofstream> m_fileStreams;
    std::vector<std::string> m_logMessages;
    std::atomic<bool> m_isAcquiring;
    std::atomic<bool> m_keepProcessing; // Kill switch for all threads

    // --- THREAD 1: SAVING (Consumer) ---
    std::thread m_saveThread;
    std::queue<DataChunk> m_saveQueue;
    std::mutex m_saveQueueMutex;
    std::condition_variable m_saveQueueCV;
    void SaveThreadLoop();

    // --- THREAD 2: DETECTOR (Peak Finding) ---
    std::thread m_detectorThread;
    std::queue<DataChunk> m_procQueue;
    std::mutex m_procQueueMutex;
    std::condition_variable m_procQueueCV;
    void DetectorThreadLoop();

    // --- THREAD 3: GENERATOR (Image Building) ---
    std::thread m_generatorThread;
    std::mutex m_lineMutex;
    std::condition_variable m_lineCV;
    bool m_linesReady;
    void GeneratorThreadLoop();

    // --- ALGORITHM STATE (Circular Buffers) ---
    std::vector<double> m_cb_M1;
    std::vector<double> m_cb_M2;
    std::vector<double> m_cb_Img;
    size_t m_cb_write_head;

    LineParams m_lineParams;
    size_t m_lp_cb_lr_w;
    size_t m_lp_cb_ud_r;
    size_t m_lp_line_r;
    size_t m_s_m1_cb_lp_r;

    bool m_top_peak_found;
    bool m_bot_peak_found;
    int m_top_peak_pos;
    int m_bot_peak_pos;
    bool m_first_buffer;

    // --- GUI Data Storage ---
    //std::mutex m_guiDataMutex;
    //std::map<U32, std::vector<float>> m_latestScopeData; // Map for multi-board scope
    std::mutex m_guiDataMutex;
    // Map: BoardID -> Vector of Channels -> Vector of Float Data
    std::map<U32, std::vector<std::vector<float>>> m_latestScopeData;
    double m_finalImage[NUMBER_OF_POINTS_PER_LINE][NUMBER_OF_POINTS_PER_LINE];

    // Sync Test Snapshots
    std::vector<float> m_syncSnapshot1;
    std::vector<float> m_syncSnapshot2;
    int m_lastCalculatedLag;
  
    std::vector<double> m_debugPeakLocations; // Stores x-indices of peaks
    std::atomic<int> m_totalLinesProcessed = 0;
    std::map<int, std::vector<std::vector<float>>> m_syncSnapshots;
};