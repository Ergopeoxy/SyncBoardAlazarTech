//#include "AcquisitionControllerMultithread.h"
//#include <stdio.h>
//#include <iostream>
//#include <sstream>
//#include <cmath> 
//#include <algorithm>
//#include <string>
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
//// ============================================================================
//// HELPER FUNCTIONS
//// ============================================================================
//
//// --- Cross-Correlation for Sync Verification ---
//static int FindLagByXCorr(const std::vector<float>& x, const std::vector<float>& y, int maxLag)
//{
//    int N = (int)x.size();
//    if (N == 0 || (int)y.size() != N) return 0;
//
//    float mean_x = 0.f, mean_y = 0.f;
//    for (int i = 0; i < N; ++i) { mean_x += x[i]; mean_y += y[i]; }
//    mean_x /= N; mean_y /= N;
//
//    std::vector<float> xc(N), yc(N);
//    for (int i = 0; i < N; ++i) { xc[i] = x[i] - mean_x; yc[i] = y[i] - mean_y; }
//
//    int bestLag = 0;
//    double bestScore = -1e308;
//
//    for (int lag = -maxLag; lag <= maxLag; ++lag)
//    {
//        double num = 0.0, denom_x = 0.0, denom_y = 0.0;
//        for (int i = 0; i < N; ++i)
//        {
//            int j = i + lag;
//            if (j < 0 || j >= N) continue;
//            num += (double)xc[i] * (double)yc[j];
//            denom_x += (double)xc[i] * (double)xc[i];
//            denom_y += (double)yc[j] * (double)yc[j];
//        }
//        double denom = sqrt(denom_x * denom_y) + 1e-20;
//        double score = num / denom;
//        if (score > bestScore) { bestScore = score; bestLag = lag; }
//    }
//    return bestLag;
//}
//
//// ============================================================================
//// CONSTRUCTOR & DESTRUCTOR
//// ============================================================================
//
//AcquisitionController::AcquisitionController() :
//    m_isAcquiring(false),
//    m_syncBoardHandle(nullptr),
//    m_keepSaving(true)
//{
//    // Start the background Consumer (Save) Thread immediately
//    m_saveThread = std::thread(&AcquisitionController::SaveThreadLoop, this);
//}
//AcquisitionController::~AcquisitionController()
//{
//    // 1. Force the acquisition loop to break
//    m_isAcquiring = false;
//
//    // 2. Stop the hardware immediately (aborts any pending DMA waits)
//    StopAcquisition();
//
//    // 3. Signal the Save Thread to die
//    m_keepSaving = false;
//    m_queueCondition.notify_all();
//
//    // 4. Wait for Save Thread to finish
//    if (m_saveThread.joinable()) {
//        m_saveThread.join();
//    }
//
//    // 5. Cleanup hardware handles
//    if (m_syncBoardHandle) {
//        sb_device_close(m_syncBoardHandle);
//        m_syncBoardHandle = nullptr;
//    }
//
//    for (AlazarDigitizer* board : m_boards) delete board;
//    m_boards.clear();
//}
//
////AcquisitionController::~AcquisitionController()
////{
////    // 1. Stop Hardware
////    if (m_isAcquiring) StopAcquisition();
////
////    // 2. Stop Save Thread
////    m_keepSaving = false;
////    m_queueCondition.notify_all(); // Wake up thread so it can check m_keepSaving and exit
////    if (m_saveThread.joinable()) {
////        m_saveThread.join();
////    }
////
////    // 3. Close SyncBoard
////    if (m_syncBoardHandle) {
////        sb_device_close(m_syncBoardHandle);
////        m_syncBoardHandle = nullptr;
////    }
////
////    // 4. Cleanup Digitizers
////    for (AlazarDigitizer* board : m_boards) delete board;
////    m_boards.clear();
////}
//
//// ============================================================================
//// THREADING: CONSUMER LOOP (Writes to Disk)
//// ============================================================================
//
//void AcquisitionController::SaveThreadLoop()
//{
//    while (m_keepSaving)
//    {
//        DataChunk chunk;
//        {
//            // Lock and Wait
//            std::unique_lock<std::mutex> lock(m_queueMutex);
//            m_queueCondition.wait(lock, [this] { return !m_saveQueue.empty() || !m_keepSaving; });
//
//            if (!m_keepSaving && m_saveQueue.empty()) return;
//
//            // Pop oldest chunk
//            chunk = std::move(m_saveQueue.front());
//            m_saveQueue.pop();
//        }
//        // Mutex is unlocked here. Writing to disk happens in parallel with acquisition.
//
//        // Determine File Index based on Board ID
//        // Assuming: Board ID 1 = Index 0, Board ID 2 = Index 1
//        int boardIdx = -1;
//        if (chunk.boardId == 1) boardIdx = 0;
//        else if (chunk.boardId == 2) boardIdx = 1;
//
//        if (boardIdx >= 0)
//        {
//            int f_idx = boardIdx * 4;
//            // Safety check to ensure files are open
//            if (f_idx + 3 < m_fileStreams.size())
//            {
//                if (!chunk.chA.empty() && m_fileStreams[f_idx + 0].is_open())
//                    m_fileStreams[f_idx + 0].write((char*)chunk.chA.data(), chunk.chA.size() * sizeof(U16));
//
//                if (!chunk.chB.empty() && m_fileStreams[f_idx + 1].is_open())
//                    m_fileStreams[f_idx + 1].write((char*)chunk.chB.data(), chunk.chB.size() * sizeof(U16));
//
//                if (!chunk.chC.empty() && m_fileStreams[f_idx + 2].is_open())
//                    m_fileStreams[f_idx + 2].write((char*)chunk.chC.data(), chunk.chC.size() * sizeof(U16));
//
//                if (!chunk.chD.empty() && m_fileStreams[f_idx + 3].is_open())
//                    m_fileStreams[f_idx + 3].write((char*)chunk.chD.data(), chunk.chD.size() * sizeof(U16));
//            }
//        }
//    }
//}
//
//// ============================================================================
//// HARDWARE SETUP & CONFIGURATION
//// ============================================================================
//
//bool AcquisitionController::DiscoverBoards()
//{
//    Log("Finding hardware...");
//
//    // 1. Discover SyncBoard
//    if (m_syncBoardHandle == nullptr) {
//        size_t count = 0;
//        sb_get_device_count(&count);
//        if (count > 0) {
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
//    //// 2. Discover Digitizers
//    U32 systemCount = AlazarNumOfSystems();
//    if (systemCount < 1) {
//        Log("Error: No AlazarTech systems found.");
//        return false;
//    }
//
//    //for (AlazarDigitizer* board : m_boards) delete board;
//    //m_boards.clear();
//
//    //for (U32 systemId = 1; systemId <= systemCount; systemId++) {
//    //    U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
//    //    for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
//    //        if (m_boards.size() >= MAX_BOARDS) break;
//
//    //        HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
//    //        if (handle == NULL) continue;
//
//    //        AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, boardId);
//    //        if (pBoard->QueryBoardInfo()) {
//    //            m_boards.push_back(pBoard);
//    //        }
//    //        else {
//    //            delete pBoard;
//    //        }
//    //    }
//    //}
//
//
//    U32 uniqueLogicalId = 1; // Start counting from 1
//
//    // --- CRITICAL FIX: CLEAN UP OLD BOARDS ---
//    // If we don't do this, we get duplicate boards (Found 4, Found 6, etc.)
//    for (AlazarDigitizer* board : m_boards) {
//        delete board; // Destructor closes the board handle
//    }
//    m_boards.clear(); // Empty the vector size to 0
//
//
//
//
//    for (U32 systemId = 1; systemId <= systemCount; systemId++) {
//        U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
//        for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
//            if (m_boards.size() >= MAX_BOARDS) break;
//
//            HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
//            if (handle == NULL) continue;
//
//            // WE PASS 'uniqueLogicalId' AS THE BOARD ID
//            // This ensures Board 1 gets ID 1, Board 2 gets ID 2, regardless of hardware settings.
//            AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, uniqueLogicalId++);
//
//            if (pBoard->QueryBoardInfo()) {
//                m_boards.push_back(pBoard);
//            }
//            else {
//                delete pBoard;
//            }
//        }
//    }
//
//
//    std::stringstream ss;
//    ss << "Found " << m_boards.size() << " digitizer(s).";
//    Log(ss.str());
//    return m_boards.size() > 0;
//}
//// ^^^ FIX: This closing brace was likely missing or corrupted in your previous code ^^^
//
//std::vector<std::string> AcquisitionController::GetBoardInfoList()
//{
//    std::vector<std::string> infoList;
//    for (AlazarDigitizer* board : m_boards) {
//        infoList.push_back(board->GetInfoString());
//    }
//    return infoList;
//}
//
////bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
////{
////    if (m_boards.empty()) {
////        Log("Error: No boards to configure.");
////        return false;
////    }
////    m_currentConfig = config;
////
////    // Configure all digitizers
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->ConfigureBoard(config)) return false;
////    }
////
////    // Configure SyncBoard (if present)
////    if (m_syncBoardHandle) {
////        // Disable trigger during config
////        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
////
////        sb_clock_conf_t clock_conf;
////        // Logic to determine if SyncBoard generates clock or passes it through
////        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
////            clock_conf.source = sb_clock_source_external; // External Clock In
////            clock_conf.sample_rate_hz = 0;
////            std::cout << "the syncboard generates clock signal";
////        }
////        else {
////            clock_conf.source = sb_clock_source_internal; // Internal Clock
////            clock_conf.sample_rate_hz = 100000000;        // Example: 100MHz
////        }
////
////        if (sb_device_set_clock(m_syncBoardHandle, clock_conf) != sb_rc_success) {
////            Log("Error: Failed to configure SyncBoard clock.");
////            return false;
////        }
////    }
////    return true;
////}
//bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
//{
//    if (m_boards.empty()) {
//        Log("Error: No boards to configure.");
//        return false;
//    }
//    m_currentConfig = config;
//
//    // Configure all digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) return false;
//    }
//
//    // Configure SyncBoard (if present)
//    if (m_syncBoardHandle) {
//        // Disable trigger during config
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//        sb_clock_conf_t clock_conf;
//
//        // Logic to determine if SyncBoard generates clock or passes it through
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            clock_conf.source = sb_clock_source_external; // External Clock In
//            clock_conf.sample_rate_hz = 0;                // Pass-through (0 usually implies follow input)
//            std::cout << "=====================================sync board generates clock=====================================";
//        }
//        else {
//            clock_conf.source = sb_clock_source_internal; // Internal Clock
//
//            // --- FIX: Use the variable from config, not a hardcoded number ---
//            // Old: clock_conf.sample_rate_hz = 100000000; 
//            clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
//        }
//
//        if (sb_device_set_clock(m_syncBoardHandle, clock_conf) != sb_rc_success) {
//            Log("Error: Failed to configure SyncBoard clock.");
//            return false;
//        }
//    }
//    return true;
//}
//// ============================================================================
//// PRODUCER: ACQUISITION LOOP
//// ============================================================================
//
//bool AcquisitionController::RunAcquisition(const AcquisitionConfig& config)
//{
//    if (m_isAcquiring) {
//        Log("Error: Acquisition already in progress.");
//        return false;
//    }
//
//
//    // [DEBUG 1] Verify board count immediately
//    std::stringstream ss_start;
//    ss_start << "RunAcquisition started. Boards in list: " << m_boards.size();
//    Log(ss_start.str());
//
//    if (m_boards.size() < 2) {
//        Log("[WARNING] Only 1 board found! Board 2 cannot be displayed.");
//    }
//
//
//    m_isAcquiring = true;
//    m_currentAcqConfig = config;
//
//
//
//
//
//    // 1. Close SyncBoard Gate
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//    // 2. Setup Memory Sizes
//    const int channelCount = 4;
//    // For Strea                                     ming/Continuous, Alazar treats records differently
//    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) ? 1 : config.recordsPerBuffer;
//    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) ? (config.samplesPerRecord / channelCount) : config.samplesPerRecord;
//
//    // Total bytes = samples * channels * 2 bytes/sample * records
//    U32 bytesPerBuffer = samplesPerChannel * channelCount * 2 * recordsPerBuffer;
//
//    // 3. Open Files (if saving enabled)
//    if (config.saveData) {
//        if (!OpenDataFiles((U32)m_boards.size())) {
//            m_isAcquiring = false;
//            return false;
//        }
//    }
//
//    // 4. Prepare Boards (Allocate & Post Buffers)
//    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//        if (!board->PrepareForAcquisition(config, channelMask)) { StopAcquisition(); return false; }
//
//        // Post all buffers to the driver to create the "hardware ring"
//        for (U32 i = 0; i < BUFFER_COUNT; i++) board->PostBuffer(i);
//
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // 5. Open SyncBoard Gate (Triggers can now pass)
//    if (m_syncBoardHandle) {
//        Log("Enabling SyncBoard Trigger...");
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//    }
//
//    std::stringstream ss;
//    ss << "Capturing " << config.buffersPerAcquisition << " buffers...";
//    Log(ss.str());
//
//    // 6. Fast Loop (The "Producer")
//    U32 buffersCompleted = 0;
//    bool success = true;
//
//    while (buffersCompleted < config.buffersPerAcquisition && success && m_isAcquiring)
//    {
//        // Round-Robin buffer index (0, 1, 2, 3, 0...)
//        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
//
//        // [DEBUG 2] Trace the Inner Loop execution ONCE
//        if (buffersCompleted == 0) {
//            std::cout << "[Debug] Processing Buffer 0. Expecting " << m_boards.size() << " iterations." << std::endl;
//        }
//
//
//        //for (AlazarDigitizer* board : m_boards)
//        //{
//        //    // A. Wait for this specific buffer to be filled by hardware
//        //    if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
//        //        success = false; break;
//        //    }
//
//        //    // B. Process Data (Copy to RAM -> Push to Queue)
//        //    IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
//        //    if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, config)) {
//        //        success = false; break;
//        //    }
//
//        //    // C. Re-Post Buffer (Give it back to the hardware immediately)
//        //    if (!board->PostBuffer(bufferIndex)) {
//        //        success = false; break;
//        //    }
//        //}
//        for (size_t i = 0; i < m_boards.size(); i++)
//        {
//            AlazarDigitizer* board = m_boards[i];
//
//            // [DEBUG 3] Print which board we are waiting for (Only once)
//            if (buffersCompleted == 0) {
//                std::cout << "   -> Waiting for Board Index " << i << " (ID: " << board->GetBoardId() << ")..." << std::endl;
//            }
//
//            // A. Wait for buffer
//            if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
//                // [DEBUG 4] Critical Failure Log
//                std::stringstream err;
//                err << "[CRITICAL ERROR] Timeout waiting for Board Index " << i << " (ID: " << board->GetBoardId() << ")";
//                Log(err.str());
//
//                success = false; break;
//            }
//
//            // [DEBUG 5] Confirm Trigger Received (Only once)
//            if (buffersCompleted == 0) {
//                std::cout << "   -> Success! Board " << board->GetBoardId() << " triggered." << std::endl;
//            }
//
//            // B. Process Data
//            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
//            if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, config)) {
//                success = false; break;
//            }
//
//            // C. Re-Post
//            if (!board->PostBuffer(bufferIndex)) {
//                success = false; break;
//            }
//
//            if (buffersCompleted % config.logInterval == 0) {
//                std::stringstream ss_prog;
//                ss_prog << "Captured " << buffersCompleted << " buffers";
//                Log(ss_prog.str());
//            }
//
//
//        }
//
//        if (!success) break;
//        buffersCompleted++;
//
//        if (buffersCompleted % 100 == 0) {
//            std::stringstream ss_prog;
//            ss_prog << "Captured " << buffersCompleted << " buffers";
//            Log(ss_prog.str());
//        }
//    }
//
//    Log("Acquisition complete.");
//    StopAcquisition();
//    return success;
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//bool AcquisitionController::ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config)
//{
//    // 1. Calculate Sizes
//    U32 channelCount = 4;
//    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) ? 1 : config.recordsPerBuffer;
//    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING) ? (config.samplesPerRecord / channelCount) : config.samplesPerRecord;
//
//    // 2. Create Data Chunk
//    DataChunk chunk;
//    chunk.boardId = board->GetBoardId();
//    size_t totalSamples = samplesPerChannel * recordsPerBuffer;
//
//    // --- DEBUG LOG START ---
//    // Only log once every 1000 frames so we don't freeze the console
//    static int debugCounter = 0;
//    if (debugCounter++ % 1000 == 0) {
//        std::stringstream ss;
//        ss << "Processing Data for Board ID: " << chunk.boardId;
//        Log(ss.str());
//    }
//    // --- DEBUG LOG END ---
//
//
//
//    chunk.chA.resize(totalSamples);
//    chunk.chB.resize(totalSamples);
//    chunk.chC.resize(totalSamples);
//    chunk.chD.resize(totalSamples);
//
//    // 3. De-interleave (RAM Copy)
//    for (U32 r = 0; r < recordsPerBuffer; r++)
//    {
//        for (U32 s = 0; s < samplesPerChannel; s++)
//        {
//            U32 interleaved_index = (r * (samplesPerChannel * 4)) + (s * 4);
//            U32 flat_index = r * samplesPerChannel + s;
//
//            chunk.chA[flat_index] = buffer[interleaved_index + 0];
//            chunk.chB[flat_index] = buffer[interleaved_index + 1];
//            chunk.chC[flat_index] = buffer[interleaved_index + 2];
//            chunk.chD[flat_index] = buffer[interleaved_index + 3];
//        }
//    }
//
//    // --- REORDERED: Update GUI FIRST (Before moving/destroying 'chunk') ---
//    //if (config.processData && chunk.boardId == 1)
//    //{
//    //    std::lock_guard<std::mutex> lock(m_guiDataMutex);
//
//    //    // Convert to float for ImPlot
//    //    m_latestScopeData.resize(samplesPerChannel);
//    //    double range = 1.0; // Input Range +/- 1V
//    //    double codeZero = 8191.5;
//    //    double codeRange = 8191.5;
//
//    //    size_t limit = (std::min)((size_t)4096, (size_t)samplesPerChannel);
//    //    for (size_t i = 0; i < limit; ++i) {
//    //        // chunk.chA is still valid here!
//    //        m_latestScopeData[i] = range * ((double)(chunk.chA[i] >> 2) - codeZero) / codeRange;
//    //    }
//    //}
//    //if (config.processData) // <-- REMOVED "&& chunk.boardId == 1"
//    //{
//    //    std::lock_guard<std::mutex> lock(m_guiDataMutex);
//
//
//    //    // --- DEBUG LOGGING START ---
//    //    // Print the size of the map (how many boards have data so far)
//    //    std::cout << "[Debug] m_latestScopeData holds data for " << m_latestScopeData.size() << " boards." << std::endl;
//
//    //    // Iterate through the map to see EXACTLY which Board IDs exist and how much data they have
//    //    for (const auto& pair : m_latestScopeData) {
//    //        U32 boardID = pair.first;
//    //        size_t dataSize = pair.second.size();
//    //        std::cout << "   -> Board ID: " << boardID << " | Data Points: " << dataSize << std::endl;
//    //    }
//    //    // --- DEBUG LOGGING END ---
//
//
//    //    // Select the specific vector for this board
//    //    std::vector<float>& scopeData = m_latestScopeData[chunk.boardId];
//    //    std::cout << "[Debug] in processBuffer getting scopedata"<< chunk.boardId;
//    //  
//
//
//   
//
//
//
//
//    //    double range = 1.0;
//    //    double codeZero = 8191.5;
//    //    double codeRange = 8191.5;
//
//    //    size_t limit = (std::min)((size_t)4096, (size_t)samplesPerChannel);
//    //    for (size_t i = 0; i < limit; ++i) {
//    //        scopeData[i] = range * ((double)(chunk.chA[i] >> 2) - codeZero) / codeRange;
//    //    }
//    //}
//    // 
//    // 
//    // 
//    
//
//    if (config.processData)
//    {
//        std::lock_guard<std::mutex> lock(m_guiDataMutex);
//
//           // --- DEBUG LOGGING START ---
//          // Print the size of the map (how many boards have data so far)
//          std::cout << "[Debug] m_latestScopeData holds data for " << m_latestScopeData.size() << " boards." << std::endl;
//
//
//
//          // IF SYNC TEST: Check if we have both boards
//          if (m_latestScopeData.count(1) && m_latestScopeData.count(2))
//          {
//              if (chunk.boardId == 2)
//              {
//                  // 1. Calculate Lag
//                  int lag = FindLagByXCorr(m_latestScopeData[1], m_latestScopeData[2], 50);
//
//                  // 2. LOGGING: Only log every X buffers
//                  static int syncCounter = 0;
//                  if (syncCounter++ % config.logInterval == 0)
//                  {
//                      std::stringstream ss;
//                      ss << "[Sync Check] Lag: " << lag << " samples";
//                      Log(ss.str());
//                  }
//
//                  // 3. Log Errors immediately (Always good to know if it fails)
//                  if (std::abs(lag) > 1) {
//                      std::stringstream err;
//                      err << "[Sync Drift] Boards drifted by " << lag << " samples!";
//                      Log(err.str());
//                  }
//
//                  m_syncSnapshot1 = m_latestScopeData[1];
//                  m_syncSnapshot2 = m_latestScopeData[2];
//              }
//          }
//
//
//
//
//
//
//          // Iterate through the map to see EXACTLY which Board IDs exist and how much data they have
//         /* for (const auto& pair : m_latestScopeData) {
//              U32 boardID = pair.first;
//              size_t dataSize = pair.second.size();
//              std::cout << "   -> Board ID: " << boardID << " | Data Points: " << dataSize << std::endl;
//          }*/
//          // --- DEBUG LOGGING END ---
//        // Get reference to the vector for THIS board ID
//        // (If m_latestScopeData is a MAP, this creates a valid entry safely)
//        std::vector<float>& scopeData = m_latestScopeData[chunk.boardId];
//
//        // Ensure we have memory allocated
//        if (scopeData.size() != samplesPerChannel) {
//            scopeData.resize(samplesPerChannel);
//        }
//
//        double range = 1.0;
//        double codeZero = 8191.5;
//        double codeRange = 8191.5;
//
//        // Protect against out-of-bounds if chunk is smaller than expected
//        size_t limit = (std::min)((size_t)4096, (size_t)chunk.chA.size());
//
//        for (size_t i = 0; i < limit; ++i) {
//            // Safe conversion
//            scopeData[i] = range * ((double)(chunk.chA[i] >> 2) - codeZero) / codeRange;
//        }
//    }
//
//
//    // --- REORDERED: Push to Save Queue LAST ---
//    if (config.saveData)
//    {
//        std::lock_guard<std::mutex> lock(m_queueMutex);
//
//        if (m_saveQueue.size() < MAX_QUEUE_SIZE) {
//            // WARNING: std::move DESTROYS 'chunk' (empties the vectors)
//            // Do not access 'chunk' after this line!
//            m_saveQueue.push(std::move(chunk));
//        }
//        else {
//            static int dropCounter = 0;
//            if (dropCounter++ % 100 == 0) Log("Warning: Disk too slow! Save Queue Full. Dropping frame.");
//        }
//        m_queueCondition.notify_one();
//    }
//
//    return true;
//}
//
//// ============================================================================
//// SYNC VERIFICATION TEST
//// ============================================================================
//
////bool AcquisitionController::RunSyncTest(const BoardConfig& config)
////{
////    if (m_isAcquiring) { Log("Busy."); return false; }
////    if (m_boards.size() < 2) { Log("Error: Need 2 boards for sync test."); return false; }
////
////    m_isAcquiring = true;
////    Log("--- Starting Sync Test ---");
////
////    // ------------------------------------------------------------------------
////    // 1. Configure SyncBoard (CRITICAL STEP)
////    // ------------------------------------------------------------------------
////    // We must set the clock *before* the digitizers try to lock onto it.
////    if (m_syncBoardHandle) {
////        // Disable trigger gate so we don't get garbage data during setup
////        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
////
////        sb_clock_conf_t clock_conf;
////        // Logic: If user wants External Clock, SyncBoard passes it through.
////        //        If Internal, SyncBoard generates it.
////        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
////            clock_conf.source = sb_clock_source_external;
////            clock_conf.sample_rate_hz = 0; // Pass-through
////        }
////        else {
////            clock_conf.source = sb_clock_source_internal;
////            clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
////        }
////
////        // Apply settings
////        auto err = sb_device_set_clock(m_syncBoardHandle, clock_conf);
////        if (err != sb_rc_success) {
////            Log("Error: SyncBoard Clock Config Failed.");
////            m_isAcquiring = false;
////            return false;
////        }
////    }
////
////    // ------------------------------------------------------------------------
////    // 2. Configure Digitizers
////    // ------------------------------------------------------------------------
////    // Now that the clock is stable, configure the boards to listen to it.
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->ConfigureBoard(config)) {
////            StopAcquisition();
////            return false;
////        }
////    }
////
////    // ------------------------------------------------------------------------
////    // 3. Setup Continuous Acquisition (NPT)
////    // ------------------------------------------------------------------------
////    AcquisitionConfig acqConfig = {};
////    acqConfig.admaMode = ADMA_NPT;
////    acqConfig.samplesPerRecord = 4096;
////    // Use a small buffer count (e.g., 10) for the test, or 1000 to run longer.
////    acqConfig.recordsPerBuffer = 1;
////    acqConfig.buffersPerAcquisition = 10000; // Run "infinitely" (or close to it)
////    acqConfig.saveData = false;  // Don't fill up the hard drive
////    acqConfig.processData = true; // Enable the GUI calculation
////
////    // Set Buffer Size (4096 samples * 4 channels * 2 bytes)
////    U32 bytesPerBuffer = 4096 * 4 * 2;
////    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
////
////    // ------------------------------------------------------------------------
////    // 4. Allocate & Arm
////    // ------------------------------------------------------------------------
////    for (AlazarDigitizer* board : m_boards) {
////        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
////        if (!board->PrepareForAcquisition(acqConfig, channelMask)) { StopAcquisition(); return false; }
////
////        // Post buffers to start the "Ring"
////        for (int i = 0; i < BUFFER_COUNT; i++) board->PostBuffer(i);
////
////        if (!board->StartCapture()) { StopAcquisition(); return false; }
////    }
////
////    // ------------------------------------------------------------------------
////    // 5. Open the Gate (Enable Triggers)
////    // ------------------------------------------------------------------------
////    Log("Boards Armed. Enabling SyncBoard Trigger...");
////    if (m_syncBoardHandle) {
////        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
////    }
////
////    // ------------------------------------------------------------------------
////    // 6. Run the Fast Loop
////    // ------------------------------------------------------------------------
////    // We re-use the RunAcquisition logic for the loop itself
////    // But we need to ensure 'processData' inside ProcessBufferData handles the math.
////
////    U32 buffersCompleted = 0;
////    bool success = true;
////
////    while (buffersCompleted < acqConfig.buffersPerAcquisition && success && m_isAcquiring)
////    {
////        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
////
////        for (AlazarDigitizer* board : m_boards)
////        {
////            if (!board->WaitFordBuffer(bufferIndex, config.triggerTimeoutMS + 1000)) {
////                success = false; break;
////            }
////
////            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
////
////            // ProcessBufferData will handle the Sync Math (Lag Calculation)
////            if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, acqConfig)) {
////                success = false; break;
////            }
////
////            if (!board->PostBuffer(bufferIndex)) {
////                success = false; break;
////            }
////        }
////        if (!success) break;
////        buffersCompleted++;
////    }
////
////    StopAcquisition();
////    return success;
////}
//
//
//bool AcquisitionController::RunSyncTest(const BoardConfig& config)
//{
//    if (m_isAcquiring) { Log("Busy."); return false; }
//    if (m_boards.size() < 2) { Log("Error: Need 2 boards."); return false; }
//
//    m_isAcquiring = true;
//    Log("--- Starting Sync Test ---");
//
//    // 1. Configure SyncBoard
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//        sb_clock_conf_t clock_conf;
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            clock_conf.source = sb_clock_source_external;
//            clock_conf.sample_rate_hz = 0;
//        }
//        else {
//            clock_conf.source = sb_clock_source_internal;
//            clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
//        }
//        sb_device_set_clock(m_syncBoardHandle, clock_conf);
//    }
//
//    // 2. Configure Digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) { StopAcquisition(); return false; }
//    }
//
//    // 3. Setup One-Shot Capture
//    AcquisitionConfig acqConfig = {};
//    acqConfig.admaMode = ADMA_NPT;
//    acqConfig.samplesPerRecord = 4096;
//    acqConfig.recordsPerBuffer = 1;
//    acqConfig.buffersPerAcquisition = 1;
//    acqConfig.saveData = true; // Sync test does NOT use the queue/disk
//
//    U32 bytesPerBuffer = 4096 * 4 * 2;
//    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//        if (!board->PrepareForAcquisition(acqConfig, channelMask)) { StopAcquisition(); return false; }
//        if (!board->PostBuffer(0)) { StopAcquisition(); return false; }
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // 4. Open Gate
//    Log("Waiting for trigger...");
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//
//    // 5. Wait & Capture
//    bool success = true;
//    std::vector<DataChunk> snapshots;
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->WaitFordBuffer(0, config.triggerTimeoutMS + 1000)) {
//            Log("Timeout on Board " + std::to_string(board->GetBoardId()));
//            success = false;
//        }
//        else {
//            // Manual processing for sync test
//            IO_BUFFER* pIoBuffer = board->GetBuffer(0);
//            U16* buffer = (U16*)pIoBuffer->pBuffer;
//
//            DataChunk chunk;
//            chunk.boardId = board->GetBoardId();
//            chunk.chA.resize(4096);
//            for (int i = 0; i < 4096; i++) chunk.chA[i] = buffer[i * 4]; // De-interleave ChA only
//            snapshots.push_back(chunk);
//        }
//    }
//
//    if (success && snapshots.size() >= 2) {
//        Log("Sync Data Captured.");
//        // Convert to float for analysis
//        std::vector<float> f_b1(snapshots[0].chA.begin(), snapshots[0].chA.end());
//        std::vector<float> f_b2(snapshots[1].chA.begin(), snapshots[1].chA.end());
//
//        // Save for GUI
//        m_syncSnapshot1 = f_b1;
//        m_syncSnapshot2 = f_b2;
//
//        PlotSyncTest_Stub(f_b1, f_b2);
//    }
//
//    StopAcquisition();
//    return success;
//}
//
//// ============================================================================
//// STATUS & CLEANUP HELPERS
//// ============================================================================
//
//void AcquisitionController::StopAcquisition()
//{
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//    for (AlazarDigitizer* b : m_boards) {
//        b->AbortAcquisition();
//        b->FreeBuffers();
//    }
//
//    CloseDataFiles();
//    m_isAcquiring = false;
//}
//
//bool AcquisitionController::OpenDataFiles(U32 boardCount)
//{
//    char currentDir[MAX_PATH];
//    if (!_getcwd(currentDir, MAX_PATH)) return false;
//    m_savePath = std::string(currentDir);
//
//    m_fileStreams.resize(boardCount * 4);
//    char channels[] = { 'A', 'B', 'C', 'D' };
//
//    for (U32 i = 0; i < boardCount; i++) {
//        for (int ch = 0; ch < 4; ch++) {
//            std::stringstream ss;
//            ss << m_savePath << "\\board" << (m_boards[i]->GetBoardId()) << "_ch" << channels[ch] << ".bin";
//            int idx = i * 4 + ch;
//            m_fileStreams[idx].open(ss.str(), std::ios::binary);
//            if (!m_fileStreams[idx].is_open()) {
//                Log("Failed to open: " + ss.str()); return false;
//            }
//        }
//    }
//    return true;
//}
//
//void AcquisitionController::CloseDataFiles()
//{
//    for (auto& f : m_fileStreams) if (f.is_open()) f.close();
//    m_fileStreams.clear();
//}
//
//void AcquisitionController::Log(std::string message)
//{
//    std::cout << message << std::endl;
//    m_logMessages.push_back(message);
//    if (m_logMessages.size() > 100) m_logMessages.erase(m_logMessages.begin());
//}
//
//bool AcquisitionController::IsAcquiring() { return m_isAcquiring; }
//std::vector<std::string> AcquisitionController::GetLogMessages() { return m_logMessages; }
//
//void AcquisitionController::GetSyncSnapshot(std::vector<float>& b1, std::vector<float>& b2) {
//    b1 = m_syncSnapshot1;
//    b2 = m_syncSnapshot2;
//}
//
////void AcquisitionController::GetLatestScopeData(std::vector<float>& chA_data) {
////    std::lock_guard<std::mutex> lock(m_guiDataMutex);
////    chA_data = m_latestScopeData;
////}
//
//void AcquisitionController::GetLatestScopeData(std::vector<float>& chA_data, U32 boardId)
//{
//    std::lock_guard<std::mutex> lock(m_guiDataMutex);
//    if (m_latestScopeData.count(boardId)) {
//        chA_data = m_latestScopeData[boardId]; // Copy specific board data
//    }
//    else {
//        chA_data.clear();
//    }
//}
//void AcquisitionController::PlotSyncTest_Stub(std::vector<float>& board1_chA, std::vector<float>& board2_chA)
//{
//    // 1. Calculate Logic (Runs on background thread)
//    int lag = FindLagByXCorr(board1_chA, board2_chA, 50);
//    m_lastCalculatedLag = lag; // Store for GUI
//
//    // 2. Log Result
//    std::stringstream ss; ss << "[Sync Check] Calculated Lag: " << lag;
//    Log(ss.str());
//    if (lag == 0) Log("[Sync Check] PASS: Perfect Sync.");
//    else Log("[Sync Check] FAIL: Offset detected.");
//}////////////////////////////////////////////////////////////