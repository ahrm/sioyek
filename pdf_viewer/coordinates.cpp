#include "coordinates.h"

WindowPos::WindowPos(float x_, float y_) {
	x = static_cast<int>(x_);
	y = static_cast<int>(y_);
}

WindowPos::WindowPos(int x_, int y_) {
	x = x_;
	y = y_;
}

WindowPos::WindowPos() {
	x = 0;
	y = 0;
}
