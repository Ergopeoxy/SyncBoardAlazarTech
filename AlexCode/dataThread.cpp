#include "dataThread.h"

#if 0
bool runDataThread() {
    // Generate time vector
    for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
        x_samples[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_segments[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_cb_segments[i] = i;
    }
    for (int i = 0; i < 500; i++) {
        x_line_samples[i] = i;
    }

    time_index = 0;
    buffer_index = 0;
    // read data from file
    FILE* dataFile;
    errno_t error_code;
    int errorCheck;
    unsigned long long fileSize;

    error_code = fopen_s(&dataFile, "C:\\Users\\illumiSonics\\Documents\\JATS_TempDataRecord\\2023-01-12-14-32-05-data.bin", "rb");

    errorCheck = _fseeki64(dataFile, 0, SEEK_END);
    fileSize = _ftelli64(dataFile);
    rewind(dataFile);

    uint16_t* dataBuffer;
    dataBuffer = (uint16_t*)malloc(fileSize);
    unsigned long long num = fread(dataBuffer, sizeof(uint16_t), fileSize / 2, dataFile);

    fclose(dataFile);

    //error_code = fopen_s(&dataFile, "C:\\Users\\ALEXPC\\Documents\\PML_DataRecord\\2023-01-12-15-58-18-data-test.bin", "wb");
    //fwrite(dataBuffer, sizeof(uint16_t), fileSize/2 ,dataFile);
    //fclose(dataFile);

    while (gui_running) {
        std::unique_lock<std::mutex> ul(data_mutex);
        for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
            for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                long long position = i * NUMBER_OF_SAMPLES_PER_SEGMENT + j + buffer_index * NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT * 4;
                channel1_V[i][j] = dataBuffer[0 * NUMBER_OF_SAMPLES_PER_SEGMENT * NUMBER_OF_SEGMENTS_PER_BUFFER + position];
                channel2_V[i][j] = dataBuffer[1 * NUMBER_OF_SAMPLES_PER_SEGMENT * NUMBER_OF_SEGMENTS_PER_BUFFER + position];
                channel3_V[i][j] = dataBuffer[2 * NUMBER_OF_SAMPLES_PER_SEGMENT * NUMBER_OF_SEGMENTS_PER_BUFFER + position];
                channel4_V[i][j] = dataBuffer[3 * NUMBER_OF_SAMPLES_PER_SEGMENT * NUMBER_OF_SEGMENTS_PER_BUFFER + position];
            }
        }

        buffer_index += 1;
        buffer_index %= NUMBER_OF_BUFFERS_PER_ACQUISITION;
        ch_data_ready = true;
        ch1_data_ready = true;
        ch2_data_ready = true;
        ch3_data_ready = true;
        ch4_data_ready = true;

        time_index += 1;
        time_index %= NUMBER_OF_PROCESSING_TIME_SAMPLES;
        if (channel_read_count != 4) {
            std::cout << "processing threads not keeping up: " << channel_read_count << "\n";
        }
        channel_read_count = 0;

        ul.unlock();
        data_cv.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

    }
    free(dataBuffer);
    return 0;
}
#endif

#if 0

