#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

// ASSERT(GLLogCall()) calls __debugbreak() when glGetError() pops something,  USING GLLogCall()
//		breaks appliction when param inputted is false
#define ASSERT(x) if (!(x)) __debugbreak()

//GLCall(x) calls glSlearError() until no more errors	-- STOPS APPLICATION IF LINE HAS AN ERROR
//		then calls function input (x)
//		then ASSERT(GLLogCall()) 
//			uses macro to input function name, file name, and line number to debugger!
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))	//asserts glGetError()


//Calls glGetError() until no more errors, clearing the error stack
static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		//If error found, print it, and return false to fail an ASSERT, calling debugbreak
		std::cout << "[OpenGL Error] (" << error << "): " << function << " : " << line << "\nFile : " << file << std::endl;
		return false;
	}
	return true;
}


//Struct for storing parsed shader source code as two strings
struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};


//Reads shader source code from filepath
// 
// 
// Returns ShaderProgramSource 
//	@ VertexSource string			(use header '#shader vertex')
//	@ FragmentSource string			(use header '#shader fragment')
// 	
static ShaderProgramSource ParseShader(const std::string& filepath)
{
	std::ifstream stream(filepath);

	//Epic enum class that is used as a MODE for placing each line read 
	//	used as INDEX for stringstream array which contains two strings
	//	string vertex shader source		// index 0 
	//	string fragment shader source	// index 1
	//	invalid index -1 for detecting if no mode is set, in which case nothing will be written
	enum class ShaderType
	{
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string line;			// holds each line read
	std::stringstream ss[2];	// two stringstreams for storing parsed shader source code
	ShaderType type = ShaderType::NONE;			//type mode for determining which type of code is being read
												//doubles as index determiner for placing lines into s
	while (std::getline(stream, line))
	{
		//If shader code line is #shader change shadertype mode
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
			{
				type = ShaderType::VERTEX;
			}
			else if (line.find("fragment") != std::string::npos)
			{
				type = ShaderType::FRAGMENT;
			}
		}
		else if (type != ShaderType::NONE)
		{
			ss[(size_t)type] << line << "\n";
		}
	}
	stream.close();

	//Return vertex and frag shader source as ShaderProgramSource type
	return { ss[0].str(), ss[1].str() };

}

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type);	//<<<<!!!!!!NOTE!!!!!!>>>>> type was "GL_VERTEX_SHADER"
	const char* src = &source[0];
	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));

	// TODO : Error handling
	// using glGetShaderiv (i = integer, v = vector) to get a specific parameter from a shader object
	// gets compile status param, to check if compilation was success, if not erorr handle
	int result;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE)
	{
		int errLength;
		GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &errLength));

		char* message = (char*)_malloca(errLength * sizeof(char));
		GLCall(glGetShaderInfoLog(id, errLength, &errLength, message));

		std::cout << "Failed to compile "
			<< (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;

		GLCall(glDeleteProgram(id));

		return 0;
	}

	return id;
}

//Give OpenGL our shader source code to be compiled by the gpu, 
//Return unique programID for glCreateProgram()
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
	//Create gpu program for shader to run on
	unsigned int programID = glCreateProgram();

	//Compile frag and vertex shaders
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	//Attach v and f shaders to program
	GLCall(glAttachShader(programID, vs));
	GLCall(glAttachShader(programID, fs));

	//Link and validate final shader program
	GLCall(glLinkProgram(programID));
	GLCall(glValidateProgram(programID));

	//Delete temporary shader files
	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));

	return programID;
}


int main(void)
{

	//Make our window :D
	GLFWwindow* window;


	//Initialize the library!
	if (!glfwInit())
	{
		return -1;
	}

	//Create windowed mode window and its OpenGL context

	window = glfwCreateWindow(640, 480, "Seizure Square", NULL, NULL);

	//Check if window is valid?
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	//Make window the current context (first creation of valid window context)
	glfwMakeContextCurrent(window);

	//Sync frame swap to monitor
	glfwSwapInterval(1);

	//Initialize GLEW libraries?
	if (glewInit() != GLEW_OK)
		std::cout << "GLEW Error!\n" << std::endl;
	else
		std::cout << "GLEW Good!\n" << std::endl;

	std::cout << glGetString(GL_VERSION) << std::endl;


	/* Define vertex buffer (of triangle) for sending to gpu  */
	// Create triangle using float array

	float tri1_position[] = {
		-0.5f, -0.5f,	// 0 BL
		0.5f, -0.5f,	// 1 BR
		0.5f, 0.5f,		// 2 TR
		-0.5f, 0.5f		// 3 TL
	};

	unsigned int indicies[] = {
		0, 1, 2,
		2, 3, 0
	};


	// Get ID for a buffer that we will make 
	unsigned int bufferID;
	GLCall(glGenBuffers(1, &bufferID));

	// Bind buffer using its ID as a memory chunk with GL_ARRAY_BUFFER
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, bufferID));

	//Give the buffer some float position data!!!!!!
	GLCall(glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), tri1_position, GL_STATIC_DRAW));

	// Get ID for an INDEX buffer object that we will make 
	unsigned int ibo;
	GLCall(glGenBuffers(1, &ibo));

	// Bind buffer using its ID as a memory chunk with GL_ARRAY_BUFFER
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));

	//Give the buffer some index data!!!!!!
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), indicies, GL_STATIC_DRAW));


	//Enable first attribute
	GLCall(glEnableVertexAttribArray(0));

	//assign to the first attribute(position), set how many make up a pos (2), 
	// type, normalized, 
	// byte size of each vertex position (float * 2)
	// no other attrib than pos so 0 offset to another attrib, 
	// if the third element in tri_positions was a Normal value(example), 0 would become 8
	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));

	//Parse shader source text from .shader file into source code that can go into either vertex or frag shader
	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);

	//Start running shader code on gpu 
	GLCall(glUseProgram(shader));

	//Making a uniform for color in the shader!
	// use gl function to get id for a shader paramater location to be used for glUniform4f()
	// allows us to set value in the shader file from cpu
	int location = glGetUniformLocation(shader, "u_Color");
	ASSERT(location != -1);

	float r = 0.8f;
	float increment = 0.05f;

	//game loop, exits on user window close
	while (!glfwWindowShouldClose(window))
	{
		GLCall(glClear(GL_COLOR_BUFFER_BIT));

		//Set color, check red val bounds and increment
		GLCall(glUniform4f(location, 0.8f, r, 0.8f, 1.f));

		if (r > 1.f)
			increment = -0.05f;
		else if (r < 0.f)
			increment = 0.05f;
		r += increment;

		//DRAW THE TRIANGLE
		// No index buffer 
		//
		// Interpret these points as verticies of a triangle
		// Start at the first index (vertex) 
		// There are three vertecies (indicies)
		//glDrawArrays(GL_TRIANGLES, 0, 6);

		//With index buffer, 6 indicies for triangles
		GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
		/*
			Swap front and back buffers
		*/
		glfwSwapBuffers(window);
		/*
			Poll events and process them
		*/
		glfwPollEvents();
	}

	GLCall(glDeleteProgram(shader));

	glfwTerminate();
	return 0;
}
//Sends floats to the gpu as a set of verticies and indicies, shader code is sent over as well then
// ran on a program on the gpu(shader) to display visual information on a window we created.