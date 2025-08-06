#version  330 core
layout(location = 0) in vec4 vertPos;
//layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//out vec3 lightFragNor;
out vec3 lightFragPos;
out vec2 texCoord;

void main()
{
	// TODO CHANGEEEEEE
	// check the texCood/glPosition
	texCoord = (vertPos.xy+vec2(1, 1))/2.0;
	gl_Position = P * V * M * vertPos;
	//lightFragNor = (V* M * vec4(vertNor, 0.0)).xyz;
	lightFragPos = vec3(V* M * vertPos);
}

