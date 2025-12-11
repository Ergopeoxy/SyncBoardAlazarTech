//#include "AcquisitionControllerLineDetect.h"
//#include <stdio.h>
//#include <iostream>
//#include <sstream>
//#include <cmath> 
//#include <algorithm>
//#include <cstring> // For memset, memmove
//
//// --- Windows/Linux Compatibility ---
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
//int AcquisitionController::FindLagByXCorr(const std::vector<float>& x, const std::vector<float>& y, int maxLag)
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
//    for (int lag = -maxLag; lag <= maxLag; ++lag) {
//        double num = 0.0, denom_x = 0.0, denom_y = 0.0;
//        for (int i = 0; i < N; ++i) {
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
//    m_keepProcessing(true),
//    // Algorithm Init
//    m_linesReady(false),
//    m_cb_write_head(0),
//    m_lp_cb_lr_w(0),
//    m_lp_cb_ud_r(0),
//    m_lp_line_r(0),
//    m_s_m1_cb_lp_r(0),
//    m_top_peak_found(false),
//    m_bot_peak_found(false),
//    m_first_buffer(true),
//    m_lastCalculatedLag(0)
//{
//    // 1. Allocate Algorithm Buffers
//    m_cb_M1.resize(CIRC_BUFFER_SIZE, 0.0);
//    m_cb_M2.resize(CIRC_BUFFER_SIZE, 0.0);
//    m_cb_Img.resize(CIRC_BUFFER_SIZE, 0.0);
//
//    memset(m_lineParams.leftright, -1, sizeof(m_lineParams.leftright));
//    memset(m_lineParams.updown, -1, sizeof(m_lineParams.updown));
//    memset(m_lineParams.mean_m2, 0, sizeof(m_lineParams.mean_m2));
//
//    // 2. Start Threads
//    m_saveThread = std::thread(&AcquisitionController::SaveThreadLoop, this);
//    m_detectorThread = std::thread(&AcquisitionController::DetectorThreadLoop, this);
//    m_generatorThread = std::thread(&AcquisitionController::GeneratorThreadLoop, this);
//}
//
//AcquisitionController::~AcquisitionController()
//{
//    m_isAcquiring = false;
//    StopAcquisition();
//
//    // Signal Threads to Die
//    m_keepProcessing = false;
//    m_saveQueueCV.notify_all();
//    m_procQueueCV.notify_all();
//    m_lineCV.notify_all();
//
//    if (m_saveThread.joinable()) m_saveThread.join();
//    if (m_detectorThread.joinable()) m_detectorThread.join();
//    if (m_generatorThread.joinable()) m_generatorThread.join();
//
//    if (m_syncBoardHandle) {
//        sb_device_close(m_syncBoardHandle);
//        m_syncBoardHandle = nullptr;
//    }
//
//    for (AlazarDigitizer* board : m_boards) delete board;
//    m_boards.clear();
//}
//
//// ============================================================================
//// HARDWARE DISCOVERY & CONFIG
//// ============================================================================
//
//bool AcquisitionController::DiscoverBoards()
//{
//    Log("Finding hardware...");
//
//    // 1. SyncBoard
//    if (m_syncBoardHandle == nullptr) {
//        size_t count = 0;
//        sb_get_device_count(&count);
//        if (count > 0) {
//            if (sb_device_open(0, &m_syncBoardHandle) == sb_rc_success)
//                Log("Found and opened ATS Sync 4X1G.");
//            else
//                Log("Error: Failed to open ATS Sync 4X1G.");
//        }
//        else {
//            Log("Warning: No ATS Sync 4X1G detected.");
//        }
//    }
//
//    // 2. Digitizers
//    U32 systemCount = AlazarNumOfSystems();
//    if (systemCount < 1) {
//        Log("Error: No AlazarTech systems found.");
//        return false;
//    }
//
//    // CLEAN UP OLD BOARDS
//    for (AlazarDigitizer* board : m_boards) delete board;
//    m_boards.clear();
//
//    U32 uniqueLogicalId = 1;
//    for (U32 systemId = 1; systemId <= systemCount; systemId++) {
//        U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
//        for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
//            HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
//            if (handle == NULL) continue;
//            AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, uniqueLogicalId++);
//            if (pBoard->QueryBoardInfo()) m_boards.push_back(pBoard);
//            else delete pBoard;
//        }
//    }
//
//    std::stringstream ss;
//    ss << "Found " << m_boards.size() << " digitizer(s).";
//    Log(ss.str());
//    return m_boards.size() > 0;
//}
//
//bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
//{
//    if (m_boards.empty()) {
//        Log("Error: No boards to configure.");
//        return false;
//    }
//    m_currentConfig = config;
//
//    // Configure Digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) return false;
//    }
//
//    // Configure SyncBoard
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//        sb_clock_conf_t clock_conf;
//
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            clock_conf.source = sb_clock_source_external;
//            clock_conf.sample_rate_hz = 0;
//            std::cout << "SyncBoard: External Clock" << std::endl;
//        }
//        else {
//            clock_conf.source = sb_clock_source_internal;
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
//
//// ============================================================================
//// ACQUISITION LOOP (PRODUCER)
//// ============================================================================
//
//bool AcquisitionController::RunAcquisition(const AcquisitionConfig& config)
//{
//    if (m_isAcquiring) { Log("Error: Acquisition already in progress."); return false; }
//    m_isAcquiring = true;
//    m_currentAcqConfig = config;
//
//    // 1. Close Gate
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//    // 2. Setup Buffers
//    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE) ? 1 : config.recordsPerBuffer;
//    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE) ? (config.samplesPerRecord / 4) : config.samplesPerRecord;
//    U32 bytesPerBuffer = samplesPerChannel * 4 * 2 * recordsPerBuffer;
//
//    if (config.saveData && !OpenDataFiles((U32)m_boards.size())) { m_isAcquiring = false; return false; }
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//        if (!board->PrepareForAcquisition(config, CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D)) { StopAcquisition(); return false; }
//        for (U32 i = 0; i < BUFFER_COUNT; i++) board->PostBuffer(i);
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // 3. Open Gate
//    if (m_syncBoardHandle) {
//        Log("Enabling SyncBoard Trigger...");
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//    }
//
//    // 4. Acquisition Loop
//    U32 buffersCompleted = 0;
//    bool success = true;
//
//    while (buffersCompleted < config.buffersPerAcquisition && success && m_isAcquiring)
//    {
//        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
//
//        for (AlazarDigitizer* board : m_boards) {
//            if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
//                success = false; break;
//            }
//
//            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
//            if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, config)) {
//                success = false; break;
//            }
//
//            if (!board->PostBuffer(bufferIndex)) {
//                success = false; break;
//            }
//        }
//
//        if (!success) break;
//        buffersCompleted++;
//        if (buffersCompleted % config.logInterval == 0) {
//            std::stringstream ss; ss << "Captured " << buffersCompleted; Log(ss.str());
//        }
//    }
//
//    Log("Acquisition complete.");
//    StopAcquisition();
//    return success;
//}
//
//// ============================================================================
//// DATA PROCESSING (Buffer -> Deinterleave -> Queues)
//// ============================================================================
//
//bool AcquisitionController::ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config)
//{
//    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE) ? 1 : config.recordsPerBuffer;
//    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE) ? (config.samplesPerRecord / 4) : config.samplesPerRecord;
//    size_t totalSamples = samplesPerChannel * recordsPerBuffer;
//
//    DataChunk chunk;
//    chunk.boardId = board->GetBoardId();
//    chunk.chA.resize(totalSamples);
//    chunk.chB.resize(totalSamples);
//    chunk.chC.resize(totalSamples);
//    chunk.chD.resize(totalSamples);
//
//    // De-interleave
//    for (U32 r = 0; r < recordsPerBuffer; r++) {
//        for (U32 s = 0; s < samplesPerChannel; s++) {
//            U32 interleaved_index = (r * (samplesPerChannel * 4)) + (s * 4);
//            U32 flat_index = r * samplesPerChannel + s;
//            chunk.chA[flat_index] = buffer[interleaved_index + 0];
//            chunk.chB[flat_index] = buffer[interleaved_index + 1];
//            chunk.chC[flat_index] = buffer[interleaved_index + 2];
//            chunk.chD[flat_index] = buffer[interleaved_index + 3];
//        }
//    }
//
//    // Update GUI Scope (Map)
//    if (config.processData) {
//        std::lock_guard<std::mutex> lock(m_guiDataMutex);
//        std::vector<float>& scope = m_latestScopeData[chunk.boardId];
//        if (scope.size() != totalSamples) scope.resize(totalSamples);
//
//        for (size_t i = 0; i < totalSamples; i++) {
//            double code = chunk.chA[i] >> 2;
//            scope[i] = (float)((code - 8191.5) / 8191.5);
//        }
//    }
//
//    // Push to Save Queue
//    if (config.saveData) {
//        std::lock_guard<std::mutex> lock(m_saveQueueMutex);
//        if (m_saveQueue.size() < 500) {
//            m_saveQueue.push(chunk);
//            m_saveQueueCV.notify_one();
//        }
//    }
//
//    // Push to Processing Queue (New Peak Detection)
//    if (config.processData) {
//        std::lock_guard<std::mutex> lock(m_procQueueMutex);
//        if (m_procQueue.size() < PROCESSING_QUEUE_MAX_SIZE) {
//            m_procQueue.push(std::move(chunk));
//            m_procQueueCV.notify_one();
//        }
//    }
//
//    return true;
//}
//
//// ============================================================================
//// SYNC TEST (Blocking Manual Capture)
//// ============================================================================
//
//bool AcquisitionController::RunSyncTest(const BoardConfig& config)
//{
//    if (m_isAcquiring) { Log("Busy."); return false; }
//    if (m_boards.size() < 2) { Log("Error: Need 2 boards."); return false; }
//    m_isAcquiring = true;
//    Log("--- Starting Sync Test ---");
//
//    // 1. Config SyncBoard
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
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
//    // 2. Config Digitizers
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) { StopAcquisition(); return false; }
//    }
//
//    // 3. One-Shot Setup
//    AcquisitionConfig acqConfig = {};
//    acqConfig.admaMode = ADMA_NPT;
//    acqConfig.samplesPerRecord = 4096;
//    acqConfig.recordsPerBuffer = 1;
//    acqConfig.buffersPerAcquisition = 1;
//
//    U32 bytesPerBuffer = 4096 * 4 * 2;
//    for (AlazarDigitizer* board : m_boards) {
//        board->AllocateBuffers(bytesPerBuffer);
//        board->PrepareForAcquisition(acqConfig, CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D);
//        board->PostBuffer(0);
//        board->StartCapture();
//    }
//
//    Log("Waiting for trigger...");
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//
//    bool success = true;
//    std::vector<std::vector<float>> snapshots;
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->WaitFordBuffer(0, config.triggerTimeoutMS + 1000)) {
//            Log("Timeout Board " + std::to_string(board->GetBoardId()));
//            success = false;
//        }
//        else {
//            IO_BUFFER* pIoBuffer = board->GetBuffer(0);
//            U16* buffer = (U16*)pIoBuffer->pBuffer;
//            std::vector<float> chA_float(4096);
//            for (int i = 0; i < 4096; i++) {
//                double code = buffer[i * 4] >> 2;
//                chA_float[i] = (float)((code - 8191.5) / 8191.5);
//            }
//            snapshots.push_back(chA_float);
//        }
//    }
//
//    if (success && snapshots.size() >= 2) {
//        m_syncSnapshot1 = snapshots[0];
//        m_syncSnapshot2 = snapshots[1];
//        m_lastCalculatedLag = FindLagByXCorr(m_syncSnapshot1, m_syncSnapshot2, 50);
//        std::stringstream ss; ss << "Sync Test Complete. Lag: " << m_lastCalculatedLag;
//        Log(ss.str());
//    }
//
//    StopAcquisition();
//    return success;
//}
//
//// ============================================================================
//// THREAD 2: DETECTOR LOOP
//// ============================================================================
//
//void AcquisitionController::DetectorThreadLoop()
//{
//    const int LR_SEARCH_AREA = 10;
//    static double temp_ld_sig_m1[2 * LR_SEARCH_AREA + NUMBER_OF_SEGMENTS_PER_BUFFER];
//    memset(temp_ld_sig_m1, 0, sizeof(temp_ld_sig_m1));
//
//    while (m_keepProcessing)
//    {
//        DataChunk chunk;
//        {
//            std::unique_lock<std::mutex> lock(m_procQueueMutex);
//            m_procQueueCV.wait(lock, [this] { return !m_procQueue.empty() || !m_keepProcessing; });
//            if (!m_keepProcessing && m_procQueue.empty()) return;
//            chunk = std::move(m_procQueue.front());
//            m_procQueue.pop();
//        }
//
//        // Fill Circular Buffers
//        size_t samples = chunk.chA.size();
//        for (size_t i = 0; i < samples; i++) {
//            size_t idx = (m_cb_write_head + i) % CIRC_BUFFER_SIZE;
//            m_cb_Img[idx] = (double)chunk.chA[i];
//            m_cb_M1[idx] = (double)chunk.chB[i];
//            m_cb_M2[idx] = (double)chunk.chC[i];
//        }
//        m_cb_write_head = (m_cb_write_head + samples) % CIRC_BUFFER_SIZE;
//
//        // Run Peak Detection
//        memmove(&temp_ld_sig_m1, &temp_ld_sig_m1[NUMBER_OF_SEGMENTS_PER_BUFFER], sizeof(double) * (2 * LR_SEARCH_AREA));
//
//        // Copy NEW data safely (handling wrap)
//        size_t read_start = (m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER);
//        if (read_start + NUMBER_OF_SEGMENTS_PER_BUFFER <= CIRC_BUFFER_SIZE) {
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA], &m_cb_M1[read_start], sizeof(double) * NUMBER_OF_SEGMENTS_PER_BUFFER);
//        }
//        else {
//            size_t p1 = CIRC_BUFFER_SIZE - read_start;
//            size_t p2 = NUMBER_OF_SEGMENTS_PER_BUFFER - p1;
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA], &m_cb_M1[read_start], sizeof(double) * p1);
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA + p1], &m_cb_M1[0], sizeof(double) * p2);
//        }
//
//        bool new_lines = false;
//        bool peak_detected;
//
//        for (int i = LR_SEARCH_AREA; i < NUMBER_OF_SEGMENTS_PER_BUFFER + LR_SEARCH_AREA; i++) {
//            if (m_first_buffer) { i += (2 * LR_SEARCH_AREA); m_first_buffer = false; }
//
//            peak_detected = true;
//            if (!m_top_peak_found) {
//                // Find Max
//                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//                    if (temp_ld_sig_m1[i] < temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//                }
//                if (peak_detected) {
//                    m_top_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
//                    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//                    m_top_peak_found = true;
//                    if (m_bot_peak_found) {
//                        m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
//                        m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
//                        m_lineParams.leftright[m_lp_cb_lr_w] = 0; // Forward
//                        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//                    }
//                    m_bot_peak_found = false;
//                }
//            }
//            else {
//                // Find Min
//                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//                    if (temp_ld_sig_m1[i] > temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//                }
//                if (peak_detected) {
//                    m_bot_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
//                    if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
//                    m_bot_peak_found = true;
//                    m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
//                    m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
//                    m_lineParams.leftright[m_lp_cb_lr_w] = 1; // Backward
//                    m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//                    m_top_peak_found = false;
//                }
//            }
//        }
//        m_s_m1_cb_lp_r = (m_s_m1_cb_lp_r + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
//
//        // Calc M2 Average
//        int temp_w = m_lp_cb_lr_w;
//        if (m_lp_cb_ud_r > temp_w) temp_w += NUMBER_OF_LINE_PARAMS;
//        if (m_lp_cb_ud_r < temp_w) new_lines = true;
//
//        for (int i = m_lp_cb_ud_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//
//            double sum_m2 = 0;
//            for (int k = 0; k < len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                sum_m2 += m_cb_M2[idx];
//            }
//            if (len > 0) m_lineParams.mean_m2[imod] = sum_m2 / len;
//
//            // Up/Down logic
//            int prev = (imod - 1 + NUMBER_OF_LINE_PARAMS) % NUMBER_OF_LINE_PARAMS;
//            m_lineParams.updown[imod] = (m_lineParams.mean_m2[imod] > m_lineParams.mean_m2[prev]) ? 0 : 1;
//            m_lp_cb_ud_r = (m_lp_cb_ud_r + 1) % NUMBER_OF_LINE_PARAMS;
//        }
//
//        if (new_lines) {
//            std::unique_lock<std::mutex> ul(m_lineMutex);
//            m_linesReady = true;
//            m_lineCV.notify_one();
//        }
//    }
//}
//
//// ============================================================================
//// THREAD 3: GENERATOR LOOP
//// ============================================================================
//
//void AcquisitionController::GeneratorThreadLoop()
//{
//    static std::vector<double> temp_line_data(NUMBER_OF_POINTS_PER_LINE);
//
//    while (m_keepProcessing)
//    {
//        std::unique_lock<std::mutex> ul(m_lineMutex);
//        m_lineCV.wait(ul, [this] { return m_linesReady || !m_keepProcessing; });
//        m_linesReady = false;
//        ul.unlock();
//        if (!m_keepProcessing) return;
//
//        long temp_w = m_lp_cb_lr_w;
//        if (temp_w < m_lp_line_r) temp_w += NUMBER_OF_LINE_PARAMS;
//
//        for (int i = m_lp_line_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//
//            int copy_len = (len > NUMBER_OF_POINTS_PER_LINE) ? NUMBER_OF_POINTS_PER_LINE : len;
//            if (copy_len <= 0) continue;
//
//            temp_line_data.assign(NUMBER_OF_POINTS_PER_LINE, 0.0);
//
//            // Fetch and Reverse if needed
//            for (int k = 0; k < copy_len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                temp_line_data[k] = m_cb_Img[idx];
//            }
//            if (m_lineParams.leftright[imod] == 1) { // Backward
//                std::reverse(temp_line_data.begin(), temp_line_data.begin() + copy_len);
//            }
//
//            // Map to Y
//            static double m2_max = 38500;
//            static double m2_min = 28500;
//            double y_norm = (m_lineParams.mean_m2[imod] - m2_min) / (m2_max - m2_min);
//            int y_idx = (int)(ceil(y_norm * NUMBER_OF_POINTS_PER_LINE) - 1);
//            if (y_idx < 0) y_idx = 0;
//            if (y_idx >= NUMBER_OF_POINTS_PER_LINE) y_idx = NUMBER_OF_POINTS_PER_LINE - 1;
//
//            // Write to Image
//            {
//                std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) {
//                    m_finalImage[y_idx][x] = temp_line_data[x];
//                }
//            }
//        }
//        m_lp_line_r = temp_w % NUMBER_OF_LINE_PARAMS;
//    }
//}
//
//// ============================================================================
//// PUBLIC GETTERS
//// ============================================================================
//
//void AcquisitionController::GetLatestImage(std::vector<std::vector<double>>& outImage) {
//    std::lock_guard<std::mutex> lock(m_guiDataMutex);
//    if (outImage.size() != NUMBER_OF_POINTS_PER_LINE)
//        outImage.resize(NUMBER_OF_POINTS_PER_LINE, std::vector<double>(NUMBER_OF_POINTS_PER_LINE));
//    for (int y = 0; y < NUMBER_OF_POINTS_PER_LINE; y++)
//        for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++)
//            outImage[y][x] = m_finalImage[y][x];
//}
//
//void AcquisitionController::GetLatestScopeData(std::vector<float>& data, U32 boardId) {
//    std::lock_guard<std::mutex> lock(m_guiDataMutex);
//    if (m_latestScopeData.count(boardId)) data = m_latestScopeData[boardId];
//    else data.clear();
//}
//
//std::vector<std::string> AcquisitionController::GetLogMessages() { return m_logMessages; }
//void AcquisitionController::GetSyncSnapshot(std::vector<float>& b1, std::vector<float>& b2) { b1 = m_syncSnapshot1; b2 = m_syncSnapshot2; }
//
//// ============================================================================
//// CLEANUP & SAVING
//// ============================================================================
//
//void AcquisitionController::StopAcquisition() {
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//    for (auto b : m_boards) { b->AbortAcquisition(); b->FreeBuffers(); }
//    CloseDataFiles();
//    m_isAcquiring = false;
//}
//
//void AcquisitionController::SaveThreadLoop() {
//    while (m_keepProcessing) {
//        DataChunk chunk;
//        {
//            std::unique_lock<std::mutex> lock(m_saveQueueMutex);
//            m_saveQueueCV.wait(lock, [this] { return !m_saveQueue.empty() || !m_keepProcessing; });
//            if (!m_keepProcessing && m_saveQueue.empty()) return;
//            chunk = std::move(m_saveQueue.front());
//            m_saveQueue.pop();
//        }
//        int boardIdx = -1;
//        if (chunk.boardId == 1) boardIdx = 0; else if (chunk.boardId == 2) boardIdx = 1;
//
//        if (boardIdx >= 0) {
//            int f = boardIdx * 4;
//            if (f + 3 < m_fileStreams.size()) {
//                if (!chunk.chA.empty() && m_fileStreams[f].is_open()) m_fileStreams[f].write((char*)chunk.chA.data(), chunk.chA.size() * 2);
//                if (!chunk.chB.empty() && m_fileStreams[f + 1].is_open()) m_fileStreams[f + 1].write((char*)chunk.chB.data(), chunk.chB.size() * 2);
//                if (!chunk.chC.empty() && m_fileStreams[f + 2].is_open()) m_fileStreams[f + 2].write((char*)chunk.chC.data(), chunk.chC.size() * 2);
//                if (!chunk.chD.empty() && m_fileStreams[f + 3].is_open()) m_fileStreams[f + 3].write((char*)chunk.chD.data(), chunk.chD.size() * 2);
//            }
//        }
//    }
//}
//
//bool AcquisitionController::OpenDataFiles(U32 boardCount) {
//    char cwd[MAX_PATH]; if (!_getcwd(cwd, MAX_PATH)) return false; m_savePath = cwd;
//    m_fileStreams.resize(boardCount * 4);
//    char chs[] = { 'A','B','C','D' };
//    for (U32 i = 0; i < boardCount; i++) {
//        for (int c = 0; c < 4; c++) {
//            std::stringstream ss; ss << m_savePath << "\\board" << m_boards[i]->GetBoardId() << "_ch" << chs[c] << ".bin";
//            m_fileStreams[i * 4 + c].open(ss.str(), std::ios::binary);
//        }
//    }
//    return true;
//}
//bool AcquisitionController::IsAcquiring() { return m_isAcquiring; }
//void AcquisitionController::CloseDataFiles() { for (auto& f : m_fileStreams) if (f.is_open()) f.close(); m_fileStreams.clear(); }
//void AcquisitionController::Log(std::string m) { std::cout << m << std::endl; m_logMessages.push_back(m); if (m_logMessages.size() > 100) m_logMessages.erase(m_logMessages.begin());

