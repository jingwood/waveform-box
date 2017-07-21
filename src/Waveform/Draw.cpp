
#include "stdafx.h"

#include "Draw.h"
#include <math.h>

#define MIN_PIXEL_PER_SECOND 100

void UpdatePixelPerSecond(DrawContext& info, int pixelPerSecond)
{
	Waveform* wf = info.wf;

	if (wf == NULL)
	{
		info.pixelPerSecond = 0;
		info.totalPixelWidth = 0;
	}
	else
	{
		if (pixelPerSecond < MIN_PIXEL_PER_SECOND) pixelPerSecond = MIN_PIXEL_PER_SECOND;
	
		if (pixelPerSecond > (int)wf->format.samplesPerSecond)
		{
			pixelPerSecond = wf->format.samplesPerSecond;
		}

		info.pixelPerSecond = pixelPerSecond;
		info.totalPixelWidth = (int)(wf->secondLength * info.pixelPerSecond);
	}
}

void RenewDrawInfo(DrawContext& info)
{
	UpdatePixelPerSecond(info, 500);
	
	info.viewStartSecond = 0;

	info.selection.start = 0.0F;
	info.selection.end = 0.0F;
	info.cursorInSecond = 0.0F;
}

void UpdateSelection(DrawContext& info, const double startSeconds, const double endSeconds, const int layer)
{
	Waveform& wf = *info.wf;

	info.selection.start = startSeconds;
	info.selection.end = endSeconds;

	if (info.selection.start < 0) info.selection.start = 0;
	if (info.selection.start > wf.secondLength) info.selection.start = wf.secondLength;

	if (info.selection.end < 0) info.selection.end = 0;
	if (info.selection.end > wf.secondLength) info.selection.end = wf.secondLength;

	info.currentLayer = layer;
}

static void DrawLayerRuler(const DrawContext& info, const RECT& bounds,
	const int layer, const HPEN boundPen)
{
	const Waveform& wf = *info.wf;
	const HDC hdc = info.hdc;

	// ruler
	const HGDIOBJ oldPen = SelectObject(hdc, boundPen);

	MoveToEx(hdc, bounds.left, bounds.bottom, NULL);
	LineTo(hdc, bounds.right, bounds.bottom);

	const int rulerTop = bounds.bottom - 10;

	int ruleLeft = (int)floor(bounds.left - info.viewStartSecond * info.pixelPerSecond);
	int x = 0;

	for (int s = 0; s <= wf.secondLength && ruleLeft <= bounds.right; s++)
	{
		x = max(ruleLeft, bounds.left);

		MoveToEx(hdc, x, rulerTop, NULL);
		LineTo(hdc, x, bounds.bottom);

		ruleLeft += info.pixelPerSecond;
	}

	SelectObject(hdc, oldPen);
}

static void DrawLayerSelection(const DrawContext& info, const RECT& bounds, const int layer)
{
	const Waveform& wf = *info.wf;
	const int viewLeft = info.viewBounds.left;

	int selStartPixel = (int)floor((double)viewLeft + (info.selection.start - info.viewStartSecond) * info.pixelPerSecond);
	int selEndPixel = (int)floor((double)viewLeft + (info.selection.end - info.viewStartSecond) * info.pixelPerSecond);

	if (selStartPixel < bounds.left) selStartPixel = bounds.left;
	if (selEndPixel > bounds.right) selEndPixel = bounds.right;

	if (selStartPixel < selEndPixel)
	{
		RECT selRect;
		SetRect(&selRect, selStartPixel, bounds.top, selEndPixel, bounds.bottom);
		FillRect(info.hdc, &selRect, info.selectionBrush);
	}
}

