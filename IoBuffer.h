// IoBuffer.h
//
// This header file defines the IO_BUFFER structure and the functions
// that manage it, as used by the official AlazarTech samples.
//
#pragma once

#include <Windows.h> // For HANDLE, OVERLAPPED, BOOL, U32, etc.
#include "AlazarApi.h" // For U32

typedef struct _IO_BUFFER
{
    void* pBuffer;              // Data buffer allocated by VirtualAlloc
    U32     uBufferLength_bytes;  // Size of the buffer in bytes
    HANDLE  hEvent;               // Event for WaitForSingleObject
    OVERLAPPED overlapped;        // Overlapped structure for async I/O
    BOOL    bPending;             // Is this buffer in use?
} IO_BUFFER;

// --- Function Declarations ---
IO_BUFFER* CreateIoBuffer(U32 uBufferLength_bytes);
BOOL DestroyIoBuffer(IO_BUFFER* pIoBuffer);
BOOL ResetIoBuffer(IO_BUFFER* pIoBuffer);