#include "AcquisitionControllerLineDetect.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cmath> 
#include <algorithm>
#include <cstring> // For memset, memmove

// --- Windows/Linux Compatibility ---
#ifdef _WIN32
#include <conio.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

// ============================================================================
// HELPER: Cross Correlation (For Sync Test)
// ============================================================================
int AcquisitionController::FindLagByXCorr(const std::vector<float>& x, const std::vector<float>& y, int maxLag)
{
    int N = (int)x.size();
    if (N == 0 || (int)y.size() != N) return 0;

    float mean_x = 0.f, mean_y = 0.f;
    for (float f : x) mean_x += f; mean_x /= N;
    for (float f : y) mean_y += f; mean_y /= N;

    std::vector<float> xc(N), yc(N);
    for (int i = 0; i < N; ++i) { xc[i] = x[i] - mean_x; yc[i] = y[i] - mean_y; }

    int bestLag = 0;
    double bestScore = -1e9;

    for (int lag = -maxLag; lag <= maxLag; ++lag) {
        double num = 0.0, denom_x = 0.0, denom_y = 0.0;
        for (int i = 0; i < N; ++i) {
            int j = i + lag;
            if (j >= 0 && j < N) {
                num += (double)xc[i] * (double)yc[j];
                denom_x += (double)xc[i] * (double)xc[i];
                denom_y += (double)yc[j] * (double)yc[j];
            }
        }
        double denom = sqrt(denom_x * denom_y) + 1e-20;
        double score = num / denom;
        if (score > bestScore) { bestScore = score; bestLag = lag; }
    }
    return bestLag;
}

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

