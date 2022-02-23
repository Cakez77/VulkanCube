#version 330 core

// interpolated values from the vertex shaders
in vec3 vColor;

// output data
out vec3 fColor;

void main()
{
	// set output color
	fColor = vColor;
}
