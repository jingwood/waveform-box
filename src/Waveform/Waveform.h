
#define MAX_SUPPORT_CHANNELS 6

#ifndef __WAVEFORM_H__
#define __WAVEFORM_H__

#include "basetype.h"

struct WaveFormat
{
	ushort channels;
	uint samplesPerSecond;
	uint bytesPerSecond;
	ushort blockAlign;
	ushort bitsPerSample;
	ushort bytesPerSample;
};

struct CalcInfo
{
	long long int sampleMaxValue;
};

struct Waveform
{
	WaveFormat format;
	double secondLength;
	uint bufferLength;
	char* buffer;
	CalcInfo calcInfo;
};

Waveform* CreateWave(ushort channels = 1, uint samplePerSecond = 8000, 
	ushort bitsPerSample = 8, double seconds = 3);

Waveform* WaveOpen(LPCTSTR filepath = NULL);
void SaveWave(Waveform& wf, LPCTSTR filepath);
void DestroyWave(Waveform* wf);

typedef enum 
{
	Set,
	Add,
	Sub,
} GenerateFlag;

typedef struct
{
	uint freq;
	int volume;
	GenerateFlag flags;
	int phase;
	int seg;
} GenWaveParams;

void GenerateWave(Waveform& wf, GenWaveParams& gen);


typedef struct
{
	int volumePercent;
	double fadeInSeconds;
	double fadeOutSeconds;
	bool applyToSelection;
} AdjustVolume;

void WaveVolume(Waveform& wf, AdjustVolume& vol, double startSecond = -1, double endSecond = -1);


typedef struct _WaveCursor
{
	int layer;
	double cursorSeconds;

	_WaveCursor(int layer = 0, double cursorSeconds = 0.0) : layer(layer), cursorSeconds(cursorSeconds) { }
} WaveCursor;

#endif /* __WAVEFORM_H__ */