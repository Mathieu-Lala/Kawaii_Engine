#version 450

layout(location = 0) in vec3 inPos;
layout(location = 3) in vec3 inNormals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 fragNormal;

void main()
{
    gl_Position = projection * view * model * vec4(inPos, 1.0f);
    fragPos = vec3(view * model * vec4(inPos, 1.0));
    fragNormal = mat3(transpose(inverse(view * model))) * inNormals;
}
