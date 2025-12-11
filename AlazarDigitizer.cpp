#include "AlazarDigitizer.h"
#include <stdio.h>
#include <sstream>
#include "IoBuffer.h" // Include new header
#include <iostream>

// Constructor
AlazarDigitizer::AlazarDigitizer(HANDLE handle, U32 systemId, U32 boardId) :
    m_handle(handle),
    m_systemId(systemId),
    m_boardId(boardId),
    m_boardType(0),
    m_bitsPerSample(0),
    m_bytesPerSample(0),
    m_bytesPerBuffer(0)
{
    m_buffers.resize(BUFFER_COUNT, NULL);
}

// Destructor
AlazarDigitizer::~AlazarDigitizer()
{
    FreeBuffers();
}

// --- QueryBoardInfo, GetInfoString, ConfigureBoard are UNCHANGED ---
bool AlazarDigitizer::QueryBoardInfo()
{
    m_boardType = AlazarGetBoardKind(m_handle);
    if (m_boardType == ATS_NONE || m_boardType == 0) return false;
    U32 memSize;
    RETURN_CODE retCode = AlazarGetChannelInfo(m_handle, &memSize, &m_bitsPerSample);
    if (retCode != ApiSuccess) return false;
    m_bytesPerSample = (m_bitsPerSample + 7) / 8;
    return true;
}

std::string AlazarDigitizer::GetInfoString()
{
    std::stringstream ss;
    ss << "  System ID: " << m_systemId << ", Board ID: " << m_boardId << "\n";
    ss << "    Board Type:    ATS" << m_boardType << "\n";
    ss << "    Bits/Sample:   " << (int)m_bitsPerSample << " (stored in " << m_bytesPerSample * 8 << " bits)\n";
    return ss.str();
}



bool AlazarDigitizer::ConfigureBoard(const BoardConfig& config)
{
    RETURN_CODE retCode;

    // --- 1. Configure Clock (Dynamic) ---
    U32 clockSource = INTERNAL_CLOCK;

    // Check if the user selected "External" (User Defined)
    if (config.sampleRateId == SAMPLE_RATE_USER_DEF)
    {
        // Select the correct external mode based on frequency
        if (config.sampleRateHz < 1000000.0) {
            clockSource = SLOW_EXTERNAL_CLOCK; // For < 1 MHz (like your 50kHz test)
            std::cout << "this is a slow external clock";

        }
        else {
            clockSource = FAST_EXTERNAL_CLOCK; // For > 1 MHz (like your 25MHz setup)
            std::cout << "this is a fast external clock";
        }
    }

    retCode = AlazarSetCaptureClock(m_handle,
        clockSource,           // Auto-selected source
        config.sampleRateId,   // ID (UserDef or specific rate)
        CLOCK_EDGE_RISING,
        0);

    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
        return false;
    }

    // --- 2. Configure Inputs ---
    U32 channels[] = { CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D };
    for (int i = 0; i < 4; i++) {
        retCode = AlazarInputControlEx(m_handle, channels[i], config.couplingId, config.inputRangeId, config.impedanceId);
        if (retCode != ApiSuccess) {
            printf("Error: AlazarInputControlEx (Ch %u) failed -- %s\n", i + 1, AlazarErrorToText(retCode));
            return false;
        }
    }

    // --- 3. Configure Trigger ---
    retCode = AlazarSetTriggerOperation(m_handle,
        TRIG_ENGINE_OP_J, TRIG_ENGINE_J,
        config.triggerSourceId, config.triggerSlopeId, config.triggerLevelCode,
        TRIG_ENGINE_K, TRIG_DISABLE,
        TRIGGER_SLOPE_POSITIVE, 128);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
        return false;
    }

    // --- 4. Configure External Trigger Input (CRITICAL FOR SYNCBOARD) ---
    // This was missing from your snippet. Even if you don't use it, it's good practice.
    // It configures the electrical properties of the TRIG IN connector.
    retCode = AlazarSetExternalTrigger(m_handle, DC_COUPLING, ETR_TTL);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetExternalTrigger failed -- %s\n", AlazarErrorToText(retCode));
        return false;
    }

    // --- 5. Timeouts ---
    U32 timeout_ticks = (U32)(config.triggerTimeoutMS / 10e-6);
    retCode = AlazarSetTriggerTimeOut(m_handle, timeout_ticks);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
        return false;
    }

    // --- 6. Configure AUX I/O (Optional but recommended) ---
    // Sets the AUX connector to output a pulse when the board triggers.
    // Useful for debugging with an oscilloscope.
    AlazarConfigureAuxIO(m_handle, AUX_OUT_TRIGGER, 0);

    return true;
}

