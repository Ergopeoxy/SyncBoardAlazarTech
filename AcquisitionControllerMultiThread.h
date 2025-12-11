//#pragma once
//#include "AlazarDigitizer.h"
//#include "ProjectSetting.h"
//#include "ATSsync.h" 
//#include <fstream> 
//#include <string>
//#include <vector>
//#include <atomic>
//#include <thread>
//#include <mutex>
//#include <condition_variable>
//#include <queue>
//
//#include <iostream>
//#include <sstream> // <--- Ensure this is here
//#include <cmath>
//#include <map> // <-- Add this include
//// --- Data Packet for the Save/Processing Queue ---
//// This represents one "Buffer" of de-interleaved data
//struct DataChunk
//{
//    std::vector<U16> chA;
//    std::vector<U16> chB;
//    std::vector<U16> chC;
//    std::vector<U16> chD;
//    U32 boardId;
//    uint64_t timestamp; // Optional: useful for alignment
//};
//
///**
// * @class AcquisitionController
// * @brief Manages acquisition using a Producer (Hardware) -> Consumer (Disk) architecture.
// */
//class AcquisitionController
//{
//public:
//    AcquisitionController();
//    ~AcquisitionController();
//
//    // --- Hardware Discovery ---
//    bool DiscoverBoards();
//    std::vector<std::string> GetBoardInfoList();
//
//    // --- Configuration ---
//    bool ConfigureAllBoards(const BoardConfig& config);
//
//    // --- Acquisition Functions ---
//    bool RunAcquisition(const AcquisitionConfig& config);
//    bool RunSyncTest(const BoardConfig& config);
//    bool SnapRunSyncTest(const BoardConfig& config);
//    // --- Status & GUI Helpers ---
//    bool IsAcquiring();
//    std::vector<std::string> GetLogMessages();
//
//    // GUI Plotting Getters (Thread-Safe)
//    void GetSyncSnapshot(std::vector<float>& chA_board1, std::vector<float>& chA_board2);
//    //void GetLatestScopeData(std::vector<float>& chA_data);
//    int GetLastSyncLag() const { return m_lastCalculatedLag; } // Getter
//    void GetLatestScopeData(std::vector<float>& chA_data, U32 boardId);
//private:
//    // --- Internal Logic ---
//    void StopAcquisition();
//    bool ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config);
//
//    // --- File Handling ---
//    bool OpenDataFiles(U32 boardCount);
//    void CloseDataFiles();
//
//    // --- Threading: The Consumer (Save) Loop ---
//    void SaveThreadLoop();
//
//    // --- Logging & Calculation ---
//    void Log(std::string message);
//    void PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA);
//
//    // --- Member Variables ---
//    std::vector<AlazarDigitizer*> m_boards;
//    sb_device_t* m_syncBoardHandle = nullptr;
//
//    // --- Threading Infrastructure ---
//    std::atomic<bool>     m_isAcquiring;      // Controls the hardware loop
//    std::atomic<bool>     m_keepSaving;       // Controls the Save Thread lifetime
//    std::thread           m_saveThread;       // The background worker
//
//    std::mutex            m_queueMutex;       // Protects m_saveQueue
//    std::condition_variable m_queueCondition; // Signals "Data Ready"
//    std::queue<DataChunk> m_saveQueue;        // The "Circular Buffer"
//    const size_t          MAX_QUEUE_SIZE = 500; // Limit memory usage if disk is slow
//
//    // --- GUI Data Protection ---
//    std::mutex            m_guiDataMutex;     // Protects plotting data
//    //std::vector<float>    m_latestScopeData;  // For the real-time oscilloscope
//
//    // --- Config & State ---
//    BoardConfig m_currentConfig;
//    AcquisitionConfig m_currentAcqConfig;
//    std::vector<std::ofstream> m_fileStreams;
//    std::string m_savePath;
//
//    std::vector<std::string> m_logMessages;
//
//    // --- Snapshot Data for Sync Test ---
//    std::vector<float> m_syncSnapshot1;
//    std::vector<float> m_syncSnapshot2;
//    int m_lastCalculatedLag = 0; // Member variable
//    std::map<U32, std::vector<float>> m_latestScopeData;
//};