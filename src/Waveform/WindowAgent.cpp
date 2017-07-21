
#include "stdafx.h"

#include "WaveformWindow.h"

WindowAgent::WindowAgent() : capturedSprite(NULL)
{
}

void WindowAgent::AddSprite(Sprite* s)
{
	if (s != NULL)
	{
		this->sprites.push_back(s);
		s->window = this;
	}
}

void WindowAgent::DestroyAllSprites()
{
	for (int i = 0; i < (int)this->sprites.size(); i++)
	{
		Sprite* sp = (Sprite*)this->sprites[i];
		delete sp;
	}
}

void WindowAgent::RefreshUI()
{
	Invalidate(hwnd, FALSE);
}

void WindowAgent::Render()
{
	for (int i = 0; i < (int)this->sprites.size(); i++)
	{
		Sprite* sp = this->sprites[i];
		
		if (sp->IsVisible())
		{
			sp->Render();
		}
	}
}

void WindowAgent::Resize(Size& s)
{
	bool showFftPanel = fftVp->IsVisible();

	Rect bounds(20, 20, s.width - 40, s.height - (showFftPanel ? 255 : 40));

	indicVp->UpdateBounds(bounds);
	waveVp->UpdateBounds(bounds);

	bounds.y += bounds.height + 20;
	bounds.height = 200;
	fftVp->UpdateBounds(bounds);

	bounds.Set(s.width - 220, 20, 200, 100);
	partialWaveVp->UpdateBounds(bounds);
}

void WindowAgent::CaptureMouse(Sprite* s)
{
	capturedSprite = s;
}

void WindowAgent::ReleaseCapture(const Sprite* s)
{
	if (this->capturedSprite == s)
	{
		this->capturedSprite = NULL;
	}
}

void WindowAgent::SetHoverSprite(Sprite* s)
{
	if (this->currentHoverSprite != s)
	{
		if (this->currentHoverSprite != NULL)
		{
			this->currentHoverSprite->OnMouseLeave();
		}

		this->currentHoverSprite = s;

		if (this->currentHoverSprite != NULL)
		{
			this->currentHoverSprite->OnMouseEnter();
		}
	}
}

Point& WindowAgent::GetLastMovePoint()
{
	return this->lastMovePoint;
}

void WindowAgent::DispatchMouseDown(MouseButtons buttons, float x, float y)
{
	if (this->capturedSprite != NULL)
	{
		this->capturedSprite->OnMouseDown(buttons, x, y);
	}
	else
	{
		bool processed = false;

		for (int i = 0; i < (int)sprites.size(); i++)
		{
			Sprite* s = sprites[i];

			if (s->GetBounds().Contains(x, y))
			{
				if (s->OnMouseDown(buttons, x, y))
				{
					processed = true;
					break;
				}
			}
		}
	}
}

void WindowAgent::DispatchMouseMove(MouseButtons buttons, Point& p)
{
	this->lastMovePoint = p;

	if (this->capturedSprite != NULL)
	{
		this->capturedSprite->OnMouseMove(buttons, p);
	}
	else
	{
		Sprite* hoverSprite = NULL;

		for (int i = 0; i < (int)sprites.size(); i++)
		{
			Sprite* s = sprites[i];

			if (s->GetBounds().Contains(p))
			{
				hoverSprite = s;

				this->SetHoverSprite(s);

				if (s->OnMouseMove(buttons, p))
				{
					break;
				}
			}
		}

		this->SetHoverSprite(hoverSprite);
	}
}

void WindowAgent::DispatchMouseUp(MouseButtons buttons, Point& p)
{
	if (this->capturedSprite != NULL)
	{
		this->capturedSprite->OnMouseUp(buttons, p);
	}
	else
	{
		for (int i = 0; i < (int)sprites.size(); i++)
		{
			Sprite* s = sprites[i];

			if (s->GetBounds().Contains(p))
			{
				if (s->OnMouseUp(buttons, p))
				{
					break;
				}
			}
		}
	}
}
