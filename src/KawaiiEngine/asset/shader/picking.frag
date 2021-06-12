#version 450

out vec4 outColor;

// Values that stay constant for the whole mesh.
uniform vec4 object_color;

void main() { outColor = object_color; }
