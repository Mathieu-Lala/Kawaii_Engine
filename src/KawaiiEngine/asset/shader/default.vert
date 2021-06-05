#version 450
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColors;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 fragColors;

void main()
{
    gl_Position = projection * view * model * vec4(inPos, 1.0f);
    fragColors = inColors;
}