/*
Base code for deferred shading
Winter 2017, updated May 2020, May 2022- ZJW (Piddington texture write)
Press 'p' to toggle deferred shading
*/

// learnopengl for deferred assistance

// TODO replace room models with new table

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
	std::shared_ptr<Program> norProg;
	std::shared_ptr<Program> deferProg;
	shared_ptr<Program> frontProg;
	shared_ptr<Program> backProg;
	shared_ptr<Program> ambientProg;
	shared_ptr<Program> volumesNoCullingProg;
	shared_ptr<Program> screenProg;

	vector<Light> lights;

	vector<shared_ptr<Shape>> bookshelf;
	vector<shared_ptr<Shape>> sofa;
	vector<shared_ptr<Shape>> coffeetable;
	vector<shared_ptr<Shape>> lamp;
	shared_ptr<Shape> wall;
	shared_ptr<Shape> lightVolume;

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
	GLuint lightDepthBuf = 0; 
	GLuint lightPositions = 0;
	GLuint lightColors = 0;
	GLuint lightMap = 0;
	GLuint lightAccumulationBuf = 0;
	GLuint lightAccumulationTexture = 0; 

	// TODO not using light radius??
	float light_radius = 0.09;


	bool FirstTime = true;
	bool DEFER = true;
	int gMat = 0;

	//camera control - you can ignore - what matters is eye location and view matrix
	double g_phi, g_theta;
	vec3 view = vec3(0, 0, 1);
	vec3 strafe = vec3(1, 0, 0);
	vec3 g_eye = vec3(0, 1, 4);
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

		frontProg = make_shared<Program>();
		frontProg->setVerbose(true);
		frontProg->setShaderNames(
			resourceDirectory + "/pointlight_front_vert.glsl",
			resourceDirectory + "/pointlight_front_frag.glsl"
		);
		frontProg->init();
		frontProg->addUniform("P");
		frontProg->addUniform("V");
		frontProg->addUniform("M");
		frontProg->addAttribute("vertPos");
		//frontProg->addAttribute("vertNor");

		backProg = make_shared<Program>();
		backProg->setVerbose(true);
		backProg->setShaderNames(
			resourceDirectory + "/pointlight_back_vert.glsl",
			resourceDirectory + "/pointlight_back_frag.glsl"
		);
		backProg->init();
		// add gbuffer uniforms to frag shader
		backProg->addUniform("gPosition");
		backProg->addUniform("gNormal");
		backProg->addUniform("gColorSpec");
		backProg->addUniform("lightPos");
		backProg->addUniform("lightCol");
		// TODO readded lightmap (sampling normals from here)
		backProg->addUniform("lightMap");
		backProg->addUniform("P");
		backProg->addUniform("V");
		backProg->addUniform("M");
		backProg->addUniform("resolution");
		backProg->addAttribute("vertPos");

		ambientProg = make_shared<Program>();
		ambientProg->setVerbose(true);
		ambientProg->setShaderNames(
			resourceDirectory + "/ambient_pass_vert.glsl",
			resourceDirectory + "/ambient_pass_frag.glsl"
		);
		ambientProg->init();
		ambientProg->addAttribute("vertPos"); 
		ambientProg->addUniform("gColorSpec");
		// ambientProg->addUniform("gNormal");
		// ambientProg->addUniform("lightDir");

		// Light Volumes Deferred Render Pass, No Stencil Culling
		volumesNoCullingProg = make_shared<Program>();
		volumesNoCullingProg->setVerbose(true);
		volumesNoCullingProg->setShaderNames(
			resourceDirectory + "/screen_vert.glsl",
			resourceDirectory + "/lightvolume_no_culling_frag.glsl"
		);
		volumesNoCullingProg->init();
		// add gbuffer uniforms to frag shader
		volumesNoCullingProg->addUniform("gPosition");
		volumesNoCullingProg->addUniform("gNormal");
		volumesNoCullingProg->addUniform("gColorSpec"); 
		volumesNoCullingProg->addUniform("lightBuf");
		//volumesNoCullingProg->addUniform("lightPos");
		//volumesNoCullingProg->addUniform("lightCol");
		volumesNoCullingProg->addAttribute("vertPos");
		//volumesNoCullingProg->addAttribute("vertNor");

		// Initialize the GLSL program.
		norProg = make_shared<Program>();
		norProg->setVerbose(true);
		norProg->setShaderNames(
			resourceDirectory + "/simple_vert.glsl",
			resourceDirectory + "/nor_frag.glsl");
		if (!norProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		norProg->addUniform("P");
		norProg->addUniform("M");
		norProg->addUniform("V");
		norProg->addAttribute("vertPos");
		norProg->addAttribute("vertNor");

		initBuffers();

	}

	void createBuffer(GLuint *buffer, int width, int height, GLenum attachment, GLuint level) {
		glGenTextures(1, buffer);
		glBindTexture(GL_TEXTURE_2D, *buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, *buffer, level);
	}
	
	void initBuffers( ) {
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height); 

		glGenFramebuffers(1, &gBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

		// - position color buffer
		// identical buffer creation AS BEFORE/PREVIOUSLY
		createBuffer(&gPosition, width, height, GL_COLOR_ATTACHMENT0, 0);
		// - normal color buffer
		createBuffer(&gNormal, width, height, GL_COLOR_ATTACHMENT1, 0);
		// - color + specular color buffer
		// use alpha channel of texture to decide specular intensity
		createBuffer(&gColorSpec, width, height, GL_COLOR_ATTACHMENT2, 0); 

		GLenum check = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (check != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "ERROR: GBUFFER is not complete! " << check << std::endl;
		}
		// Just reuse gDepthBuf

		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

		//////////////////////////////////////////////////////////////////////////////////////////////
		glGenRenderbuffers(1, &depthBuf);
		//set up depth necessary as rendering a mesh that needs depth test
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
		// error check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;

		//more FBO set up
		GLenum DrawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, DrawBuffers);


		///////////////////////////////////////////////////////////////////////////////////////////////
        // generate the light accumulation buffer
		glGenFramebuffers(1, &lightAccumulationBuf);
		glGenTextures(1, &lightAccumulationTexture);
		// do I call gen again?
		glGenRenderbuffers(1, &depthBuf);

		// PREVIOUSLY
		// bind to buffer
		//glBindFramebuffer(GL_FRAMEBUFFER, lightAccumulationBuf);k
		//createBuffer(&lightAccumulationTexture, width, height, GL_COLOR_ATTACHMENT0, 0); 
				//glGenTextures(1, buffer);
				//glBindTexture(GL_TEXTURE_2D, *buffer);
				//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				//glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, *buffer, level);

		// BASE CODE
		createFBO(lightAccumulationBuf, lightAccumulationTexture);
		//set up depth necessary since we are rendering a mesh that needs depth test
		//yes need this for lights if you want their full geom with depth resolved
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);


		GLenum DrawBuffer[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// DO I NEED THIS?
		//createFBO(LframeBuf[1], LtexBuf[1]); ??

		initLights();

	}

	float niceRandom(){
		return -1.0 + 2.0 * (float(rand()) / float(RAND_MAX));
	}

	void initLights() {

		// TODO WIP light placement
	    // ambient ceiling lights
		lights.push_back({vec3(0.0f, 2.0f, -1.0f), vec3(1.0f, 0.9f, 0.8f)});
		lights.push_back({vec3(-2.0f, 2.0f, -1.0f), vec3(1.0f, 0.9f, 0.8f)});
		lights.push_back({vec3(2.0f, 2.0f, -1.0f), glm::vec3(1.0f, 0.9f, 0.8f)});
		for (int i = 0; i < 1000; i++){
			lights.push_back({vec3(niceRandom()*10 - 5, niceRandom(), niceRandom()*3 - 1), vec3(1.0f, 1.0f, 1.0f)});
		}

		// lights behind couch
		for (float x = -1.5f; x <= 1.5f; x += 0.75f) {
			lights.push_back({glm::vec3(x, 0.8f, -2.0f), glm::vec3(0.3f, 0.7f, 1.0f)}); // Cool blue
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

		//Initialize the geometry to render a quad to the screen
		initQuad();

		sofa = initMultiMesh("/objs/sofa.obj", sofa); 
		bookshelf = initMultiMesh("/objs/bookcase.obj", bookshelf);
		coffeetable = initMultiMesh("/objs/table2.obj", coffeetable);
		wall = initMesh("/objs/wall.obj", wall);
		lamp = initMultiMesh("/objs/desk_lamp.obj", lamp);
		// use sphere mesh to represent light volume
		lightVolume = initMesh("/objs/smoothSphere.obj", lightVolume);

	}

	void cameraUpdate() {
      //camera movement - made continuous while keypressed
      float speed = 0.05;
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

		//SetModel(prog, )

		// ceiling??

		// unbind after geometry pass
		prog->unbind();

	}

	// void render(float frametime) {
	// 	// Get current frame buffer size.
	// 	int width, height;
	// 	glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
	// 	glViewport(0, 0, width, height);

	// 	 //camera movement - made continuous while keypressed
	// 	cameraUpdate();

    //     // bind to gBuffer so the prog shader will write geometry to gbuffer for deferred pass
	// 	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	// 	// Clear framebuffer.
	// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
	// 	// GEOMETRY PASS (sets up gbuffer for scene)
	// 	drawGeometry();

	// 	// PREVIOUSLY AND BASE CODE
	// 	// bind the framebuffer to the lightAccumulation frame buffer, so that each light's computations are additively blended
	// 	glBindFramebuffer(GL_FRAMEBUFFER, lightAccumulationBuf);
	// 	// clear the color buffer before all light computations
	// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	// 	norProg->bind();
	// 	mat4 P = SetProjectionMatrix(norProg); 
	// 	mat4 V = SetView(norProg);

	// 	// simplifies because we are only writing the normal data of the lights to the framebuffer
	// 	for (const Light& light : lights) {
	// 		//glUniform3f(volumesNoCullingProg->getUniform("lightPos"), light.Position.x, light.Position.y, light.Position.z);
	// 		//glUniform3f(volumesNoCullingProg->getUniform("lightCol"), light.Color.r, light.Color.g, light.Color.b);
	// 		SetModel(norProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
	// 		lightVolume->draw(norProg);
	// 	}
	// 	norProg->unbind();

	// 	//glCullFace(GL_BACK);  // Restore normal culling
	// 	//glDepthFunc(GL_LESS); // Restore default depth testing

	// 	//code to write out the FBO (texture) just once -an example
	// 	if (FirstTime) {
	// 		assert(GLTextureWriter::WriteImage(gBuffer, "gBuf.png"));
	// 		assert(GLTextureWriter::WriteImage(gPosition, "gPos.png"));
	// 		assert(GLTextureWriter::WriteImage(gNormal, "gNorm.png"));
	// 		assert(GLTextureWriter::WriteImage(gColorSpec, "gColorSpec.png"));
	// 		assert(GLTextureWriter::WriteImage(lightAccumulationBuf, "lightAccumBufNew.png"));
	// 		assert(GLTextureWriter::WriteImage(lightAccumulationTexture, "lightAccumulation.png"));
	// 		FirstTime = false;
	// 	}
	// 	else {

	// 		// read from gBuffer
	// 		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
	// 		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	// 		// from (0, 0) to (width, height)
	// 		glBlitFramebuffer(
	// 			0, 0, width, height, 
	// 			0, 0, width, height,
	// 			GL_DEPTH_BUFFER_BIT, GL_NEAREST
	// 		);
			
	// 		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// 		// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // clear everytime you bind to new framebuffer
	// 		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // clear everytime you bind to new framebuffer

	// 		// ambientProg->bind();
	// 		// 	glActiveTexture(GL_TEXTURE0);
	// 		// 	glBindTexture(GL_TEXTURE_2D, gColorSpec); 
	// 		// 	glUniform1i(ambientProg->getUniform("gColorSpec"), 0);

	// 		// 	glEnableVertexAttribArray(0);
	// 		// 	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	// 		// 	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	// 		// 	glDrawArrays(GL_TRIANGLES, 0, 6);
	// 		// 	glDisableVertexAttribArray(0);
	// 		// ambientProg->unbind();

	// 		// 1. enable blending
	// 		glEnable(GL_BLEND);
	// 		// 2. setup ADDITIVE blending
	// 		glBlendFunc(GL_ONE, GL_ONE);
	// 		// 3. enable stencil testing 
	// 		// conditionally eliminates pixels based on comparison test
	// 		glEnable(GL_STENCIL_TEST);
	// 		// TODO check
	// 		// // 7. enable face culling to only view front faces
	// 		glEnable(GL_CULL_FACE);
		
	// 		for (const Light& light : lights) {
	// 			glEnable(GL_DEPTH_TEST);
	// 		    // FRONT PASS
	// 			// 1. disable writing to the color buffer for the first pass
	// 			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	// 			// 2. disable writing to the depth buffer for the first pass
	// 			glDepthMask(GL_FALSE);
	// 			// 4. init the stencil buffer to 0
	// 			glClear(GL_STENCIL_BUFFER_BIT);
	// 			// 5. func = always, ref = 0, mask = 0xFF
	// 			// we set this to always because we want every fragment 
	// 			// to pass the stencil test so we can check the DEPTH
	// 			glStencilFunc(GL_ALWAYS, 0, 0xFF);
	// 			// TODO this test is GL_LEQUAL according to diagram
	// 			//https://cglearn.eu/pub/advanced-computer-graphics/deferred-rendering
	// 			glDepthFunc(GL_LEQUAL); // TODO check this depth test 
	// 			// 6. configure the stencil operations to keep frag
	// 			// for all front facing polygons:
	// 			//    if the stencil fails (it wont) then keep stencil value
	// 			//    if the stencil passes but the depth fails, keep the stencil value
	// 			//    if the stencil and the depth pass, increment (discard)
	// 			//	  if everything passes, KEEP
	// 			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR, GL_KEEP);

	// 			// 8. cull back faces
	// 			glCullFace(GL_BACK);

	// 			// 9. bind, send uniforms, draw
	// 			frontProg->bind();

	// 			P = SetProjectionMatrix(frontProg);
	// 			V = SetView(frontProg);
				
	// 			SetModel(frontProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
	// 			// 10. DRAW light volumes
	// 			lightVolume->draw(frontProg);

	// 			frontProg->unbind();

	// 			// BACK PASS
	// 			// 1. reenable color buffer
	// 			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	// 			// 2. change stencil func
	// 			// check where 0 is written to the stencil buffer
	// 			// (can flip stencil op if not working peroperly)
	// 			glStencilFunc(GL_EQUAL, 0, 0xFF);
    
	// 			// do not have to write to the stencil, only read and chek against it
	// 			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// 			// for all BACK facing polygons:
	// 			//    if the stencil fails (it wont) then keep stencil value
	// 			//    if the stencil passes but the depth fails, DISCARD stencil value (depth >= check ??)
	// 			//    if the stencil and the depth pass, KEEP
	// 			// TODO how to configure depth and stencil testing for back faces

	// 			// cull FRONT faces
	// 			glCullFace(GL_FRONT);
				
	// 			backProg->bind();
	// 			glActiveTexture(GL_TEXTURE0);
	// 			glBindTexture(GL_TEXTURE_2D, gPosition); 
	// 			glActiveTexture(GL_TEXTURE1); 
	// 			glBindTexture(GL_TEXTURE_2D, gNormal); 
	// 			glActiveTexture(GL_TEXTURE2); 
	// 			glBindTexture(GL_TEXTURE_2D, gColorSpec); 
	// 			glActiveTexture(GL_TEXTURE3); 
	// 			glBindTexture(GL_TEXTURE_2D, lightAccumulationTexture); 

	// 			// GLint loc = backProg->getUniform("gPosition");
	// 			// if (loc == -1) { std::cerr << "gPosition uniform not found!" << std::endl; }
	// 			glUniform1i(backProg->getUniform("gPosition"), 0);
	// 			glUniform1i(backProg->getUniform("gNormal"), 1);
	// 			glUniform1i(backProg->getUniform("gColorSpec"), 2);
	// 			glUniform1i(backProg->getUniform("lightMap"), 3);

	// 			glUniform3f(backProg->getUniform("lightPos"), light.Position.x, light.Position.y, light.Position.z);
	// 			glUniform3f(backProg->getUniform("lightCol"), light.Color.r, light.Color.g, light.Color.b);
	// 			glUniform2f(backProg->getUniform("resolution"), width, height);
	// 			//cout << "width: " << width << "height: " << height << endl;

	// 			P = SetProjectionMatrix(backProg);
	// 			V = SetView(backProg);
	// 			SetModel(backProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
	// 			lightVolume->draw(backProg);
	// 			backProg->unbind(); 
	// 		} 

	// 		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// 		glClear(GL_COLOR_BUFFER_BIT); 

	// 		// RESTORE ALLLL previous opengl settings
	// 		glDisable(GL_BLEND);
	// 		glDisable(GL_STENCIL_TEST);
	// 		glDisable(GL_CULL_FACE);
	// 		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	// 		glDepthMask(GL_TRUE);
	// 		glDepthFunc(GL_LESS);
	// 		glStencilFunc(GL_ALWAYS, 0, 0xFF);
	// 		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// 	}


	// }
	 
	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		 //camera movement - made continuous while keypressed
		cameraUpdate();

        // bind to gBuffer so the prog shader will write geometry to gbuffer for deferred pass
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
		// GEOMETRY PASS (sets up gbuffer for scene)
		drawGeometry();

		// PREVIOUSLY AND BASE CODE
		// bind the framebuffer to the lightAccumulation frame buffer, so that each light's computations are additively blended
		glBindFramebuffer(GL_FRAMEBUFFER, lightAccumulationBuf);
		// clear the color buffer before all light computations
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
		norProg->bind();
		mat4 P = SetProjectionMatrix(norProg); 
		mat4 V = SetView(norProg);

		// simplifies because we are only writing the normal data of the lights to the framebuffer
		for (const Light& light : lights) {
			//glUniform3f(volumesNoCullingProg->getUniform("lightPos"), light.Position.x, light.Position.y, light.Position.z);
			//glUniform3f(volumesNoCullingProg->getUniform("lightCol"), light.Color.r, light.Color.g, light.Color.b);
			SetModel(norProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
			lightVolume->draw(norProg);
		}
		norProg->unbind();

		//glCullFace(GL_BACK);  // Restore normal culling
		//glDepthFunc(GL_LESS); // Restore default depth testing

		//code to write out the FBO (texture) just once -an example
		if (FirstTime) {
			assert(GLTextureWriter::WriteImage(gBuffer, "gBuf.png"));
			assert(GLTextureWriter::WriteImage(gPosition, "gPos.png"));
			assert(GLTextureWriter::WriteImage(gNormal, "gNorm.png"));
			assert(GLTextureWriter::WriteImage(gColorSpec, "gColorSpec.png"));
			assert(GLTextureWriter::WriteImage(lightAccumulationBuf, "lightAccumBufNew.png"));
			assert(GLTextureWriter::WriteImage(lightAccumulationTexture, "lightAccumulation.png"));
			FirstTime = false;
		}
		else {

			// read from gBuffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			// from (0, 0) to (width, height)
			glBlitFramebuffer(
				0, 0, width, height, 
				0, 0, width, height,
				GL_DEPTH_BUFFER_BIT, GL_NEAREST
			);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // clear everytime you bind to new framebuffer
			glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // clear everytime you bind to new framebuffer

			// 1. enable blending
			glEnable(GL_BLEND);
			// 2. setup ADDITIVE blending
			glBlendFunc(GL_ONE, GL_ONE);

			// WRITE AMBIENT CONTRIBUTION TO CORRESPONDING DEPTH
			glDepthMask(GL_FALSE);
			ambientProg->bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, gColorSpec); 
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, gNormal); 

				glUniform1i(ambientProg->getUniform("gColorSpec"), 0);
				// glUniform1i(ambientProg->getUniform("gNormal"), 1);
				// glUniform3f(ambientProg->getUniform("lightDir"), 1, -1, 0);

				// draw quad
				glEnableVertexAttribArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glDisableVertexAttribArray(0);

			ambientProg->unbind();

			// 3. enable stencil testing 
			// conditionally eliminates pixels based on comparison test
			glEnable(GL_STENCIL_TEST);
			// TODO check
			// // 7. enable face culling to only view front faces
			glEnable(GL_CULL_FACE);
		
			for (const Light& light : lights) {
				glEnable(GL_DEPTH_TEST);
			    // FRONT PASS
				// 1. disable writing to the color buffer for the first pass
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				// 2. disable writing to the depth buffer for the first pass
				// glDepthMask(GL_FALSE);
				// 4. init the stencil buffer to 0
				glClear(GL_STENCIL_BUFFER_BIT);
				// 5. func = always, ref = 0, mask = 0xFF
				// we set this to always because we want every fragment 
				// to pass the stencil test so we can check the DEPTH
				glStencilFunc(GL_ALWAYS, 0, 0xFF);
				// TODO this test is GL_LEQUAL according to diagram
				//https://cglearn.eu/pub/advanced-computer-graphics/deferred-rendering
				glDepthFunc(GL_LEQUAL); // TODO check this depth test 
				// 6. configure the stencil operations to keep frag
				// for all front facing polygons:
				//    if the stencil fails (it wont) then keep stencil value
				//    if the stencil passes but the depth fails, keep the stencil value
				//    if the stencil and the depth pass, increment (discard)
				//	  if everything passes, KEEP
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR, GL_KEEP);

				// 8. cull back faces
				glCullFace(GL_BACK);

				// 9. bind, send uniforms, draw
				frontProg->bind();

				P = SetProjectionMatrix(frontProg);
				V = SetView(frontProg);
				
				SetModel(frontProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
				// 10. DRAW light volumes
				lightVolume->draw(frontProg);

				frontProg->unbind();

				// BACK PASS
				// 1. reenable color buffer
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				// 2. change stencil func
				// check where 0 is written to the stencil buffer
				// (can flip stencil op if not working peroperly)
				glStencilFunc(GL_EQUAL, 0, 0xFF);
    
				// do not have to write to the stencil, only read and chek against it
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

				// for all BACK facing polygons:
				//    if the stencil fails (it wont) then keep stencil value
				//    if the stencil passes but the depth fails, DISCARD stencil value (depth >= check ??)
				//    if the stencil and the depth pass, KEEP
				// TODO how to configure depth and stencil testing for back faces

				// cull FRONT faces
				glCullFace(GL_FRONT);
				
				backProg->bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, gPosition); 
				glActiveTexture(GL_TEXTURE1); 
				glBindTexture(GL_TEXTURE_2D, gNormal); 
				glActiveTexture(GL_TEXTURE2); 
				glBindTexture(GL_TEXTURE_2D, gColorSpec); 
				glActiveTexture(GL_TEXTURE3); 
				glBindTexture(GL_TEXTURE_2D, lightAccumulationTexture); 

				// GLint loc = backProg->getUniform("gPosition");
				// if (loc == -1) { std::cerr << "gPosition uniform not found!" << std::endl; }
				glUniform1i(backProg->getUniform("gPosition"), 0);
				glUniform1i(backProg->getUniform("gNormal"), 1);
				glUniform1i(backProg->getUniform("gColorSpec"), 2);
				glUniform1i(backProg->getUniform("lightMap"), 3);

				glUniform3f(backProg->getUniform("lightPos"), light.Position.x, light.Position.y, light.Position.z);
				glUniform3f(backProg->getUniform("lightCol"), light.Color.r, light.Color.g, light.Color.b);
				glUniform2f(backProg->getUniform("resolution"), width, height);
				//cout << "width: " << width << "height: " << height << endl;

				P = SetProjectionMatrix(backProg);
				V = SetView(backProg);
				SetModel(backProg, light.Position, 0.0f, 0.0f, light_radius, light_radius, light_radius);
				lightVolume->draw(backProg);
				backProg->unbind(); 
			} 

			// glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// glClear(GL_COLOR_BUFFER_BIT); 

			// RESTORE ALLLL previous opengl settings
			glDisable(GL_BLEND);
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_CULL_FACE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		}


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

		g_theta += deltaX*0.2;
		g_phi += deltaY*0.2;
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
			FirstTime = !FirstTime;
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
	
	// SCALE WINDOW
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
	// glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

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