bool runDataThread() {
    // Generate time vector
    for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
        x_samples[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_segments[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_cb_segments[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_POINTS_PER_LINE; i++) {
        x_line_samples[i] = i;
    }
    //direct copy paste of NPT
    U32 systemId = 1;
    U32 boardId = 1;

    // Get a handle to the board

    running = true;

    // Code necesary for communication with PARS-OCT cards
    //    HANDLE boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
    //    ALAZAR_BOARDTYPES boardType = AlazarGetBoardKind(boardHandle);
    //    if (boardType ==18){
    //        U32 systemId = 2;
    //        HANDLE boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
    //    }
    //    if (boardHandle == NULL)
    //    {
    //        printf("Error: Unable to open board system Id %u board Id %u\n", systemId, boardId);
    //        return;
    //    }

    // Get a handle to each board in the board system
    HANDLE boardHandleArray[2] = { NULL };
    U32 boardIndex;
    for (boardIndex = 0; boardIndex < 2; boardIndex++) {
        U32 boardId = boardIndex + 1;
        boardHandleArray[boardIndex] = AlazarGetBoardBySystemID(systemId, boardId);
        if (boardHandleArray[boardIndex] == NULL) {
            printf("Error in AlazarGetBoardBySystemID(): Unable to open board system Id %u board Id %u\n", systemId, boardId);
            return -1;
        }
    }

    // Configure the board's sample rate, input, and trigger settings
    for (boardIndex = 0; boardIndex < 2; boardIndex++) {
        if (!ConfigureBoard(boardHandleArray[boardIndex]))
        {
            std::cout << "Error: Configure board failed for board number:" << boardIndex + 1 << "with handle" << boardHandleArray[boardIndex];
            return -1;
        }
    }

    // Start acquisition loop 
    // NOTE: Passing just the first of the two boards which acts as the systemHandle
    if (!AcquireData(boardHandleArray))
    {
        printf("Error: Acquisition failed\n");
        return -1;
    }

    return 0;
}

// Alazartech board configuration
bool ConfigureBoard(HANDLE boardHandle)
{
    RETURN_CODE retCode;

    // TODO: Specify the sample rate (see sample rate id below)

    samplesPerSec = 250000000.0;

    // TODO: Select clock parameters as required to generate this sample rate.
    //
    // For example: if samplesPerSec is 100.e6 (100 MS/s), then:
    // - select clock source INTERNAL_CLOCK and sample rate SAMPLE_RATE_100MSPS
    // - select clock source FAST_EXTERNAL_CLOCK, sample rate SAMPLE_RATE_USER_DEF, and connect a
    //   100 MHz signal to the EXT CLK BNC connector.

    retCode = AlazarSetCaptureClock(boardHandle,
        INTERNAL_CLOCK,
        SAMPLE_RATE_250MSPS,
        CLOCK_EDGE_RISING,
        0);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }


    // Channel settings
    // Select channel A input parameters as required
    retCode = AlazarInputControlEx(boardHandle, CHANNEL_A, DC_COUPLING, INPUT_RANGE_PM_1_V_25, IMPEDANCE_50_OHM);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarInputControlEx failed -- %s\n", AlazarErrorToText(retCode)); return FALSE;
    }

    // Select channel A bandwidth limit as required
    retCode = AlazarSetBWLimit(boardHandle, CHANNEL_A, 0);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode)); return FALSE;
    }

    // Select channel B input parameters as required
    retCode = AlazarInputControlEx(boardHandle, CHANNEL_B, DC_COUPLING, INPUT_RANGE_PM_1_V_25, IMPEDANCE_50_OHM);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarInputControlEx failed -- %s\n", AlazarErrorToText(retCode)); return FALSE;
    }

    // Select channel B bandwidth limit as required
    retCode = AlazarSetBWLimit(boardHandle, CHANNEL_B, 0);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode)); return FALSE;
    }

    // TODO: Select trigger inputs and levels as required

    retCode = AlazarSetTriggerOperation(boardHandle,
        TRIG_ENGINE_OP_J,
        TRIG_ENGINE_J,
        TRIG_EXTERNAL,
        TRIGGER_SLOPE_POSITIVE,
        150,
        TRIG_ENGINE_K,
        TRIG_DISABLE,
        TRIGGER_SLOPE_POSITIVE,
        128);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    // TODO: Select external trigger parameters as required

    retCode = AlazarSetExternalTrigger(boardHandle,
        DC_COUPLING,
        ETR_5V);

    // TODO: Set trigger delay as required.

    double triggerDelay_sec = 0;
    U32 triggerDelay_samples = (U32)(triggerDelay_sec * samplesPerSec + 0.5);
    retCode = AlazarSetTriggerDelay(boardHandle, triggerDelay_samples);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarSetTriggerDelay failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    // TODO: Set trigger timeout as required.

    // NOTE:
    //
    // The board will wait for a for this amount of time for a trigger event. If a trigger event
    // does not arrive, then the board will automatically trigger. Set the trigger timeout value to
    // 0 to force the board to wait forever for a trigger event.
    //
    // IMPORTANT:
    //
    // The trigger timeout value should be set to zero after appropriate trigger parameters have
    // been determined, otherwise the board may trigger if the timeout interval expires before a
    // hardware trigger event arrives.

    retCode = AlazarSetTriggerTimeOut(boardHandle, 0);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    // TODO: Configure AUX I/O connector as required

    retCode = AlazarConfigureAuxIO(boardHandle, AUX_OUT_TRIGGER, 0);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarConfigureAuxIO failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    return TRUE;

}

// Alazartech acquire data run loop

