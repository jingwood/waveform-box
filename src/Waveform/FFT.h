
#include "Waveform.h"
#include "Draw.h"

#define MAX_FFT_FREQ 300000

struct FFTInfo
{
	Waveform* wf;
	double startSeconds;
	double endSeconds;
	int layer;

	//int amplitudeBufferLength;
	double ampBuffer[MAX_FFT_FREQ / 2];
	int minFreq;
	int maxFreq;

	long double maxAmplitude;

	double cursorFreq;

};

void FFT(FFTInfo& fft);

void DrawFFT(const DrawContext& dc, const FFTInfo& fft, const Rect& bounds);
void DrawFFTCursor(const DrawContext dc, const FFTInfo& fft, const Rect& bounds);