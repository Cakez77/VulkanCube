// Use to switch Vulkan ON and OFF
#define USE_VULKAN

// include C++ headers
// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#ifdef USE_VULKAN
// Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#else
// OpenGL
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#endif

// Standard Library
#include <iostream>
#include <cstdio>
#include <vector>
#include <array>
#include <map>

// ##########################################################
// 				Global Variables
// ##########################################################
//define screen size
unsigned int SCREEN_WIDTH = 800;
unsigned int SCREEN_HEIGHT = 800;

struct VertexColor // vertex attribute format
{
	glm::vec3 position;
	glm::vec3 color;
};

std::map<std::string, glm::mat4> gModelMatrix; // object matrices
glm::mat4 gViewMatrix;						   // view matrix
glm::mat4 gProjectionMatrix;				   // projection matrix
glm::mat4 MVP;

std::vector<GLfloat> vertices =
	{
		// colour cube
		-0.5f, 0.5f, 0.5f,	 // vertex 0: position
		1.0f, 0.0f, 1.0f,	 // vertex 0: colour
		-0.5f, -0.5f, 0.5f,	 // vertex 1: position
		1.0f, 0.0f, 0.0f,	 // vertex 1: colour
		0.5f, 0.5f, 0.5f,	 // vertex 2: position
		1.0f, 1.0f, 1.0f,	 // vertex 2: colour
		0.5f, -0.5f, 0.5f,	 // vertex 3: position
		1.0f, 1.0f, 0.0f,	 // vertex 3: colour
		-0.5f, 0.5f, -0.5f,	 // vertex 4: position
		0.0f, 0.0f, 1.0f,	 // vertex 4: colour
		-0.5f, -0.5f, -0.5f, // vertex 5: position
		0.0f, 0.0f, 0.0f,	 // vertex 5: colour
		0.5f, 0.5f, -0.5f,	 // vertex 6: position
		0.0f, 1.0f, 1.0f,	 // vertex 6: colour
		0.5f, -0.5f, -0.5f,	 // vertex 7: position
		0.0f, 1.0f, 0.0f,	 // vertex 7: colour
};

// colour cube indices
static std::array<GLuint, 36> indices = {
	0, 1, 2, // triangle 1
	2, 1, 3, // triangle 2
	4, 5, 0, // triangle 3
	0, 5, 1, // triangle 4
	2, 3, 6, // triangle 5
	6, 3, 7, // triangle 6
	4, 0, 6, // triangle 7
	6, 0, 2, // triangle 8
	1, 5, 3, // triangle 9
	3, 5, 7, // triangle 10
	5, 4, 7, // triangle 11
	7, 4, 6, // triangle 12
};

// ##########################################################
// 				Vulkan or OpenGL Includes
// ##########################################################

#ifdef USE_VULKAN

#include "vulkan_renderer.h"
#else
// inculde shader
#include "ShaderProgram.h"

// scene content
ShaderProgram gShader; // shader program object
GLuint gVBO = 0;	   // vertex buffer object identifier
GLuint gIBO = 0;	   // index buffer object identifier
GLuint gVAO = 0;	   // vertex array object identifier

GLuint FBO = 0;
GLuint FTexture = 0;
GLuint BTexture = 0;
ShaderProgram finalShader;

static void init_opengl(GLFWwindow *window)
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // set the color the color buffer should be cleared to

	glEnable(GL_DEPTH_TEST); // enable depth buffer test
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);

	glCreateFramebuffers(1, &FBO);
	glCreateTextures(GL_TEXTURE_2D, 1, &FTexture);
	glTextureParameteri(FTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(FTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, FTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0, FTexture, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &BTexture);
	glTextureParameteri(BTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(BTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, BTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT3, BTexture, 0);

	gShader.compileAndLink("modelViewProj.vert", "color.frag"); // compile and link a vertex and fragment shader pair
	finalShader.compileAndLink("frame.vert", "frame.frag");

	// create VBO
	glGenBuffers(1, &gVBO); // generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	// generate identifier for IBO and copy data to GPU
	glGenBuffers(1, &gIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO);		 // generate unused VAO identifier
	glBindVertexArray(gVAO);			 // create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO); // bind the VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
						  reinterpret_cast<void *>(offsetof(VertexColor, position))); // specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
						  reinterpret_cast<void *>(offsetof(VertexColor, color))); // specify format of colour data

	glEnableVertexAttribArray(0); // enable vertex attributes
	glEnableVertexAttribArray(1);
}

