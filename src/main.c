#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <glad/glad.h>
#include <SDL.h>

#define SCREEN_WIDTH		1280
#define SCREEN_HEIGHT	960

#define LOG_CAPACITY 1024

#define SHADER_VERTEX_SOURCE_FILE_PATH 	"./resources/shaders/shader.vert"
#define SHADER_FRAGMENT_SOURCE_FILE_PATH 	"./resources/shaders/shader.frag"

static void get_info(void);
static void framebuffer_size_callback(SDL_Window *window, int width, int height);
static const char *gl_shader_type_as_cstr(unsigned int shader_type);
static unsigned int gl_create_and_compile_shader(unsigned int shader_type, const char *shader_source_file_path);
static unsigned int gl_create_and_link_program(const char *vertex_shader_source_file_path, const char *fragment_shader_source_file_path);
const char *file_as_cstr(const char *filepath);

int main(int argc, char *argv[]) {
	// Initialize the SDL library and start VIDEO subsystem.
	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
		abort();
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	
	SDL_Window *window = SDL_CreateWindow("SDL2 + OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
														SCREEN_WIDTH, SCREEN_HEIGHT, 
														SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if(window == NULL) {
		fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
		abort();
	}
	
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync.
	
	// Load all OpenGL functions.
	if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		fprintf(stderr, "Failed to initialize OpenGL context\n");
		return EXIT_FAILURE;
	}

	// Display some infos about SDL2 and OpenGL.
	get_info();

	// Specifies to OpenGl the size of the rendering window.
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	unsigned int shader_program = gl_create_and_link_program(SHADER_VERTEX_SOURCE_FILE_PATH, SHADER_FRAGMENT_SOURCE_FILE_PATH);
	glUseProgram(shader_program);

	// Set up vertex data (and buffer(s)) and configure vertex attributes.
	float vertices[] = {
		-1.0f, -1.0f, 	// left
		 1.0f, -1.0f, 	// right
		 0.0f,  0.0f,  // top

		-1.0f,  1.0f, 	// left
		 1.0f,  1.0f, 	// right
		 0.0f,  0.0f  	// top
	}; 

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	// Copy our vertices array in a buffer for OpenGL to use.
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);  

	// Set the vertex attributes pointers.
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind.
	//glBindBuffer(GL_ARRAY_BUFFER, 0); 

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	//glBindVertexArray(0);

	bool quit = false;
	SDL_Event event;
	while(!quit) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT: {
					quit = 1;
				} break;

				case SDL_WINDOWEVENT: {
					if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
						framebuffer_size_callback(window, event.window.data1, event.window.data2);
					}
				} break;
			}
		}

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//glBindVertexArray(VAO); // Seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized.
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shader_program);

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;	
}

static unsigned int gl_create_and_link_program(const char *vertex_shader_source_file_path, const char *fragment_shader_source_file_path) {
	// Build and compile our shader program.

	// Vertex shader.
	unsigned int vertex_shader = gl_create_and_compile_shader(GL_VERTEX_SHADER, vertex_shader_source_file_path);
	// Fragment shader.
	unsigned int fragment_shader = gl_create_and_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source_file_path);

	// Link shaders.
	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	// Check for linking errors.
	int linked;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &linked);
	if(linked != GL_TRUE) {
		unsigned int log_length = 0;
		char log_info[LOG_CAPACITY];
		glGetProgramInfoLog(shader_program, LOG_CAPACITY, &log_length, log_info);

		fprintf(stderr, "Program failed linkage: %.*s", log_length, log_info);
		abort();
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_program;
}

static unsigned int gl_create_and_compile_shader(unsigned int shader_type, const char *shader_source_file_path) {
	const char *shader_source = file_as_cstr(shader_source_file_path);

	unsigned int shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);
	
	// Check for shader compile errors.
	int compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(compiled != GL_TRUE) {
		unsigned int log_length = 0;
		char log_info[LOG_CAPACITY];
		glGetShaderInfoLog(shader, LOG_CAPACITY, &log_length, log_info);
		
		fprintf(stderr, "%s failed compiled:\n", gl_shader_type_as_cstr(shader_type));
		fprintf(stderr, "%s:%.*s", shader_source, log_length, log_info);	

		abort();
	}

	return shader;
}

static const char *gl_shader_type_as_cstr(unsigned int shader_type) {
	switch (shader_type) {
		case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
		case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
    	default: return "UNKNOWN";
	}
}

static void framebuffer_size_callback(SDL_Window *window, int width, int height) {
	glViewport(0, 0, width, height);
}

const char *file_as_cstr(const char *filepath) {
	assert(filepath);

	FILE *fp = fopen(filepath, "rb");
	if(!fp) {
		fprintf(stderr, "Could not open file '%s': %s\n", filepath, strerror(errno));
		abort();
	}

	int code = fseek(fp, 0, SEEK_END);
	if(code != 0) {
		fprintf(stderr, "Could not find the end of file %s: %s\n", filepath, strerror(errno));
		abort();
	}

	long v = ftell(fp);
	if(v < 0) {
		fprintf(stderr, "Coult not get current position in file %s: %s\n", filepath, strerror(errno));
		abort();
	}
	size_t count = (size_t) v;

	code = fseek(fp, 0, SEEK_SET);
	if(code != 0) {
		fprintf(stderr, "Could not find the beginning of file %s: %s\n", filepath, strerror(errno));
		abort();
	}

	char *buffer = calloc(count + 1, sizeof(char));
	if(!buffer) {
		fprintf(stderr, "Could not allocate memory for file %s: %s\n", filepath, strerror(errno));
		abort();
	}

	size_t n = fread(buffer, 1, count, fp);
	if(n != count) {
		fprintf(stderr, "Could not read file %s: %s\n", filepath, strerror(errno));
		abort();
	}

	return buffer;
}

static void get_info(void) {
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);

	printf("SDL2 comiled version: %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	printf("SDL2 linked version: %d.%d.%d\n", linked.major, linked.minor, linked.patch);

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
}