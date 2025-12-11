#pragma once
#include "ProjectSetting.h"
#include "IoBuffer.h" // <-- FIX: Include the new header

/**
 * @class AlazarDigitizer
 * @brief Manages all API calls for a single AlazarTech board.
 */
class AlazarDigitizer
{
public:
    AlazarDigitizer(HANDLE handle, U32 systemId, U32 boardId);
    ~AlazarDigitizer();

    bool QueryBoardInfo();
    std::string GetInfoString();
    bool ConfigureBoard(const BoardConfig& config);
    bool AllocateBuffers(U32 bytesPerBuffer);
    bool FreeBuffers();
    bool PrepareForAcquisition(const AcquisitionConfig& config, U32 channelMask);
    bool PostBuffer(U32 bufferIndex);
    bool StartCapture();
    bool WaitFordBuffer(U32 bufferIndex, U32 timeoutMS);
    void AbortAcquisition();

    // --- Getters ---
    HANDLE GetHandle() const { return m_handle; }
    U32 GetBoardId() const { return m_boardId; }
    IO_BUFFER* GetBuffer(U32 bufferIndex) { return m_buffers[bufferIndex]; } // <-- FIX: Return IO_BUFFER*
    U32 GetBytesPerBuffer() const { return m_bytesPerBuffer; }

private:
    HANDLE m_handle;
    U32    m_systemId;
    U32    m_boardId;
    U32    m_boardType;
    U8     m_bitsPerSample;
    U32    m_bytesPerSample;

    // --- FIX: Store pointers to IO_BUFFER structs ---
    std::vector<IO_BUFFER*> m_buffers;
    U32 m_bytesPerBuffer;
};