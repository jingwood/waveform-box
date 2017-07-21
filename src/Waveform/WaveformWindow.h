#pragma once

#include "resource.h"

#include "Waveform.h"
#include "Draw.h"
#include "FFT.h"

#include <vector>

class WaveformBoxViewport : public Viewport
{
protected:
	DrawContext& dc;

public:
	WaveformBoxViewport(DrawContext& ctx)
		: dc(ctx)
	{
	}
};

class BufferedViewport : public WaveformBoxViewport
{
private:
	HBITMAP membmp;
	HDC memdc;
	HDC hdc;

protected:
	Size bufferSize;
	bool dirty;

	void UpdateMemoryBitmap();
	virtual void DrawBuffer(HDC hdc, Size& bufferSize) = 0;

	BufferedViewport(DrawContext& dc, HDC hdc);
	virtual ~BufferedViewport();

public:
	void Dirty();
	bool IsDirty();
	void UpdateBounds(Rect& bounds);
	void Render();
};

class WaveViewport : public BufferedViewport
{
protected:
	void DrawBuffer(HDC hdc, Size& bufferSize);

public:
	WaveViewport(DrawContext& dc, HDC hdc);
};

class IndicatorViewport : public WaveformBoxViewport
{
private:
	Point lastMovePoint;

protected:
	bool rangeSelecting;

public:
	void Render();

	IndicatorViewport(DrawContext& dc);

	bool OnMouseDown(MouseButtons buttons, float x, float y);
	bool OnMouseMove(MouseButtons buttons, Point& p);
	bool OnMouseUp(MouseButtons buttons, Point& p);
};

class FFTViewport : public WaveformBoxViewport
{
protected:
	bool hover;
	void Render();

public:
	FFTViewport(DrawContext& dc);

	bool OnMouseMove(MouseButtons buttons, Point& p);
	void OnMouseEnter();
	void OnMouseLeave();
	void DoAnalysis();
};

class PartialWaveViewport : public WaveformBoxViewport
{
private:
	WaveCursor cursor;

protected:
	void Render();

public:
	PartialWaveViewport(DrawContext& dc);

	const WaveCursor& GetCursor();
	void SetCursor(WaveCursor& cursor);
};

class WindowAgent : public IWindowAgent
{
private:
	Sprite* capturedSprite;
	Sprite* currentHoverSprite;
	std::vector<Sprite*> sprites;
	Point lastMovePoint;

public:
	HWND hwnd;

	WindowAgent();

	void AddSprite(Sprite* s);
	void DestroyAllSprites();

	void RefreshUI();
	void Render();
	void Resize(Size& s);

	void CaptureMouse(Sprite* s);
	void ReleaseCapture(const Sprite* s);
	void SetHoverSprite(Sprite* s = NULL);
	Point& GetLastMovePoint();
	
	void DispatchMouseDown(MouseButtons buttons, float x, float y);
	void DispatchMouseMove(MouseButtons buttons, Point& p);
	void DispatchMouseUp(MouseButtons buttons, Point& p);
};

struct NewWave
{
	WaveFormat format;
	uint seconds;
};

struct OperationParameters
{
	bool leftButtonDown;
	bool rangeSelecting;
	int startPixel;
	int endPixel;
	int lastMoveX;
	int lastMoveY;
	double startSeconds;
};

#define Invalidate(hwnd, updateWave) \
	if (updateWave && waveVp != NULL && !waveVp->IsDirty()) { waveVp->Dirty(); } \
	InvalidateRect(hwnd, NULL, FALSE)

extern Waveform* wave;
extern DrawContext dc;
extern FFTInfo fft;
extern struct NewWave newWave;
extern GenWaveParams genWave;
extern AdjustVolume vol;
extern struct OperationParameters op;

extern WaveViewport* waveVp;
extern IndicatorViewport* indicVp;
extern FFTViewport* fftVp;
extern PartialWaveViewport* partialWaveVp;

extern HBRUSH backgroundBrush;