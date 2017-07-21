
#ifndef __PLAINVIEW_H__
#define __PLAINVIEW_H__

#include "basetype.h"

struct Point
{
	float x;
	float y;

	Point() : x(0), y(0) { }
	Point(const float x, const float y) : x(x), y(y) { }
};

struct Size
{
	float width;
	float height;

	Size() : width(0), height(0) { }
	Size(const float width, const float height) : width(width), height(height) { }
};

struct Rect
{
	union {
		struct {
			float x;
			float y;
			float width;
			float height;
		};

		struct {
			Point location;
			Size size;
		};
	};

	Rect()
		: x(0), y(0), width(0), height(0)
	{
	}

	Rect(const float x, const float y, const float width, const float height)
		: x(x), y(y), width(width), height(height) { }

	float Right() const;
	float Bottom() const;

	inline void Set(float x, float y, float width, float height)
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

	bool Contains(const Point& p) const;
	bool Contains(const float x, const float y) const;
};

enum MouseButtons
{
	None = 0x0,
	Left = 0x1,
	Right = 0x2,
	Middle = 0x4,
};

inline MouseButtons operator |= (MouseButtons& b1, const MouseButtons b2)
{
	return (MouseButtons)(b1 = (MouseButtons)((byte)b1 | (byte)b2));
}

class IWindowAgent;

class Sprite
{
protected:
	Rect bounds;
	bool visible;

public:
	IWindowAgent* window;

	inline Rect& GetBounds()
	{
		return this->bounds;
	}

	inline bool IsVisible()
	{
		return this->visible;
	}

	inline void SetVisible(bool visible)
	{
		this->visible = visible;
	}

	virtual void UpdateBounds(const Rect& bounds)
	{
		this->bounds = bounds;
	}

	virtual void Render() = 0;

	virtual bool OnMouseDown(MouseButtons buttons, float x, float y) { return false; }
	virtual bool OnMouseMove(MouseButtons buttons, Point& p) { return false; }
	virtual bool OnMouseUp(MouseButtons buttons, Point& p) { return false; }

	virtual void OnMouseEnter() { }
	virtual void OnMouseLeave() { }

	Sprite() : visible(true) { }
	virtual ~Sprite() { }
};

class IWindowAgent
{
public:
	virtual void CaptureMouse(Sprite* s) = 0;
	virtual void ReleaseCapture(const Sprite* s) = 0;
	virtual void RefreshUI() = 0;
	virtual Point& GetLastMovePoint() = 0;
};

class Viewport : public Sprite
{
protected:
	virtual void OnScroll(int x, int y) { }
};

#endif /* __PLAINVIEW_H__ */