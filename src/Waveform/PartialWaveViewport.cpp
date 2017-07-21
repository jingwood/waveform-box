
#include "stdafx.h"

#include "WaveformWindow.h"

PartialWaveViewport::PartialWaveViewport(DrawContext& dc) : WaveformBoxViewport(dc)
{
}

void PartialWaveViewport::Render()
{
	Point& p = window->GetLastMovePoint();

	RECT rect;
	SetRect(&rect, this->bounds.x, this->bounds.y, this->bounds.Right(), this->bounds.Bottom());

	HPEN pen = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
	HGDIOBJ oldPen = SelectObject(dc.hdc, pen);
	HGDIOBJ oldBrush = SelectObject(dc.hdc, backgroundBrush);
	
	Rectangle(dc.hdc, rect.left, rect.top, rect.right, rect.bottom);

	DrawPartialWave(dc, this->cursor, this->bounds);
	
	SelectObject(dc.hdc, oldBrush);
	SelectObject(dc.hdc, oldPen);
	DeleteObject(pen);
}

const WaveCursor& PartialWaveViewport::GetCursor()
{
	return this->cursor;
}

void PartialWaveViewport::SetCursor(WaveCursor& cursor)
{
	if (this->cursor.layer != cursor.layer 
		|| this->cursor.cursorSeconds != cursor.cursorSeconds)
	{
		this->cursor = cursor;
		this->window->RefreshUI();
	}
}