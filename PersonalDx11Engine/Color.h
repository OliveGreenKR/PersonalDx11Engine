#pragma once
#include "Math.h"



namespace Color
{
	static const Vector4 Red()
	{
		static Vector4 color = Vector4(1, 0, 0, 1);
		return color;
	}
	static const Vector4 Green()
	{
		static Vector4 color = Vector4(0, 1, 0, 1);
		return color;
	}
	static const Vector4 Blue()
	{
		static Vector4 color = Vector4(0, 0, 1, 1);
		return color;
	}

	static const Vector4 White()
	{
		static Vector4 color = Vector4(1, 1, 1, 1);
		return color;
	}
	static const Vector4 Black()
	{
		static Vector4 color = Vector4(0, 0, 0, 1);
		return color;
	}
	static const Vector4 Yellow()
	{
		static Vector4 color = Vector4(1, 1, 0, 1);
		return color;
	}
}



