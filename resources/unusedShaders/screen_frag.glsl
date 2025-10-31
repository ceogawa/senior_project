#version 330 core
in vec2 texCoord;

uniform sampler2D lightAccumulation;

out vec4 FragColor;

void main() {

    //vec3 texColor = texture(lightAccumulation, texCoord).rgb;
    //FragColor = vec4(texColor.r, texColor.g, texColor.b, 1.0);
    //FragColor = vec4(0.2, 0.4, 0.6, 1.0);

    FragColor = texture(lightAccumulation, texCoord);
} 