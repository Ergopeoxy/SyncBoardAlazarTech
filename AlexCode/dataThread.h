#pragma once
#include "parsguiconfig.h"

// Standard library includes
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <iostream>

// Alazartech library includes
#if 0
#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"
#endif

#if 1
#include "CsPrototypes.h"
#include "CsExpert.h"
#include "CsTchar.h"
#include "CsAppSupport.h"
#endif

extern std::mutex sig_mutex;
extern std::condition_variable sig_cv;

// Gage card variables
#if 1
typedef struct
{
    uInt32		u32TransferTimeout;
    TCHAR		strResultFile[MAX_PATH];
    BOOL		bDoAnalysis;			/* Turn on or off data analysis */
}CSSTMCONFIG, * PCSSTMCONFIG;

#define TRANSFER_TIMEOUT	10000
#define STM_SECTION _T("StmConfig")				/* section name in ini file */
#define MAX_SEGMENTS_COUNT	100000000			// Max number of segments in a single file


#endif

// Data Thread
bool runDataThread();

// Alazartech dual card functions
#if 0
bool ConfigureBoard(HANDLE boardHandle);
bool AcquireData(HANDLE* boardHandleArray);
//double InputRangeIdToVolts(U32 inputRangeId);
#endif

// Gage Card Functions
#if 1 
int32 InitializeStream(CSHANDLE hSystem);
int32 LoadStmConfiguration(LPCTSTR szIniFile, PCSSTMCONFIG pConfig);

int64 SumBufferData(void* pBuffer, uInt32 u32Size, uInt32 u32SampleBits);
void UpdateProgress(DWORD dwTickStart, uInt32 u32UpdateInterval, unsigned long long llSamples);
void SaveResults(int16* saveData);

#endif