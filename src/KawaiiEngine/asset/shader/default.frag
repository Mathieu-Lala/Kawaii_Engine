#version 450

in vec3 fragPos;
in vec4 fragColors;
in vec3 fragNormal;
in vec3 fragLightPos;

out vec4 outColor;

vec3 lightColor = vec3(1.0, 1.0, 1.0);
float ambientStrength = 1;
float specularStrength = 0.5;

void main()
{
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(fragLightPos - fragPos);
    vec3 viewDir = normalize(-fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    // ambient
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec4 result = vec4(ambient + diffuse + specular, 1.0) * fragColors;
    outColor = result;
}