AcquisitionController::AcquisitionController() :
    m_isAcquiring(false),
    m_syncBoardHandle(nullptr),
    m_keepProcessing(true),
    // Algorithm Init
    m_linesReady(false),
    m_cb_write_head(0),
    m_lp_cb_lr_w(0),
    m_lp_cb_ud_r(0),
    m_lp_line_r(0),
    m_s_m1_cb_lp_r(0),
    m_top_peak_found(false),
    m_bot_peak_found(false),
    m_first_buffer(true),
    m_lastCalculatedLag(0)
{
    // 1. Allocate Large Circular Buffers
    m_cb_M1.resize(CIRC_BUFFER_SIZE, 0.0);
    m_cb_M2.resize(CIRC_BUFFER_SIZE, 0.0);
    m_cb_Img.resize(CIRC_BUFFER_SIZE, 0.0);
	m_cb_D.resize(CIRC_BUFFER_SIZE, 0.0);

    memset(m_lineParams.leftright, -1, sizeof(m_lineParams.leftright));
    memset(m_lineParams.updown, -1, sizeof(m_lineParams.updown));
    memset(m_lineParams.mean_m2, 0, sizeof(m_lineParams.mean_m2));

    // 2. Start Threads
    m_saveThread = std::thread(&AcquisitionController::SaveThreadLoop, this);
    m_detectorThread = std::thread(&AcquisitionController::DetectorThreadLoop, this);
    m_generatorThread = std::thread(&AcquisitionController::GeneratorThreadLoop, this);
}

AcquisitionController::~AcquisitionController()
{
    m_isAcquiring = false;
    StopAcquisition();

    // Signal Threads to Die
    m_keepProcessing = false;
    m_saveQueueCV.notify_all();
    m_procQueueCV.notify_all();
    m_lineCV.notify_all();

    // Join Threads
    if (m_saveThread.joinable()) m_saveThread.join();
    if (m_detectorThread.joinable()) m_detectorThread.join();
    if (m_generatorThread.joinable()) m_generatorThread.join();

    // Close Hardware
    if (m_syncBoardHandle) {
        sb_device_close(m_syncBoardHandle);
        m_syncBoardHandle = nullptr;
    }

    for (AlazarDigitizer* board : m_boards) delete board;
    m_boards.clear();
}

// ============================================================================
// HARDWARE DISCOVERY & CONFIG
// ============================================================================

bool AcquisitionController::DiscoverBoards()
{
    Log("Finding hardware...");

    // 1. SyncBoard Discovery
    if (m_syncBoardHandle == nullptr) {
        size_t count = 0;
        sb_get_device_count(&count);
        if (count > 0) {
            if (sb_device_open(0, &m_syncBoardHandle) == sb_rc_success)
                Log("Found and opened ATS Sync 4X1G.");
            else
                Log("Error: Failed to open ATS Sync 4X1G.");
        }
        else {
            Log("Warning: No ATS Sync 4X1G detected.");
        }
    }

    // 2. Digitizer Discovery
    U32 systemCount = AlazarNumOfSystems();
    if (systemCount < 1) {
        Log("Error: No AlazarTech systems found.");
        return false;
    }

    // Clean up old handles
    for (AlazarDigitizer* board : m_boards) delete board;
    m_boards.clear();

    U32 uniqueLogicalId = 1;
    for (U32 systemId = 1; systemId <= systemCount; systemId++) {
        U32 boardsInThisSystem = AlazarBoardsInSystemBySystemID(systemId);
        for (U32 boardId = 1; boardId <= boardsInThisSystem; boardId++) {
            HANDLE handle = AlazarGetBoardBySystemID(systemId, boardId);
            if (handle == NULL) continue;
            AlazarDigitizer* pBoard = new AlazarDigitizer(handle, systemId, uniqueLogicalId++);
            if (pBoard->QueryBoardInfo()) m_boards.push_back(pBoard);
            else delete pBoard;
        }
    }

    std::stringstream ss;
    ss << "Found " << m_boards.size() << " digitizer(s).";
    Log(ss.str());
    return m_boards.size() > 0;
}

bool AcquisitionController::ConfigureAllBoards(const BoardConfig& config)
{
    if (m_boards.empty()) {
        Log("Error: No boards to configure.");
        return false;
    }
    m_currentConfig = config;

    // Configure Digitizers
    for (AlazarDigitizer* board : m_boards) {
        if (!board->ConfigureBoard(config)) return false;
    }

    // Configure SyncBoard Clock
    if (m_syncBoardHandle) {
        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
        sb_clock_conf_t clock_conf;

        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
            clock_conf.source = sb_clock_source_external;
            clock_conf.sample_rate_hz = 0;
            std::cout << "SyncBoard: External Clock Mode" << std::endl;
        }
        else {
            clock_conf.source = sb_clock_source_internal;
            clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
        }

        if (sb_device_set_clock(m_syncBoardHandle, clock_conf) != sb_rc_success) {
            Log("Error: Failed to configure SyncBoard clock.");
            return false;
        }
    }
    return true;
}

// ============================================================================
// ACQUISITION LOOP (PRODUCER)
// ============================================================================

bool AcquisitionController::RunAcquisition(const AcquisitionConfig& config)
{
    if (m_isAcquiring) { Log("Error: Acquisition already in progress."); return false; }
    m_isAcquiring = true;
    m_currentAcqConfig = config;

    // 1. Close Gate
    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);

    // 2. Setup Buffers
    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE) ? 1 : config.recordsPerBuffer;
    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE) ? (config.samplesPerRecord / 4) : config.samplesPerRecord;
    U32 bytesPerBuffer = samplesPerChannel * 4 * 2 * recordsPerBuffer;

    if (config.saveData && !OpenDataFiles((U32)m_boards.size())) { m_isAcquiring = false; return false; }

    for (AlazarDigitizer* board : m_boards) {
        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
        if (!board->PrepareForAcquisition(config, CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D)) { StopAcquisition(); return false; }
        for (U32 i = 0; i < BUFFER_COUNT; i++) board->PostBuffer(i);
        if (!board->StartCapture()) { StopAcquisition(); return false; }
    }

    // 3. Open Gate
    if (m_syncBoardHandle) {
        Log("Enabling SyncBoard Trigger...");
        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
    }

    // 4. Acquisition Loop
    U32 buffersCompleted = 0;
    bool success = true;

    while (buffersCompleted < config.buffersPerAcquisition && success && m_isAcquiring)
    {
        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;

        for (AlazarDigitizer* board : m_boards) {
            if (!board->WaitFordBuffer(bufferIndex, m_currentConfig.triggerTimeoutMS + 1000)) {
                success = false; break;
            }

            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
            if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, config)) {
                success = false; break;
            }

            if (!board->PostBuffer(bufferIndex)) {
                success = false; break;
            }
        }

        if (!success) break;
        buffersCompleted++;
        if (buffersCompleted % config.logInterval == 0) {
            std::stringstream ss; ss << "Captured " << buffersCompleted; Log(ss.str());
        }
    }

    Log("Acquisition complete.");
    StopAcquisition();
    return success;
}

// ============================================================================
// DATA PROCESSING (Buffer -> Deinterleave -> Queues)
// 
// 
// This function, ProcessBufferData, is the "engine room" of your acquisition software.
// It takes a raw block of memory filled with data from the Alazar card,
// organizes it into separate channels, and then dispatches it to three different destinations: 
// the GUI display, the Disk Saver, and the Processing algorithm.
//  
// ============================================================================

bool AcquisitionController::ProcessBufferData(AlazarDigitizer* board, U16* buffer, const AcquisitionConfig& config)
{
    // if the data acquisition is continuous just treat it as one buffer else use the cofiguration that is given in the setting
    U32 recordsPerBuffer = (config.admaMode == ADMA_CONTINUOUS_MODE) ? 1 : config.recordsPerBuffer;
    U32 samplesPerChannel = (config.admaMode == ADMA_CONTINUOUS_MODE) ? (config.samplesPerRecord / 4) : config.samplesPerRecord;
    size_t totalSamples = samplesPerChannel * recordsPerBuffer;

    // assign memory space for each of the channels of the board
    DataChunk chunk;
    chunk.boardId = board->GetBoardId();
    chunk.chA.resize(totalSamples);
    chunk.chB.resize(totalSamples);
    chunk.chC.resize(totalSamples);
    chunk.chD.resize(totalSamples);

    // De-interleave Logic
    /*
        interleaved_index: Calculates where the current sample lives in the raw buffer.

        s * 4: Since data is packed as groups of 4 (A,B,C,D), we jump 4 spots to find the next sample for the same channel.

        + 0, + 1, etc.: Offsets to pick A, B, C, or D from that group.

        flat_index: Calculates where to put the sample in the clean chunk vector (0, 1, 2, 3...)
    */
    for (U32 r = 0; r < recordsPerBuffer; r++) {
        for (U32 s = 0; s < samplesPerChannel; s++) {
            U32 interleaved_index = (r * (samplesPerChannel * 4)) + (s * 4);
            U32 flat_index = r * samplesPerChannel + s;
            chunk.chA[flat_index] = buffer[interleaved_index + 0];
            chunk.chB[flat_index] = buffer[interleaved_index + 1];
            chunk.chC[flat_index] = buffer[interleaved_index + 2];
            chunk.chD[flat_index] = buffer[interleaved_index + 3];
        }
    }

    // --- UPDATE GUI SCOPE (MULTI-CHANNEL) ---
    /*
    std::lock_guard: This is Thread Safety. It prevents the GUI from reading the data while this function is writing to it,
    which would cause the program to crash.BIT_SHIFT = 2: The ATS9440 is a 14-bit card,
    but data is stored in a 16-bit container.The data is often "left-aligned" or padded. 
    This shift corrects the integer value.(codeA - 8191.5) / 8191.5: This converts the raw integer "code" into Volts
    (normalized float between -1.0 and 1.0).
    2^14 = 16384.Half of that is approx 8192.
    This formula centers the data so 0V is roughly 0.0 in the float array.
    */



    if (config.processData) {
        std::lock_guard<std::mutex> lock(m_guiDataMutex);

        // Access specific board's channel storage
        std::vector<std::vector<float>>& boardScope = m_latestScopeData[chunk.boardId];

        // Ensure 4 channels exist (A, B, C, D)
        if (boardScope.size() < 4)
        {
            std::cout << "size of boardscope was less than 3 (channels)";
            boardScope.resize(4);
        }

        // Resize vectors if needed
        if (boardScope[0].size() != totalSamples) {
            boardScope[0].resize(totalSamples);
            boardScope[1].resize(totalSamples);
            boardScope[2].resize(totalSamples);
            boardScope[3].resize(totalSamples);
        }




        /*
        double codeA = chunk.chA[i] >> BIT_SHIFT;
        chunk.chA[i]: This is the raw 16-bit data from the digitizer (0 to 65535).

        BIT_SHIFT (2): The ATS9440 is a 14-bit card, but the data is stored in a 16-bit container.

        Alazar cards "left-align" the data to the Most Significant Bit (MSB). This means the 14 bits of data are at the top, and the bottom 2 bits are effectively zero or noise.

        Right-shifting by 2 (>> 2) discards those empty bits and gives you the true 14-bit integer value (range 0 to 16383
        */




        // Fill Data (Convert to Float Volts)
        const int BIT_SHIFT = 2; // Adjust based on bit-depth
        for (size_t i = 0; i < totalSamples; i++) {


            /*
            8191.5 (Midpoint):A 14-bit unsigned integer goes from 0 to 16383.The center (0 Volts) is halfway: $16383 / 2 = 8191.5
            $.codeA - 8191.5 shifts the range to be centered around zero: [-8191.5 ... +8191.5].
            Division by 8191.5:Dividing by the midpoint scales the result to be between -1.0 and +1.0.Example:Code 0 $\rightarrow$ $(0 - 8191.5) / 8191.5 = \mathbf{-1.0}$
            (Negative Full Scale)Code 8192 $\rightarrow$ $(8192 - 8191.5) / 8191.5 \approx \mathbf{0.0}$ (Zero Volts)
            Code 16383 $\rightarrow$ $(16383 - 8191.5) / 8191.5 = \mathbf{+1.0}$ (Positive Full Scale
            */


            // ChA (Image)
            double codeA = chunk.chA[i] >> BIT_SHIFT;
            boardScope[0][i] = (float)((codeA - 8191.5) / 8191.5);

            // ChB (M1 / Scanner)
            double codeB = chunk.chB[i] >> BIT_SHIFT;
            boardScope[1][i] = (float)((codeB - 8191.5) / 8191.5);

            // ChC (M2 / Ramp)
            double codeC = chunk.chC[i] >> BIT_SHIFT;
            boardScope[2][i] = (float)((codeC - 8191.5) / 8191.5);
			// ChD (Depth)
            double codeD = chunk.chD[i] >> BIT_SHIFT;
            boardScope[3][i] = (float)((codeD - 8191.5) / 8191.5);
        }

        /*
        
        Units: This effectively converts the raw data into "Percent of Input Range".
        If your input range is set to $\pm 1V$, a value of 0.5 means 0.5 Volts.
        If your input range is $\pm 400mV$, a value of 0.5 means 200mV
        */



    }






    // Push to Save Queue
    /*
    lock_guard: Protects the queue shared with the disk-writing thread.

    if (... < 500): Backpressure protection. If your hard drive is too slow and the queue fills up to 500 chunks,
    this code drops the data (skips the push). This prevents your RAM from filling up until the computer crashes.
    .push(chunk): This makes a copy of the data chunk into the queue
    */
    if (config.saveData) {
        std::lock_guard<std::mutex> lock(m_saveQueueMutex);
        if (m_saveQueue.size() < 500) {
            m_saveQueue.push(chunk);
            m_saveQueueCV.notify_one();
        }
    }

    // Push to Processing Queue (Peak Detection)
    /*
    std::move(chunk): Important Optimization.

    In the previous step (Save Queue), we used .push(chunk), which copied the data because we still needed chunk for this step.

    Here, we use std::move. This transfers ownership of the memory vectors to the queue without copying. It is extremely fast.
    After this line, the local chunk variable is empty.
    */
    if (config.processData) {
        std::lock_guard<std::mutex> lock(m_procQueueMutex);
        if (m_procQueue.size() < PROCESSING_QUEUE_MAX_SIZE) {
            m_procQueue.push(std::move(chunk));
            m_procQueueCV.notify_one();
        }
    }

    return true;
}