bool AcquireData(HANDLE* boardHandleArray)
{
    // There are no pre-trigger samples in NPT mode
    //U32 preTriggerSamples = 0;
    preTriggerSamples = PRE_TRIGGER_SAMPLES; //JATS CHANGE: making these accessible from other functions

    // TODO: Select the number of post-trigger samples per record
    //postTriggerSamples = 256;
    postTriggerSamples = POST_TRIGGER_SAMPLES; //JATS CHANGE: making these accessible from other functions

    // TODO: Specify the number of records per DMA buffer
    //U32 recordsPerBuffer = 2000;
    recordsPerBuffer = NUMBER_OF_SEGMENTS_PER_BUFFER; //JATS CHANGE: making these accessible from other functions

    // TODO: Specify the total number of buffers to capture
    buffersPerAcquisition = NUMBER_OF_BUFFERS_PER_ACQUISITION;

    //continuousSaveComplete = false;
    //dataReady = false;

    // TODO: Select which channels to capture (A, B, or both)
    U32 channelMask = CHANNEL_A | CHANNEL_B;

    HANDLE systemHandle = boardHandleArray[0];

    // Calculate the number of enabled channels from the channel mask
    int channelCount = 0;
    int channelsPerBoard = 2;
    for (int channel = 0; channel < channelsPerBoard; channel++)
    {
        U32 channelId = 1U << channel;
        if (channelMask & channelId)
            channelCount++;
    }

    // Get the sample size in bits, and the on-board memory size in samples per channel
    U8 bitsPerSample;
    U32 maxSamplesPerChannel;
    RETURN_CODE retCode = AlazarGetChannelInfo(systemHandle, &maxSamplesPerChannel, &bitsPerSample);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    // Calculate the size of each DMA buffer in bytes
    float bytesPerSample = (float)((bitsPerSample + 7) / 8);
    U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
    U32 bytesPerRecord = (U32)(bytesPerSample * samplesPerRecord +
        0.5); // 0.5 compensates for double to integer conversion
    bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount;

    // JATS CHANGE: Add in a saveBuffer which is specified as the number of buffers per acquisition * samples per buffer

    //saveBuffer = new U16[2 * bytesPerBuffer / 2 * buffersPerAcquisition];
    //waitTimeBuffer = new double[buffersPerAcquisition * 2];

    // JATS CHANGE: Add in sendEmit flag to help GUI only receive emits when it is ready
    //bool sendEmit = false;
    //std::atomic<bool> sendEmitAtom = false;

    BOOL success = TRUE;
    // Allocate memory for DMA buffers
    U32 bufferIndex;
    U32 boardIndex;
    for (boardIndex = 0; boardIndex < 2 && success; boardIndex++) {
        for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
        {
            // Allocate page aligned memory
            BufferArray[boardIndex][bufferIndex] =
                (U16*)AlazarAllocBufferU16(boardHandleArray[boardIndex], bytesPerBuffer);
            if (BufferArray[boardIndex][bufferIndex] == NULL)
            {
                printf("Error: Alloc %u bytes failed\n", bytesPerBuffer);
                success = FALSE;
            }
        }
    }

    // Prepare each board for an AutoDMA acquisition
    for (boardIndex = 0; boardIndex < 2 && success; boardIndex++) {
        // Configure the record size
        if (success)
        {
            retCode = AlazarSetRecordSize(boardHandleArray[boardIndex], preTriggerSamples, postTriggerSamples);
            if (retCode != ApiSuccess)
            {
                printf("Error: AlazarSetRecordSize failed -- %s\n", AlazarErrorToText(retCode));
                success = FALSE;
            }
        }


        // JATS CHANGE: Checking NPT requirements
        //    U32 retValue;
        //    retCode = AlazarQueryCapability(boardHandle,CAP_MAX_NPT_PRETRIGGER_SAMPLES,0, &retValue);

        // Configure the board to make an NPT AutoDMA acquisition
        if (success)
        {
            //U32 recordsPerAcquisition = recordsPerBuffer * buffersPerAcquisition;
            U32 infiniteRecords = 0x7FFFFFFF; // Acquire until aborted or timeout. // JATS CHANGE: want infinite acquisition
            U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT;
            retCode = AlazarBeforeAsyncRead(boardHandleArray[boardIndex], channelMask, -(long)preTriggerSamples,
                samplesPerRecord, recordsPerBuffer, infiniteRecords,
                admaFlags);
            if (retCode != ApiSuccess)
            {
                printf("Error: AlazarBeforeAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
                success = FALSE;
            }
        }

        // Add the buffers to a list of buffers available to be filled by the board

        for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
        {
            U16* pBuffer = BufferArray[boardIndex][bufferIndex];
            retCode = AlazarPostAsyncBuffer(boardHandleArray[boardIndex], pBuffer, bytesPerBuffer);
            if (retCode != ApiSuccess)
            {
                printf("Error: AlazarPostAsyncBuffer %u failed -- %s\n", bufferIndex,
                    AlazarErrorToText(retCode));
                success = FALSE;
            }
        }
    }

    // Arm the board system to wait for a trigger event to begin the acquisition
    if (success)
    {
        retCode = AlazarStartCapture(systemHandle);
        if (retCode != ApiSuccess)
        {
            printf("Error: AlazarStartCapture failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
        }
    }

    // Wait for each buffer to be filled, process the buffer, and re-post it to
    // the board.
    if (success)
    {
        //JATS CHANGE: printf("Capturing %d buffers ... press any key to abort\n", buffersPerAcquisition);

        U32 startTickCount = GetTickCount();
        U32 buffersCompleted = 0;
        INT64 bytesTransferred = 0;

        while (running && gui_running)
        {
            // TODO: Set a buffer timeout that is longer than the time
            //       required to capture all the records in one buffer.
            U32 timeout_ms = 100000;

            // Wait for the buffer at the head of the list of available buffers
            // to be filled by the board.
            bufferIndex = buffersCompleted % BUFFER_COUNT;

            for (boardIndex = 0; (boardIndex < 2) && success; boardIndex++) {

                U16* pBuffer = BufferArray[boardIndex][bufferIndex];

                retCode = AlazarWaitAsyncBufferComplete(boardHandleArray[boardIndex], pBuffer, timeout_ms);

                if (retCode != ApiSuccess)
                {
                    std::cout << "Error: AlazarWaitAsyncBufferComplete failed --" << AlazarErrorToText(retCode);
                    success = FALSE;
                }

                // Update data variables
                if (success)
                {
                    std::unique_lock<std::mutex> ul(data_mutex);
                    if (boardIndex == 0) {
                        memmove(channel1_V, pBuffer, 2 * NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT);
                        memmove(channel2_V, &pBuffer[NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT], 2 * NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT);
                        ch1_data_ready = true;
                        ch2_data_ready = true;
                    }
                    else {
                        memmove(channel3_V, pBuffer, 2 * NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT);
                        memmove(channel4_V, &pBuffer[NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT], 2 * NUMBER_OF_SEGMENTS_PER_BUFFER * NUMBER_OF_SAMPLES_PER_SEGMENT);
                        ch3_data_ready = true;
                        ch4_data_ready = true;
                    }
                    ul.unlock();
                }

                // Add the buffer to the end of the list of available buffers.
                if (success)
                {
                    retCode = AlazarPostAsyncBuffer(boardHandleArray[boardIndex], pBuffer, bytesPerBuffer);
                    if (retCode != ApiSuccess)
                    {
                        std::cout << "Error: AlazarPostAsyncBuffer failed --" << AlazarErrorToText(retCode);
                        success = FALSE;
                    }
                }
            }
            // If buffer acquisition successful, notify consumers and update indexing variables
            if (success) {
                std::unique_lock<std::mutex> ul(data_mutex);
                buffer_index = (buffer_index + 1) % NUMBER_OF_BUFFERS_PER_ACQUISITION;
                time_index = (time_index + 1) % NUMBER_OF_PROCESSING_TIME_SAMPLES;
                ch_data_ready = true;

                // Error check on channel count
                if (channel_read_count != 4) {
                    std::cout << "processing threads not keeping up: " << channel_read_count << "\n";
                }
                channel_read_count = 0;

                ul.unlock();
                data_cv.notify_all();
            }

            // If the acquisition failed, exit the acquisition loop
            if (!success) {
                std::cout << "Acquisition has failed. Exiting acquisition loop...";
                break;
            }
            buffersCompleted++;
            bytesTransferred += bytesPerBuffer;

            std::unique_lock<std::mutex> ul(sig_mutex);
            sig_cv.wait(ul, []() {return true; });
            ul.unlock();

        }

        // Display results
        double transferTime_sec = (GetTickCount() - startTickCount) / 1000.;
        //printf("Capture completed in %.2lf sec\n", transferTime_sec);

        double buffersPerSec;
        double bytesPerSec;
        double recordsPerSec;
        U32 recordsTransferred = recordsPerBuffer * buffersCompleted;

        if (transferTime_sec > 0.)
        {
            buffersPerSec = buffersCompleted / transferTime_sec;
            bytesPerSec = bytesTransferred / transferTime_sec;
            recordsPerSec = recordsTransferred / transferTime_sec;
        }
        else
        {
            buffersPerSec = 0.;
            bytesPerSec = 0.;
            recordsPerSec = 0.;
        }

    }

    // Abort the acquisition
    for (boardIndex = 0; boardIndex < 2; boardIndex++) {
        retCode = AlazarAbortAsyncRead(boardHandleArray[boardIndex]);
        if (retCode != ApiSuccess)
        {
            //printf("Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
            std::cout << "Error: AlazarAbortAsyncRead failed -- " << AlazarErrorToText(retCode);
            success = FALSE;
        }
    }

    // Free all memory allocated
    for (boardIndex = 0; boardIndex < 2; boardIndex++) {
        for (bufferIndex = 0; bufferIndex < BUFFER_COUNT; bufferIndex++)
        {
            if (BufferArray[bufferIndex] != NULL)
            {
                AlazarFreeBufferU16(boardHandleArray[boardIndex], BufferArray[boardIndex][bufferIndex]);
            }
        }
    }

    //free(saveBuffer);
    std::cout << "DTh: Data acquisition run loop finished...";
    return success;
}

#endif


#if 1
bool runDataThread() {
    int32						i32Status = CS_SUCCESS;
    void* pBuffer1 = NULL;
    void* pBuffer2 = NULL;
    void* pCurrentBuffer = NULL;
    void* pWorkBuffer = NULL;
    uInt32						u32Mode;
    CSHANDLE					hSystem = 0;
    CSSYSTEMINFO				CsSysInfo = { 0 };
    CSACQUISITIONCONFIG			CsAcqCfg = { 0 };
    CSSTMCONFIG					StmConfig;
    LPCTSTR						szIniFile = _T("C:\\Users\\CAF1\\Documents\\GitHub\\pars_visualization_gui\\pars_visualization_gui\\Stream2Analysis.ini");
    BOOL						bDone = FALSE;
    BOOL						bErrorData = FALSE;
    uInt32						u32LoopCount = 0;
    uInt32						u32SaveCount = 0;
    uInt32						u32TickStart = 0;
    long long					llTotalSamples = 0;
    uInt32						u32TransferSize = 0;
    uInt32						u32DataSegmentInBytes = 0;
    uInt32						u32DataSegmentWithTailInBytes = 0;
    uInt32						u32ErrorFlag = 0;
    //int64*						pi64Sums;
    uInt32						u32ActualLength = 0;
    uInt8						u8EndOfData = 0;
    BOOL						bStreamCompletedSuccess = FALSE;
    uInt32						u32SegmentTail_Bytes = 0;

    //JATS CHANGE: Added variables
    uInt32                      u32BufferSizeWithTailInBytes = 0;
    // Generate time vector
    for (int i = 0; i < NUMBER_OF_SAMPLES_PER_SEGMENT; i++) {
        x_samples[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_segments[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_SEGBUFFERS_IN_CIRCBUFFER * NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
        x_cb_segments[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_POINTS_PER_LINE; i++) {
        x_line_samples[i] = i;
    }
    for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
        x_time_samples[i] = i;
    }
    // Get a handle to the board

    running = true;
    ////////////////////////////////////////////////////////////////////////////////////
    /// Initialize Gage Card
    ////////////////////////////////////////////////////////////////////////////////////
    // Initializes the CompuScope boards found in the system. If the
    // system is not found a message with the error code will appear.
    // Otherwise i32Status will contain the number of systems found.
    i32Status = CsInitialize();

    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        return 0;
    }

    // Get System. This sample program only supports one system. If
    // 2 systems or more are found, the first system that is found
    // will be the system that will be used. hSystem will hold a unique
    // system identifier that is used when referencing the system.
    i32Status = CsGetSystem(&hSystem, 0, 0, 0, 0);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        return 0;
    }

    // Get System information. The u32Size field must be filled in
    // prior to calling CsGetSystemInfo
    CsSysInfo.u32Size = sizeof(CSSYSTEMINFO);
    i32Status = CsGetSystemInfo(hSystem, &CsSysInfo);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        CsFreeSystem(hSystem);
        return 0;
    }

    if (CsSysInfo.u32BoardCount > 1)
    {
        _ftprintf(stdout, _T("This sample program does not support CompuScope M/S system."));
        CsFreeSystem(hSystem);
        return 0;
    }

    // Display the system name from the driver
    //_ftprintf(stdout, _T("\nBoard Name: %s"), CsSysInfo.strBoardName);

    // In this example we're only using 1 trigger source
    i32Status = CsAs_ConfigureSystem(hSystem, (int)CsSysInfo.u32ChannelCount, 1, (LPCTSTR)szIniFile, &u32Mode);

    if (CS_FAILED(i32Status))
    {
        if (CS_INVALID_FILENAME == i32Status)
        {
            // Display message but continue on using defaults.
            _ftprintf(stdout, _T("\nCannot find %s - using default parameters."), szIniFile);
        }
        else
        {
            // Otherwise the call failed.  If the call did fail we should free the CompuScope
            // system so it's available for another application
            DisplayErrorString(i32Status);
            CsFreeSystem(hSystem);
            return 0;
        }
    }

    // If the return value is greater than  1, then either the application,
    // acquisition, some of the Channel and / or some of the Trigger sections
    // were missing from the ini file and the default parameters were used.
    if (CS_USING_DEFAULT_ACQ_DATA & i32Status)
        _ftprintf(stdout, _T("\nNo ini entry for acquisition. Using defaults."));

    if (CS_USING_DEFAULT_CHANNEL_DATA & i32Status)
        _ftprintf(stdout, _T("\nNo ini entry for one or more Channels. Using defaults for missing items."));

    if (CS_USING_DEFAULT_TRIGGER_DATA & i32Status)
        _ftprintf(stdout, _T("\nNo ini entry for one or more Triggers. Using defaults for missing items."));


    // Load application specific information from the ini file
    i32Status = LoadStmConfiguration(szIniFile, &StmConfig);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        CsFreeSystem(hSystem);
        return 0;
    }
    if (CS_USING_DEFAULTS == i32Status)
    {
        _ftprintf(stdout, _T("\nNo ini entry for Stm configuration. Using defaults."));
    }

    // Streaming Configuration
    // Validate if the board supports hardware streaming  If  it is not supported,
    // we'll exit gracefully.
    i32Status = InitializeStream(hSystem);
    if (CS_FAILED(i32Status))
    {
        // Error string is displayed in InitializeStream
        CsFreeSystem(hSystem);
        return 0;
    }

    // Get user's acquisition data to use for various parameters for transfer
    CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);
    i32Status = CsGet(hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        CsFreeSystem(hSystem);
        return 0;
    }

    // For this sample program only, check for a limited segment size
    if (CsAcqCfg.i64SegmentSize < 0 || CsAcqCfg.i64Depth < 0)
    {
        _ftprintf(stderr, _T("\n\nThis sample program does not support acquisitions with infinite segment size.\n"));
        CsFreeSystem(hSystem);
        return 0;
    }

    // In Streaming, some hardware related information are placed at the end of each segment. These samples contain also timestamp\
    // information for the segemnt. The number of extra bytes may be different depending on CompuScope card model.
    i32Status = CsGet(hSystem, 0, CS_SEGMENTTAIL_SIZE_BYTES, &u32SegmentTail_Bytes);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        ExitThread(1);
    }


    // We need to allocate a buffer for transferring the data. Buffer is allocated as void with
    // a size of length * number of channels * sample size. All channels in the mode are transferred
    // within the same buffer, so the mode tells us the number of channels.
    // The buffer must be allocated by a call to CsStmAllocateBuffer.  This routine will
    // allocate a buffer suitable for streaming.  In this program we're allocating 2 streaming buffers
    // so we can transfer to one while doing analysis on the other.
    u32DataSegmentInBytes = (uInt32)(CsAcqCfg.i64SegmentSize * CsAcqCfg.u32SampleSize * (CsAcqCfg.u32Mode & CS_MASKED_MODE));

    u32DataSegmentWithTailInBytes = u32DataSegmentInBytes + u32SegmentTail_Bytes;

    // Round the buffersize to a 16 byte boundary. This is necessary for streaming.
    u32DataSegmentWithTailInBytes = u32DataSegmentWithTailInBytes / 16 * 16;

    // JATS CHANGE: Adding a u32 variable that is actual buffer size (choosing 128 as it is divisible by 16 and is big enough to avoid the card internally buffering)
    int numberOfSegmentsInBuffer = NUMBER_OF_SEGMENTS_PER_BUFFER;
    // JATS CHANGE: Rounding this segments in buffer number to 16. re: comment above about necessity for streaming.
    //numberOfSegmentsInBuffer = numberOfSegmentsInBuffer / 16 * 16;
    u32BufferSizeWithTailInBytes = u32DataSegmentWithTailInBytes * numberOfSegmentsInBuffer;


    // JATS CHANGE: This seems to represent the buffer size in units of sample (so number of int16s)
    u32TransferSize = u32BufferSizeWithTailInBytes / CsAcqCfg.u32SampleSize;

    //_ftprintf (stderr, _T("\n(Actual buffer size used for data streaming = %u Bytes)\n"), u32DataSegmentWithTailInBytes );
    std::cout << "Actual buffer size used for data streaming = " << u32BufferSizeWithTailInBytes << "(Bytes)";

    i32Status = CsStmAllocateBuffer(hSystem, 0, u32BufferSizeWithTailInBytes, &pBuffer1);
    if (CS_FAILED(i32Status))
    {
        _ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 1.\n"));
        //        free(pi64Sums);
        CsFreeSystem(hSystem);
        return 0;
    }

    i32Status = CsStmAllocateBuffer(hSystem, 0, u32BufferSizeWithTailInBytes, &pBuffer2);
    if (CS_FAILED(i32Status))
    {
        _ftprintf(stderr, _T("\nUnable to allocate memory for stream buffer 2.\n"));
        CsStmFreeBuffer(hSystem, 0, pBuffer1);
        //        free(pi64Sums);
        CsFreeSystem(hSystem);
        return 0;
    }


    // Commit the values to the driver.  This is where the values get sent to the
    // hardware.  Any invalid parameters will be caught here and an error returned.
    i32Status = CsDo(hSystem, ACTION_COMMIT);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        //        free(pi64Sums);
        CsStmFreeBuffer(hSystem, 0, pBuffer1);
        CsStmFreeBuffer(hSystem, 0, pBuffer2);
        CsFreeSystem(hSystem);
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    /// Start Gage Card Streaming
    ////////////////////////////////////////////////////////////////////////////////////

    printf("\nStart streaming. Press ESC to abort\n\n");

    // Start the data acquisition
    i32Status = CsDo(hSystem, ACTION_START);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        //        free(pi64Sums);
        CsStmFreeBuffer(hSystem, 0, pBuffer1);
        CsStmFreeBuffer(hSystem, 0, pBuffer2);
        CsFreeSystem(hSystem);
        return 0;
    }


    u32TickStart = GetTickCount();

    // Loop until either we've done the number of segments we want, or running is turned false.
    // While we loop we transfer into pCurrentBuffer and do our analysis on pWorkBuffer

    // JATS CHANGE: changes to make it work with infinite buffers
