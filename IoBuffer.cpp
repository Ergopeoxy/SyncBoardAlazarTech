// IoBuffer.cpp : Manage IO_BUFFER structures
// (This is the file you provided)

#include <stdio.h>
#include "AlazarApi.h"
#include "IoBuffer.h"

//----------------------------------------------------------------------------
//
// Function    :  CreateIoBuffer
//
// Description :  Allocate and initialize an IO_BUFFER structure
//
//----------------------------------------------------------------------------
IO_BUFFER* CreateIoBuffer(U32 uBufferLength_bytes)
{
	BOOL bSuccess = FALSE;

	// allocate an IO_BUFFER structure
	IO_BUFFER* pIoBuffer = (IO_BUFFER*)malloc(sizeof(IO_BUFFER));
	if (pIoBuffer == NULL)
	{
		printf("Error: Alloc IO_BUFFER failed\n");
		return NULL;
	}
	memset(pIoBuffer, 0, sizeof(IO_BUFFER));

	// allocate a buffer for sample data
	pIoBuffer->pBuffer =
		VirtualAlloc(
			NULL,
			uBufferLength_bytes,
			MEM_COMMIT,
			PAGE_READWRITE
		);

	if (pIoBuffer->pBuffer == NULL)
	{
		printf("Error: Alloc %lu bytes failed\n", uBufferLength_bytes);
		goto error;
	}
	pIoBuffer->uBufferLength_bytes = uBufferLength_bytes;

	// create an event
	pIoBuffer->hEvent =
		CreateEvent(
			NULL,	// lpEventAttributes
			TRUE,	// bManualReset
			FALSE,	// bInitialState
			NULL	// lpName
		);
	if (pIoBuffer->hEvent == NULL)
	{
		printf("Error: CreateEvent failed -- %u\n", GetLastError());
		goto error;
	}
	pIoBuffer->overlapped.hEvent = pIoBuffer->hEvent;

	bSuccess = TRUE;

error:
	if (!bSuccess)
	{
		DestroyIoBuffer(pIoBuffer);
		pIoBuffer = NULL;
	}
	return pIoBuffer;
}

//----------------------------------------------------------------------------
//
// Function    :  DestroyIoBuffer
//
// Description :  Release an IO_BUFFER's resources
//
//----------------------------------------------------------------------------
BOOL DestroyIoBuffer(IO_BUFFER* pIoBuffer)
{
	if (pIoBuffer != NULL)
	{
		if (pIoBuffer->bPending)
		{
			printf("Error: Buffer is in use.\n");
			return FALSE;
		}
		else
		{
			if (pIoBuffer->hEvent != NULL)
				CloseHandle(pIoBuffer->hEvent);

			if (pIoBuffer->pBuffer != NULL)
			{
				VirtualFree(
					pIoBuffer->pBuffer,
					0,
					MEM_RELEASE
				);
			}
			free(pIoBuffer);
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//
// Function    :  ResetIoBuffer
//
// Description :  Initialize an IO_BUFFER for a data transfer
//
//----------------------------------------------------------------------------
BOOL ResetIoBuffer(IO_BUFFER* pIoBuffer)
{
	if (pIoBuffer == NULL)
	{
		printf("Error: NULL IoBuffer\n");
		return FALSE;
	}

	if (!ResetEvent(pIoBuffer->hEvent))
	{
		DWORD dwErrorCode = GetLastError();
		printf("Error: ResetEvent failed -- %u\n", GetLastError());
		return FALSE;
	}

	pIoBuffer->bPending = FALSE;
	pIoBuffer->overlapped.Internal = 0;
	pIoBuffer->overlapped.InternalHigh = 0;
	pIoBuffer->overlapped.Offset = 0;
	pIoBuffer->overlapped.OffsetHigh = 0;
	return TRUE;
}