// ============================================================================
// SYNC TEST (Blocking Manual Capture)
// ============================================================================


bool AcquisitionController::RunSyncTest(const BoardConfig& config, int board1_ch, int board2_ch)
{
    if (m_isAcquiring) { Log("Busy."); return false; }
    if (m_boards.size() < 2) { Log("Error: Need 2 boards."); return false; }
    m_isAcquiring = true;
    Log("--- Starting Multi-Channel Sync Test ---");

    // 1. Config SyncBoard (Standard)
    if (m_syncBoardHandle) {
        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
        sb_clock_conf_t clock_conf;
        clock_conf.source = (config.sampleRateId == SAMPLE_RATE_USER_DEF) ? sb_clock_source_external : sb_clock_source_internal;
        clock_conf.sample_rate_hz = (config.sampleRateId == SAMPLE_RATE_USER_DEF) ? 0 : (int64_t)config.sampleRateHz;
        sb_device_set_clock(m_syncBoardHandle, clock_conf);
    }

    // 2. Config Digitizers
    for (AlazarDigitizer* board : m_boards) {
        if (!board->ConfigureBoard(config)) { StopAcquisition(); return false; }
    }

    // 3. Setup One-Shot Capture
    AcquisitionConfig acqConfig = {};
    acqConfig.admaMode = ADMA_NPT;
    acqConfig.samplesPerRecord = 4096;
    acqConfig.recordsPerBuffer = 1;
    acqConfig.buffersPerAcquisition = 1;

    // Allocate enough for 4 channels
    U32 bytesPerBuffer = 4096 * 4 * 2;

    for (AlazarDigitizer* board : m_boards) {
        board->AllocateBuffers(bytesPerBuffer);
        // Ensure we capture ALL channels
        board->PrepareForAcquisition(acqConfig, CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D);
        board->PostBuffer(0);
        board->StartCapture();
    }

    Log("Waiting for trigger...");
    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);

    bool success = true;

    // Clear old snapshots
    {
        std::lock_guard<std::mutex> lock(m_guiDataMutex);
        m_syncSnapshots.clear();
    }

    // 4. Retrieve and De-Interleave Data
    for (AlazarDigitizer* board : m_boards) {
        if (!board->WaitFordBuffer(0, config.triggerTimeoutMS + 1000)) {
            Log("Timeout Board " + std::to_string(board->GetBoardId()));
            success = false;
        }
        else {
            IO_BUFFER* pIoBuffer = board->GetBuffer(0);
            U16* buffer = (U16*)pIoBuffer->pBuffer;

            // Temporary storage for this board's 4 channels
            std::vector<std::vector<float>> boardChannels(4, std::vector<float>(4096));

            // De-interleave (A, B, C, D, A, B, C, D...)
            for (int i = 0; i < 4096; i++) {
                // Convert raw U16 to Volts (-1.0 to 1.0)
                boardChannels[0][i] = (float)(((buffer[i * 4 + 0] >> 2) - 8191.5) / 8191.5); // Ch A
                boardChannels[1][i] = (float)(((buffer[i * 4 + 1] >> 2) - 8191.5) / 8191.5); // Ch B
                boardChannels[2][i] = (float)(((buffer[i * 4 + 2] >> 2) - 8191.5) / 8191.5); // Ch C
                boardChannels[3][i] = (float)(((buffer[i * 4 + 3] >> 2) - 8191.5) / 8191.5); // Ch D
            }

            // Store thread-safely
            {
                std::lock_guard<std::mutex> lock(m_guiDataMutex);
                m_syncSnapshots[board->GetBoardId()] = boardChannels;
            }
        }
    }

    // 5. Calculate Lag (Using USER SELECTED channels)
    if (success && m_syncSnapshots.count(1) && m_syncSnapshots.count(2)) {

        // Safety check: Ensure requested channels exist
        if (board1_ch < 4 && board2_ch < 4) {

            // Log what we are comparing
            std::stringstream info;
            info << "Calculating Lag: B1_Ch" << (char)('A' + board1_ch)
                << " vs B2_Ch" << (char)('A' + board2_ch);
            Log(info.str());

            // --- THE FIX IS HERE ---
            // Use the passed arguments instead of hardcoded [0]
            std::vector<float>& sig1 = m_syncSnapshots[1][board1_ch];
            std::vector<float>& sig2 = m_syncSnapshots[2][board2_ch];

            m_lastCalculatedLag = FindLagByXCorr(sig1, sig2, 50);

            std::stringstream ss;
            ss << "Sync Test Complete. Lag: " << m_lastCalculatedLag;
            Log(ss.str());
        }
        else {
            Log("Error: Invalid channel selection for sync.");
        }
    }

    StopAcquisition();
    return success;
}

// Add this Getter to your class
void AcquisitionController::GetSyncSnapshotMulti(int boardID, int channelID, std::vector<float>& outData) {
    std::lock_guard<std::mutex> lock(m_guiDataMutex);
    if (m_syncSnapshots.count(boardID) && channelID < 4) {
        outData = m_syncSnapshots[boardID][channelID];
    }
    else {
        outData.clear();
    }
}


//bool AcquisitionController::RunSyncTest(const BoardConfig& config)
//{
//    if (m_isAcquiring) { Log("Busy."); return false; }
//    if (m_boards.size() < 2) { Log("Error: Need 2 boards for sync test."); return false; }
//
//    m_isAcquiring = true;
//    Log("--- Starting Sync Test ---");
//
//    // ------------------------------------------------------------------------
//    // 1. Configure SyncBoard (CRITICAL STEP)
//    // ------------------------------------------------------------------------
//    // We must set the clock *before* the digitizers try to lock onto it.
//    if (m_syncBoardHandle) {
//        // Disable trigger gate so we don't get garbage data during setup
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//
//        sb_clock_conf_t clock_conf;
//        // Logic: If user wants External Clock, SyncBoard passes it through.
//        //        If Internal, SyncBoard generates it.
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            clock_conf.source = sb_clock_source_external;
//            clock_conf.sample_rate_hz = 0; // Pass-through
//        }
//        else {
//            clock_conf.source = sb_clock_source_internal;
//            clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
//        }
//
//        // Apply settings
//        auto err = sb_device_set_clock(m_syncBoardHandle, clock_conf);
//        if (err != sb_rc_success) {
//            Log("Error: SyncBoard Clock Config Failed.");
//            m_isAcquiring = false;
//            return false;
//        }
//    }
//
//    // ------------------------------------------------------------------------
//    // 2. Configure Digitizers
//    // ------------------------------------------------------------------------
//    // Now that the clock is stable, configure the boards to listen to it.
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) {
//            StopAcquisition();
//            return false;
//        }
//    }
//
//    // ------------------------------------------------------------------------
//    // 3. Setup Continuous Acquisition (NPT)
//    // ------------------------------------------------------------------------
//    AcquisitionConfig acqConfig = {};
//    acqConfig.admaMode = ADMA_NPT;
//    acqConfig.samplesPerRecord = 4096;
//    // Use a small buffer count (e.g., 10) for the test, or 1000 to run longer.
//    acqConfig.recordsPerBuffer = 1;
//    acqConfig.buffersPerAcquisition = 10000; // Run "infinitely" (or close to it)
//    acqConfig.saveData = false;  // Don't fill up the hard drive
//    acqConfig.processData = true; // Enable the GUI calculation
//
//    // Set Buffer Size (4096 samples * 4 channels * 2 bytes)
//    U32 bytesPerBuffer = 4096 * 4 * 2;
//    U32 channelMask = CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D;
//
//    // ------------------------------------------------------------------------
//    // 4. Allocate & Arm
//    // ------------------------------------------------------------------------
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->AllocateBuffers(bytesPerBuffer)) { StopAcquisition(); return false; }
//        if (!board->PrepareForAcquisition(acqConfig, channelMask)) { StopAcquisition(); return false; }
//
//        // Post buffers to start the "Ring"
//        for (int i = 0; i < BUFFER_COUNT; i++) board->PostBuffer(i);
//
//        if (!board->StartCapture()) { StopAcquisition(); return false; }
//    }
//
//    // ------------------------------------------------------------------------
//    // 5. Open the Gate (Enable Triggers)
//    // ------------------------------------------------------------------------
//    Log("Boards Armed. Enabling SyncBoard Trigger...");
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//    }
//
//    // ------------------------------------------------------------------------
//    // 6. Run the Fast Loop
//    // ------------------------------------------------------------------------
//    // We re-use the RunAcquisition logic for the loop itself
//    // But we need to ensure 'processData' inside ProcessBufferData handles the math.
//
//    U32 buffersCompleted = 0;
//    bool success = true;
//
//    while (buffersCompleted < acqConfig.buffersPerAcquisition && success && m_isAcquiring)
//    {
//        U32 bufferIndex = buffersCompleted % BUFFER_COUNT;
//
//        for (AlazarDigitizer* board : m_boards)
//        {
//            if (!board->WaitFordBuffer(bufferIndex, config.triggerTimeoutMS + 1000)) {
//                success = false; break;
//            }
//
//            IO_BUFFER* pIoBuffer = board->GetBuffer(bufferIndex);
//
//            // ProcessBufferData will handle the Sync Math (Lag Calculation)
//            if (!ProcessBufferData(board, (U16*)pIoBuffer->pBuffer, acqConfig)) {
//                success = false; break;
//            }
//
//            if (!board->PostBuffer(bufferIndex)) {
//                success = false; break;
//            }
//        }
//        if (!success) break;
//        buffersCompleted++;
//    }
//
//    StopAcquisition();
//    return success;
//}

