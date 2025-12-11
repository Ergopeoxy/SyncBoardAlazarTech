//#pragma once
//#include "AlazarDigitizer.h"
//#include "ProjectSetting.h"
//#include "ATSsync_types.h"
//#include "ATSsync.h" // <-- REQUIRED: The SyncBoard API
//#include <fstream> 
//#include <string>
//#include <vector>
//#include <atomic> // <-- For thread-safe flags
//
//
///**
// * @class AcquisitionController
// * @brief Manages the entire acquisition system (Digitizers + SyncBoard).
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
//    // --- Standard Acquisition ---
//    bool RunAcquisition(const AcquisitionConfig& config);
//
//    // --- Special Verification Test ---
//    bool RunSyncTest(BoardConfig syncConfig);
//
//    // --- Status & Logging (Thread-Safe) ---
//    bool IsAcquiring();
//    std::vector<std::string> GetLogMessages();
//
//private:
//    // --- Internal Helpers ---
//    void StopAcquisition();
//    bool ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config);
//    bool OpenDataFiles(U32 boardCount);
//    void CloseDataFiles();
//    void Log(std::string message);
//
//    // --- Plotting Stubs ---
//    void PlotOscilloscope_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, U32 boardId);
//    void PlotSpectrum_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, double sampleRate, U32 boardId);
//    void PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA);
//
//    // --- Member Variables ---
//    std::vector<AlazarDigitizer*> m_boards;
//
//    // Handle for the SyncBoard (External 4X1G)
//    sb_device_t* m_syncBoardHandle = nullptr;
//
//    std::atomic<bool>     m_isAcquiring;
//    std::vector<std::string> m_logMessages;
//
//    BoardConfig m_currentConfig;
//    AcquisitionConfig m_currentAcqConfig;
//    std::vector<std::ofstream> m_fileStreams;
//    std::string m_savePath;
//};
//
////
/////**
//// * @class AcquisitionController
//// * @brief Manages the entire acquisition system.
//// */
////class AcquisitionController
////{
////public:
////    AcquisitionController();
////    ~AcquisitionController();
////
////    bool DiscoverBoards();
////    std::vector<std::string> GetBoardInfoList();
////    bool ConfigureAllBoards(const BoardConfig& config);
////    bool RunAcquisition(const AcquisitionConfig& config);
////    bool RunSyncTest(const BoardConfig& config); // Added from previous step
////
////    // --- GUI-Facing Methods ---
////    bool IsAcquiring();
////    std::vector<std::string> GetLogMessages();
////
////private:
////    void StopAcquisition();
////    bool ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config);
////    bool OpenDataFiles(U32 boardCount);
////    void CloseDataFiles();
////
////    void Log(std::string message); // Internal logger
////
////    // --- Plotting Stubs ---
////    void PlotOscilloscope_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, U32 boardId);
////    void PlotSpectrum_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, double sampleRate, U32 boardId);
////    void PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA); // Added from previous step
////
////    // --- Member Variables ---
////    std::vector<AlazarDigitizer*> m_boards;
////    std::atomic<bool>     m_isAcquiring; // Thread-safe flag
////    std::vector<std::string> m_logMessages; // Log message list
////
////    BoardConfig m_currentConfig;
////    AcquisitionConfig m_currentAcqConfig;
////
////    std::vector<std::ofstream> m_fileStreams;
////    std::string m_savePath;
////};
////
/////**
//// * @class AcquisitionController
//// * @brief Manages the entire acquisition system.
//// */
////class AcquisitionController
////{
////public:
////    AcquisitionController();
////    ~AcquisitionController();
////
////    bool DiscoverBoards();
////    std::vector<std::string> GetBoardInfoList();
////    bool ConfigureAllBoards(const BoardConfig& config);
////    bool RunAcquisition(const AcquisitionConfig& config);
////
////    bool RunSyncTest(const BoardConfig& config);
////
////private:
////    void StopAcquisition();
////    bool ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config);
////    bool OpenDataFiles(U32 boardCount);
////    void CloseDataFiles();
////
////    void PlotOscilloscope_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, U32 boardId);
////    void PlotSpectrum_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, double sampleRate, U32 boardId);
////
////    void PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA);
////
////    std::vector<AlazarDigitizer*> m_boards;
////    bool m_isAcquiring;
////
////    BoardConfig m_currentConfig;       // Stores the last used board config
////    AcquisitionConfig m_currentAcqConfig; // <-- FIX: Stores the current acq config
////
////    std::vector<std::ofstream> m_fileStreams;
////    std::string m_savePath;
////};
//