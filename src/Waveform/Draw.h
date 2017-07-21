
#ifndef __DRAW_H__
#define __DRAW_H__

#include "windows.h"
#include "Waveform.h"
#include "PlainView.h"

struct DrawContext
{
	Waveform* wf;

	uint pixelPerSecond;
	int totalPixelWidth;

	uint channelColors[MAX_SUPPORT_CHANNELS];

	int currentLayer;
	struct 
	{
		double start;
		double end;
	} selection;

	bool scrollToCursor;
	bool fftAfterSelect;
	bool fftShowCursor;
	
	RECT viewBounds;
	double viewStartSecond;
	double viewDisplaySeconds;

	double playStartInSecond;
	double cursorInSecond;

	HDC hdc;
	HPEN selectionPen;
	HBRUSH selectionBrush;
	HPEN cursorPen;
};

void UpdatePixelPerSecond(DrawContext& info, int pixelPerSecond);
void RenewDrawInfo(DrawContext& info);
void UpdateSelection(DrawContext& info, const double startSeconds, const double endSeconds, const int layer);

void WaveDraw(const DrawContext& info, HDC hdc, const Rect& viewBounds);
void WaveDrawSelection(const DrawContext& info, const Rect& viewBounds);

void DrawPartialWave(DrawContext& dc, const WaveCursor& cursor, Rect& viewBounds);

#endif __DRAW_H__