static void DrawLayerCursor(const DrawContext& info, const RECT& bounds, const int layer)
{
	const Waveform& wf = *info.wf;
	const HDC hdc = info.hdc;
	const int viewLeft = info.viewBounds.left;

	int selStartPixel = (int)(viewLeft + (info.selection.start - info.viewStartSecond) * info.pixelPerSecond);

	if (selStartPixel >= bounds.left && selStartPixel <= bounds.right)
	{
		HGDIOBJ oldPen = SelectObject(hdc, info.selectionPen);
		MoveToEx(hdc, selStartPixel, bounds.top, NULL);
		LineTo(hdc, selStartPixel, bounds.bottom);
		SelectObject(hdc, oldPen);
	}

	int cursorPixel = (int)(viewLeft + (info.cursorInSecond - info.viewStartSecond) * info.pixelPerSecond);
	
	if (cursorPixel >= bounds.left && cursorPixel <= bounds.right)
	{
		HGDIOBJ oldPen = SelectObject(hdc, info.cursorPen);
		MoveToEx(hdc, cursorPixel, bounds.top, NULL);
		LineTo(hdc, cursorPixel, bounds.bottom);
		SelectObject(hdc, oldPen);
	}
}

static void DrawLayerWave(const DrawContext& info, HDC hdc, const RECT& bounds, 
	const int layerIndex, const int bytesPerPixel, const int pixelOffset)
{
	Waveform& wf = *info.wf;

	const int viewHeight = (bounds.bottom - bounds.top);
	const float viewHeightPercent = (float)viewHeight / wf.calcInfo.sampleMaxValue;
	const char* buffend = wf.buffer + wf.bufferLength;
	const int stride = wf.format.blockAlign;
	
	const float cy = (float)bounds.top + viewHeight / 2.0F;
	const COLORREF layerColor = info.channelColors[layerIndex];
	
	HPEN channelPen = CreatePen(PS_SOLID, 1, layerColor);
	HGDIOBJ oldPen = SelectObject(hdc, channelPen);

	MoveToEx(hdc, bounds.left, (int)cy, NULL);

	char* scan = wf.buffer + layerIndex * wf.format.bytesPerSample + pixelOffset;

	for (float x = (float)bounds.left; x < bounds.right && scan < buffend; x++)
	{
		long val = 0;

		int vmin = 0, vmax = 0;

		for (int i = 0; i < bytesPerPixel && scan < buffend; i++)
		{
			switch (wf.format.bitsPerSample)
			{
			case 8:
				val = (long)(*(byte*)scan) - 127; break;
			case 16:
				val = (long)(*(short*)scan); break;
			case 32:
				val = (long)(*(int*)scan); break;
			case 64:
				val = (*(long*)scan); break;
			}

			val = (long)(val * viewHeightPercent);

			if (vmin > val) vmin = val;
			if (vmax < val) vmax = val;

			scan += stride;
		}

		if (vmin == 0 || vmax == 0)
		{
			LineTo(hdc, (int)x, (int)(cy + val));
		}
		else
		{
			LineTo(hdc, (int)x, (int)(cy + vmax));
			LineTo(hdc, (int)x, (int)(cy + vmin));
		}
	}

	SelectObject(hdc, oldPen);
	DeleteObject(channelPen);
}

void WaveDraw(const DrawContext& info, HDC hdc, const Rect& viewBounds)
{
	Waveform& wf = *info.wf;

	const ushort channels = wf.format.channels;

	const float viewHeight = viewBounds.height / channels;
	const float bottom = viewBounds.Bottom();
	const float right = viewBounds.Right();
	float viewTop = viewBounds.y;

	const int bytesPerPixel = max((int)(ceil((double)wf.format.bytesPerSecond / info.pixelPerSecond
		/ wf.format.channels / wf.format.bytesPerSample)), 1);

	int pixelOffset = (int)(info.viewStartSecond * wf.format.bytesPerSecond);
	pixelOffset -= (pixelOffset % wf.format.blockAlign);

	HPEN boundPen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
	HGDIOBJ oldPen = SelectObject(hdc, boundPen);

	MoveToEx(hdc, (int)viewBounds.x, (int)viewBounds.y, NULL);
	LineTo(hdc, (int)viewBounds.x, (int)bottom);

	SelectObject(hdc, oldPen);

	for (int c = 0; c < channels && c < MAX_SUPPORT_CHANNELS; c++)
	{
		RECT bounds;
		SetRect(&bounds, (int)viewBounds.x, (int)viewTop, (int)right, (int)(viewTop + viewHeight));

		if (wf.buffer != NULL)
		{
			DrawLayerWave(info, hdc, bounds, c, bytesPerPixel, pixelOffset);
		}

		viewTop += viewHeight + 1;
	}

	DeleteObject(boundPen);
}

