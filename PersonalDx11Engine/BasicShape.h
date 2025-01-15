#pragma once
#include "Mesh.h"

class FBasicShape
{
public:
	
	static const FVertexSimple GetTriangle() { 
		//lazy initialzie
		static const FVertexSimple TriangleVertices[]=
		{
			{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
			{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right vertex (green)
			{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }  // Bottom-left vertex (blue)
		};
	}
};