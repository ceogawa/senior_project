#version 330 core 
in vec3 fragNor;
in vec3 WPos;
//to send the color to a frame buffer color attachment 0
layout(location = 0) out vec4 color;

/* frame buffer to write out only normals for lights*/
void main()
{
	vec3 normal = normalize(fragNor);
	//shift the normal as images 0-1 (reshift on read out)
	color = vec4(0.5f*(normal+vec3(1.0)), 1.0);
}