static void render_scene_opengl()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glNamedFramebufferDrawBuffer(FBO, GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT3};
	glNamedFramebufferDrawBuffers(FBO, 2, drawbuffers);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clean color and depth buffers
	gShader.use();										// Use the shader program

	glBindVertexArray(gVAO); // make VAO active

	gShader.setUniform("uModelViewProjectionMatrix", MVP); // set Uniform variable
	glCullFace(GL_BACK);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // display vertices base indices and primitive type for the front side
	glCullFace(GL_FRONT);
	glNamedFramebufferDrawBuffer(FBO, GL_COLOR_ATTACHMENT3);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // display vertices base indices and primitive type for the back side

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	finalShader.use();
	glBindTextureUnit(0, FTexture); //FTexture contains the Front-side
	glBindTextureUnit(1, BTexture); //BTexture contains the Back-side
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDrawArrays(GL_QUADS, 0, 4);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	//glFlush(); //flush the graphic pipeline

	glfwSwapBuffers(app_window);
}
#endif

static void update_scene()
{
	static float rotationAngle = 0.0f;
	rotationAngle += 0.016f;
	gModelMatrix["cube"] = glm::rotate(rotationAngle, glm::vec3(1.0f, 0.0f, 0.0f)); // Update model matrix

	MVP = gProjectionMatrix * gViewMatrix * gModelMatrix["cube"];
}

static void error_callback(int error, const char *error_description)
{
	std::cerr << error_description << std::endl; // show error description
}

int main(void)
{
	// Global Data init
	gViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // initialise view matrix
	float aspectRatio = static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT;											  // initialise projection matrix
	gProjectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
	gModelMatrix["cube"] = glm::mat4(1.0f);

	GLFWwindow *app_window = nullptr;	  // Define application window
	glfwSetErrorCallback(error_callback); // Set GLFW error callback function

	if (!glfwInit())
	{
		exit(EXIT_FAILURE); //if GLFW failed to initialise -> Exit
	}

#ifdef USE_VULKAN
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#endif

	app_window = glfwCreateWindow(SCREEN_HEIGHT, SCREEN_HEIGHT, "OpenGL Cube", nullptr, nullptr); // Create the window

	if (app_window == nullptr) // Check if app_window is created successfully
	{
		glfwTerminate();
		exit(EXIT_FAILURE); // "exit(EXIT_FAILURE)" can also be written as "(return -1)"
	}

#ifdef USE_VULKAN
	if (!init_vulkan(app_window))
	{
		std::cerr << "Vulkan Failed to initialise" << std::endl;
		exit(EXIT_FAILURE);
	}
#else
	glfwMakeContextCurrent(app_window); //  set window context as current context
	glfwSwapInterval(1);				//	swap buffer interval
	if (glewInit() != GLEW_OK)			// if GLEW failed to initialise -> Exit
	{
		std::cerr << "GLEW Failed to initialise" << std::endl;
		exit(EXIT_FAILURE);
	}

	init_opengl(app_window); // initialise scene and render settings
#endif

	while (!glfwWindowShouldClose(app_window)) // the rendering loop
	{
		update_scene(); // update the scene
#ifdef USE_VULKAN
		render_scene_vulkan();
#else

		render_scene_opengl(); // render the scene
#endif

		glfwPollEvents();
	}

	// Clean ups
#ifdef USE_VULKAN
#else
	glDeleteBuffers(1, &gVBO);
	glDeleteBuffers(1, &gIBO);
	glDeleteVertexArrays(1, &gVAO);
#endif

	glfwDestroyWindow(app_window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
