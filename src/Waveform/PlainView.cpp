
#include "stdafx.h"

#include "PlainView.h"

float Rect::Right() const
{
	return this->x + this->width;
}

float Rect::Bottom() const
{
	return this->y + this->height;
}

bool Rect::Contains(const Point& p) const
{
	return this->Contains(p.x, p.y);
}

bool Rect::Contains(const float x, const float y) const
{
	return x >= this->x && x <= this->Right()
		&& y >= this->y && y <= this->Bottom();
}