void WaveDrawSelection(const DrawContext& info, const Rect& viewBounds)
{
	const HDC hdc = info.hdc;
	const ushort channels = info.wf->format.channels;

	const double viewWidth = viewBounds.width;
	const float viewHeight = (viewBounds.height / channels);
	float viewTop = (viewBounds.y);

	float viewBottom = viewBounds.Bottom();
	float viewRight = viewBounds.Right();

	HPEN boundPen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));

	for (int c = 0; c < channels && c < MAX_SUPPORT_CHANNELS; c++)
	{
		RECT bounds;
		SetRect(&bounds, (int)viewBounds.x, (int)viewTop, (int)viewRight, (int)(viewTop + viewHeight));

		if (info.currentLayer == c)
		{
			DrawLayerSelection(info, bounds, c);
		}

		DrawLayerRuler(info, bounds, c, boundPen);

		DrawLayerCursor(info, bounds, c);

		viewTop += viewHeight + 1;
	}

	TCHAR msg[32];
	_stprintf_s(msg, _T("%.2fs"), info.viewStartSecond);
	TextOut(hdc, (int)(viewBounds.x - 10), (int)viewBottom, msg, _tcslen(msg));

	_stprintf_s(msg, _T("%2.f%%"), viewWidth * 100.0F / info.totalPixelWidth);
	TextOut(hdc, (int)(viewRight - 20), (int)viewBottom, msg, _tcslen(msg));

	DeleteObject(boundPen);
}

void DrawPartialWave(DrawContext& dc, const WaveCursor& cursor, Rect& viewBounds)
{
	if (dc.wf == NULL || dc.wf->buffer == NULL || dc.wf->bufferLength <= 0) return;

	Waveform& wf = *dc.wf;
	const double cursorSeconds = cursor.cursorSeconds;
	const int layer = cursor.layer;

	int pixelOffset = (int)(cursorSeconds * wf.format.bytesPerSecond - viewBounds.width / 2);
	pixelOffset -= (pixelOffset % wf.format.blockAlign);

	if (pixelOffset < 0)pixelOffset = 0;
	if (pixelOffset + viewBounds.width > wf.bufferLength) pixelOffset = (int)(wf.bufferLength - viewBounds.width);
	
	const char* scan = wf.buffer + pixelOffset;
	const float viewHeightPercent = viewBounds.height / wf.calcInfo.sampleMaxValue;

	float cy = viewBounds.y + viewBounds.height / 2;
	float x = viewBounds.x;

	const COLORREF layerColor = dc.channelColors[layer];
	HPEN channelPen = CreatePen(PS_SOLID, 1, layerColor);
	HGDIOBJ oldPen = SelectObject(dc.hdc, channelPen);

	MoveToEx(dc.hdc, (int)x, (int)cy, NULL);
	
	float right = viewBounds.Right();
	for (float x = viewBounds.x; x < right; x++, scan += wf.format.blockAlign)
	{
		long val = 0;
		switch (wf.format.bitsPerSample)
		{
		default: case 8: val = (byte)(*scan) - 127; break;
		case 16: val = (long)*(short*)scan; break;
		case 32: val = (long)*(int*)scan; break;
		case 64: val = *(long*)scan; break;
		}

		float y = (viewHeightPercent * val);

		LineTo(dc.hdc, (int)x, (int)(cy + y));
	}

	SelectObject(dc.hdc, oldPen);
	DeleteObject(channelPen);
}

