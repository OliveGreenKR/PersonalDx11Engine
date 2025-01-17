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


	static const FVector Zero()
	{
		const static FVector zero(0.0f, 0.0f, 0.0f);
		return zero;
	}

	static const FVector Up()
	{
		const static FVector up(0.0f, 1.0f, 0.0f);
		return up;
	}
	static const FVector Forward()
	{
		const static FVector forward(0.0f, 0.0f, 1.0f);
		return forward;
	}
	static const FVector Right()
	{
		const static FVector right(1.0f, 0.0f, 0.0f);
		return right;
	}

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

	static const FVector4 Zero()
	{
		const static FVector4 zero(0.0f, 0.0f, 0.0f, 0.0f);
		return zero;
	}
};

static FVector operator* (const FVector& a, const float b)
{
	return FVector(a.x * b, a.y * b, a.z * b);
}
static __forceinline FVector operator* (float a, const FVector& b)
{
	return b * a;
}