//    dataBuffer = new int16[u32DataSegmentInBytes/2*CsAcqCfg.u32SegmentCount];
    //UINT32 numberOfBuffersInSaveBuffer = BUFFERS_PER_SAVEBUFFER;
    //UINT32 saveBufferSegmentCount = numberOfSegmentsInBuffer * numberOfBuffersInSaveBuffer;
    //saveBuffer = new int16[u32DataSegmentInBytes / 2 * saveBufferSegmentCount];
    //totalBufferCount = 0;
    /*bool sendEmit = false;
    flag = false;*/

    bytesPerBuffer = numberOfSegmentsInBuffer * u32DataSegmentInBytes;
    //buffersPerAcquisition = numberOfBuffersInSaveBuffer;

    preTriggerSamples = PRE_TRIGGER_SAMPLES;
    postTriggerSamples = POST_TRIGGER_SAMPLES;

    std::atomic<bool> sendEmitAtom = false;

    bool first = true;
    auto td_start = std::chrono::high_resolution_clock::now();
    auto td_end = std::chrono::high_resolution_clock::now();



    while (!(bDone || bStreamCompletedSuccess) && running)
    {
        // Determine where new data transfer data will go. We alternate
        // between our 2 DMA buffers
        if (u32LoopCount & 1)
        {
            pCurrentBuffer = pBuffer2;
        }
        else
        {
            pCurrentBuffer = pBuffer1;
        }

        i32Status = CsStmTransferToBuffer(hSystem, 0, pCurrentBuffer, u32TransferSize);
        if (CS_FAILED(i32Status))
        {
            DisplayErrorString(i32Status);
            break;
        }

        if (NULL != pWorkBuffer)
        {
            // JATS CHANGE: saving buffer to larger data buffer which is the size of the acquisition
            // JATS CHANGE: also remove tail from each captured segment

            auto start = std::chrono::high_resolution_clock::now();

            std::unique_lock<std::mutex> ul(data_mutex);

            int16* tempBuff = (int16*)pWorkBuffer;
            //for (int i = 0; i < numberOfSegmentsInBuffer; i++) {
            //    for (int j = 0; j < u32DataSegmentInBytes / 2; j++) {
            //        tempBuff[j + i * u32DataSegmentInBytes / 2 + (numSaveBufferAtom % numberOfBuffersInSaveBuffer) * numberOfSegmentsInBuffer * u32DataSegmentInBytes / 2] = tempBuff[j + i * u32DataSegmentWithTailInBytes / 2];
            //    }
            //}

            uint64 index_offset;
            for (int i = 0; i < NUMBER_OF_SEGMENTS_PER_BUFFER; i++) {
                // could put a dereferencing section here for the first loop for two dimensional vectors to save some compute time
                index_offset = i * NUMBER_OF_SAMPLES_PER_SEGMENT * 4  +  i * u32SegmentTail_Bytes / 2;
                for (int j = 0; j < NUMBER_OF_SAMPLES_PER_SEGMENT; j++) {
                    channel1_V[i][j] = tempBuff[j * 4 + 0 + index_offset];
                    channel2_V[i][j] = tempBuff[j * 4 + 1 + index_offset];
                    channel3_V[i][j] = tempBuff[j * 4 + 2 + index_offset];
                    channel4_V[i][j] = tempBuff[j * 4 + 3 + index_offset];
                }
                index_offset = index_offset + NUMBER_OF_SAMPLES_PER_SEGMENT * 4;
                memcpy(&timestamps[i], &tempBuff[index_offset], sizeof(uint64_t));
            }
            
            ch1_data_ready = true;
            ch2_data_ready = true;
            ch3_data_ready = true;
            ch4_data_ready = true;
            ul.unlock();

            auto end = std::chrono::high_resolution_clock::now();
            data_deinterlacing_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
                avg_data_deinterlacing_time += data_deinterlacing_time[i];
            }
            avg_data_deinterlacing_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;
        }

        // notify other threads
        if (NULL != pWorkBuffer) {
            std::unique_lock<std::mutex> ul(data_mutex);
            buffer_index = (buffer_index + 1) % NUMBER_OF_BUFFERS_PER_ACQUISITION;
            time_index = (time_index + 1) % NUMBER_OF_PROCESSING_TIME_SAMPLES;
            ch_data_ready = true;

            // Error check on channel count
            //if (channel_read_count != 4) {
            //    std::cout << "processing threads not keeping up: " << channel_read_count << "\n";
            //}
            channel_read_count = 0;

            ul.unlock();
            data_cv.notify_all();
        }

        //if (NULL != pWorkBuffer) {
        //    std::unique_lock<std::mutex> ul(sig_mutex);
        //    sig_cv.wait(ul, []() {return true; });
        //    ul.unlock();
        //}

        // Wait for the DMA transfer on the current buffer to complete so we can loop
        // back around to start a new one. Calling thread will sleep until
        // the transfer completes

        i32Status = CsStmGetTransferStatus(hSystem, 0, StmConfig.u32TransferTimeout, &u32ErrorFlag, &u32ActualLength, &u8EndOfData);
        if (CS_SUCCEEDED(i32Status))
        {
            llTotalSamples += u32TransferSize;
            bStreamCompletedSuccess = (0 != u8EndOfData);
        }
        else
        {
            bDone = TRUE;
            if (CS_STM_TRANSFER_TIMEOUT == i32Status)
            {
                // Timeout on CsStmGetTransferStatus().
                // Data Stransfer has not yet completed. We can repeat calling CsStmGetTransferStatus()  until the function
                // returns the status CS_SUCCESS.
                // In this sample program, we just stop and return the error.
                DisplayErrorString(i32Status);
            }
            else /* some other error */
            {
                DisplayErrorString(i32Status);
            }
        }

        pWorkBuffer = pCurrentBuffer;

        u32LoopCount++;
        
        if (first) {
            td_start = std::chrono::high_resolution_clock::now();
            first = false;
        }
        else {
            td_end = std::chrono::high_resolution_clock::now();
            data_acquisition_time[time_index] = std::chrono::duration_cast<std::chrono::nanoseconds>(td_end - td_start).count();
            for (int i = 0; i < NUMBER_OF_PROCESSING_TIME_SAMPLES; i++) {
                avg_data_acquisition_time += data_acquisition_time[i];
            }
            avg_data_acquisition_time /= NUMBER_OF_PROCESSING_TIME_SAMPLES;

            td_start = std::chrono::high_resolution_clock::now();
        }

    }

    // Abort the current acquisition
    CsDo(hSystem, ACTION_ABORT);


    // Because the loop count above tests at the start of the loop, we may break the loop
    // while there's still data in pWorkBuffer to analyze. That would be the last data transferred
    // into pBuffer2. So need to process
