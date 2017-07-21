#include "stdafx.h"

#include <stdio.h>
#include <math.h>

#include "Waveform.h"

#define PI 3.14159265
#define WAVE_FORMAT_PCM 1

typedef struct _wavFmt
{
	ushort audioFormat;
	ushort channels;
	uint samplePerSecond;
	uint bytesPerSecond;
	ushort blockAlign;
	ushort bitsPerSample;
} WaveFileFormat;

// Wave File Define

typedef struct _chunk
{
	char id[4];
	uint size;
} ChunkHead;

typedef struct _riffChunk
{
	ChunkHead head;
	char format[4];
} RiffChunk;

typedef struct _wavFmtChunk
{
	ChunkHead chunk;
	WaveFileFormat format;
} WaveFormatChunk;

typedef ChunkHead WaveDataChunkHead;

// End of Wave File Define

void MakeWaveformInfo(Waveform& wf)
{
	wf.format.bytesPerSample = wf.format.bitsPerSample / 8;
	wf.calcInfo.sampleMaxValue = (long long)pow(2.0, wf.format.bitsPerSample);
}

Waveform* WaveOpen(LPCTSTR filepath)
{
	if (filepath == NULL)
	{
		return NULL;
	}

	FILE* file = NULL;
	_tfopen_s(&file, filepath, _T("rb"));

	RiffChunk riff;
	fread(&riff, sizeof(RiffChunk), 1, file);

	if (strncmp(riff.head.id, "RIFF", 4) != 0)
	{
		fclose(file);
		return NULL;
	}

	if (strncmp(riff.format, "WAVE", 4) != 0)
	{
		fclose(file);
		return NULL;
	}

	Waveform* waveform = new Waveform();
	ChunkHead chunk;

	while (!feof(file))
	{
		ZeroMemory(&chunk, sizeof(ChunkHead));
		fread(&chunk, sizeof(ChunkHead), 1, file);

		if (chunk.size <= 0) break;

		if (strncmp(chunk.id, "fmt ", 4) == 0)
		{
			WaveFileFormat format;
			fread(&format, min(chunk.size, sizeof(WaveFileFormat)) , 1, file);

			waveform->format.blockAlign = format.blockAlign;
			waveform->format.bytesPerSecond = format.bytesPerSecond;
			waveform->format.channels = format.channels;
			waveform->format.samplesPerSecond = format.samplePerSecond;

			if (format.audioFormat == WAVE_FORMAT_PCM)
			{
				waveform->format.bitsPerSample = format.bitsPerSample;
			}
			else
			{
				waveform->format.bitsPerSample = format.bytesPerSecond / format.channels / format.samplePerSecond * 8;
			}

			MakeWaveformInfo(*waveform);
			
			if (chunk.size > sizeof(WaveFileFormat))
			{
				fseek(file, chunk.size - sizeof(WaveFileFormat), SEEK_CUR);
			}
		}
		else if (strncmp(chunk.id, "data", 4) == 0)
		{
			waveform->bufferLength = chunk.size;
			waveform->buffer = new char[chunk.size];
			fread(waveform->buffer, chunk.size, 1, file);
		}
		else
		{
			fseek(file, chunk.size, SEEK_CUR);
		}
	}

	waveform->secondLength = (double)waveform->bufferLength / waveform->format.bytesPerSecond / waveform->format.channels;

	fclose(file);

	return waveform;
}

void DestroyWave(Waveform* wf)
{
	if (wf != NULL)
	{
		if (wf->buffer != NULL)
		{
			delete wf->buffer;
			wf->buffer = NULL;
		}

		delete wf;
	}
}

Waveform* CreateWave(ushort channels, uint samplesPerSecond, ushort bitsPerSample, double seconds)
{
	Waveform* wf = new Waveform();

	WaveFormat& format = wf->format;
	format.channels = channels;
	format.samplesPerSecond = samplesPerSecond;
	format.bitsPerSample = bitsPerSample;
	format.blockAlign = format.channels * format.bitsPerSample / 8;
	format.bytesPerSecond = format.blockAlign * format.samplesPerSecond;

	MakeWaveformInfo(*wf);

	wf->secondLength = seconds;
	wf->bufferLength = (int)ceil(format.bytesPerSecond * seconds);
	
	return wf;
}

