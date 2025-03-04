#version  330 core
layout(location = 0) in vec4 vertPos;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec2 texCoord;

void main()
{
	texCoord = (vertPos.xy+vec2(1, 1))/2.0;
	gl_Position = P * V * M * vertPos;
}

