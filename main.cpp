/*
Base code for deferred shading
Winter 2017, updated May 2020, May 2022- ZJW (Piddington texture write)
Press 'p' to toggle deferred shading
*/

// learnopengl for deferred assistance



#include <chrono>
#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "WindowManager.h"
#include "GLTextureWriter.h"
#include "initGeom.h"
#include "stb_image.h"
#include "stb_image_write.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define NUM_LIGHTS 16

using namespace std;
using namespace glm;
enum Mat {jade=0, brass, copper, grey, tone1, tone2, tone3, tone4, turquoise, shadow};

struct Light {
	vec3 Position;
	vec3 Color;
};



class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;
	std::shared_ptr<Program> texProg;
	std::shared_ptr<Program> deferProg;
	vec3 light_positions[NUM_LIGHTS]{};
	vec3 light_colors[NUM_LIGHTS]{};

	vector<shared_ptr<Shape>> bookshelf;
	vector<shared_ptr<Shape>> sofa;
	vector<shared_ptr<Shape>> coffeetable;
	vector<shared_ptr<Shape>> lamp;
	shared_ptr<Shape> wall;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	//geometry for texture render
	GLuint quad_VertexArrayID;
	GLuint quad_vertexbuffer;

	//reference to texture FBO
	//reference to texture FBO
	GLuint gBuffer = 0;      // Initialize to 0 (or any value you prefer)
	GLuint gPosition = 0;
	GLuint gNormal = 0;
	GLuint gColorSpec = 0;
	GLuint depthBuf = 0;
	GLuint lightPositions = 0;
	GLuint lightColors = 0;
	GLuint lightMap = 0;


	bool FirstTime = true;
	bool DEFER = true;
	int gMat = 0;

	//camera control - you can ignore - what matters is eye location and view matrix
	double g_phi, g_theta;
	vec3 view = vec3(0, 0, 1);
	vec3 strafe = vec3(1, 0, 0);
	vec3 g_eye = vec3(0, 1, 0);
	vec3 g_lookAt = vec3(0, 1, -1); 
	bool MOVEF = false;
	bool MOVEB = false;
	bool MOVER = false;
	bool MOVEL = false;

	vec3 g_light = vec3(2, 6, 6);

	void initGL(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		g_phi = 0;
		g_theta = -3.14/2.0;

		// Set background color.
		glClearColor(0.01, 0.01, 0.01, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);

		prog->setShaderNames(
			resourceDirectory + "/geometry_pass_vert.glsl",
			resourceDirectory + "/geometry_pass_frag.glsl");
		prog->init();
		// uniforms for geometry pass
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		// attributes for geom pass
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		// vert shader passes
		//		vec3 fragPos;
		//		vec3 fragNor; 
		// to the geometry frag shader
		

		// TODO texprog? modify to pass color to defer
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(
			resourceDirectory + "/pass_vert.glsl",
			resourceDirectory + "/tex_frag.glsl");
		texProg->init();
		texProg->addUniform("texBuf");
		texProg->addAttribute("vertPos");
		//texProg->addUniform("Ldir");


		// deferred shader init
		deferProg = make_shared<Program>();
		deferProg->setVerbose(true);
		deferProg->setShaderNames(
			resourceDirectory + "/deferred_vert.glsl",
			resourceDirectory + "/deferred_frag.glsl"
		);
		deferProg->init();
		// add gbuffer uniforms to frag shader
		deferProg->addUniform("gPosition");
		deferProg->addUniform("gNormal");
		deferProg->addUniform("gColorSpec");
		// replace list of lights with lightmap
		deferProg->addUniform("lightMap");
		// cam
		deferProg->addUniform("camPos");
		// vertPos for shader. converts position to texcoord
		deferProg->addAttribute("vertPos");


		initBuffers();


	}

	void createBuffer(GLuint *buffer, int width, int height, GLenum attachment) {
		glGenTextures(1, buffer);
		glBindTexture(GL_TEXTURE_2D, *buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, *buffer, 0);

	}
	
	void initBuffers( ) {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		glGenFramebuffers(1, &gBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

		// - position color buffer
		// TODO rewrite with createFBO()
		createBuffer(&gPosition, width, height, GL_COLOR_ATTACHMENT0);
		// - normal color buffer
		createBuffer(&gNormal, width, height, GL_COLOR_ATTACHMENT1);
		// - color + specular color buffer
		// use alpha channel of texture to decide specular intensity
		createBuffer(&gColorSpec, width, height, GL_COLOR_ATTACHMENT2);

		//more FBO set up
		GLenum DrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, DrawBuffers);

		int lmap_width, lmap_height, nrChannels;
		// "../resources/textures/light_map.png"
		// ../resources/textures/light_map_with_depth1.jpg
		unsigned char* lmap_data = stbi_load("../resources/textures/light_map_with_depth1.jpg", &lmap_width, &lmap_height, &nrChannels, STBI_rgb);

		glGenTextures(1, &lightMap);
		glBindTexture(GL_TEXTURE_2D, lightMap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (lmap_data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lmap_width, lmap_height, 0, GL_RGB, GL_UNSIGNED_BYTE, lmap_data);
			glGenerateMipmap(GL_TEXTURE_2D);  // Generate mipmaps
		}
		else {
			std::cerr << "Failed to load lightmap texture: " << stbi_failure_reason() << std::endl;
		}

		//cout << "lmap_data[0]" << lmap_data[0] << endl;
		stbi_image_free(lmap_data);


		//// load in light map data and texture
		//int lmap_width, lmap_height, nrChannels;
		//unsigned char* lmap_data = stbi_load("../resources/textures/light_map_ALL_WHITE.jpg", &lmap_width, &lmap_height, &nrChannels, STBI_rgb);

		////unsigned char* lmap_data = stbi_load("../resources/textures/light_map_ALL_WHITE.jpg", &lmap_width, &lmap_height, &nrChannels, 0);

		//glGenTextures(1, &lightMap);
		//glBindTexture(GL_TEXTURE_2D, lightMap);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//if (lmap_data)
		//{
		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lmap_width, lmap_height, 0, GL_RGB, GL_UNSIGNED_BYTE, lmap_data);
		//	//         texture_target mipmap            w          h     legacy 0        img data type    img data
		//}
		//else
		//{
		//	std::cout << "Failed to load lightmap texture" << std::endl;
		//}
		//stbi_image_free(lmap_data);

		glGenRenderbuffers(1, &depthBuf);
		//set up depth necessary as rendering a mesh that needs depth test
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
		// error check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		initLights();


	}


	void initLights() {
		//Light lights[32]{};
		// TODO determine color in artistic way
		srand(0);
		for (int i = 0; i < NUM_LIGHTS; ++i) {
			//vec3 lightPos = vec3(i, 8.0f, -2.0f);
			float max_number = 13.0f;
			float minimum_number = -10.0f; 
			vec3 lightPos;
			if (i % 2 == 0) {
				lightPos = vec3(rand() % 5, 5.0f, -3.0f);
			}
			else {
				lightPos = vec3(rand() % 5, 0.0f, (rand() % 10));
			}
			light_positions[i] = lightPos;
			float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			//light_colors[i] = vec3(r, g, b);
			light_colors[i] = vec3(0.7f);
			cout << "light colors: " << r << ", " << g << ", " << b << endl;
		}
	}

	void createFBO(GLuint& fb, GLuint& tex) {
		//initialize FBO
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		//set up framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		//set up texture
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			cout << "Error setting up frame buffer - exiting" << endl;
			exit(0);
		}
	}

	void initGeom(const std::string& resourceDirectory)
	{

		sofa = initMultiMesh("/objs/sofa.obj", sofa);
		bookshelf = initMultiMesh("/objs/bookcase.obj", bookshelf);
		coffeetable = initMultiMesh("/objs/table2.obj", coffeetable);
		wall = initMesh("/objs/wall.obj", wall);
		lamp = initMultiMesh("/objs/desk_lamp.obj", lamp);

		//Initialize the geometry to render a quad to the screen
		initQuad();
	}

	void cameraUpdate() {
      //camera movement - made continuous while keypressed
      float speed = 0.1;
      if (MOVEL){
        g_eye -= speed*strafe;
        g_lookAt -= speed*strafe;
      } else if (MOVER) {
        g_eye += speed*strafe;
        g_lookAt += speed*strafe;
      } else if (MOVEF) {
        g_eye -= speed*view;
        g_lookAt -= speed*view;
      } else if (MOVEB) {
        g_eye += speed*view;
        g_lookAt += speed*view;
      }
    }
	

	void drawGeometry() {
		prog->bind();

		// Create projection and view matricies
		mat4 P = SetProjectionMatrix(prog);
		mat4 V = SetView(prog);

		// set model sets up Model matrix

		// SOFA
		for (int i = 0; i < sofa.size(); i++) {
			/*mat4 scaleUnit = scale(mat4(1.0f), vec3(1.0 / (sofa[i]->max.x - sofa[i]->min.x)));
			mat4 trans = translate(glm::mat4(1.0f), vec3(i, 0.5f, -1.0f));*/
			SetModel(prog, vec3(0, 0.3f, -1.0f), 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
			SetMaterial(0);
			sofa[i]->draw(prog);
		}

		// COFFEE TABLE
		for (int i = 0; i < coffeetable.size(); i++) {
			SetModel(prog, vec3(1.0f, 0.3f, 0.9f), 0.0f, 0.0f, 0.012f, 0.009f, 0.011f);
			SetMaterial(2);
			coffeetable[i]->draw(prog);
		}

		// LAMP
		for (int i = 0; i < lamp.size(); i++) {
			SetModel(prog, vec3(1.4f, 0.7f, 0.9f), 3.14f , 0.0f, 2.0f, 2.0f, 2.0f);
			SetMaterial(3);
			lamp[i]->draw(prog);
		}

		//left wall
		SetModel(prog, vec3(-4.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.9f, 0.55f, 0.9f);
		SetMaterial(1);
		wall->draw(prog);

		//right wall
		SetModel(prog, vec3(4.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.9f, 0.55f, 0.9f);
		SetMaterial(1);
		wall->draw(prog);

		//back wall
		SetModel(prog, vec3(0.0f, 0.0f, -4.0f), 1.57f, 0.0f, 1.5f, 0.55f, 0.9f);
		SetMaterial(1);
		wall->draw(prog);

		// floor
		SetModel(prog, vec3(0.0f, 0.0f, -4.0f), 1.57f, 1.57f, 1.5f, 1.2f, 0.9f);
		SetMaterial(1);
		wall->draw(prog);

		// ceiling??


		// unbind after geometry pass
		prog->unbind();

	}


	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		 //camera movement - made continuous while keypressed
		cameraUpdate();

		if (DEFER)
		{
			// bind to gBuffer so the prog shader will write geometry to gbuffer for deferred pass
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
		float aspect = width/(float)height;

		// set up model, view, projection matrices
		drawGeometry();

		
		// TODO moving lights??
		/*	for (int m = 0; m < 32; ++m) {
			light_positions[m].x += 0.5 * sin(frametime);
			light_positions[m].y += 0.2 * sin(frametime);
			light_positions[m].z += 0.1 * sin(frametime);
		}*/
		

		if (!FirstTime)
		{
			// now draw the actual output 
			// render to the screen LIGHTING PASS
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			deferProg->bind();
			// bind textures for pos, normals, color
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gColorSpec);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, lightMap);


			glUniform1i(deferProg->getUniform("gPosition"), 0);
			glUniform1i(deferProg->getUniform("gNormal"), 1);
			glUniform1i(deferProg->getUniform("gColorSpec"), 2);

			GLint loc = deferProg->getUniform("lightMap");
			if (loc == -1) { std::cerr << "lightMap uniform not found!" << std::endl; }
			glUniform1i(deferProg->getUniform("lightMap"), 3);

			glUniform3fv(deferProg->getUniform("lightPos"), NUM_LIGHTS, &light_positions[0].x);
			glUniform3fv(deferProg->getUniform("lightCol"), NUM_LIGHTS, &light_colors[0].x);
			glUniform3f(deferProg->getUniform("camPos"), g_eye.x, g_eye.y, g_eye.z);
			glUniform3f(deferProg->getUniform("viewPos"), view.x, view.y, view.z);
			glUniform3f(deferProg->getUniform("lookAt"), g_lookAt.x, g_lookAt.y, g_lookAt.z);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(0);

			deferProg->unbind();

		}
		//code to write out the FBO (texture) just once -an example
		if (FirstTime) {
				assert(GLTextureWriter::WriteImage(gBuffer, "gBuf.png"));
				assert(GLTextureWriter::WriteImage(gPosition, "gPos.png"));
				assert(GLTextureWriter::WriteImage(gNormal, "gNorm.png"));
				assert(GLTextureWriter::WriteImage(gColorSpec, "gColorSpec.png"));
				FirstTime = false;
		}
	}
	
	void DrawQuad(GLuint inTex) {

		// example applying of 'drawing' the FBO texture - change shaders
		texProg->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, inTex);
		glUniform1i(texProg->getUniform("texBuf"), 0);
		glUniform3f(texProg->getUniform("Ldir"), 1, -1, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(0);
		texProg->unbind();
	}

	/* helper functions for sending matrix data to the GPU */
	mat4 SetProjectionMatrix(shared_ptr<Program> curShade) {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		mat4 Projection = perspective(radians(50.0f), aspect, 0.1f, 100.0f);
		glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(Projection));
		return Projection;
	}

	void SetModel(shared_ptr<Program> curS, vec3 trans, float rotY, float rotX, float sx, float sy, float sz) {
		mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
		mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
		mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sx, sy, sz));
		mat4 ctm = Trans * RotX * RotY * ScaleS;
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	/*normal game camera */
	mat4 SetView(shared_ptr<Program> curShade) {
		mat4 Cam = lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
		glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
		return Cam;
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
    	cout << "use two finger mouse scroll" << endl;
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	/**** geometry set up for a quad *****/
	void initQuad() {
		//now set up a simple quad for rendering FBO
		glGenVertexArrays(1, &quad_VertexArrayID);
		glBindVertexArray(quad_VertexArrayID);

		static const GLfloat g_quad_vertex_buffer_data[] =
		{
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			1.0f,  1.0f, 0.0f,
		};

		static const GLfloat left_wall_data[] =
		{  
			0.0f, 1.0f, -1.0f,
			0.0f, -1.0f, 1.0f,
			0.0f, 1.0f, 1.0f,
			0.0f, 1.0f, 1.0f,
			0.0f, -1.0f, 1.0f,
			0.0f, 1.0f, -1.0f,
		};

		glGenBuffers(1, &quad_vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Unbind VAO
		glBindVertexArray(0);
	}

	/* much of the camera is here */
	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		vec3 diff, newV;

		g_theta += deltaX*0.25;
		g_phi += deltaY*0.25;
		newV.x = cosf(g_phi)*cosf(g_theta);
		newV.y = -1.0*sinf(g_phi);
		newV.z = 1.0*cosf(g_phi)*cosf((3.14/2.0-g_theta));
		diff.x = (g_lookAt.x - g_eye.x) - newV.x;
		diff.y = (g_lookAt.y - g_eye.y) - newV.y;
		diff.z = (g_lookAt.z - g_eye.z) - newV.z;
		g_lookAt.x = g_lookAt.x - diff.x;
		g_lookAt.y = g_lookAt.y - diff.y;
		g_lookAt.z = g_lookAt.z - diff.z;
		view = g_eye - g_lookAt;
		strafe = cross(vec3(0, 1,0), view);
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			MOVEL = true;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			MOVER = true;
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS) {
			MOVEF = true;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			MOVEB = true;
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
			g_light.x += 0.5; 
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
			g_light.x -= 0.5; 
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		/*if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			DEFER = !DEFER;
		}*/
		if (action == GLFW_RELEASE){
			MOVER = MOVEF = MOVEB = MOVEL = false;
		}
	}

	// helper function to set materials for shading
	void SetMaterial(int i)
	{
		switch (i) {
		case 0: //shiny blue plastic
		glUniform3f(prog->getUniform("MatAmb"), 0.02f, 0.04f, 0.2f);
		glUniform3f(prog->getUniform("MatDif"), 0.0f, 0.16f, 0.9f);
		break;
		case 1: // flat grey
		glUniform3f(prog->getUniform("MatAmb"), 0.13f, 0.13f, 0.14f);
		glUniform3f(prog->getUniform("MatDif"), 0.3f, 0.3f, 0.4f);
		break;
		case 2: //brass
		glUniform3f(prog->getUniform("MatAmb"), 0.3294f, 0.2235f, 0.02745f);
		glUniform3f(prog->getUniform("MatDif"), 0.7804f, 0.5686f, 0.11373f);
		break;
		 case 3: //copper
		 glUniform3f(prog->getUniform("MatAmb"), 0.1913f, 0.0735f, 0.0225f);
		 glUniform3f(prog->getUniform("MatDif"), 0.7038f, 0.27048f, 0.0828f);
		 break;
		}
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(960, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	// setup shaders, setup gbuffer, create and attach depth buffer
	application->initGL(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = std::chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
		chrono::duration_cast<std::chrono::microseconds>(
			chrono::high_resolution_clock::now() - lastTime)
		.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