//
//bool AcquisitionController::RunSyncTest(const BoardConfig& config)
//{
//    if (m_isAcquiring) { Log("Busy."); return false; }
//    if (m_boards.size() < 2) { Log("Error: Need 2 boards."); return false; }
//    m_isAcquiring = true;
//    Log("--- Starting Sync Test ---");
//
//    if (m_syncBoardHandle) {
//        sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
//        sb_clock_conf_t clock_conf;
//        if (config.sampleRateId == SAMPLE_RATE_USER_DEF) {
//            clock_conf.source = sb_clock_source_external; clock_conf.sample_rate_hz = 0;
//        }
//        else {
//            clock_conf.source = sb_clock_source_internal; clock_conf.sample_rate_hz = (int64_t)config.sampleRateHz;
//        }
//        sb_device_set_clock(m_syncBoardHandle, clock_conf);
//    }
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->ConfigureBoard(config)) { StopAcquisition(); return false; }
//    }
//
//    AcquisitionConfig acqConfig = {};
//    acqConfig.admaMode = ADMA_NPT; acqConfig.samplesPerRecord = 4096; acqConfig.recordsPerBuffer = 1; acqConfig.buffersPerAcquisition = 1;
//    U32 bytesPerBuffer = 4096 * 4 * 2;
//
//    for (AlazarDigitizer* board : m_boards) {
//        board->AllocateBuffers(bytesPerBuffer);
//        board->PrepareForAcquisition(acqConfig, CHANNEL_A | CHANNEL_B | CHANNEL_C | CHANNEL_D);
//        board->PostBuffer(0);
//        board->StartCapture();
//    }
//
//    Log("Waiting for trigger...");
//    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_enabled);
//
//    bool success = true;
//    std::vector<std::vector<float>> snapshots;
//
//    for (AlazarDigitizer* board : m_boards) {
//        if (!board->WaitFordBuffer(0, config.triggerTimeoutMS + 1000)) {
//            Log("Timeout Board " + std::to_string(board->GetBoardId())); success = false;
//        }
//        else {
//            IO_BUFFER* pIoBuffer = board->GetBuffer(0);
//            U16* buffer = (U16*)pIoBuffer->pBuffer;
//            std::vector<float> chA_float(4096);
//            for (int i = 0; i < 4096; i++) {
//                double code = buffer[i * 4] >> 2;
//                chA_float[i] = (float)((code - 8191.5) / 8191.5);
//            }
//            snapshots.push_back(chA_float);
//        }
//    }
//
//    if (success && snapshots.size() >= 2) {
//        m_syncSnapshot1 = snapshots[0]; m_syncSnapshot2 = snapshots[1];
//        m_lastCalculatedLag = FindLagByXCorr(m_syncSnapshot1, m_syncSnapshot2, 50);
//        std::stringstream ss; ss << "Sync Test Complete. Lag: " << m_lastCalculatedLag; Log(ss.str());
//    }
//
//    StopAcquisition();
//    return success;
//}

// ============================================================================
// THREAD 2: DETECTOR LOOP (Peak Detection)
// ============================================================================
// 
// 

