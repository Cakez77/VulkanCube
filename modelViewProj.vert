#version 330 core

// input data
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

// ModelViewProjection matrix
uniform mat4 uModelViewProjectionMatrix;

// output data
out vec3 vColor;

void main()
{
	// set vertex position
    gl_Position = uModelViewProjectionMatrix * vec4(aPosition, 1.0f);

	// set vertex shader output color 
	// will be interpolated for each fragment
	vColor = aColor;
}
