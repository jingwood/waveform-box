
#include "stdafx.h"

#include "WaveformWindow.h"

void BufferedViewport::UpdateMemoryBitmap()
{
	if (this->membmp != NULL)
	{
		DeleteObject(this->membmp);
	}

	this->membmp = CreateCompatibleBitmap(this->hdc, (int)bufferSize.width, (int)bufferSize.height);
}

BufferedViewport::BufferedViewport(DrawContext& dc, HDC hdc)
	: WaveformBoxViewport(dc), memdc(NULL), membmp(NULL), dirty(true), hdc(hdc)
{
	this->memdc = CreateCompatibleDC(this->hdc);
	this->UpdateMemoryBitmap();
	SelectObject(this->memdc, this->membmp);
}

BufferedViewport::~BufferedViewport()
{
	if (membmp != NULL) DeleteObject(membmp);
	if (memdc != NULL) DeleteDC(memdc);
}

void BufferedViewport::Dirty()
{
	this->dirty = true;
}

bool BufferedViewport::IsDirty()
{
	return this->dirty;
}

void BufferedViewport::UpdateBounds(Rect& bounds)
{
	bufferSize.width = bounds.width;
	bufferSize.height = bounds.height;

	if (this->memdc != NULL)
	{
		Viewport::UpdateBounds(bounds);

		this->UpdateMemoryBitmap();
		SelectObject(this->memdc, this->membmp);
	}
}

void BufferedViewport::Render()
{
	if (this->dirty)
	{
		DrawBuffer(this->memdc, this->bufferSize);
		this->dirty = false;
	}

	BitBlt(this->dc.hdc, (int)this->bounds.x, (int)this->bounds.y, (int)bufferSize.width, (int)bufferSize.height, memdc, 0, 0, SRCPAINT);
}