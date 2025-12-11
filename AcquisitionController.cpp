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
////            //printf("currently outputing board index : %u" , boardIndex);
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
////        m_fileStreams[f_idx + 0].write((char*)chanA_full.data(), chanA_full.size() * sizeof(U16));
////        m_fileStreams[f_idx + 1].write((char*)chanB_full.data(), chanB_full.size() * sizeof(U16));
////        m_fileStreams[f_idx + 2].write((char*)chanC_full.data(), chanC_full.size() * sizeof(U16));
////        m_fileStreams[f_idx + 3].write((char*)chanD_full.data(), chanD_full.size() * sizeof(U16));
////    }
////
////    if (config.processData)
////    {
////        std::vector<float> volts_ChA(samplesPerChannel);
////        std::vector<float> volts_ChB(samplesPerChannel);
////        std::vector<float> volts_ChC(samplesPerChannel);
////        std::vector<float> volts_ChD(samplesPerChannel);
////        U32 bitShift = 2;
////        float codeZero = (float)((1 << (14 - 1)) - 0.5);
////        float codeRange = (float)((1 << (14 - 1)) - 0.5);
////        float inputRange = (float)m_currentConfig.inputRangeVolts;
////
////        for (U32 i = 0; i < samplesPerChannel; i++)
////        {
////            volts_ChA[i] = inputRange * ((float)(chanA_full[i] >> bitShift) - codeZero) / codeRange;
////            volts_ChB[i] = inputRange * ((float)(chanB_full[i] >> bitShift) - codeZero) / codeRange;
////            volts_ChC[i] = inputRange * ((float)(chanC_full[i] >> bitShift) - codeZero) / codeRange;
////            volts_ChD[i] = inputRange * ((float)(chanD_full[i] >> bitShift) - codeZero) / codeRange;
////        }
////        PlotOscilloscope_Stub(volts_ChA, volts_ChB, volts_ChC, volts_ChD, board->GetBoardId());
////        PlotSpectrum_Stub(volts_ChA, volts_ChB, volts_ChC, volts_ChD, m_currentConfig.sampleRateHz, board->GetBoardId());
////    }
////    return true;
////}
////
////
////// --- Sync Test Function ---
////bool AcquisitionController::RunSyncTest(const BoardConfig& config)
////{
////    if (m_isAcquiring) {
////        Log("ERROR: An acquisition is already in progress.");
////        return false;
////    }
////    if (m_boards.size() < 2) {
////        Log("ERROR: Sync test requires at least 2 boards.");
////        return false;
////    }
////
////    m_isAcquiring = true;
////    Log("--- Starting Synchronization Test ---");
////
////    AcquisitionConfig testConfig = {};
////    testConfig.admaMode = ADMA_NPT;
////    testConfig.samplesPerRecord = 4096;
////    testConfig.recordsPerBuffer = 1;
////    testConfig.buffersPerAcquisition = 1;
////    testConfig.saveData = true;
////    testConfig.processData = false;
////
////    if (!ConfigureAllBoards(config)) {
////        Log("ERROR: Sync Test failed during board configuration.");
////        m_isAcquiring = false;
////        return false;
////    }
////
////    const int channelCount = 4;
////    U32 bytesPerSample = 2;
////    U32 samplesPerChannel = testConfig.samplesPerRecord;
////    U32 recordsPerBuffer = testConfig.recordsPerBuffer;
////    U32 samplesPerRecordAllChannels = samplesPerChannel * channelCount;
////    U32 bytesPerRecord = samplesPerRecordAllChannels * bytesPerSample;
////    U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer;
////
////    for (AlazarDigitizer* board : m_boards)
////    {
////        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
////        if (!board->PrepareForAcquisition(testConfig, (CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D))) { StopAcquisition(); return false; }
////        if (!board->PostBuffer(0)) { StopAcquisition(); return false; }
////        if (!board->StartCapture()) { StopAcquisition(); return false; }
////    }
////
////    Log("Boards armed. Waiting for trigger...");
////
////    bool success = true;
////    for (AlazarDigitizer* board : m_boards)
////    {
////        if (!board->WaitFordBuffer(0, m_currentConfig.triggerTimeoutMS + 1000)) {
////            success = false;
////        }
////    }
////
////    if (success)
////    {
////        Log("Trigger received. Processing data...");
////        U16* pBufferB1 = (U16*)m_boards[0]->GetBuffer(0)->pBuffer;
////        U16* pBufferB2 = (U16*)m_boards[1]->GetBuffer(0)->pBuffer;
////
////        std::vector<float> volts_B1_ChA(samplesPerChannel);
////        std::vector<float> volts_B2_ChA(samplesPerChannel);
////
////        U32 bitShift = 2;
////        float codeZero = (float)((1 << (14 - 1)) - 0.5);
////        float codeRange = (float)((1 << (14 - 1)) - 0.5);
////        float inputRange = (float)m_currentConfig.inputRangeVolts;
////
////        for (U32 s = 0; s < samplesPerChannel; s++)
////        {
////            U16 raw_B1_A = pBufferB1[s * channelCount + 0];
////            volts_B1_ChA[s] = inputRange * ((float)(raw_B1_A >> bitShift) - codeZero) / codeRange;
////
////            U16 raw_B2_A = pBufferB2[s * channelCount + 0];
////            volts_B2_ChA[s] = inputRange * ((float)(raw_B2_A >> bitShift) - codeZero) / codeRange;
////        }
////
////        PlotSyncTest_Stub(volts_B1_ChA, volts_B2_ChA);
////    }
////    else
////    {
////        Log("ERROR: Sync Test failed (timeout or other error).");
////    }
////
////    StopAcquisition();
////    return success;
////}
////
////// --- Plotting Stubs ---
////void AcquisitionController::PlotOscilloscope_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, U32 boardId)
////{
////    std::stringstream ss_plot;
////    ss_plot << "  Board " << boardId << " [Oscilloscope]: ChA[0]=" << chA[0] << "V, ChB[0]=" << chB[0] << "V, ...";
////    Log(ss_plot.str());
////}
////
////void AcquisitionController::PlotSpectrum_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, double sampleRate, U32 boardId)
////{
////    // Log("  [Spectrum]: FFT calculation and plotting needed.");
////}
////
////void AcquisitionController::PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA)
////{
////    float diff = 0.f;
////    if (board1_chA.empty()) {
////        Log("  [Sync Test]: FAILED. No data to compare.");
////        return;
////    }
////    for (size_t i = 0; i < board1_chA.size(); ++i) {
////        diff += std::abs(board1_chA[i] - board2_chA[i]);
////    }
////    float avg_diff = diff / board1_chA.size();
////
////    std::stringstream ss_sync;
////    ss_sync << "  [Sync Test]: Average voltage difference = " << avg_diff << " V";
////    Log(ss_sync.str());
////
////    if (avg_diff < 0.05) { // (50mV tolerance, adjust as needed)
////        Log("  [Sync Test]: SUCCESS! Boards are synchronized.");
////    }
////    else {
////        Log("  [Sync Test]: FAILED. Boards are not synchronized.");
////    }
////}
//
//
//#include "AcquisitionController.h"
//#include <stdio.h>
//#include <iostream>
//#include <sstream>
//#include <cmath> 
//
//// --- Windows/Linux Compatibility Headers ---
//#ifdef _WIN32
//#include <conio.h>
//#include <direct.h>
//#define GetCurrentDir _getcwd
//#else
//#include <unistd.h>
//#define GetCurrentDir getcwd
//#endif
//
//// --- Constructor ---
//AcquisitionController::AcquisitionController() :
//    m_isAcquiring(false), m_syncBoardHandle(nullptr)
//{
//}
//
//// --- Destructor ---
//AcquisitionController::~AcquisitionController()
//{
//    if (m_isAcquiring) StopAcquisition();
//
//    // Close SyncBoard if open
//    if (m_syncBoardHandle) {
//        sb_device_close(m_syncBoardHandle);
//        m_syncBoardHandle = nullptr;
//    }
//
//    for (AlazarDigitizer* board : m_boards) delete board;
//    m_boards.clear();
//}
//
//// --- Logging Helper ---
//void AcquisitionController::Log(std::string message)
//{
//    std::cout << message << std::endl;
//    m_logMessages.push_back(message);
//    if (m_logMessages.size() > 100) m_logMessages.erase(m_logMessages.begin());
//}
//
//// --- Status Getters ---
//bool AcquisitionController::IsAcquiring() { return m_isAcquiring; }
//std::vector<std::string> AcquisitionController::GetLogMessages() { return m_logMessages; }
//
//// --- Board Discovery ---
//bool AcquisitionController::DiscoverBoards()
//{
//    Log("Finding hardware...");
//
//    // 1. Discover SyncBoard 4X1G
//    if (m_syncBoardHandle == nullptr) {
//        size_t count = 0;
//        sb_get_device_count(&count);
//        if (count > 0) {
//            // Open the first SyncBoard found (Index 0)
//            if (sb_device_open(0, &m_syncBoardHandle) == sb_rc_success) {
//                Log("Found and opened ATS Sync 4X1G.");
//            }
//            else {
//                Log("Error: Failed to open ATS Sync 4X1G.");
//            }
//        }
//        else {
//            Log("Warning: No ATS Sync 4X1G detected.");
//        }
//    }
//
//    // 2. Discover Digitizers
//    U32 systemCount = AlazarNumOfSystems();
//    if (systemCount < 1) {
//        Log("Error: No AlazarTech systems found.");
//        return false;
//    }
//
//    for (AlazarDigitizer* board : m_boards) delete board;
//    m_boards.clear();
//
//    for (U32 systemId = 1; systemId <= systemCount; systemId++) {
//        U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
//        for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
//            if (m_boards.size() >= MAX_BOARDS) break;
//            HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
//            if (handle == NULL) continue;
//
//            AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, boardId);
//            if (pBoard->QueryBoardInfo()) {
//                m_boards.push_back(pBoard);
//            }
//            else {
//                delete pBoard;
//            }
//        }
//    }
//    std::stringstream ss;
//    ss << "Found " << m_boards.size() << " digitizer(s).";
//    Log(ss.str());
//    return m_boards.size() > 0;
//}
//
//std::vector<std::string> AcquisitionController::GetBoardInfoList()
//{
//    std::vector<std::string> infoList;
//    for (AlazarDigitizer* board : m_boards) infoList.push_back(board->GetInfoString());
//    return infoList;
//}
//
//// --- Configuration (Standard) ---
//bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
//{
//    if (m_boards.empty()) {
//        Log("Error: No boards to configure.");
//        return false;
//    }
//    m_currentConfig = config;
//
//    // 1. Configure Digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) return false;
//    }
//
//    // 2. Configure SyncBoard (if present)
//    if (m_syncBoardHandle) {
//        // Always disable trigger output during config
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//        // Configure Clock Distribution
//        sb_clock_conf_t clock_conf;
//
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            // External Clock Mode: SyncBoard passes ECLK IN -> ECLK OUT
//            clock_conf.source = sb_clock_source_external;
//            clock_conf.sample_rate_hz = 0;
//        }
//        else {
//            // Internal Clock Mode: SyncBoard generates clock -> ECLK OUT
//            clock_conf.source = sb_clock_source_internal;
//            // Map the enum ID to a frequency (Simplified example)
//            if (config.sampleRateId == SAMPLE_RATE_100MSPS) clock_conf.sample_rate_hz = 100000000;
//            else clock_conf.sample_rate_hz = 10000000; // Default fallback
//        }
//
//        if (sb_device_set_clock(m_syncBoardHandle, clock_conf) != sb_rc_success) {
//            Log("Error: Failed to configure SyncBoard clock.");
//            return false;
//        }
//    }
//
//    return true;
//}
//
//// --- Standard Acquisition Run ---
//bool AcquisitionController::RunAcquisition(const AcquisitionConfig& config)
//{
//    if (m_isAcquiring) {
//        Log("Error: Acquisition already in progress.");
//        return false;
//    }
//    m_isAcquiring = true;
//    m_currentAcqConfig = config;
//
//    // 1. Ensure Gate is CLOSED
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//    }
//
//    // 2. Calculate Buffers
//    const int channelCount = 4;
//    U32 bytesPerSample = 2;
//    U32 samplesPerChannel = config.samplesPerRecord;
//    U32 recordsPerBuffer;
//
//    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) {
//        recordsPerBuffer = 1;
//        // For streaming, input is total size, divide by channels
//        samplesPerChannel = config.samplesPerRecord / channelCount;
//    }
//    else {
//        recordsPerBuffer = config.recordsPerBuffer;
//    }
//
//    U32 bytesPerBuffer = samplesPerChannel * channelCount * bytesPerSample * recordsPerBuffer;
//
//    // 3. Open Files
//    if (config.saveData) {
//        if (!OpenDataFiles(m_boards.size())) {
//            m_isAcquiring = false;
//            return false;
//        }
//    }
//
//    // 4. Allocate Buffers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//    }
//
//    // 5. Prepare Boards
//    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->PrepareForAcquisition(config, channelMask)) { StopAcquisition(); return false; }
//    }
//
//    // 6. Post Buffers
//    for (AlazarDigitizer* board : m_boards) {
//        for (U32 i = 0; i < BUFFER_COUNT; i++) {
//            if (!board->PostBuffer(i)) { StopAcquisition(); return false; }
//        }
//    }
//
//    // 7. Start Capture (ARM BOARDS)
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // 8. OPEN GATE (Enable SyncBoard)
//    if (m_syncBoardHandle) {
//        Log("Enabling SyncBoard Trigger...");
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//    }
//
//    std::stringstream ss;
//    ss << "Capturing " << config.buffersPerAcquisition << " buffers...";
//    Log(ss.str());
//
//    // 9. Acquisition Loop
//    U32 buffersCompleted = 0;
//    bool success = true;
//
//    while (buffersCompleted < config.buffersPerAcquisition && success)
//    {
//        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
//        for (U32 boardIndex = 0; boardIndex < m_boards.size(); boardIndex++)
//        {
//            AlazarDigitizer* board = m_boards[boardIndex];
//
//            if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
//                success = false;
//                break;
//            }
//
//            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
//            U16* pBuffer = (U16*)pIoBuffer->pBuffer;
//
//            if (!ProcessBufferData(board, pBuffer, config)) {
//                success = false;
//                break;
//            }
//            if (!board->PostBuffer(bufferIndex)) {
//                success = false;
//                break;
//            }
//        }
//
//        if (!success) break;
//
//        buffersCompleted++;
//        if (buffersCompleted % 10 == 0) {
//            std::stringstream ss_prog;
//            ss_prog << "Completed " << buffersCompleted << " buffers";
//            Log(ss_prog.str());
//        }
//
//        // Optional: Check for console abort
//#ifdef _WIN32
//        if (_kbhit()) { Log("Abort request..."); break; }
//#endif
//    }
//
//    Log("Capture complete.");
//    StopAcquisition();
//    return success;
//}
//
//// ----------------------------------------------------------------------------
//// --- SYNC TEST FUNCTION ---
//// Forces correct settings, runs capture, and saves specific channel snapshots.
//// ----------------------------------------------------------------------------
//bool AcquisitionController::RunSyncTest(BoardConfig syncConfig)
//{
//    if (m_isAcquiring) { Log("Busy."); return false; }
//    if (m_boards.size() < 2) { Log("Error: Need 2 boards for sync test."); return false; }
//    if (!m_syncBoardHandle) { Log("Error: No SyncBoard detected for test."); return false; }
//
//    m_isAcquiring = true;
//    Log("--- Starting SyncBoard Verification Test ---");
//
//    // 1. Force Hardware Config
//    //BoardConfig syncConfig = {};
//    //syncConfig.sampleRateId = SAMPLE_RATE_USER_DEF;  // Listen to SyncBox Clock
//    //syncConfig.sampleRateHz = 25000000.0;            // Expect 25MHz external clock
//    //syncConfig.inputRangeId = INPUT_RANGE_PM_1_V;
//    //syncConfig.couplingId = DC_COUPLING;
//    //syncConfig.impedanceId = IMPEDANCE_50_OHM;
//    //syncConfig.triggerSourceId = TRIG_EXTERNAL;      // Listen to SyncBox Trigger
//    //syncConfig.triggerSlopeId = TRIGGER_SLOPE_POSITIVE;
//    //syncConfig.triggerLevelCode = 140;               // ~0.5V
//    //syncConfig.triggerTimeoutMS = 10000;              // 2 sec timeout
//
//    // 2. Configure SyncBoard (Close Gate)
//    sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//    sb_clock_conf_t clock_conf = { sb_clock_source_external, 0 };
//    sb_device_set_clock(m_syncBoardHandle, clock_conf);
//
//    // 3. Configure Digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(syncConfig)) { StopAcquisition(); return false; }
//    }
//
//    // 4. Prepare Acquisition (NPT, 1 Record)
//    AcquisitionConfig acqConfig = {};
//    acqConfig.admaMode = ADMA_NPT;
//    acqConfig.samplesPerRecord = 4096;
//    acqConfig.recordsPerBuffer = 1;
//    acqConfig.buffersPerAcquisition = 1;
//    acqConfig.saveData = false;
//    acqConfig.processData = false;
//
//    U32 bytesPerBuffer = 4096 * 4 * 2; // 4k samples * 4 ch * 2 bytes
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//        /*if (!board->PrepareForAcquisition(acqConfig, CHANNEL_ALL)) { StopAcquisition(); return false; }*/
//        // Enable A, B, C, and D explicitly
//        U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
//        if (!board->PrepareForAcquisition(acqConfig, channelMask)) { StopAcquisition(); return false; }
//        if (!board->PostBuffer(0)) { StopAcquisition(); return false; }
//    }
//
//    // 5. ARM THE DIGITIZERS
//    Log("Arming boards...");
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // 6. OPEN THE GATE
//    Log("Enabling SyncBoard Trigger...");
//    sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//
//    // 7. Wait for Data
//    bool success = true;
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->WaitFordBuffer(0, 5000)) {
//            Log("Error: Timeout waiting for Board " + std::to_string(board->GetBoardId()));
//            success = false;
//        }
//    }
//
//    if (success) {
//        Log("Data captured successfully.");
//
//        // 8. SAVE SNAPSHOTS
//        char currentDir[MAX_PATH];
//        if (_getcwd(currentDir, MAX_PATH)) {
//            std::string path = std::string(currentDir);
//
//            auto SaveChannel = [&](AlazarDigitizer* board, int channelIndex, std::string filename) {
//                U16* buffer = (U16*)board->GetBuffer(0)->pBuffer;
//                std::vector<U16> chData(4096);
//                // De-interleave
//                for (int i = 0; i < 4096; i++) chData[i] = buffer[i * 4 + channelIndex];
//
//                std::ofstream outfile(path + "\\" + filename, std::ios::binary);
//                outfile.write((char*)chData.data(), chData.size() * sizeof(U16));
//                outfile.close();
//                Log("Saved " + filename);
//                return chData;
//                };
//
//            auto b1_chA = SaveChannel(m_boards[0], 0, "sync_snapshot_b1_chA.bin");
//            auto b2_chA = SaveChannel(m_boards[1], 0, "sync_snapshot_b2_chA.bin");
//
//            // Convert for plot check
//            std::vector<float> f_b1(4096), f_b2(4096);
//            for (int i = 0; i < 4096; i++) f_b1[i] = (float)b1_chA[i];
//            for (int i = 0; i < 4096; i++) f_b2[i] = (float)b2_chA[i];
//
//            PlotSyncTest_Stub(f_b1, f_b2);
//        }
//    }
//
//    StopAcquisition();
//    return success;
//}
//
//// --- Stop & Cleanup ---
//void AcquisitionController::StopAcquisition()
//{
//    // Close gate
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//    }
//
//    for (AlazarDigitizer* board : m_boards) {
//        board->AbortAcquisition();
//        board->FreeBuffers();
//    }
//    CloseDataFiles();
//    m_isAcquiring = false;
//}
//
//// --- File Handling ---
//bool AcquisitionController::OpenDataFiles(U32 boardCount)
//{
//    char currentDir[MAX_PATH];
//    if (!_getcwd(currentDir, MAX_PATH)) {
//        Log("Error: Could not get current working directory.");
//        return false;
//    }
//    m_savePath = std::string(currentDir);
//    Log("Saving data to: " + m_savePath);
//
//    m_fileStreams.resize(boardCount * 4);
//    char channels[] = { 'A', 'B', 'C', 'D' };
//
//    for (U32 boardIndex = 0; boardIndex < boardCount; boardIndex++)
//    {
//        for (int ch = 0; ch < 4; ch++)
//        {
//            std::stringstream ss;
//            ss << m_savePath << "\\board" << (m_boards[boardIndex]->GetBoardId()) << "_ch" << channels[ch] << ".bin";
//            int file_index = boardIndex * 4 + ch;
//            m_fileStreams[file_index].open(ss.str(), std::ios::binary);
//            if (!m_fileStreams[file_index].is_open()) {
//                Log("Error: Unable to create file " + ss.str());
//                return false;
//            }
//        }
//    }
//    return true;
//}
//
//void AcquisitionController::CloseDataFiles()
//{
//    for (auto& fileStream : m_fileStreams) {
//        if (fileStream.is_open()) fileStream.close();
//    }
//    m_fileStreams.clear();
//}
//
//// --- Processing ---
//bool AcquisitionController::ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config)
//{
//    U32 samplesPerChannel = config.samplesPerRecord;
//    U32 recordsPerBuffer;
//    U32 channelCount = 4;
//
//    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) {
//        recordsPerBuffer = 1;
//        samplesPerChannel = config.samplesPerRecord / channelCount;
//    }
//    else {
//        recordsPerBuffer = config.recordsPerBuffer;
//    }
//
//    // De-interleaving
//    std::vector<U16> chanA_full(samplesPerChannel * recordsPerBuffer);
//    std::vector<U16> chanB_full(samplesPerChannel * recordsPerBuffer);
//    std::vector<U16> chanC_full(samplesPerChannel * recordsPerBuffer);
//    std::vector<U16> chanD_full(samplesPerChannel * recordsPerBuffer);
//
//    for (U32 r = 0; r < recordsPerBuffer; r++)
//    {
//        for (U32 s = 0; s < samplesPerChannel; s++)
//        {
//            U32 interleaved_index = (r * (samplesPerChannel * channelCount)) + (s * channelCount);
//            U32 flat_index = r * samplesPerChannel + s;
//            chanA_full[flat_index] = buffer[interleaved_index + 0];
//            chanB_full[flat_index] = buffer[interleaved_index + 1];
//            chanC_full[flat_index] = buffer[interleaved_index + 2];
//            chanD_full[flat_index] = buffer[interleaved_index + 3];
//        }
//    }
//
//    if (config.saveData)
//    {
//        U32 boardIndex = 0;
//        for (U32 i = 0; i < m_boards.size(); ++i) {
//            if (m_boards[i]->GetHandle() == board->GetHandle()) {
//                boardIndex = i;
//                break;
//            }
//        }
//
//        int f_idx = boardIndex * 4;
//        m_fileStreams[f_idx + 0].write((char*)chanA_full.data(), chanA_full.size() * sizeof(U16));
//        m_fileStreams[f_idx + 1].write((char*)chanB_full.data(), chanB_full.size() * sizeof(U16));
//        m_fileStreams[f_idx + 2].write((char*)chanC_full.data(), chanC_full.size() * sizeof(U16));
//        m_fileStreams[f_idx + 3].write((char*)chanD_full.data(), chanD_full.size() * sizeof(U16));
//    }
//
//    if (config.processData)
//    {
//        // Plotting Logic (Convert to Volts)
//        std::vector<float> volts_ChA(samplesPerChannel);
//        std::vector<float> volts_ChB(samplesPerChannel);
//        std::vector<float> volts_ChC(samplesPerChannel);
//        std::vector<float> volts_ChD(samplesPerChannel);
//        U32 bitShift = 2;
//        float codeZero = (float)((1 << (14 - 1)) - 0.5);
//        float codeRange = (float)((1 << (14 - 1)) - 0.5);
//        float inputRange = (float)m_currentConfig.inputRangeVolts;
//
//        for (U32 i = 0; i < samplesPerChannel; i++)
//        {
//            volts_ChA[i] = inputRange * ((float)(chanA_full[i] >> bitShift) - codeZero) / codeRange;
//            volts_ChB[i] = inputRange * ((float)(chanB_full[i] >> bitShift) - codeZero) / codeRange;
//            volts_ChC[i] = inputRange * ((float)(chanC_full[i] >> bitShift) - codeZero) / codeRange;
//            volts_ChD[i] = inputRange * ((float)(chanD_full[i] >> bitShift) - codeZero) / codeRange;
//        }
//        PlotOscilloscope_Stub(volts_ChA, volts_ChB, volts_ChC, volts_ChD, board->GetBoardId());
//        PlotSpectrum_Stub(volts_ChA, volts_ChB, volts_ChC, volts_ChD, m_currentConfig.sampleRateHz, board->GetBoardId());
//    }
//    return true;
//}
//
//// --- Plotting Stubs ---
//void AcquisitionController::PlotOscilloscope_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, U32 boardId)
//{
//    std::stringstream ss;
//    ss << "  Board " << boardId << " Plot: ChA[0]=" << chA[0] << "V";
//    Log(ss.str());
//}
//
//void AcquisitionController::PlotSpectrum_Stub(std::vector<float>& chA, std::vector<float>& chB, std::vector<float>& chC, std::vector<float>& chD, double sampleRate, U32 boardId)
//{
//}
//
////void AcquisitionController::PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA)
////{
////    float diff = 0.f;
////    if (board1_chA.empty()) return;
////
////    for (size_t i = 0; i < board1_chA.size(); ++i) {
////        diff += std::abs(board1_chA[i] - board2_chA[i]);
////    }
////    float avg_diff = diff / board1_chA.size();
////
////    std::stringstream ss;
////    ss << "  [Sync Check]: Avg voltage difference = " << avg_diff << " V";
////    Log(ss.str());
////
////    if (avg_diff < 0.05) Log("  [Sync Check]: PASSED. Boards are synchronized.");
////    else Log("  [Sync Check]: FAILED. Difference detected.");
////}
//
//
///**
// * @brief Calculates the time delay (lag) between two signals using Normalized Cross-Correlation.
// * * This function "slides" signal Y across signal X and calculates a similarity score
// * (Pearson correlation coefficient) for every possible shift position.
// * The shift that produces the highest score is the true time delay between the signals.
// * * Math: Score = sum((x[i] - mean_x) * (y[i+lag] - mean_y)) / (std_dev_x * std_dev_y)
// * * @param x The reference signal (e.g., Board 1, Channel A).
// * @param y The signal to check (e.g., Board 2, Channel A).
// * @param maxLag The maximum number of samples to shift left or right.
// * (e.g., 50 means check delays from -50 to +50 samples).
// * @return int The lag in samples.
// * 0 = Perfectly synced.
// * Positive = Y is delayed (arrived later than X).
// * Negative = Y is ahead (arrived earlier than X).
// */
//static int FindLagByXCorr(const std::vector<float>& x, const std::vector<float>& y, int maxLag)
//{
//    // --- 1. Safety Checks ---
//    int N = (int)x.size();
//    // If signals are empty or different lengths, we can't compare them.
//    if (N == 0 || (int)y.size() != N) return 0;
//
//    // --- 2. Center the Signals (Mean Removal) ---
//    // Cross-correlation works best on the *shape* of the wave, not the voltage offset.
//    // We calculate the average voltage (DC offset) and subtract it from every point.
//    // This makes both signals oscillate around 0.
//    float mean_x = 0.f, mean_y = 0.f;
//    for (int i = 0; i < N; ++i) {
//        mean_x += x[i];
//        mean_y += y[i];
//    }
//    mean_x /= N;
//    mean_y /= N;
//
//    // Create new temporary vectors for the centered data
//    std::vector<float> xc(N), yc(N);
//    for (int i = 0; i < N; ++i) {
//        xc[i] = x[i] - mean_x;
//        yc[i] = y[i] - mean_y;
//    }
//
//    // --- 3. The Search Loop ---
//    int bestLag = 0;
//    double bestScore = -1e308; // Start with a very low score (negative infinity)
//
//    // Try every shift from -maxLag (left) to +maxLag (right)
//    for (int lag = -maxLag; lag <= maxLag; ++lag)
//    {
//        double num = 0.0;        // Numerator: The sum of products (Covariance)
//        double denom_x = 0.0;    // Denominator X: Energy of signal X
//        double denom_y = 0.0;    // Denominator Y: Energy of signal Y
//
//        // Loop through every sample 'i' in the signals
//        for (int i = 0; i < N; ++i)
//        {
//            int j = i + lag; // The index in the shifted signal Y
//
//            // Bounds check: If shifting moves index 'j' past the start or end of the buffer,
//            // we skip this point (it has no overlapping partner).
//            if (j < 0 || j >= N) continue;
//
//            // Accumulate the correlation product
//            // If both waves go UP at the same time, num grows (+ * + = +)
//            // If both waves go DOWN at the same time, num grows (- * - = +)
//            // If one goes UP while other goes DOWN, num shrinks (+ * - = -)
//            num += (double)xc[i] * (double)yc[j];
//
//            // Accumulate the energy (sum of squares) for normalization
//            denom_x += (double)xc[i] * (double)xc[i];
//            denom_y += (double)yc[j] * (double)yc[j];
//        }
//
//        // --- 4. Calculate Normalized Score ---
//        // Pearson Correlation = Covariance / (StdDev_X * StdDev_Y)
//        // The 'sqrt' calculates the standard deviation part.
//        // +1e-20 prevents division by zero if a signal is perfectly flat.
//        double denom = sqrt(denom_x * denom_y) + 1e-20;
//        double score = num / denom;
//
//        // --- 5. Check for Winner ---
//        // If this shift produced a better match than any previous shift, record it.
//        if (score > bestScore) {
//            bestScore = score;
//            bestLag = lag;
//        }
//    }
//
//    // Return the lag that produced the highest similarity score.
//    return bestLag;
//}
//void AcquisitionController::PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA)
//{
//    if (board1_chA.empty() || board2_chA.empty()) return;
//
//    // 1. Calculate the Lag
//    // We search +/- 50 samples. If they are synced, it should be exactly 0.
//    int lag = FindLagByXCorr(board1_chA, board2_chA, 50);
//
//    std::stringstream ss;
//    ss << "  [Sync Check]: Calculated Lag = " << lag << " samples";
//    Log(ss.str());
//
//    // 2. Interpret Result
//    if (lag == 0)
//    {
//        Log("  [Sync Check]: SUCCESS! Boards are perfectly synchronized (Lag 0).");
//    }
//    else if (std::abs(lag) <= 1)
//    {
//        Log("  [Sync Check]: PASS. (Lag <= 1 sample is usually acceptable jitter).");
//    }
//    else
//    {
//        std::stringstream err;
//        err << "  [Sync Check]: FAILED. Boards are offset by " << lag << " samples.";
//        Log(err.str());
//    }
//}
//
//
//// Simple normalized cross-correlation to find lag (naive O(N^2) for clarity).
//// For large N you can replace with FFT-based correlation (O(N log N)).
//
////