//    if ( StmConfig.bDoAnalysis )
//    {
//        //TODO: Need to double check that  this is a sensible operation in infinite acquisition...
//        if ( u32SaveCount < CsAcqCfg.u32SegmentCount )
//        {
//            int16 * tempBuff = (int16*)pWorkBuffer;
//            // JATS CHANGE: note the CsAcqCfg.u32SegmentCount%numberOfSegmentsInBuffer is to handle the last buffer where it will be partially filled
//            // JATS CHANGE: note the line with just numberOfSegmentsInBuffer assumes that we have infinite acqusition mode running
//            //for( int i = 0; i < CsAcqCfg.u32SegmentCount%numberOfSegmentsInBuffer; i++){
//            for( int i = 0; i < numberOfSegmentsInBuffer; i++){
//                for(int j =0; j < u32DataSegmentInBytes/2; j++){
//                    saveBuffer[j+i*u32DataSegmentInBytes/2+(totalBufferCount%numberOfBuffersInSaveBuffer)*numberOfSegmentsInBuffer*u32DataSegmentInBytes/2] = tempBuff[j+i*u32DataSegmentWithTailInBytes/2];
//                }
//                //qDebug() << "Segment number: "<< i;
//            }
//            qDebug() << "\n" << "Buffer number: " << totalBufferCount << "\n";
//        }

