#version 330 core
out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D gColorSpec;
uniform sampler2D gNormal;
uniform vec3 lightDir;

void main() {

    float intensity = 0.6;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    vec3 Normal = texture(gNormal, texCoord).rgb;

    vec3 normal = vec3(-1.0) + 2.0*Normal.rgb;
    vec3 diffuse = max(dot(normalize(lightDir), normalize(normal)), 0.0) * Albedo; //* lightCol * Albedo;

    vec3 lighting = diffuse * intensity;
    FragColor = vec4(lighting, 1.0);
    

} 