void AcquisitionController::DetectorThreadLoop()
{
    // --- FIX: Use Vector instead of Array ---
    // This allows the buffer to grow if the hardware sends more data than expected
    static std::vector<double> temp_ld_sig_m1;

    std::vector<double> local_peaks_this_pass;

    // --- STATE MACHINE VARIABLES ---
    static bool inside_high_zone = false;
    static double current_max_val = -1e9;
    static int current_max_pos = 0;

    static bool inside_low_zone = false;
    static double current_min_val = 1e9;
    static int current_min_pos = 0;

    while (m_keepProcessing)
    {
        // 1. Get Chunk
        DataChunk chunk;
        {
            std::unique_lock<std::mutex> lock(m_procQueueMutex);
            m_procQueueCV.wait(lock, [this] { return !m_procQueue.empty() || !m_keepProcessing; });
            if (!m_keepProcessing && m_procQueue.empty()) return;
            chunk = std::move(m_procQueue.front());
            m_procQueue.pop();
        }

        local_peaks_this_pass.clear();

        // 2. Fill Buffers (Standard)
        size_t samples = chunk.chA.size();
        for (size_t i = 0; i < samples; i++) {
            size_t idx = (m_cb_write_head + i) % CIRC_BUFFER_SIZE;
            m_cb_Img[idx] = (double)chunk.chA[i];
            m_cb_M1[idx] = (double)chunk.chB[i];
            m_cb_M2[idx] = (double)chunk.chC[i];
            m_cb_D[idx] = (double)chunk.chD[i];
        }

        size_t chunk_start_idx_in_cb = m_cb_write_head;
        m_cb_write_head = (m_cb_write_head + samples) % CIRC_BUFFER_SIZE;

        // 3. Process the Local Chunk
        // --- FIX: Resize vector to match incoming data size ---
        if (temp_ld_sig_m1.size() < samples) {
            temp_ld_sig_m1.resize(samples);
        }

        // Copy data (Now safe from access violations)
        for (size_t i = 0; i < samples; i++) {
            temp_ld_sig_m1[i] = (double)chunk.chA[i];
        }

        // 4. State Machine Loop
        double THRESH_HIGH = m_algoThreshHigh;
        double THRESH_LOW = m_algoThreshLow;
        bool new_lines = false;

        for (int i = 0; i < samples; i++) {

            double val = temp_ld_sig_m1[i];
            int current_global_pos = (chunk_start_idx_in_cb + i) % CIRC_BUFFER_SIZE;

            // ============================
            // TOP PEAK TRACKER
            // ============================
            if (val > THRESH_HIGH) {
                if (!inside_high_zone) {
                    inside_high_zone = true;
                    current_max_val = val;
                    current_max_pos = current_global_pos;
                }
                else {
                    if (val > current_max_val) {
                        current_max_val = val;
                        current_max_pos = current_global_pos;
                    }
                }
            }
            else {
                if (inside_high_zone) {
                    inside_high_zone = false;

                    m_top_peak_pos = current_max_pos - m_phaseAdjust;
                    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
                    if (m_top_peak_pos >= CIRC_BUFFER_SIZE) m_top_peak_pos -= CIRC_BUFFER_SIZE;

                    // Visualize
                    int dist_from_start = current_max_pos - chunk_start_idx_in_cb;
                    if (dist_from_start < 0) dist_from_start += CIRC_BUFFER_SIZE;
                    local_peaks_this_pass.push_back((double)dist_from_start);

                    if (m_bot_peak_found) {
                        m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
                        m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
                        m_lineParams.leftright[m_lp_cb_lr_w] = 0;
                        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
                        m_bot_peak_found = false;
                    }
                    m_top_peak_found = true;
                }
            }

            // ============================
            // BOTTOM PEAK TRACKER
            // ============================
            if (val < THRESH_LOW) {
                if (!inside_low_zone) {
                    inside_low_zone = true;
                    current_min_val = val;
                    current_min_pos = current_global_pos;
                }
                else {
                    if (val < current_min_val) {
                        current_min_val = val;
                        current_min_pos = current_global_pos;
                    }
                }
            }
            else {
                if (inside_low_zone) {
                    inside_low_zone = false;

                    m_bot_peak_pos = current_min_pos - m_phaseAdjust;
                    if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
                    if (m_bot_peak_pos >= CIRC_BUFFER_SIZE) m_bot_peak_pos -= CIRC_BUFFER_SIZE;

                    // Visualize
                    int dist_from_start = current_min_pos - chunk_start_idx_in_cb;
                    if (dist_from_start < 0) dist_from_start += CIRC_BUFFER_SIZE;
                    local_peaks_this_pass.push_back((double)dist_from_start);

                    m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
                    m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
                    m_lineParams.leftright[m_lp_cb_lr_w] = 1;
                    m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;

                    m_totalLinesProcessed++;
                    m_top_peak_found = false;
                    m_bot_peak_found = true;
                }
            }
        }

        // 5. Update Read Pointers & Process Lines (Standard Logic)
        int temp_w = m_lp_cb_lr_w;
        if (m_lp_cb_ud_r > temp_w) temp_w += NUMBER_OF_LINE_PARAMS;
        if (m_lp_cb_ud_r < temp_w) new_lines = true;

        for (int i = m_lp_cb_ud_r; i < temp_w; i++) {
            int imod = i % NUMBER_OF_LINE_PARAMS;
            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
            double sum_m2 = 0;
            for (int k = 0; k < len; k++) sum_m2 += m_cb_M2[(m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE];
            if (len > 0) m_lineParams.mean_m2[imod] = sum_m2 / len;
            m_lp_cb_ud_r = (m_lp_cb_ud_r + 1) % NUMBER_OF_LINE_PARAMS;
        }

        // 6. Publish Snapshot
        if (chunk.boardId == 1) {
            std::lock_guard<std::mutex> lock(m_guiDataMutex);
            if (m_guiSnapshotWaveform.size() != chunk.chA.size()) m_guiSnapshotWaveform.resize(chunk.chA.size());
            for (size_t k = 0; k < chunk.chA.size(); k++) {
                double code = chunk.chA[k] >> 2;
                m_guiSnapshotWaveform[k] = (float)((code - 8191.5) / 8191.5);
            }
            m_guiSnapshotPeaks = local_peaks_this_pass;
        }

        if (new_lines) {
            std::unique_lock<std::mutex> ul(m_lineMutex);
            m_linesReady = true;
            m_lineCV.notify_one();
        }
    }
}



//void AcquisitionController::DetectorThreadLoop()
//{
//    // Local buffer to store the current chunk for fast processing
//    // (We don't need the sliding window memmove buffer for the state machine logic)
//    static double temp_ld_sig_m1[NUMBER_OF_SEGMENTS_PER_BUFFER];
//
//    std::vector<double> local_peaks_this_pass;
//
//    // --- STATE MACHINE VARIABLES ---
//    // These must be static or member variables so they persist between chunks!
//    static bool inside_high_zone = false;
//    static double current_max_val = -1e9;
//    static int current_max_pos = 0;
//
//    static bool inside_low_zone = false;
//    static double current_min_val = 1e9;
//    static int current_min_pos = 0;
//
//    while (m_keepProcessing)
//    {
//        // 1. Get Chunk
//        DataChunk chunk;
//        {
//            std::unique_lock<std::mutex> lock(m_procQueueMutex);
//            m_procQueueCV.wait(lock, [this] { return !m_procQueue.empty() || !m_keepProcessing; });
//            if (!m_keepProcessing && m_procQueue.empty()) return;
//            chunk = std::move(m_procQueue.front());
//            m_procQueue.pop();
//        }
//
//        local_peaks_this_pass.clear();
//
//        // 2. Fill Buffers (Standard)
//        size_t samples = chunk.chA.size();
//        for (size_t i = 0; i < samples; i++) {
//            size_t idx = (m_cb_write_head + i) % CIRC_BUFFER_SIZE;
//            m_cb_Img[idx] = (double)chunk.chA[i];
//            m_cb_M1[idx] = (double)chunk.chA[i]; // Using ChA for detection (HACK)
//            m_cb_M2[idx] = (double)chunk.chC[i];
//        }
//
//        // Save where this chunk starts in the circular buffer for math later
//        size_t chunk_start_idx_in_cb = m_cb_write_head;
//        m_cb_write_head = (m_cb_write_head + samples) % CIRC_BUFFER_SIZE;
//
//        // 3. Process the Local Chunk directly
//        // Copy data to local buffer for speed & ease of access
//        for (size_t i = 0; i < samples; i++) {
//            temp_ld_sig_m1[i] = (double)chunk.chA[i];
//        }
//
//        // 4. State Machine Loop
//        double THRESH_HIGH = m_algoThreshHigh;
//        double THRESH_LOW = m_algoThreshLow;
//        bool new_lines = false;
//
//        for (int i = 0; i < samples; i++) {
//
//            double val = temp_ld_sig_m1[i];
//            // Calculate actual position in Circular Buffer
//            // We use this "global" index for the line_params so lines can span across chunks
//            int current_global_pos = (chunk_start_idx_in_cb + i) % CIRC_BUFFER_SIZE;
//
//            // ============================
//            // TOP PEAK TRACKER
//            // ============================
//            if (val > THRESH_HIGH) {
//                if (!inside_high_zone) {
//                    // Entered the Zone
//                    inside_high_zone = true;
//                    current_max_val = val;
//                    current_max_pos = current_global_pos;
//                }
//                else {
//                    // Tracking: Is this new point higher?
//                    if (val > current_max_val) {
//                        current_max_val = val;
//                        current_max_pos = current_global_pos;
//                    }
//                }
//            }
//            else {
//                // We are below threshold. Did we just exit?
//                if (inside_high_zone) {
//                    // YES. The peak was 'current_max_pos' (the highest point seen while in the zone)
//                    inside_high_zone = false;
//
//                    // Register the Peak
//                    m_top_peak_pos = current_max_pos - m_phaseAdjust; // Apply user phase shift
//
//                    // Handle Circular Buffer Wrapping for the stored position
//                    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//                    if (m_top_peak_pos >= CIRC_BUFFER_SIZE) m_top_peak_pos -= CIRC_BUFFER_SIZE;
//
//                    // Visualize (Map global back to local 0..4096 for GUI)
//                    // We calculate distance from the START of this chunk to the Peak
//                    int dist_from_start = current_max_pos - chunk_start_idx_in_cb;
//                    if (dist_from_start < 0) dist_from_start += CIRC_BUFFER_SIZE;
//                    local_peaks_this_pass.push_back((double)dist_from_start);
//
//                    // Logic to pair with bottom
//                    if (m_bot_peak_found) {
//                        m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
//                        m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
//                        m_lineParams.leftright[m_lp_cb_lr_w] = 0; // Forward
//                        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//                        m_bot_peak_found = false;
//                    }
//                    m_top_peak_found = true;
//                }
//            }
//
//            // ============================
//            // BOTTOM PEAK TRACKER
//            // ============================
//            if (val < THRESH_LOW) {
//                if (!inside_low_zone) {
//                    inside_low_zone = true;
//                    current_min_val = val;
//                    current_min_pos = current_global_pos;
//                }
//                else {
//                    if (val < current_min_val) {
//                        current_min_val = val;
//                        current_min_pos = current_global_pos;
//                    }
//                }
//            }
//            else {
//                if (inside_low_zone) {
//                    inside_low_zone = false;
//
//                    m_bot_peak_pos = current_min_pos - m_phaseAdjust;
//                    if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
//                    if (m_bot_peak_pos >= CIRC_BUFFER_SIZE) m_bot_peak_pos -= CIRC_BUFFER_SIZE;
//
//                    // Visualize
//                    int dist_from_start = current_min_pos - chunk_start_idx_in_cb;
//                    if (dist_from_start < 0) dist_from_start += CIRC_BUFFER_SIZE;
//                    local_peaks_this_pass.push_back((double)dist_from_start);
//
//                    m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
//                    m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
//                    m_lineParams.leftright[m_lp_cb_lr_w] = 1; // Backward
//                    m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//                    m_top_peak_found = false;
//                    m_bot_peak_found = true;
//                }
//            }
//        }
//
//        // 5. Update Read Pointers & Process Lines (Standard Logic)
//        int temp_w = m_lp_cb_lr_w;
//        if (m_lp_cb_ud_r > temp_w) temp_w += NUMBER_OF_LINE_PARAMS;
//        if (m_lp_cb_ud_r < temp_w) new_lines = true;
//
//        for (int i = m_lp_cb_ud_r; i < temp_w; i++) {
//            // M2 Averaging Logic
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//
//            double sum_m2 = 0;
//            for (int k = 0; k < len; k++) {
//                sum_m2 += m_cb_M2[(m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE];
//            }
//            if (len > 0) m_lineParams.mean_m2[imod] = sum_m2 / len;
//
//            // Note: Up/Down logic (Comparing to prev mean) can go here if needed
//
//            m_lp_cb_ud_r = (m_lp_cb_ud_r + 1) % NUMBER_OF_LINE_PARAMS;
//        }
//
//        // 6. Publish Snapshot (GUI Update)
//        if (chunk.boardId == 1) {
//            std::lock_guard<std::mutex> lock(m_guiDataMutex);
//
//            // Resize if needed
//            if (m_guiSnapshotWaveform.size() != chunk.chA.size())
//                m_guiSnapshotWaveform.resize(chunk.chA.size());
//
//            // Copy Waveform Data (Float conversion)
//            for (size_t k = 0; k < chunk.chA.size(); k++) {
//                double code = chunk.chA[k] >> 2;
//                m_guiSnapshotWaveform[k] = (float)((code - 8191.5) / 8191.5);
//            }
//
//            // Copy Peak Data
//            m_guiSnapshotPeaks = local_peaks_this_pass;
//        }
//
//        // 7. Notify Generator Thread
//        if (new_lines) {
//            std::unique_lock<std::mutex> ul(m_lineMutex);
//            m_linesReady = true;
//            m_lineCV.notify_one();
//        }
//    }
//}
//void AcquisitionController::DetectorThreadLoop()
//{
//    const int LR_SEARCH_AREA = 10;
//    static double temp_ld_sig_m1[2 * LR_SEARCH_AREA + NUMBER_OF_SEGMENTS_PER_BUFFER];
//    memset(temp_ld_sig_m1, 0, sizeof(temp_ld_sig_m1));
//
//    while (m_keepProcessing)
//    {
//        DataChunk chunk;
//        {
//            std::unique_lock<std::mutex> lock(m_procQueueMutex);
//            m_procQueueCV.wait(lock, [this] { return !m_procQueue.empty() || !m_keepProcessing; });
//            if (!m_keepProcessing && m_procQueue.empty()) return;
//            chunk = std::move(m_procQueue.front());
//            m_procQueue.pop();
//        }
//
//        // Fill Circular Buffers
//        size_t samples = chunk.chA.size();
//        for (size_t i = 0; i < samples; i++) {
//            size_t idx = (m_cb_write_head + i) % CIRC_BUFFER_SIZE;
//            /*m_cb_Img[idx] = (double)chunk.chA[i];
//            m_cb_M1[idx] = (double)chunk.chB[i];
//            m_cb_M2[idx] = (double)chunk.chC[i];*/
//            m_cb_Img[idx] = (double)chunk.chA[i];
//
//            // HACK: Use Channel A for Peak Detection too!
//            m_cb_M1[idx] = (double)chunk.chA[i];
//
//            m_cb_M2[idx] = (double)chunk.chC[i];
//        }
//        m_cb_write_head = (m_cb_write_head + samples) % CIRC_BUFFER_SIZE;
//
//        // Peak Detection Logic
//        memmove(&temp_ld_sig_m1, &temp_ld_sig_m1[NUMBER_OF_SEGMENTS_PER_BUFFER], sizeof(double) * (2 * LR_SEARCH_AREA));
//        size_t read_start = (m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER);
//        if (read_start + NUMBER_OF_SEGMENTS_PER_BUFFER <= CIRC_BUFFER_SIZE) {
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA], &m_cb_M1[read_start], sizeof(double) * NUMBER_OF_SEGMENTS_PER_BUFFER);
//        }
//        else {
//            size_t p1 = CIRC_BUFFER_SIZE - read_start;
//            size_t p2 = NUMBER_OF_SEGMENTS_PER_BUFFER - p1;
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA], &m_cb_M1[read_start], sizeof(double) * p1);
//            memcpy(&temp_ld_sig_m1[2 * LR_SEARCH_AREA + p1], &m_cb_M1[0], sizeof(double) * p2);
//        }
//
//        bool new_lines = false;
//        bool peak_detected;
//    
//        for (int i = LR_SEARCH_AREA; i < NUMBER_OF_SEGMENTS_PER_BUFFER + LR_SEARCH_AREA; i++) {
//            // 1. Define the Phase Adjustment Offset once (Apply to both Min and Max)
//        // (2 * LR) aligns the detection event (end of window) back to the actual peak (middle of window)
//        // m_phaseAdjust allows you to slide it manually in the GUI
//            int calculated_offset = (2 * LR_SEARCH_AREA) - m_phaseAdjust;
//
//            peak_detected = true;
//
//            // ==========================================================
//            // TOP PEAK (MAX) LOGIC
//            // ==========================================================
//            if (!m_top_peak_found) {
//                // Check neighbors
//                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//                    if (temp_ld_sig_m1[i] < temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//                }
//
//                if (peak_detected) {
//                    // Calculate Position with Phase Adjust
//                    m_top_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - calculated_offset;
//
//                    // Handle Circular Buffer Wrapping
//                    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//                    if (m_top_peak_pos >= CIRC_BUFFER_SIZE) m_top_peak_pos -= CIRC_BUFFER_SIZE;
//
//                    m_top_peak_found = true;
//
//                    // --- DEBUG VISUALIZATION (RED LINES) ---
//                    {
//                        std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                        // Calculate relative position for the current scope plot
//                        double relative_pos = (double)(i - (2 * LR_SEARCH_AREA) + m_phaseAdjust);
//                        m_debugPeakLocations.push_back(relative_pos);
//                        if (m_debugPeakLocations.size() > 100) m_debugPeakLocations.erase(m_debugPeakLocations.begin());
//                    }
//                    // ---------------------------------------
//
//                    // Pair with previous Bottom Peak to form a line
//                    if (m_bot_peak_found) {
//                        m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
//                        m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
//                        m_lineParams.leftright[m_lp_cb_lr_w] = 0; // Forward Scan
//                        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//                    }
//                    m_bot_peak_found = false;
//                }
//            }
//            // ==========================================================
//            // BOTTOM PEAK (MIN) LOGIC
//            // ==========================================================
//            else {
//                // Check neighbors (Logic reversed: current < neighbors)
//                for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//                    if (temp_ld_sig_m1[i] > temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//                }
//
//                if (peak_detected) {
//                    // Calculate Position with Phase Adjust
//                    m_bot_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - calculated_offset;
//
//                    // Handle Circular Buffer Wrapping
//                    if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
//                    if (m_bot_peak_pos >= CIRC_BUFFER_SIZE) m_bot_peak_pos -= CIRC_BUFFER_SIZE;
//
//                    m_bot_peak_found = true;
//
//                    // --- DEBUG VISUALIZATION (RED LINES) ---
//                    {
//                        std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                        // Same calculation ensures valleys align with peaks visually
//                        double relative_pos = (double)(i - (2 * LR_SEARCH_AREA) + m_phaseAdjust);
//                        m_debugPeakLocations.push_back(relative_pos);
//                        if (m_debugPeakLocations.size() > 100) m_debugPeakLocations.erase(m_debugPeakLocations.begin());
//                    }
//                    // ---------------------------------------
//
//                    // Pair with previous Top Peak to form a line
//                    m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
//                    m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
//                    m_lineParams.leftright[m_lp_cb_lr_w] = 1; // Backward Scan
//                    m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//
//                    m_top_peak_found = false;
//                }
//            }
//            
//            /*if (m_first_buffer) { i += (2 * LR_SEARCH_AREA); m_first_buffer = false; }
//            peak_detected = true;*/
//
//            //// --- FIND TOP PEAK (MAX) ---
//            //if (!m_top_peak_found) {
//            //    for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//            //        if (temp_ld_sig_m1[i] < temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//            //    }
//            //    // 1. REVERT to the safe math (2 * LR)
//            //    // 2. ADD the manual phase adjustment
//            //    int calculated_offset = (2 * LR_SEARCH_AREA) - m_phaseAdjust;
//
//            //    m_top_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - calculated_offset;
//
//            //    // Handle Circular Wrap
//            //    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//            //    if (m_top_peak_pos >= CIRC_BUFFER_SIZE) m_top_peak_pos -= CIRC_BUFFER_SIZE;
//
//            //    m_top_peak_found = true;
//
//            //    // DEBUG: Save relative position for GUI Red Lines
//            //    {
//            //        std::lock_guard<std::mutex> lock(m_guiDataMutex);
//            //        // We subtract the "History" size (2*LR) so 0 aligns with 0
//            //        // Then we apply the phase adjust so you can slide it visually
//            //        double relative_pos = (double)(i - (2 * LR_SEARCH_AREA) + m_phaseAdjust);
//            //        m_debugPeakLocations.push_back(relative_pos);
//            //        if (m_debugPeakLocations.size() > 100) m_debugPeakLocations.erase(m_debugPeakLocations.begin());
//            //    }
//            //    //if (peak_detected) {
//            //    //    // ----- OFFSET FIX IS HERE -----
//            //    //    // Old Code: ... - (2 * LR_SEARCH_AREA);
//            //    //    m_top_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - LR_SEARCH_AREA;
//            //    //    // ------------------------------
//
//            //    //    if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//            //    //    m_top_peak_found = true;
//
//            //    //    if (m_bot_peak_found) {
//            //    //        m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
//            //    //        m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
//            //    //        m_lineParams.leftright[m_lp_cb_lr_w] = 0;
//            //    //        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//            //    //    }
//            //    //    m_bot_peak_found = false;
//            //    //}
//            //}
//            //// --- FIND BOTTOM PEAK (MIN) ---
//            //else {
//            //    for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//            //        if (temp_ld_sig_m1[i] > temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//            //    }
//            //    if (peak_detected) {
//            //        // ----- OFFSET FIX IS HERE -----
//            //        // Old Code: ... - (2 * LR_SEARCH_AREA);
//            //        m_bot_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - 2* LR_SEARCH_AREA;
//            //        // ------------------------------
//
//            //        if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
//            //        m_bot_peak_found = true;
//
//            //        m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
//            //        m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
//            //        m_lineParams.leftright[m_lp_cb_lr_w] = 1;
//            //        m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//            //        m_top_peak_found = false;
//            //    }
//            //}
//        }
//        //for (int i = LR_SEARCH_AREA; i < NUMBER_OF_SEGMENTS_PER_BUFFER + LR_SEARCH_AREA; i++) {
//        //    if (m_first_buffer) { i += (2 * LR_SEARCH_AREA); m_first_buffer = false; }
//        //    peak_detected = true;
//        //    if (!m_top_peak_found) {
//        //        // MAX
//        //        for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//        //            if (temp_ld_sig_m1[i] < temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//        //        }
//        //        if (peak_detected) {
//        //            m_top_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
//        //            if (m_top_peak_pos < 0) m_top_peak_pos += CIRC_BUFFER_SIZE;
//        //            m_top_peak_found = true;
//        //            if (m_bot_peak_found) {
//        //                m_lineParams.start[m_lp_cb_lr_w] = m_bot_peak_pos;
//        //                m_lineParams.end[m_lp_cb_lr_w] = m_top_peak_pos;
//        //                m_lineParams.leftright[m_lp_cb_lr_w] = 0;
//        //                m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//        //            }
//        //            m_bot_peak_found = false;
//        //            // DEBUG: Save this position for GUI
//        //            {
//        //                std::lock_guard<std::mutex> lock(m_guiDataMutex);
//        //                // We only want peaks relevant to the current chunk for display
//        //                // Convert circular buffer index back to local chunk index roughly
//        //                // This is just for visualization, approximate is fine.
//        //                double relative_pos = (double)(i - LR_SEARCH_AREA);
//        //                m_debugPeakLocations.push_back(relative_pos);
//
//        //                // Keep vector small
//        //                if (m_debugPeakLocations.size() > 100) m_debugPeakLocations.erase(m_debugPeakLocations.begin());
//        //            }
//
//
//
//
//        //        }
//        //    }
//        //    else {
//        //        // MIN
//        //        for (int j = -LR_SEARCH_AREA; j < LR_SEARCH_AREA + 1; j++) {
//        //            if (temp_ld_sig_m1[i] > temp_ld_sig_m1[i + j]) { peak_detected = false; break; }
//        //        }
//        //        if (peak_detected) {
//        //            m_bot_peak_pos = i + m_s_m1_cb_lp_r * NUMBER_OF_SEGMENTS_PER_BUFFER - (2 * LR_SEARCH_AREA);
//        //            if (m_bot_peak_pos < 0) m_bot_peak_pos += CIRC_BUFFER_SIZE;
//        //            m_bot_peak_found = true;
//        //            m_lineParams.start[m_lp_cb_lr_w] = m_top_peak_pos;
//        //            m_lineParams.end[m_lp_cb_lr_w] = m_bot_peak_pos;
//        //            m_lineParams.leftright[m_lp_cb_lr_w] = 1;
//        //            m_lp_cb_lr_w = (m_lp_cb_lr_w + 1) % NUMBER_OF_LINE_PARAMS;
//        //            m_top_peak_found = false;
//        //        }
//        //    }
//        //}
//        m_s_m1_cb_lp_r = (m_s_m1_cb_lp_r + 1) % NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER;
//
//        // Calc M2 Avg
//        int temp_w = m_lp_cb_lr_w;
//        if (m_lp_cb_ud_r > temp_w) temp_w += NUMBER_OF_LINE_PARAMS;
//        if (m_lp_cb_ud_r < temp_w) new_lines = true;
//
//        for (int i = m_lp_cb_ud_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//            double sum_m2 = 0;
//            for (int k = 0; k < len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                sum_m2 += m_cb_M2[idx];
//            }
//            if (len > 0) m_lineParams.mean_m2[imod] = sum_m2 / len;
//            int prev = (imod - 1 + NUMBER_OF_LINE_PARAMS) % NUMBER_OF_LINE_PARAMS;
//            m_lineParams.updown[imod] = (m_lineParams.mean_m2[imod] > m_lineParams.mean_m2[prev]) ? 0 : 1;
//            m_lp_cb_ud_r = (m_lp_cb_ud_r + 1) % NUMBER_OF_LINE_PARAMS;
//        }
//
//        if (new_lines) {
//            std::unique_lock<std::mutex> ul(m_lineMutex);
//            m_linesReady = true;
//            m_lineCV.notify_one();
//        }
//    }
//}

