#version 450

// input data
layout(location = 0) in vec3 vColor;

// output data
layout(location = 0) out vec3 fColor;

void main()
{
	// set output color
	fColor = vColor;
}