//bool AlazarDigitizer::ConfigureBoard(const BoardConfig& config)
//{
//    RETURN_CODE retCode;
//    //retCode = AlazarSetCaptureClock(m_handle, INTERNAL_CLOCK, config.sampleRateId, CLOCK_EDGE_RISING, 0);
//    retCode = AlazarSetCaptureClock(m_handle, FAST_EXTERNAL_CLOCK, config.sampleRateId, CLOCK_EDGE_RISING, 0);
//    if (retCode != ApiSuccess) {
//        printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
//        return false;
//    }
//    U32 channels[] = { CHANNEL_A, CHANNEL_B, CHANNEL_C, CHANNEL_D };
//    for (int i = 0; i < 4; i++) {
//        retCode = AlazarInputControlEx(m_handle, channels[i], config.couplingId, config.inputRangeId, config.impedanceId);
//        if (retCode != ApiSuccess) {
//            printf("Error: AlazarInputControlEx (Ch %u) failed -- %s\n", i + 1, AlazarErrorToText(retCode));
//            return false;
//        }
//    }
//    retCode = AlazarSetTriggerOperation(m_handle,
//        TRIG_ENGINE_OP_J, TRIG_ENGINE_J,
//        config.triggerSourceId, config.triggerSlopeId, config.triggerLevelCode,
//        TRIG_ENGINE_K, TRIG_DISABLE,
//        TRIGGER_SLOPE_POSITIVE, 128);
//    if (retCode != ApiSuccess) {
//        printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
//        return false;
//    }
//    U32 timeout_ticks = (U32)(config.triggerTimeoutMS / 10e-6);
//    retCode = AlazarSetTriggerTimeOut(m_handle, timeout_ticks);
//    if (retCode != ApiSuccess) {
//        printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
//        return false;
//    }
//    return true;
//}
// --- End of UNCHANGED section ---


// --- FIX: Replaced AlazarAllocBufferU16 with CreateIoBuffer ---
bool AlazarDigitizer::AllocateBuffers(U32 bytesPerBuffer)
{
    m_bytesPerBuffer = bytesPerBuffer;
    for (U32 i = 0; i < BUFFER_COUNT; i++)
    {
        m_buffers[i] = CreateIoBuffer(m_bytesPerBuffer); // <-- CHANGED
        if (m_buffers[i] == NULL) {
            // CreateIoBuffer prints its own error
            return false;
        }
    }
    printf("Board %u: Allocated %d buffers of %u bytes each (using VirtualAlloc).\n", m_boardId, BUFFER_COUNT, bytesPerBuffer);
    return true;
}

// --- FIX: Replaced AlazarFreeBufferU16 with DestroyIoBuffer ---
bool AlazarDigitizer::FreeBuffers()
{
    for (U32 i = 0; i < BUFFER_COUNT; i++)
    {
        if (m_buffers[i] != NULL) {
            DestroyIoBuffer(m_buffers[i]); // <-- CHANGED
            m_buffers[i] = NULL;
        }
    }
    return true;
}

