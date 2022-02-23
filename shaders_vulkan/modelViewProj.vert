#version 450

// input data
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

// output data
layout(location = 0) out vec3 vColor;

// ModelViewProjection matrix
layout(set = 0, binding = 0) uniform GlobalUBO
{
	mat4 MVPMatrix;
};

void main()
{
	// set vertex position
    gl_Position = MVPMatrix * vec4(aPosition, 1.0);

	// set vertex shader output color 
	// will be interpolated for each fragment
	vColor = aColor;
}