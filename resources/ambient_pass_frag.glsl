#version 330 core
out vec4 FragColor;
in vec2 texCoord;

uniform sampler2D gColorSpec;


void main() {

    float intensity = 10.0;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    vec3 lighting = Albedo * intensity;

    FragColor = vec4(lighting, 1.0);
} 