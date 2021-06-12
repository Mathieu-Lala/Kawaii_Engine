#version 450

in vec3 fragPos;
in vec3 fragNormal;

out vec4 outColor;

void main()
{
    vec3 norm = normalize(fragNormal);

    outColor = vec4(norm, 1.0);
}
