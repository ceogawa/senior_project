#version 330 core
out vec4 FragColor;
in vec2 texCoord;

in vec3 lightFragPos;
in vec3 fragNor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColorSpec;
uniform sampler2D lightMap;
uniform vec3 camPos;



void main() {
    vec3 sceneFragPos = texture(gPosition, texCoord).rgb;
    vec3 Normal = texture(gNormal, texCoord).rgb;
    vec3 Albedo = texture(gColorSpec, texCoord).rgb;
    float Spec = texture(gColorSpec, texCoord).a;
    vec3 Light = texture(lightMap, texCoord).rgb; // black and white image

    // float depth = texture(positionBuffer, uv).z;  // If using world space???
    float stencil = 0.0; // initialize stencil to zero
    if (lightFragPos.z <= sceneFragPos.z){
        stencil = 1.0;
    }

} 