//        //JATS CHANGE: GUI interaction code
//        {
//            QMutexLocker locker(&mutex);
//            totalBufferCount++;
//            sendEmit = !flag;
//            flag = true;
//        }
//        if (sendEmit){
//            emit dataReady(this);
//        }
//    }


    //////////////////////////////////////////////////////////////
    // Free system buffers / compuscope systems
    //////////////////////////////////////////////////////////////

    // Free all buffers that have been allocated. Buffers allocated with
    // CsStmAllocateBuffer must be freed with CsStmFreeBuffer
    CsStmFreeBuffer(hSystem, 0, pBuffer1);
    CsStmFreeBuffer(hSystem, 0, pBuffer2);
    // Free the CompuScope system and any resources it's been using
    i32Status = CsFreeSystem(hSystem);

    if (bErrorData)
    {
        _ftprintf(stdout, _T("\nStream aborted on error.\n"));
    }
    else
    {
        if (u32LoopCount < CsAcqCfg.u32SegmentCount)
        {
            _ftprintf(stdout, _T("\nStream aborted by user or error.\n"));
        }
        else
        {
            _ftprintf(stdout, _T("\nStream has finsihed %u loops.\n"), CsAcqCfg.u32SegmentCount);
        }
    }

    // JATS CHANGE: print out raw data list to binary file
    //FILE *myFile;
    //errno_t err = fopen_s(&myFile,"C:\\Users\\labadmin\\Documents\\GitHub\\Temp_DataOutput\\test.bin","wb");