// ============================================================================
// THREAD 3: GENERATOR LOOP (Image Generation)
// ============================================================================
// 

//void AcquisitionController::GeneratorThreadLoop()
//{
//    // Persistent buffer to avoid memory fragmentation/allocation per line
//    static std::vector<double> temp_line_data(NUMBER_OF_POINTS_PER_LINE);
//
//    // --- DIAGNOSTIC Y-COUNTER ---
//    // Use this to force sequential stacking to prove X-axis logic is working
//    static int diagnostic_row_index = 0;
//
//    while (m_keepProcessing)
//    {
//        // 1. Thread Synchronization: Wait for lines ready from DetectorThread
//        std::unique_lock<std::mutex> ul(m_lineMutex);
//        m_lineCV.wait(ul, [this] { return m_linesReady || !m_keepProcessing; });
//        m_linesReady = false;
//        ul.unlock();
//
//        if (!m_keepProcessing) return;
//
//        // 2. Identify range of new lines in the parameter buffer
//        long temp_w = m_lp_cb_lr_w;
//        if (temp_w < m_lp_line_r) {
//            temp_w += NUMBER_OF_LINE_PARAMS;
//        }
//
//        // 3. Process each detected line
//        for (int i = m_lp_line_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//
//            // A. Calculate physical length of the line from peak detection indices
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//
//            // B. Safety check: Ignore lines that are too short (noise triggers) or empty
//            int copy_len = (len > NUMBER_OF_POINTS_PER_LINE) ? NUMBER_OF_POINTS_PER_LINE : len;
//            if (copy_len <= 0) continue;
//
//            // OPTIONAL DIAGNOSTIC: Uncomment to see if detector is finding full lines
//            // if (imod % 100 == 0) std::cout << "Detected Line Length: " << len << std::endl;
//
//            // C. Reset temporary row buffer and copy intensity data from circular buffer
//            temp_line_data.assign(NUMBER_OF_POINTS_PER_LINE, 0.0);
//            for (int k = 0; k < copy_len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                temp_line_data[k] = m_cb_Img[idx];
//            }
//
//            // D. Bidirectional Alignment: Reverse odd-numbered lines (Backward scans)
//            // This is what creates the "Snake" or continuous sweep pattern
//            if (m_lineParams.leftright[imod] == 1) {
//                std::reverse(temp_line_data.begin(), temp_line_data.begin() + copy_len);
//            }
//
//            // E. Map to Y-Coordinate (Sequential Debug Mode)
//            // We use diagnostic_row_index to force the rows to stack perfectly.
//            // If this results in a maze, your voltage-to-Y formula was the problem.
//            int y_idx = diagnostic_row_index;
//            diagnostic_row_index++;
//
//            if (diagnostic_row_index >= NUMBER_OF_POINTS_PER_LINE) {
//                diagnostic_row_index = 0;
//            }
//
//            // F. Atomic Write to Final Image Array
//            {
//                std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) {
//                    m_finalImage[y_idx][x] = temp_line_data[x];
//                }
//            }
//        }
//
//        // 4. Update the read pointer to confirm these lines are finished
//        m_lp_line_r = temp_w % NUMBER_OF_LINE_PARAMS;
//    }
//}


//
//void AcquisitionController::GeneratorThreadLoop()
//{
//    // Static buffer to hold one line of data (avoids re-allocation)
//    static std::vector<double> temp_line_data(NUMBER_OF_POINTS_PER_LINE);
//
//    // --- AUTO-RANGE VARIABLES (Added for fix) ---
//    // These track the min and max Y-voltage seen so far.
//    // Initialized to inverted values so they update immediately on the first frame.
//    static double seen_min = 1e9;
//    static double seen_max = -1e9;
//
//    while (m_keepProcessing)
//    {
//        // 1. Wait for new lines from the Detector Thread
//        std::unique_lock<std::mutex> ul(m_lineMutex);
//        m_lineCV.wait(ul, [this] { return m_linesReady || !m_keepProcessing; });
//        m_linesReady = false;
//        ul.unlock();
//
//        if (!m_keepProcessing) return;
//
//        // 2. Determine how many lines to process
//        long temp_w = m_lp_cb_lr_w;
//        if (temp_w < m_lp_line_r) temp_w += NUMBER_OF_LINE_PARAMS;
//
//        // 3. Process the batch of lines
//        for (int i = m_lp_line_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//
//            // A. Calculate Line Length
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//
//            // B. Prepare Data Buffer
//            int copy_len = (len > NUMBER_OF_POINTS_PER_LINE) ? NUMBER_OF_POINTS_PER_LINE : len;
//            if (copy_len <= 0) continue;
//
//            temp_line_data.assign(NUMBER_OF_POINTS_PER_LINE, 0.0); // Reset to 0 (black)
//
//            // C. Copy Pixels from Circular Buffer
//            for (int k = 0; k < copy_len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                temp_line_data[k] = m_cb_Img[idx];
//            }
//
//            // D. Reverse Backward Scans (Bidirectional Alignment)
//            if (m_lineParams.leftright[imod] == 1) {
//                std::reverse(temp_line_data.begin(), temp_line_data.begin() + copy_len);
//            }
//
//            // ---------------------------------------------------------
//            // E. Y-AXIS MAPPING (AUTO-RANGING UPDATE)
//            // ---------------------------------------------------------
//            double current_m2 = m_lineParams.mean_m2[imod];
//
//            // Update seen range
//            if (current_m2 < seen_min) seen_min = current_m2;
//            if (current_m2 > seen_max) seen_max = current_m2;
//
//            // Calculate range (with safety against divide-by-zero)
//            double range = seen_max - seen_min;
//            if (range < 1.0) range = 1.0;
//
//            // Normalize (0.0 to 1.0)
//            double y_norm = (current_m2 - seen_min) / range;
//
//            // Map to Pixel Index (0 to 1023)
//            // Note: If image is upside down, change to: (1.0 - y_norm)
//            int y_idx = (int)(ceil(y_norm * NUMBER_OF_POINTS_PER_LINE) - 1);
//
//            // Safety Clamps
//            if (y_idx < 0) y_idx = 0;
//            if (y_idx >= NUMBER_OF_POINTS_PER_LINE) y_idx = NUMBER_OF_POINTS_PER_LINE - 1;
//
//            // ---------------------------------------------------------
//
//            // F. Write to Final Image (Thread Safe)
//            {
//                std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                // Copy the prepared line into the specific row of the 2D image
//                for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) {
//                    m_finalImage[y_idx][x] = temp_line_data[x];
//                }
//            }
//        }
//
//        // Update Read Pointer
//        m_lp_line_r = temp_w % NUMBER_OF_LINE_PARAMS;
//    }
//}
//





