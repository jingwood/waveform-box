
#include "stdafx.h"

#include "FFT.h"
#include <math.h>

#define PI 3.14159265

typedef struct
{
	double r;
	double i;
} complex;

static complex fftBuffer[MAX_FFT_FREQ];

inline int bitr(int bit, int r)
{
	int bitr = 0;

	for (int i = 0; i < r; i++)
	{
		bitr <<= 1;
		bitr |= (bit & 1);
		bit >>= 1;
	}

	return bitr;
}

inline void swap(complex* d0, complex* d1)
{
	double a;

	a = d0->r;
	d0->r = d1->r;
	d1->r = a;

	a = d0->i;
	d0->i = d1->i;
	d1->i = a;
}

void FFT(FFTInfo& fft)
{
	Waveform& wf = *fft.wf;
	double startSeconds = fft.startSeconds;
	double endSeconds = fft.endSeconds;

	if (wf.bufferLength <= 0 || wf.buffer == NULL || startSeconds >= endSeconds || endSeconds == 0)
	{
		return;
	}

	int startBytes = (int)(startSeconds * wf.format.bytesPerSecond);
	startBytes -= startBytes % wf.format.blockAlign;
	startBytes += fft.layer * wf.format.bytesPerSample;

	int endBytes = (int)(endSeconds * wf.format.bytesPerSecond);
	endBytes -= endBytes % wf.format.blockAlign;

	int maxBytes = wf.format.samplesPerSecond / wf.format.blockAlign;

	if ((endBytes - startBytes) > maxBytes)
	{
		endBytes = startBytes + maxBytes;
	}

	int N = endBytes - startBytes;
	
	complex* x = fftBuffer;

	ZeroMemory(x, (MAX_FFT_FREQ) * sizeof(complex));
	
	int j = 0;
	for (int j = 0, i = startBytes; i <= endBytes; i += wf.format.blockAlign, j++)
	{
		switch (wf.format.bitsPerSample)
		{
		default:
		case 8: x[j].r = (double) *((byte*)(wf.buffer + i)) - 127; break;
		case 16: x[j].r = (double) *((short*)(wf.buffer + i)); break;
		case 32: x[j].r = (double) *((int*)(wf.buffer + i)); break;
		case 64: x[j].r = (double) *((long*)(wf.buffer + i)); break;
		}

		x[j].i = 0;
	}

	int bitLen = 0;

	int i = N >> 1;
	while (i > 0)
	{
		i >>= 1;
		bitLen++;
	}

	int n = 1 << bitLen;
	int j2 = n;
	int k = 0;
	int j1 = bitLen - 1;
	
	for (int j = 0; j < bitLen; j++)
	{
		j2 >>= 1;

		while (true)
		{
			for (int i = 0; i < j2; i++)
			{
				int p = bitr(k >> j1, bitLen);
				double freq = 2.0 * PI / n * p;

				int k1 = k + j2;

				double a = x[k1].r * cos(freq) + x[k1].i * sin(freq);
				double b = x[k1].i * cos(freq) - x[k1].r * sin(freq);

				x[k1].r = x[k].r - a;
				x[k1].i = x[k].i - b;
				x[k].r = x[k].r + a;
				x[k].i = x[k].i + b;

				k++;
			}

			k += j2;
			if (k >= n) break;
		}

		k = 0;
		j1--;
	}

	for (int k = 0; k < n; k++)
	{
		int bit = bitr(k, bitLen);
		if (bit > k) swap(&x[k], &x[bit]);
	}

	fft.minFreq = 0;
	fft.maxFreq = n / 2;
	fft.maxAmplitude = 0;

	for (int k = 0; k < fft.maxFreq; k++)
	{
		double amp = (x[k].r * x[k].r + x[k].i * x[k].i);
		fft.ampBuffer[k] = amp;
		if (fft.maxAmplitude < amp) fft.maxAmplitude = amp;
	}

}

void DrawFFT(const DrawContext& dc, const FFTInfo& fft, const Rect& bounds)
{
	float right = bounds.Right();

	COLORREF color = RGB(110, 110, 140);

	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HGDIOBJ oldPen = SelectObject(dc.hdc, pen);

	float bottom = (float)(bounds.Bottom() - 10);

	MoveToEx(dc.hdc, (int)(bounds.x), (int)bottom, NULL);
	LineTo(dc.hdc, (int)(right), (int)bottom);

	TCHAR szFreq[16];
	
	if (fft.wf != NULL)
	{
		const int maxSteps = 15;
		
		float xStep = (float)(bounds.width / maxSteps);
		float freqRulerStep = (float)(fft.wf->format.samplesPerSecond) / 2 / maxSteps;
		float freqStep = (float)(fft.maxFreq - fft.minFreq) / (float)(bounds.width);

		float viewHeight = (bottom - bounds.y);

		float x = bounds.x;
		float f = 0, rf = 0;

		for (int i = 0; i <= maxSteps; i++, x += xStep, rf += freqRulerStep)
		{
			if (fft.maxFreq > 0)
			{
				for (float xx = x; xx < x + xStep && f <= fft.maxFreq; xx++, f += freqStep)
				{
					MoveToEx(dc.hdc, (int)xx, (int)bottom, NULL);
					LineTo(dc.hdc, (int)xx, (int)(bottom - (fft.ampBuffer[(int)f] / fft.maxAmplitude * viewHeight)));
				}

				MoveToEx(dc.hdc, (int)x, (int)bottom, NULL);
				LineTo(dc.hdc, (int)x, (int)(bottom + 4));
			}

			if (i < maxSteps)
			{
				_stprintf_s(szFreq, _T("%.2f"), rf / 1000);
				TextOut(dc.hdc, (int)(x - 10), (int)(bottom + 4), szFreq, _tcslen(szFreq));
			}
		}

		_stprintf_s(szFreq, _T("%.2f kHz"), (float)fft.wf->format.samplesPerSecond / 2000);
		TextOut(dc.hdc, (int)(right - 40), (int)(bottom + 3), szFreq, _tcslen(szFreq));
	}

	SelectObject(dc.hdc, oldPen);
	DeleteObject(pen);
}

void DrawFFTCursor(const DrawContext dc, const FFTInfo& fft, const Rect& bounds)
{
	if (fft.cursorFreq > 0)
	{
		float x = (float)bounds.x + (float)fft.cursorFreq / (dc.wf->format.samplesPerSecond / 2) * bounds.width;
		float bottom = bounds.Bottom();

		HPEN pen = CreatePen(PS_DOT, 1, RGB(178, 128, 128));
		HGDIOBJ oldPen = SelectObject(dc.hdc, pen);
		
		float viewHeight = (bottom - 10 - bounds.y);

		float amp = max((float)(fft.ampBuffer[(int)fft.cursorFreq] / fft.maxAmplitude * viewHeight), 0.0F);
		float y = bottom - amp - 10;

		//Ellipse(dc.hdc, x - 3, y - 3, x + 3, y + 3);

		MoveToEx(dc.hdc, (int)x, (int)bounds.y, NULL);
		LineTo(dc.hdc, (int)x, (int)y);

		SelectObject(dc.hdc, oldPen);
		DeleteObject(pen);

		TCHAR szFreq[16];
		_stprintf_s(szFreq, _T("%.2f kHz"), fft.cursorFreq / 1000.0F);
		TextOut(dc.hdc, (int)(x + 2), (int)bounds.y, szFreq, _tcslen(szFreq));
	}
}