#version 430 core
layout(location = 0) out vec4 FragColor;
layout(binding = 0) uniform sampler2D Front;
layout(binding = 1) uniform sampler2D Back;
in InOutVars
{
    vec2 TexCoord;
} inData;


void main()
{
    vec3 frontCoordinate = texture(Front, inData.TexCoord).xyz;
    vec3 backCoordinate = texture(Back, inData.TexCoord).xyz;

    vec3 rayDirection = backCoordinate-frontCoordinate;

    vec3 color = rayDirection;
    FragColor = vec4(color, 1.0);
}
