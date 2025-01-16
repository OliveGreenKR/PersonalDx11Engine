#pragma once

//based on d3d coordinates
struct FVector
{
	FVector() = default;
	FVector(float a, float b, float c) : x(a), y(b), z(c) {};
	
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	__forceinline FVector operator+ (const FVector& V) const
	{
		return FVector(x + V.x, y + V.y, z + V.z);
	}

	__forceinline void operator+= (const FVector & other)
	{
		x += other.x; y += other.y; z += other.z;
	}
	__forceinline void operator*= (const float val)
	{
		x *= val; y *= val; z *= val;
	}


	static const FVector Zero;
	static const FVector Up;
	static const FVector Forward;
	static const FVector Right;

};

struct FVector4
{
	FVector4() = default;
	FVector4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {};
	FVector4(const FVector& V) : x(V.x), y(V.y), z(V.z), w(0.0f) {};

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;

	__forceinline FVector4 operator+ (const FVector4& V) const
	{
		return FVector4(x + V.x, y + V.y, z + V.z, w +V.w);
	}

	__forceinline void operator+= (const FVector4& other)
	{
		x += other.x; y += other.y; z += other.z; w += other.w;
	}

	static const FVector4 Zero;
};

static FVector operator* (const FVector& a, const float b)
{
	return FVector(a.x * b, a.y * b, a.z * b);
}
static __forceinline FVector operator* (float a, const FVector& b)
{
	return b * a;
}