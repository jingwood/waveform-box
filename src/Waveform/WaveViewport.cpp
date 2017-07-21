
#include "stdafx.h"

#include "WaveformWindow.h"

WaveViewport::WaveViewport(DrawContext& dc, HDC hdc) : BufferedViewport(dc, hdc)
{
}

void WaveViewport::DrawBuffer(HDC hdc, Size& bufferSize)
{
	Waveform* wf = this->dc.wf;

	RECT rect;
	SetRect(&rect, 0, 0, (int)bufferSize.width, (int)bufferSize.height);
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (wf != NULL)
	{
		WaveDraw(this->dc, hdc, Rect(0, 0, bufferSize.width, bufferSize.height));
	}
}
