#version 450

#define MAX_LIGHTS 8

struct PointLight {
    vec3 position;
    float intensity;

    vec3 DiffuseColor;
    vec3 SpecularColor;
};

uniform PointLight pointLights[MAX_LIGHTS];
uniform uint lightCount;

uniform mat4 view;

uniform sampler2D texSampler;

in vec3 fragPos;
in vec4 fragColors;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 outColor;

vec3 lightColor = vec3(1.0, 1.0, 1.0);
float ambientStrength = 0.1;

void main()
{
    vec3 norm = normalize(fragNormal);
    vec3 V = normalize(-fragPos);

    // to be used inside a material struct.
    float shininess = 1.0;
    float opacity = 1.0;

    // ambient
    vec3 ambient = ambientStrength * lightColor;

    vec3 finalColor = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < lightCount; i++) {
        vec3 L = normalize(vec3(view * vec4(pointLights[i].position, 1.0)) - fragPos);
        vec3 R = reflect(-L, norm);

        vec3 diffuseColor = pointLights[i].intensity * pointLights[i].DiffuseColor * max(dot(L, norm), 0.0);
        vec3 specularColor =
            pointLights[i].intensity * pointLights[i].SpecularColor * pow(max(dot(V, R), 0.0), shininess);

        finalColor += diffuseColor + specularColor;
    }

    vec4 result = vec4(finalColor, opacity) * fragColors;
    outColor = texture2D(texSampler, fragTexCoord) * result;
}