void AcquisitionController::GeneratorThreadLoop()
{
    // Persistent buffer to avoid memory fragmentation/allocation per line
    static std::vector<double> temp_line_data(NUMBER_OF_POINTS_PER_LINE);

    // --- DIAGNOSTIC Y-COUNTER ---
    // Use this to force sequential stacking to prove X-axis logic is working.
    // This ignores the voltage from Channel C (mean_m2) and just stacks lines 0, 1, 2...
    static int diagnostic_row_index = 0;

    // (We keep these variables here so they don't cause compile errors, 
    // but they are unused in this specific debug version)
    static double seen_min = 1e9;
    static double seen_max = -1e9;

    while (m_keepProcessing)
    {
        // 1. Thread Synchronization: Wait for lines ready from DetectorThread
        std::unique_lock<std::mutex> ul(m_lineMutex);
        m_lineCV.wait(ul, [this] { return m_linesReady || !m_keepProcessing; });
        m_linesReady = false;
        ul.unlock();

        if (!m_keepProcessing) return;

        // 2. Identify range of new lines in the parameter buffer
        long temp_w = m_lp_cb_lr_w;
        if (temp_w < m_lp_line_r) {
            temp_w += NUMBER_OF_LINE_PARAMS;
        }

        // 3. Process each detected line
        for (int i = m_lp_line_r; i < temp_w; i++) {
            int imod = i % NUMBER_OF_LINE_PARAMS;

            // A. Calculate physical length of the line from peak detection indices
            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);

            // B. Safety check: Ignore lines that are too short (noise triggers) or empty
            int copy_len = (len > NUMBER_OF_POINTS_PER_LINE) ? NUMBER_OF_POINTS_PER_LINE : len;
            if (copy_len <= 0) continue;

            // C. Reset temporary row buffer and copy intensity data from circular buffer
            temp_line_data.assign(NUMBER_OF_POINTS_PER_LINE, 0.0);
            for (int k = 0; k < copy_len; k++) {
                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
                temp_line_data[k] = m_cb_Img[idx];
            }

            // D. Bidirectional Alignment: Reverse odd-numbered lines (Backward scans)
            if (m_lineParams.leftright[imod] == 1) {
                std::reverse(temp_line_data.begin(), temp_line_data.begin() + copy_len);
            }

            // ---------------------------------------------------------
            // E. SEQUENTIAL ROW MAPPING (DEBUG MODE)
            // ---------------------------------------------------------
            // Instead of calculating y_idx from voltage, we just increment.
            int y_idx = diagnostic_row_index;
            diagnostic_row_index++;

            // Wrap around if we hit the bottom of the image
            if (diagnostic_row_index >= NUMBER_OF_POINTS_PER_LINE) {
                diagnostic_row_index = 0;
            }
            // ---------------------------------------------------------

            // F. Atomic Write to Final Image Array
            {
                std::lock_guard<std::mutex> lock(m_guiDataMutex);
                for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) {
                    m_finalImage[y_idx][x] = temp_line_data[x];
                }
            }
        }

        // 4. Update the read pointer to confirm these lines are finished
        m_lp_line_r = temp_w % NUMBER_OF_LINE_PARAMS;
    }
}




//
//void AcquisitionController::GeneratorThreadLoop()
//{
//    static std::vector<double> temp_line_data(NUMBER_OF_POINTS_PER_LINE);
//    while (m_keepProcessing)
//    {
//        std::unique_lock<std::mutex> ul(m_lineMutex);
//        m_lineCV.wait(ul, [this] { return m_linesReady || !m_keepProcessing; });
//        m_linesReady = false;
//        ul.unlock();
//        if (!m_keepProcessing) return;
//
//        long temp_w = m_lp_cb_lr_w;
//        if (temp_w < m_lp_line_r) temp_w += NUMBER_OF_LINE_PARAMS;
//
//        for (int i = m_lp_line_r; i < temp_w; i++) {
//            int imod = i % NUMBER_OF_LINE_PARAMS;
//            int len = (m_lineParams.end[imod] >= m_lineParams.start[imod])
//                ? (m_lineParams.end[imod] - m_lineParams.start[imod])
//                : (m_lineParams.end[imod] + CIRC_BUFFER_SIZE - m_lineParams.start[imod]);
//            int copy_len = (len > NUMBER_OF_POINTS_PER_LINE) ? NUMBER_OF_POINTS_PER_LINE : len;
//            if (copy_len <= 0) continue;
//
//            temp_line_data.assign(NUMBER_OF_POINTS_PER_LINE, 0.0);
//            for (int k = 0; k < copy_len; k++) {
//                int idx = (m_lineParams.start[imod] + k) % CIRC_BUFFER_SIZE;
//                temp_line_data[k] = m_cb_Img[idx];
//            }
//            if (m_lineParams.leftright[imod] == 1) std::reverse(temp_line_data.begin(), temp_line_data.begin() + copy_len);
//
//            static double m2_max = 38500; static double m2_min = 28500;
//            double y_norm = (m_lineParams.mean_m2[imod] - m2_min) / (m2_max - m2_min);
//            int y_idx = (int)(ceil(y_norm * NUMBER_OF_POINTS_PER_LINE) - 1);
//            if (y_idx < 0) y_idx = 0;
//            if (y_idx >= NUMBER_OF_POINTS_PER_LINE) y_idx = NUMBER_OF_POINTS_PER_LINE - 1;
//
//            {
//                std::lock_guard<std::mutex> lock(m_guiDataMutex);
//                for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) m_finalImage[y_idx][x] = temp_line_data[x];
//            }
//        }
//        m_lp_line_r = temp_w % NUMBER_OF_LINE_PARAMS;
//    }
//}
//
// ============================================================================
// PUBLIC GETTERS
// ============================================================================

void AcquisitionController::GetLatestImage(std::vector<std::vector<double>>& outImage) {
    std::lock_guard<std::mutex> lock(m_guiDataMutex);
    if (outImage.size() != NUMBER_OF_POINTS_PER_LINE) {
        outImage.resize(NUMBER_OF_POINTS_PER_LINE, std::vector<double>(NUMBER_OF_POINTS_PER_LINE));
    }
    for (int y = 0; y < NUMBER_OF_POINTS_PER_LINE; y++) {
        for (int x = 0; x < NUMBER_OF_POINTS_PER_LINE; x++) {
            //std::cout << "the x and y detected were " << x << "," << y;
            outImage[y][x] = m_finalImage[y][x];
        }
    }
}

// Updated Scope Getter (Supports index 0=A, 1=B, 2=C)
void AcquisitionController::GetLatestScopeData(std::vector<float>& data, U32 boardId, int channelIndex) {
    std::lock_guard<std::mutex> lock(m_guiDataMutex);
    if (m_latestScopeData.count(boardId)) {
        auto& channels = m_latestScopeData[boardId];
        if (channelIndex >= 0 && channelIndex < channels.size()) {
            data = channels[channelIndex];
            return;
        }
    }
    data.clear();
}

std::vector<std::string> AcquisitionController::GetLogMessages() { return m_logMessages; }
void AcquisitionController::GetSyncSnapshot(std::vector<float>& b1, std::vector<float>& b2) { b1 = m_syncSnapshot1; b2 = m_syncSnapshot2; }
bool AcquisitionController::IsAcquiring() { return m_isAcquiring; }

// ============================================================================
// CLEANUP & SAVING
// ============================================================================

void AcquisitionController::StopAcquisition() {
    if (m_syncBoardHandle) sb_device_set_trigger_status(m_syncBoardHandle, sb_trigger_status_disabled);
    for (auto b : m_boards) { b->AbortAcquisition(); b->FreeBuffers(); }
    CloseDataFiles(); m_isAcquiring = false;
}

void AcquisitionController::SaveThreadLoop() {
    while (m_keepProcessing) {
        DataChunk chunk;
        {
            std::unique_lock<std::mutex> lock(m_saveQueueMutex);
            m_saveQueueCV.wait(lock, [this] { return !m_saveQueue.empty() || !m_keepProcessing; });
            if (!m_keepProcessing && m_saveQueue.empty()) return;
            chunk = std::move(m_saveQueue.front());
            m_saveQueue.pop();
        }
        int boardIdx = -1;
        if (chunk.boardId == 1) boardIdx = 0; else if (chunk.boardId == 2) boardIdx = 1;
        if (boardIdx >= 0) {
            int f = boardIdx * 4;
            if (f + 3 < m_fileStreams.size()) {
                if (!chunk.chA.empty() && m_fileStreams[f].is_open()) m_fileStreams[f].write((char*)chunk.chA.data(), chunk.chA.size() * 2);
                if (!chunk.chB.empty() && m_fileStreams[f + 1].is_open()) m_fileStreams[f + 1].write((char*)chunk.chB.data(), chunk.chB.size() * 2);
                if (!chunk.chC.empty() && m_fileStreams[f + 2].is_open()) m_fileStreams[f + 2].write((char*)chunk.chC.data(), chunk.chC.size() * 2);
                if (!chunk.chD.empty() && m_fileStreams[f + 3].is_open()) m_fileStreams[f + 3].write((char*)chunk.chD.data(), chunk.chD.size() * 2);
            }
        }
    }
}

bool AcquisitionController::OpenDataFiles(U32 boardCount) {
    char cwd[MAX_PATH]; if (!_getcwd(cwd, MAX_PATH)) return false; m_savePath = cwd;
    m_fileStreams.resize(boardCount * 4); char chs[] = { 'A','B','C','D' };
    for (U32 i = 0; i < boardCount; i++) {
        for (int c = 0; c < 4; c++) {
            std::stringstream ss; ss << m_savePath << "\\board" << m_boards[i]->GetBoardId() << "_ch" << chs[c] << ".bin";
            m_fileStreams[i * 4 + c].open(ss.str(), std::ios::binary);
        }
    }
    return true;
}
void AcquisitionController::GetLatestPeaks(std::vector<double>& peaks) {
    std::lock_guard<std::mutex> lock(m_guiDataMutex);
    peaks = m_debugPeakLocations;
}
void AcquisitionController::GetLatestSnapshot(std::vector<float>& waveform, std::vector<double>& peaks) {
    std::lock_guard<std::mutex> lock(m_guiDataMutex);
    waveform = m_guiSnapshotWaveform;
    peaks = m_guiSnapshotPeaks;
}
void AcquisitionController::CloseDataFiles() { for (auto& f : m_fileStreams) if (f.is_open()) f.close(); m_fileStreams.clear(); }
void AcquisitionController::Log(std::string m) { std::cout << m << std::endl; m_logMessages.push_back(m); if (m_logMessages.size() > 100) m_logMessages.erase(m_logMessages.begin()); }//////////