//    fwrite(reinterpret_cast<char*>(dataBuffer),1,u32DataSegmentInBytes/2*CsAcqCfg.u32SegmentCount*2,myFile);
    //fwrite(reinterpret_cast<char*>(dataBuffer),1,u32DataSegmentInBytes/2*dataBufferSegmentCount*2,myFile);
    //err = fclose(myFile);
    //free(pi64Sums);

    return 0;

}

int32 InitializeStream(CSHANDLE hSystem)
{
    int32	i32Status = CS_SUCCESS;
    int64	i64ExtendedOptions = 0;

    CSACQUISITIONCONFIG CsAcqCfg = { 0 };

    CsAcqCfg.u32Size = sizeof(CSACQUISITIONCONFIG);

    // Get user's acquisition Data
    i32Status = CsGet(hSystem, CS_ACQUISITION, CS_CURRENT_CONFIGURATION, &CsAcqCfg);
    if (CS_FAILED(i32Status))
    {
        DisplayErrorString(i32Status);
        return (i32Status);
    }

    // Check if selected system supports Expert Stream
    // And set the correct image to be used.
    CsGet(hSystem, CS_PARAMS, CS_EXTENDED_BOARD_OPTIONS, &i64ExtendedOptions);

    if (i64ExtendedOptions & CS_BBOPTIONS_STREAM)
    {
        _ftprintf(stdout, _T("\nSelecting Expert Stream from image 1."));
        CsAcqCfg.u32Mode |= CS_MODE_USER1;
    }
    else if ((i64ExtendedOptions >> 32) & CS_BBOPTIONS_STREAM)
    {
        _ftprintf(stdout, _T("\nSelecting Expert Stream from image 2."));
        CsAcqCfg.u32Mode |= CS_MODE_USER2;
    }
    else
    {
        _ftprintf(stdout, _T("\nCurrent system does not support Expert Stream."));
        _ftprintf(stdout, _T("\nApplication terminated."));
        return CS_MISC_ERROR;
    }

    // Sets the Acquisition values down the driver, without any validation,
    // for the Commit step which will validate system configuration.
    i32Status = CsSet(hSystem, CS_ACQUISITION, &CsAcqCfg);
    if (CS_FAILED(i32Status))
    {
        //DisplayErrorString(i32Status);
        return CS_MISC_ERROR;
    }

    return CS_SUCCESS; // Success
}