void SaveWave(Waveform& wf, LPCTSTR filepath)
{
	RiffChunk riff;
	memcpy_s(riff.head.id, 4, "RIFF", 4);
	memcpy_s(riff.format, 4, "WAVE", 4);
	riff.head.size = sizeof(RiffChunk) + sizeof(WaveFileFormat)
		+ sizeof(WaveDataChunkHead) + wf.bufferLength;

	WaveFormatChunk fmt;
	memcpy_s(fmt.chunk.id, 4, "fmt ", 4);
	fmt.chunk.size = sizeof(WaveFileFormat);
	fmt.format.audioFormat = WAVE_FORMAT_PCM;
	fmt.format.channels = wf.format.channels;
	fmt.format.samplePerSecond = wf.format.samplesPerSecond;
	fmt.format.bytesPerSecond = wf.format.bytesPerSecond;
	fmt.format.blockAlign = wf.format.blockAlign;
	fmt.format.bitsPerSample = wf.format.bitsPerSample;

	WaveDataChunkHead data;
	memcpy_s(data.id, 4, "data", 4);
	data.size = wf.bufferLength;

	FILE* file;
	_tfopen_s(&file, filepath, _T("wb"));

	fwrite(&riff, sizeof(RiffChunk), 1, file);
	fwrite(&fmt, sizeof(WaveFormatChunk), 1, file);
	fwrite(&data, sizeof(WaveDataChunkHead), 1, file);
	fwrite(wf.buffer, wf.bufferLength, 1, file);

	fclose(file);
}

void GenerateWave(Waveform& wf, GenWaveParams& gen)
{
	const uint len = wf.bufferLength;

	GenerateFlag flag = gen.flags;

	if (wf.buffer == NULL)
	{
		wf.buffer = new char[len];
		flag = GenerateFlag::Set;
	}

	const float step = (float)(PI * 2.0F / wf.format.samplesPerSecond * gen.freq);
	float angle = (float)((float)gen.phase * PI / 180.0F);
	float segAngle = angle;

	const int stride = wf.format.bitsPerSample / 8;
	const unsigned long long maxValue = (unsigned long long)pow(2.0, wf.format.bitsPerSample) / 100 / 2;
	const double gain = (double)gen.volume * maxValue;

	for (uint i = 0; i < len;)
	{
		long val = (long)(sin(segAngle) * gain);

		for (int c = 0; c < wf.format.channels; c++)
		{
			switch (wf.format.bitsPerSample)
			{
				default:
				case 8:
					{
						char* p = (char*)(wf.buffer + i);
						switch (flag)
						{
						case GenerateFlag::Set: *p = (char)val + 127; break;
							case GenerateFlag::Add: *p += (char)val + 127; break;
							case GenerateFlag::Sub: *p -= (char)val + 127; break;
						}
					}
					break;
				case 16:
					{
						short* p = (short*)(wf.buffer + i);
						switch (flag)
						{
							case GenerateFlag::Set: *p = (short)val; break;
							case GenerateFlag::Add: *p += (short)val; break;
							case GenerateFlag::Sub: *p -= (short)val; break;
						}
					}
					break;
				case 32:
					{
						int* p = (int*)(wf.buffer + i);
						switch (flag)
						{
							case GenerateFlag::Set: *p = (int)val; break;
							case GenerateFlag::Add: *p += (int)val; break;
							case GenerateFlag::Sub: *p -= (int)val; break;
						}
					}

					break;
				case 64:
					{
						long* p = (long*)(wf.buffer + i);
						switch (flag)
						{
							case GenerateFlag::Set: *p = (long)val; break;
							case GenerateFlag::Add: *p += (long)val; break;
							case GenerateFlag::Sub: *p -= (long)val; break;
						}
					}
					break;
			}

			i += stride;
		}
	
		angle += step;

		if (angle > (float)(PI * 2)) angle = angle - (float)(PI * 2);
		
		if (gen.seg > 0)
		{
			//int deg = angle / PI * 180.0F;
			//deg -= deg % (int)(180.0F * gen.seg / 100.0F);
			const float step = gen.seg / 100.f;
			segAngle = (float)floorf(angle / step * step);// deg * PI / 180.0F;
		}
		else
		{
			segAngle = angle;
		}
	}
}

