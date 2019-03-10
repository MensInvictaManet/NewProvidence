#pragma once

struct Color
{
	Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
	{
		R = r;
		G = g;
		B = b;
		A = a;
	}

	Color(const Color& other)
	{
		R = other.R;
		G = other.G;
		B = other.B;
		A = other.A;
	}

	union {
		struct { float R, G, B, A; };
		float colorValues[4];
	};
};

inline bool operator==(const Color& lhs, const Color& rhs) { return ((lhs.R == rhs.R) && (lhs.G == rhs.G) && (lhs.B == rhs.B) && (lhs.A == rhs.A)); }
inline bool operator!=(const Color& lhs, const Color& rhs) { return ((lhs.R != rhs.R) || (lhs.G != rhs.G) || (lhs.B != rhs.B) || (lhs.A != rhs.A)); }

static Color COLOR_WHITE(1.0f, 1.0f, 1.0f, 1.0f);
static Color COLOR_WHITE_FADED(1.0f, 1.0f, 1.0f, 0.25f);
static Color COLOR_RED(1.0f, 0.0f, 0.0f, 1.0f);
static Color COLOR_GREEN(0.0f, 1.0f, 0.0f, 1.0f);
static Color COLOR_BLUE(0.0f, 0.0f, 1.0f, 1.0f);
static Color COLOR_BLACK(0.0f, 0.0f, 0.0f, 1.0f);
static Color COLOR_GRAY(0.4f, 0.4f, 0.4f, 1.0f);
static Color COLOR_LIGHTRED(1.0f, 0.2f, 0.2f, 1.0f);
static Color COLOR_LIGHTGREEN(0.3f, 1.0f, 0.3f, 1.0f);
static Color COLOR_LIGHTBLUE(0.3f, 0.3f, 1.0f, 1.0f);