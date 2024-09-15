#pragma once
#include "Engine/Math/Vec3.hpp"
// #include "Engine/Math/Vec4.hpp"

struct Rgba8
{
public:
	Rgba8() = default;
	//default color is white
	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 255;
	unsigned char a = 255;

	explicit Rgba8(unsigned char initialr, unsigned char initialg, unsigned char initialb, unsigned char initiala);
	explicit Rgba8(unsigned char initialr, unsigned char initialg, unsigned char initialb);
	// explicit Rgba8(Vec4 const& float4);

	// constant static colors
	static Rgba8 const RED;
	static Rgba8 const RED_TRANSPARENT;
	static Rgba8 const RED_CLEAR;
	static Rgba8 const BURNT_RED;
	static Rgba8 const WARM_PURPLE;
	static Rgba8 const ROYAL_PURPLE;
	static Rgba8 const GREEN;
	static Rgba8 const BLUE;
	static Rgba8 const NEON_BLUE;
	static Rgba8 const PASTEL_BLUE;
	static Rgba8 const TEAL_BLUE;
	static Rgba8 const CYSTAL_BLUE;
	static Rgba8 const PURPLE_BLUE;
	static Rgba8 const BLUE_MVT;
	static Rgba8 const BLUE_MVTHL;
	static Rgba8 const CYAN;
	static Rgba8 const MAGENTA;
	static Rgba8 const NEON_PINK;
	static Rgba8 const SALMON_PINK;
	static Rgba8 const BLUSH_PINK;
	static Rgba8 const YELLOW;
	static Rgba8 const Naples_Yellow;
	static Rgba8 const CANDLE_YELLOW;
	static Rgba8 const BRIGHT_ORANGE;
	static Rgba8 const LIGHT_ORANGE;
	static Rgba8 const DEEP_ORANGE;
	static Rgba8 const PEACH_ORANGE;
	static Rgba8 const BLACK;
	static Rgba8 const BLACK_TRANSPARENT;
	static Rgba8 const WHITE;
	static Rgba8 const WHITE_TRANSPARENT;
	static Rgba8 const GRAY;
	static Rgba8 const GRAY_Dark;
	static Rgba8 const GRAY_TRANSPARENT;

	bool  SetFromText(char const* text);
	void  GetAsFloats(float* colorAsFloats) const;
};