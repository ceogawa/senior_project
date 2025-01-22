/*
Base code for deferred shading
Winter 2017, updated May 2020, May 2022- ZJW (Piddington texture write)
Press 'p' to toggle deferred shading
*/



#include <chrono>
#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "WindowManager.h"
#include "GLTextureWriter.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define NUM_LIGHTS 32

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
	vec3 light_positions[32]{};
	vec3 light_colors[NUM_LIGHTS]{};

	// Shape to be used (from obj file)
	shared_ptr<Shape> nef;

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
		
		//set up the shaders to blur the FBO just a PLACEHODLER pass thru now
		//next lab modify and possibly add other shaders to complete blur

		// TODO rewrite without texprog
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(
			resourceDirectory + "/pass_vert.glsl",
			resourceDirectory + "/tex_frag.glsl");
		texProg->init();
		texProg->addUniform("texBuf");
		texProg->addAttribute("vertPos");
		texProg->addUniform("Ldir");


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
		// add list of lights to frag shader
		deferProg->addUniform("lightPos");
		deferProg->addUniform("lightCol");
		// cam
		deferProg->addUniform("viewPos");
		// vertPos for shader. converts position to texcoord
		deferProg->addAttribute("vertPos");


		initBuffers();


	}
	
	void initBuffers() {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		//initialize the buffers -- from learnopengl.com
		glGenFramebuffers(1, &gBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

		// - position color buffer
		// TODO rewrite with createFBO()
		glGenTextures(1, &gPosition);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

		// - normal color buffer
		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

		// - color + specular color buffer
		// use alpha channel of texture to decide specular intensity
		glGenTextures(1, &gColorSpec);
		glBindTexture(GL_TEXTURE_2D, gColorSpec);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

		//glGenRenderbuffers(1, &depthBuf);
		////set up depth necessary as rendering a mesh that needs depth test
		//glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

		//more FBO set up
		GLenum DrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, DrawBuffers);


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
		// TODO rewrite light to include light color
		srand(0);
		for (int i = 0; i < 32; ++i) {
			vec3 lightPos = vec3(i, 2.0f, -2.0f);
			//vec3 lightColor = vec3(0.0f, 0.3f, 0.8f);
			//lights[i] = Light(lightPos, lightColor);
			light_positions[i] = lightPos;
			float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			light_colors[i] = vec3(r, g, b);
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
		//Nefertiti
		vector<tinyobj::shape_t> TOshapesN;
		vector<tinyobj::material_t> objMaterialsN;
		string errStr;
    	//load in the mesh and make the shape(s)
		bool rc = tinyobj::LoadObj(TOshapesN, objMaterialsN, errStr, (resourceDirectory + "/Nefertiti-100K.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			nef = make_shared<Shape>();
			nef->createShape(TOshapesN[0]);
			nef->measure();
			nef->init();
		}

		//Initialize the geometry to render a quad to the screen
		initQuad();
	}

	void cameraUpdate() {
      //camera movement - made continuous while keypressed
      float speed = 0.2;
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

		//Draw our scene - meshes - right now to a texture
		prog->bind();

		// Create the matrices
		mat4 P = SetProjectionMatrix(prog);
		mat4 V = SetView(prog);

 		//draw a circle of Nefs
		float tx, tz, theta = 0;
		mat4 transO = translate(glm::mat4(1.0f), vec3(-nef->min.x, -nef->min.y, -nef->min.z));
		mat4 scaleUnit = scale(glm::mat4(1.0f), vec3(1.0/(nef->max.x-nef->min.x)));
		for (int i = 0; i < 10; i++) {
			tx = (4.f) * sin(theta);
			tz = (4.f) * cos(theta);
			mat4 trans;
			mat4 r1, r2;
			trans = translate(glm::mat4(1.0f), vec3(tx, 0.5f, tz));
			r2 = rotate(glm::mat4(1.0f), 3.14f + theta, vec3(0, 1, 0));
			r1 = rotate(glm::mat4(1.0f), -radians(90.0f), vec3(1, 0, 0));
			SetModel(prog, trans*r2*r1*scaleUnit*transO);
			//SetMaterial(i % 4);
			SetMaterial(1);
			nef->draw(prog);
			theta += 6.28f / 10.f;
		}

		// TODO moving lights??
		for (int m = 0; m < 32; ++m) {
			light_positions[m].x += 0.5 * sin(frametime);
			light_positions[m].y += 0.2 * sin(frametime);
			light_positions[m].z += 0.1 * sin(frametime);
		}
		

		if (DEFER && !FirstTime)
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

			glUniform1i(deferProg->getUniform("gPosition"), 0);
			glUniform1i(deferProg->getUniform("gNormal"), 1);
			glUniform1i(deferProg->getUniform("gColorSpec"), 2);
			glUniform3fv(deferProg->getUniform("lightPos"), NUM_LIGHTS, &light_positions[0].x);
			glUniform3fv(deferProg->getUniform("lightCol"), NUM_LIGHTS, &light_colors[0].x);
			glUniform3f(deferProg->getUniform("viewPos"), view.x, view.y, view.z);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(0);

			deferProg->unbind();




		}
		//code to write out the FBO (texture) just once -an example
		if (DEFER && FirstTime) {
				assert(GLTextureWriter::WriteImage(gBuffer, "gBuf.png"));
				assert(GLTextureWriter::WriteImage(gPosition, "gPos.png"));
				assert(GLTextureWriter::WriteImage(gNormal, "gNorm.png"));
				assert(GLTextureWriter::WriteImage(gColorSpec, "gColorSpec.png"));

				//// render the texture to the framebuffer
				//glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
				//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				//texProg->bind();
				//glActiveTexture(GL_TEXTURE0);
				//glBindTexture(GL_TEXTURE_2D, gPosition);
				//glUniform1i(texProg->getUniform("texBuf"), 0);
				//glUniform3f(texProg->getUniform("Ldir"), g_light.x, g_light.y, g_light.z);
				//glEnableVertexAttribArray(0);
				//glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
				//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				//glDrawArrays(GL_TRIANGLES, 0, 6);
				//glDisableVertexAttribArray(0);
				//texProg->unbind();
			

				FirstTime = false;
		}
	}
	

	// To complete image processing on the specificed texture
	// Right now just draws large quad to the screen that is texture mapped
	// with the prior scene image - next lab we will process
	void DrawQuad(GLuint inTex) {

		// example applying of 'drawing' the FBO texture - change shaders
		texProg->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, inTex);
		glUniform1i(texProg->getUniform("texBuf"), 0);
		glUniform3f(texProg->getUniform("Ldir"), 1, -1, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
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
 	/* model transforms - normal */
	void SetModel(shared_ptr<Program> curS, mat4 m) {
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(m));
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
		if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			DEFER = !DEFER;
		}
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

