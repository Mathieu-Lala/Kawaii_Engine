#version 450

uniform sampler2D texSampler;

in vec4 fragColors;
in vec2 fragTexCoord;

out vec4 outColor;

void main() { outColor = texture(texSampler, fragTexCoord) * fragColors; }
