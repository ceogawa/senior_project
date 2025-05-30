#version  330 core
layout(location = 0) in vec4 vertPos;

out vec2 texCoord;

void main()
{
	texCoord = (vertPos.xy+vec2(1, 1))/2.0;
	// already in NDC
	gl_Position = vertPos;

}

