#include "stdafx.h"

#include "Waveform.h"
#include "Playback.h"

#include "windows.h"
#include "mmsystem.h"
#pragma comment(lib, "Winmm.lib")

static HWAVEOUT hwav = NULL;
static WAVEFORMATEX wavFmt;

#define BUFFER_COUNT 2
static WAVEHDR hdrs[BUFFER_COUNT];
static int bufferIndex = 0;

static struct PlaybackInfo
{
	Waveform* wf;
	bool stopRequest;
	bool playing;
	long playbackBytePosition;
	long bufferedBytePosition;
	int alignBytesPerSecond;
	HWND hwnd;
} pi;

bool IsPlaying() 
{
	return pi.playing;
}

double GetPlayingSecondPosition()
{
	if (!pi.playing || pi.wf == NULL)
	{
		return 0;
	}
	else
	{
		return (double)pi.playbackBytePosition * pi.wf->format.bytesPerSecond;
	}
}

void PlaybackStop(bool forceStop)
{
	if (hwav == NULL) return;

	MMRESULT res;
	TCHAR msg[256];

	if (forceStop)
	{
		pi.stopRequest = true;
		waveOutReset(hwav);
	}
	
	pi.playing = false;

	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		if (hdrs[i].dwFlags & WHDR_DONE)
		{
			res = ::waveOutUnprepareHeader(hwav, hdrs + i, sizeof(WAVEHDR));
			if (res != MMSYSERR_NOERROR)
			{
				waveInGetErrorText(res, msg, 256);
				MessageBox(NULL, msg, NULL, MB_OK);
			}
		}
	}

	if (pi.hwnd != NULL)
	{
		SendMessage(pi.hwnd, WUM_PLAYSTOP, NULL, NULL);
	}
}

void PlaybackClose()
{
	if (hwav != NULL)
	{
		if (pi.playing)
		{
			PlaybackStop(true);
		}

		MMRESULT res;
		TCHAR msg[256];

		res = ::waveOutClose(hwav);
		if (res != MMSYSERR_NOERROR)
		{
			waveInGetErrorText(res, msg, 256);
			MessageBox(NULL, msg, NULL, MB_OK);
		}

		hwav = NULL;
		pi.hwnd = NULL;
		pi.wf = NULL;
	}
}

bool WriteBuffer()
{
	if (pi.wf == NULL || pi.stopRequest) return false;

	MMRESULT res;
	TCHAR msg[256];

	Waveform& wf = *pi.wf;

	if (pi.bufferedBytePosition >= wf.bufferLength)
	{
		res = ::waveOutUnprepareHeader(hwav, hdrs + bufferIndex, sizeof(WAVEHDR));
		if (res != MMSYSERR_NOERROR)
		{
			waveInGetErrorText(res, msg, 256);
			MessageBox(NULL, msg, NULL, MB_OK);
		}

		for (int i = 0; i < BUFFER_COUNT; i++)
		{
			if (!(hdrs[i].dwFlags & WHDR_DONE))
			{
				return false;
			}
		}

		PlaybackStop(false);
		
		return false;
	}

	WAVEHDR& hdr = hdrs[bufferIndex];
	ZeroMemory(&hdr, sizeof(WAVEHDR));

	if (pi.bufferedBytePosition == 0)
	{
		hdr.dwFlags = WHDR_BEGINLOOP;
	}

	int blockSize = pi.alignBytesPerSecond;

	if (pi.bufferedBytePosition + blockSize >= wf.bufferLength)
	{
		blockSize = wf.bufferLength - pi.bufferedBytePosition;
	}
	
	if (pi.bufferedBytePosition + blockSize * 2 >= wf.bufferLength)
	{
		hdr.dwFlags = WHDR_ENDLOOP;
	}

	hdr.dwBufferLength = blockSize;
	hdr.lpData = (wf.buffer + pi.bufferedBytePosition);

	res = ::waveOutPrepareHeader(hwav, &hdr, sizeof(WAVEHDR));
	if (res != MMSYSERR_NOERROR)
	{
		waveInGetErrorText(res, msg, 256);
		MessageBox(NULL, msg, NULL, MB_OK);
		return false;
	}

	pi.bufferedBytePosition += blockSize;

	res = ::waveOutWrite(hwav, &hdr, sizeof(WAVEHDR));
	if (res != MMSYSERR_NOERROR)
	{
		waveInGetErrorText(res, msg, 256);
		MessageBox(NULL, msg, NULL, MB_OK);
		return false;
	}

	bufferIndex++;

	if (bufferIndex >= BUFFER_COUNT)
	{
		bufferIndex = 0;
	}

	return true;
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WOM_DONE:
		if (pi.hwnd != NULL)
		{
			//PostMessage(pi.hwnd, WUM_PLAYING,
			//	(double)(pi.playbackBytePosition / pi.wf->format.bytesPerSecond), NULL);
		}

		WriteBuffer();
		break;
	}
}


void Playback(HWND hwnd, Waveform& wf, double startSecond)
{
 	if (wf.buffer == NULL) return;

	MMRESULT res;
	TCHAR msg[256];

	if (hwav == NULL
		|| (wavFmt.nChannels != wf.format.channels
		|| wavFmt.nAvgBytesPerSec != wf.format.bytesPerSecond
		|| wavFmt.nBlockAlign != wf.format.blockAlign))
	{
		if (hwav != NULL)
		{
			PlaybackClose();
		}

		wavFmt.wFormatTag = WAVE_FORMAT_PCM;
		wavFmt.nChannels = wf.format.channels;
		wavFmt.nSamplesPerSec = wf.format.samplesPerSecond;
		wavFmt.wBitsPerSample = wf.format.bitsPerSample;
		wavFmt.nBlockAlign = wf.format.blockAlign;
		wavFmt.nAvgBytesPerSec = wf.format.bytesPerSecond;
		wavFmt.cbSize = 0;

		res = ::waveOutOpen(&hwav, WAVE_MAPPER, &wavFmt, (DWORD_PTR)&waveOutProc, NULL, CALLBACK_FUNCTION);

		if (res != MMSYSERR_NOERROR)
		{
			waveInGetErrorText(res, msg, 256);
			MessageBox(hwnd, msg, NULL, MB_OK);
			return;
		}
	}

	pi.wf = &wf;
	pi.hwnd = hwnd;
	pi.stopRequest = false;

	int startBytes = (int)(startSecond * wf.format.bytesPerSecond);
	startBytes -= startBytes % wf.format.blockAlign;

	pi.bufferedBytePosition = startBytes;
	pi.alignBytesPerSecond = wf.format.bytesPerSecond - (wf.format.bytesPerSecond % wf.format.blockAlign);
	
	if (WriteBuffer())
	{
		pi.playing = true;

		if (pi.hwnd != NULL)
		{
			SendMessage(pi.hwnd, WUM_PLAYSTART, NULL, NULL);
		}	
		
		if (pi.bufferedBytePosition < wf.bufferLength)
		{
			WriteBuffer();
		}
	}
}