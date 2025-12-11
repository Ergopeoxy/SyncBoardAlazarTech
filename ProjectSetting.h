#pragma once

// --- AlazarTech API Headers ---
#include "AlazarApi.h"
#include "AlazarCmd.h"
#include "AlazarError.h"

// --- Standard Headers ---
#include <string>
#include <vector>

// =======================================================
// --- GLOBAL DEFINES & CONFIGURATION ---
// =======================================================

// --- DMA Settings ---
#define BUFFER_COUNT 8
#define MAX_BOARDS 8

// --- FIX: Moved settings here from main.cpp ---
// SAMPLES_PER_RECORD *always* means samples *per channel*
#define SAMPLES_PER_RECORD_NPT_TR 384 
// Use the large 8MB buffer size (1M samples/ch) from the official sample.
// This will now work because we are using VirtualAlloc.
//#define SAMPLES_PER_RECORD_STREAMING 1024000 
// To this (a much smaller buffer size):
#define SAMPLES_PER_RECORD_STREAMING (64 * 1024) // 65,536 samples

#define RECORDS_PER_BUFFER 200
#define BUFFERS_PER_ACQUISITION 50

// --- Board Config ---
struct BoardConfig
{
    U32    sampleRateId;
    double sampleRateHz;
    U32    inputRangeId;
    double inputRangeVolts;
    U32    couplingId;
    U32    impedanceId;
    U32    triggerSourceId;
    U32    triggerSlopeId;
    U8     triggerLevelCode;
    U32    triggerTimeoutMS;
};

// --- Acquisition Config ---
struct AcquisitionConfig
{
    U32    admaMode;
    U32    samplesPerRecord;
    U32    recordsPerBuffer;
    U32    buffersPerAcquisition;
    bool   saveData;
    bool   processData;
    bool isSyncTest = false; // New flag
    // NEW: How often to print logs (e.g., every 1000 buffers)
    U32 logInterval = 1000;
};