int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig)
{
    TCHAR	szDefault[MAX_PATH];
    TCHAR	szString[MAX_PATH];
    TCHAR	szFilePath[MAX_PATH];
    int		nDummy;

    CSSTMCONFIG CsStmCfg;

    // Set defaults in case we can't read the ini file
    CsStmCfg.u32TransferTimeout = TRANSFER_TIMEOUT;
    //strcpy((char*)CsStmCfg.strResultFile, (char*)_T(OUT_FILE));

    if (NULL == pConfig)
    {
        return (CS_INVALID_PARAMETER);
    }

    GetFullPathName(szIniFile, MAX_PATH, szFilePath, NULL);

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(szFilePath))
    {
        *pConfig = CsStmCfg;
        return (CS_USING_DEFAULTS);
    }

    if (0 == GetPrivateProfileSection(STM_SECTION, szString, 100, szFilePath))
    {
        *pConfig = CsStmCfg;
        return (CS_USING_DEFAULTS);
    }

    nDummy = 0;
    CsStmCfg.bDoAnalysis = (0 != GetPrivateProfileInt(STM_SECTION, _T("DoAnalysis"), nDummy, szFilePath));

    nDummy = CsStmCfg.u32TransferTimeout;
    CsStmCfg.u32TransferTimeout = GetPrivateProfileInt(STM_SECTION, _T("TimeoutOnTransfer"), nDummy, szFilePath);

    _stprintf(szDefault, _T("%s"), CsStmCfg.strResultFile);
    GetPrivateProfileString(STM_SECTION, _T("ResultsFile"), szDefault, szString, MAX_PATH, szFilePath);
    _tcscpy(CsStmCfg.strResultFile, szString);

    *pConfig = CsStmCfg;
    return (CS_SUCCESS);
}

#endif