// --- PrepareForAcquisition is UNCHANGED from our previous version ---
// (It correctly calculates sizes and passes flags)
bool AlazarDigitizer::PrepareForAcquisition(const AcquisitionConfig& config, U32 channelMask)
{
    U32 admaFlags = config.admaMode | ADMA_EXTERNAL_STARTCAPTURE | ADMA_INTERLEAVE_SAMPLES;

    U32 recordsPerBuffer = config.recordsPerBuffer;
    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING)
        recordsPerBuffer = 1;

    U32 totalRecords = config.buffersPerAcquisition * recordsPerBuffer;
    if (config.admaMode == ADMA_CONTINUOUS_MODE || config.admaMode == ADMA_TRIGGERED_STREAMING)
        totalRecords = 0x7FFFFFFF;

    RETURN_CODE retCode = AlazarBeforeAsyncRead(m_handle,
        channelMask,
        0,
        config.samplesPerRecord,
        recordsPerBuffer,
        totalRecords,
        admaFlags);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarBeforeAsyncRead (Board %u) failed -- %s\n", m_boardId, AlazarErrorToText(retCode));
        return false;
    }
    return true;
}

// --- FIX: Replaced AlazarPostAsyncBuffer with AlazarAsyncRead ---
bool AlazarDigitizer::PostBuffer(U32 bufferIndex)
{
    IO_BUFFER* pIoBuffer = m_buffers[bufferIndex];

    if (!ResetIoBuffer(pIoBuffer)) {
        printf("Error: ResetIoBuffer (Board %u) failed\n", m_boardId);
        return false;
    }

    // Post the buffer using AlazarAsyncRead
    RETURN_CODE retCode = AlazarAsyncRead(m_handle,
        pIoBuffer->pBuffer,
        pIoBuffer->uBufferLength_bytes,
        &pIoBuffer->overlapped);

    if (retCode != ApiDmaPending) {
        printf("Error: AlazarAsyncRead (Board %u) failed -- %s\n", m_boardId, AlazarErrorToText(retCode));
        return false;
    }

    pIoBuffer->bPending = TRUE; // Mark as in-use
    return true;
}

// --- StartCapture is UNCHANGED ---
bool AlazarDigitizer::StartCapture()
{
    RETURN_CODE retCode = AlazarStartCapture(m_handle);
    if (retCode != ApiSuccess) {
        printf("Error: AlazarStartCapture (Board %u) failed -- %s\n", m_boardId, AlazarErrorToText(retCode));
        return false;
    }
    return true;
}


// --- FIX: Replaced AlazarWaitAsyncBufferComplete with WaitForSingleObject ---
bool AlazarDigitizer::WaitFordBuffer(U32 bufferIndex, U32 timeoutMS)
{
    IO_BUFFER* pIoBuffer = m_buffers[bufferIndex];

    DWORD waitResult = WaitForSingleObject(pIoBuffer->hEvent, timeoutMS);

    if (waitResult != WAIT_OBJECT_0) {
        if (waitResult == WAIT_TIMEOUT) {
            printf("\nError: WaitForSingleObject timeout (Board %u). No trigger?\n", m_boardId);
        }
        else {
            printf("\nError: WaitForSingleObject (Board %u) failed. Code: %lu\n", m_boardId, GetLastError());
        }
        return false;
    }

    pIoBuffer->bPending = FALSE; // No longer pending

    DWORD bytesTransferred;
    BOOL success = GetOverlappedResult(m_handle, &pIoBuffer->overlapped, &bytesTransferred, FALSE);

    if (!success) {
        U32 error = GetLastError();
        if (error != ERROR_OPERATION_ABORTED) {
            printf("\nError: GetOverlappedResult (Board %u) failed. Code: %u\n", m_boardId, error);
        }
        // ERROR_OPERATION_ABORTED is OK, it means we called AbortAcquisition
        return false;
    }

    return true;
}

// --- AbortAcquisition is UNCHANGED ---
void AlazarDigitizer::AbortAcquisition()
{
    AlazarAbortAsyncRead(m_handle);
}