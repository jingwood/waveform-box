
#include "stdafx.h"

#include "WaveformWindow.h"

void FFTViewport::Render()
{
	DrawFFT(dc, fft, this->bounds);

	if (dc.fftShowCursor && this->hover)
	{
		DrawFFTCursor(dc, fft, this->bounds);
	}
}

FFTViewport::FFTViewport(DrawContext& dc) : WaveformBoxViewport(dc)
{
}

bool FFTViewport::OnMouseMove(MouseButtons buttons, Point& p)
{
	int maxFreq = wave->format.samplesPerSecond / 2;

	fft.cursorFreq = (double)fft.minFreq + (p.x - dc.viewBounds.left) / this->bounds.width * maxFreq;

	if (fft.cursorFreq > maxFreq) 
	{
		fft.cursorFreq = maxFreq;
	}

	window->RefreshUI();

	return true;
}

void FFTViewport::OnMouseEnter()
{
	if (!this->hover)
	{
		this->hover = true;
		window->RefreshUI();
	}
}

void FFTViewport::OnMouseLeave()
{
	if (this->hover)
	{
		this->hover = false;
		window->RefreshUI();
	}
}

void FFTViewport::DoAnalysis()
{
	fft.startSeconds = dc.selection.start;
	fft.endSeconds = dc.selection.end;

	if (dc.selection.start >= 0 && dc.selection.start < dc.selection.end)
	{
		fft.layer = dc.currentLayer;
		FFT(fft);
		
		window->RefreshUI();
	}
}