void WaveVolume(Waveform& wf, AdjustVolume& vol, double startSecond, double endSecond)
{
	long fadeInEndBytes = 0, fadeOutFromBytes = 0;
	float fadeInStep = 0, fadeOutStep = 0;
	float fadeInVol = 0, fadeOutVol = 0;

	if (wf.buffer == NULL || wf.bufferLength <= 0)
	{
		return;
	}

	int startBytes = startSecond == -1 ? 0 : (int)(wf.format.bytesPerSecond * startSecond);
	startBytes -= startBytes % wf.format.blockAlign;

	int endBytes = endSecond == -1 ? wf.bufferLength : (int)(wf.format.bytesPerSecond * endSecond);
	endBytes -= endBytes % wf.format.blockAlign;

	if (endBytes > (int)wf.bufferLength) endBytes = wf.bufferLength;

	const float volPercent = (float)vol.volumePercent / 100.f;
	float volume = volPercent;

	if (vol.fadeInSeconds > 0)
	{
		long fadeInBytes = (long)((double)wf.format.bytesPerSecond * vol.fadeInSeconds);
		fadeInBytes -= fadeInBytes % wf.format.blockAlign;

		fadeInEndBytes = startBytes + fadeInBytes;

		fadeInStep = volPercent / fadeInBytes;
		fadeInVol = 0;
	}

	if (vol.fadeOutSeconds > 0)
	{
		long fadeOutBytes = (long)((double)wf.format.bytesPerSecond * vol.fadeOutSeconds);
		fadeOutBytes -= fadeOutBytes % wf.format.blockAlign;

		fadeOutFromBytes = endBytes - fadeOutBytes;

		fadeOutStep = volPercent / fadeOutBytes;
		fadeOutVol = 1;
	}
	
	const int pixelPerSample = wf.format.bytesPerSample;

	const int stride = wf.format.blockAlign;
	const int bytesPerSample = wf.format.bytesPerSample;
	const int channels = wf.format.channels;

	for (int i = startBytes; i < endBytes; )
	{
		volume = volPercent;

		if (fadeInVol <= 1 && fadeInStep > 0 && i <= fadeInEndBytes)
		{
			volume *= fadeInVol;
			fadeInVol += fadeInStep;
		}

		if (fadeOutVol >= 0 && fadeOutStep > 0 && i > fadeOutFromBytes)
		{
			volume *= fadeOutVol;
			fadeOutVol -= fadeOutStep;
		}

		switch (wf.format.bitsPerSample)
		{
			case 8: 
			{
				for (int c = 0; c < channels; c++)
				{
					int val = (int)(wf.buffer[i] * volume);
					if (val > 127)val = 127; else if (val < -127) val = -127;
					wf.buffer[i] = (char)val;
					i++;
				}
			}
			break;
			case 16:
			{
				for (int c = 0; c < channels; c++)
				{
					*(short*)(wf.buffer + i) *= (short)volume;
					i += bytesPerSample;
				}
			}
			break;
			case 32:
			{
				for (int c = 0; c < channels; c++)
				{
					*(int*)(wf.buffer + i) *= (int)volume;
					i += bytesPerSample;
				}
			}
			break;
			case 64: 
			{
				for (int c = 0; c < channels; c++)
				{
					*(long*)(wf.buffer + i) *= (long)volume;
					i += bytesPerSample;
				}
			}
			break;
		}
	}
}

