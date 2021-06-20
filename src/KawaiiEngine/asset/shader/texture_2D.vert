#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColors;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec4 fragColors;
out vec2 fragTexCoord;
out vec3 fragNormal;

void main()
{
    gl_Position = projection * view * model * vec4(inPos, 1.0f);
    fragPos = vec3(view * model * vec4(inPos, 1.0));
    fragColors = inColors;
    fragTexCoord = inTexCoord;
    fragNormal = mat3(transpose(inverse(view * model))) * inNormals;
}
