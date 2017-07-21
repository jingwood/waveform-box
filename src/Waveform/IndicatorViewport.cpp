
#include "stdafx.h"

#include "WaveformWindow.h"

IndicatorViewport::IndicatorViewport(DrawContext& dc) : WaveformBoxViewport(dc) {	}

void IndicatorViewport::Render()
{
	Waveform* wf = this->dc.wf;

	if (wf != NULL)
	{
		WaveDrawSelection(this->dc, this->bounds);
	}
}

bool IndicatorViewport::OnMouseDown(MouseButtons buttons, float x, float y)
{
	if ((buttons & MouseButtons::Left) == MouseButtons::Left)
	{
		if (GetKeyState(VK_SHIFT) < 0)
		{
			double endSeconds = dc.viewStartSecond + ((double)x - dc.viewBounds.left) / dc.pixelPerSecond;
			UpdateSelection(dc, min(op.startSeconds, endSeconds), max(op.startSeconds, endSeconds), dc.currentLayer);
		}
		else
		{
			int layer = (int)((float)(y - this->bounds.y) * wave->format.channels / (this->bounds.height));

			op.startSeconds = dc.viewStartSecond + ((double)x - dc.viewBounds.left) / dc.pixelPerSecond;
			UpdateSelection(dc, op.startSeconds, op.startSeconds, layer);
		}

		this->window->CaptureMouse(this);
		this->window->RefreshUI();

		this->rangeSelecting = true;

		return true;
	}

	return false;
}

bool IndicatorViewport::OnMouseMove(MouseButtons buttons, Point& p)
{
	if (this->rangeSelecting
		&& (buttons & MouseButtons::Left) == MouseButtons::Left)
	{
		if (GetKeyState(VK_SHIFT) >= 0)
		{
			double endSeconds = dc.viewStartSecond + ((double)p.x - dc.viewBounds.left) / dc.pixelPerSecond;
			UpdateSelection(dc, min(op.startSeconds, endSeconds), max(op.startSeconds, endSeconds), dc.currentLayer);

			this->window->RefreshUI();
		}
	}

	if (partialWaveVp->IsVisible() && this->lastMovePoint.x != p.x)
	{
		this->lastMovePoint.x = p.x;
	
		const double startSeconds = dc.viewStartSecond + (p.x - indicVp->GetBounds().x) / dc.pixelPerSecond;
		int layer = (int)((float)(p.y - this->bounds.y) * wave->format.channels / (this->bounds.height));
		partialWaveVp->SetCursor(WaveCursor(layer, startSeconds));

		this->window->RefreshUI();
	}

	return false;
}

bool IndicatorViewport::OnMouseUp(MouseButtons buttons, Point& p)
{
	if (this->rangeSelecting)
	{
		this->window->ReleaseCapture(this);
		this->rangeSelecting = false;

		if (dc.fftAfterSelect)
		{
			fftVp->DoAnalysis();
		}

		return true;
	}

	return false;
}
