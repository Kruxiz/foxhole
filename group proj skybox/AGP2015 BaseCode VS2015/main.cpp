// Simple 3D renderer for class test

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include <stdlib.h>
#include <math.h>
#include <sdl.h>

#include "md2model.h"

#include <SDL_ttf.h>

using namespace std;

#define DEG_TO_RADIAN 0.017453293

typedef struct {
	Uint8 type;
	Uint8 state;
	Uint16 x, y;
	Sint16 xrel, yrel;
} SOL;

//angle of rotation
float xpos = 0, ypos = 0, zpos = 0, xrot = 0, yrot = 0, angle = 0.0;

float lastx, lasty;

// Globals
// Real programs don't use globals :-D

GLuint meshIndexCount = 0;
GLuint meshObjects[3];

GLuint shaderProgram;
GLuint skyboxProgram;
GLuint textureProgram;

GLfloat r = 0.0f;

glm::vec3 eye(0.0f, 1.0f, 0.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
stack<glm::mat4> mvStack;

// TEXTURE STUFF
GLuint textures[4];
GLuint skybox[5];
GLuint labels[1];

glm::vec3 scale(1.0f, 1.0f, 2.0f);

glm::vec3 position(1.0f, -0.1f, -1.0f);

//start position
glm::vec3 startPosition = position;
//position that marks death position
glm::vec3 deathPosition(-5.0f, 1.5f, -100.0f);

glm::vec3 cubePositions[11];
glm::vec3 cubeScales[11];

//jumping variables
GLuint jumpCounter = 0;
GLfloat jumpIncrement = 0.1f;
const GLfloat baseJumpIncrement = 0.1f;
const GLfloat maxJumpIncrement = 0.5f;
const GLuint jumpMax = 30;
bool falling = false;
bool jumping = false;
bool jumpIncrementSet = false;
const GLfloat jumpBase = -0.1f;
GLfloat accnValue = 1.1f;

int lastCollision = -1;

//// md2 stuff
//md2model tmpModel;
//GLuint md2VertCount = 0;
//int currentAnim = 0;

rt3d::lightStruct light0 = {
	{ 0.3f, 0.3f, 0.3f, 1.0f }, // ambient
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // specular
	{ -10.0f, 10.0f, 10.0f, 1.0f }  // position
};
glm::vec4 lightPos(0.0f, 2.0f, -6.0f, 1.0f); //light position

rt3d::materialStruct material0 = {
	{ 0.2f, 0.4f, 0.2f, 1.0f }, // ambient
	{ 0.5f, 1.0f, 0.5f, 1.0f }, // diffuse
	{ 0.0f, 0.1f, 0.0f, 1.0f }, // specular
	2.0f  // shininess
};

rt3d::materialStruct material1 = {
	{ 0.0f, 0.0f, 1.0f, 1.0f }, // ambient
	{ 0.5f, 1.0f, 0.5f, 1.0f }, // diffuse
	{ 0.0f, 0.1f, 0.0f, 1.0f }, // specular
	2.0f  // shininess
};

rt3d::materialStruct material2 = {
	{ 1.0f, 0.4f, 0.0f, 1.0f }, // ambient
	{ 0.5f, 1.0f, 0.5f, 1.0f }, // diffuse
	{ 0.0f, 0.1f, 0.0f, 1.0f }, // specular
	2.0f  // shininess
};

TTF_Font * textFont;

// textToTexture
GLuint textToTexture(const char * str, GLuint textID/*, TTF_Font *font, SDL_Color colour, GLuint &w,GLuint &h */) {
	TTF_Font *font = textFont;
	SDL_Color colour = { 255, 255, 255 };
	SDL_Color bg = { 0, 0, 0 };

	SDL_Surface *stringImage;
	stringImage = TTF_RenderText_Blended(font, str, colour);

	if (stringImage == NULL)
		//exitFatalError("String surface not created.");
		std::cout << "String surface not created." << std::endl;

	GLuint w = stringImage->w;
	GLuint h = stringImage->h;
	GLuint colours = stringImage->format->BytesPerPixel;

	GLuint format, internalFormat;
	if (colours == 4) {   // alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGBA;
		else
			format = GL_BGRA;
	}
	else {             // no alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGB;
		else
			format = GL_BGR;
	}
	internalFormat = (colours == 4) ? GL_RGBA : GL_RGB;

	GLuint texture = textID;

	if (texture == 0) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} //Do this only when you initialise the texture to avoid memory leakage

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stringImage->w, stringImage->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, stringImage->pixels);
	glBindTexture(GL_TEXTURE_2D, NULL);

	SDL_FreeSurface(stringImage);
	return texture;
}


// Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context) {
	SDL_Window * window;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
		rt3d::exitFatalError("Unable to initialize SDL");

	// Request an OpenGL 3.0 context.

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)

	// Create 800x600 window
	window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window) // Check window was created OK
		rt3d::exitFatalError("Unable to create window");

	context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
	SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

// A simple texture loading function
// lots of room for improvement - and better error checking!
GLuint loadBitmap(char *fname) {
	GLuint texID;
	glGenTextures(1, &texID); // generate texture ID

	// load file - using core SDL library
	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface) {
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;

	GLuint externalFormat, internalFormat;
	if (format->Amask) {
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0,
		externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(tmpSurface); // texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}


// A simple cubemap loading function
// lots of room for improvement - and better error checking!
GLuint loadCubeMap(const char *fname[6], GLuint *texID)
{
	glGenTextures(1, texID); // generate texture ID
	GLenum sides[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
						GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Y };
	SDL_Surface *tmpSurface;

	glBindTexture(GL_TEXTURE_CUBE_MAP, *texID); // bind texture and set parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint externalFormat;
	for (int i = 0;i < 6;i++)
	{
		// load file - using core SDL library
		tmpSurface = SDL_LoadBMP(fname[i]);
		if (!tmpSurface)
		{
			std::cout << "Error loading bitmap" << std::endl;
			return *texID;
		}

		// skybox textures should not have alpha (assuming this is true!)
		SDL_PixelFormat *format = tmpSurface->format;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;

		glTexImage2D(sides[i], 0, GL_RGB, tmpSurface->w, tmpSurface->h, 0,
			externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
		// texture loaded, free the temporary buffer
		SDL_FreeSurface(tmpSurface);
	}
	return *texID;	// return value of texure ID, redundant really
}


void init(void) {

	shaderProgram = rt3d::initShaders("phong-tex.vert", "phong-tex.frag");
	rt3d::setLight(shaderProgram, light0);
	rt3d::setMaterial(shaderProgram, material0);

	//matching textureUnits
	GLuint uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit0");
	glUniform1i(uniformIndex, 0);
	uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit1");
	glUniform1i(uniformIndex, 1);
	uniformIndex = glGetUniformLocation(shaderProgram, "textureUnit2");
	glUniform1i(uniformIndex, 2);

	skyboxProgram = rt3d::initShaders("cubeMap.vert", "cubeMap.frag");
	textureProgram = rt3d::initShaders("textured.vert", "textured.frag");


	const char *cubeTexFiles[6] = {
		"Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp", "Town-skybox/grass1.bmp"
	};
	loadCubeMap(cubeTexFiles, &skybox[0]);


	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();
	textures[0] = loadBitmap("fabric.bmp");
	textures[1] = loadBitmap("studdedmetal.bmp");
	textures[2] = loadBitmap("water.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), meshIndexCount, indices.data());

	vector<GLfloat> verts1;
	vector<GLfloat> norms1;
	vector<GLfloat> tex_coords1;
	vector<GLuint> indices1;
	rt3d::loadObj("fox.obj", verts1, norms1, tex_coords1, indices1);
	meshIndexCount = indices1.size();
	textures[0] = loadBitmap("fabric.bmp");
	textures[1] = loadBitmap("studdedmetal.bmp");
	meshObjects[1] = rt3d::createMesh(verts1.size() / 3, verts1.data(), nullptr, norms1.data(), tex_coords1.data(), meshIndexCount, indices1.data());

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// set up TrueType / SDL_ttf
	if (TTF_Init() == -1)
		cout << "TTF failed to initialise." << endl;

	textFont = TTF_OpenFont("MavenPro-Regular.ttf", 48);
	if (textFont == NULL)
		cout << "Failed to open font." << endl;

	labels[0] = 0;//First init to 0 to avoid memory leakage if it is dynamic
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::sin(r*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(r*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::cos(r*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(r*DEG_TO_RADIAN));
}

double GetTimeScalar()
{
	static unsigned int lastticks = 0;

	int ticks = SDL_GetTicks();
	int tickssince = ticks - lastticks;

	double scalar = (double)tickssince / (double)60.0f;

	lastticks = ticks;

	//cout << scalar << endl; // for debugging
	if (scalar < 0.2 || scalar > 1.0)
		scalar = 0.28333;

	return scalar;

}

void respawnToStart() {
	position = startPosition;
	std::cout << "respawn\n";
}

void checkIfDead() {
	if ((position.x > (deathPosition.x - 19)) && (position.x < (deathPosition.x + 19)))
	{
		if ((position.z > (deathPosition.z - 50)) && (position.z < (deathPosition.z + 50)))
		{
			if (position.y == startPosition.y)
				respawnToStart();
		}
	}
}

void checkIfAtEnd() {
	if ((position.z < -180)) respawnToStart();
}

bool checkCollisionWithCubes()
{
	for (int i = 0; i < 11; i++) {
		if (cubePositions[i].x + cubeScales[i].x >= position.x && cubePositions[i].x - cubeScales[i].x <= position.x)
		{
			if (cubePositions[i].z + cubeScales[i].z >= position.z - (scale.z) && cubePositions[i].z - cubeScales[i].z <= position.z + (scale.z))
			{
				if (cubePositions[i].y + cubeScales[i].y >= position.y + (scale.y) && cubePositions[i].y - cubeScales[i].y <= position.y + (scale.y))
				{
					std::cout << "collided\n\n";
					lastCollision = i;
					return true;
				}
			}
		}
	}

	return false;
}

void fallToGround() {
	GLfloat miniJumpIncrement = baseJumpIncrement;
	GLfloat originalPosY = position.y;
	while (position.y > jumpBase) {
		if (checkCollisionWithCubes()) {
			position.y = originalPosY;
			break;
		}
		position.y -= miniJumpIncrement;
	}
	if (position.y <= jumpBase) {
		position.y = jumpBase;
	}
}

void update(void) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_COMMA]) r -= 1.0f;
	if (keys[SDL_SCANCODE_PERIOD]) r += 1.0f;

	glm::vec3 tempPosition = position;

	if (keys[SDL_SCANCODE_W]) {
		position = moveForward(position, r, 0.1f / GetTimeScalar());
		if (checkCollisionWithCubes()) {
			position = tempPosition;
		}
		if (position.y > jumpBase && !jumping && !falling) {
			fallToGround();
		}
	}

	if (keys[SDL_SCANCODE_A]) {
		position = moveRight(position, r, -0.1f / GetTimeScalar());
		if (checkCollisionWithCubes()) {
			position = tempPosition;
		}
		else if (position.y > jumpBase) {
			fallToGround();
		}
	}

	if (keys[SDL_SCANCODE_S]) {
		position = moveForward(position, r, -0.1f / GetTimeScalar());
		if (checkCollisionWithCubes()) {
			position = tempPosition;
		}
		else if (position.y > jumpBase) {
			fallToGround();
		}
	}

	if (keys[SDL_SCANCODE_D]) {
		position = moveRight(position, r, 0.1f / GetTimeScalar());
		if (checkCollisionWithCubes()) {
			position = tempPosition;
		}
		else if (position.y > jumpBase) {
			fallToGround();
		}
	}

	if (keys[SDL_SCANCODE_Z]) position[1] += 0.1;
	if (keys[SDL_SCANCODE_X]) position[1] -= 0.1;

	if (keys[SDL_SCANCODE_SPACE] && !falling)
	{
		jumpCounter++;

		if (jumpCounter > 0 && jumpCounter < jumpMax)
		{
			jumpIncrement *= accnValue;
			if (jumpIncrement >= maxJumpIncrement)
				jumpIncrement = maxJumpIncrement;
			position.y += jumpIncrement;
		}
		else if (jumpCounter >= jumpMax)
			falling = true;

		jumping = true;
	}

	if (falling || (!keys[SDL_SCANCODE_SPACE] && jumping))
	{
		tempPosition = position;

		if (jumpIncrement != baseJumpIncrement && !jumpIncrementSet)
		{
			jumpIncrement = baseJumpIncrement;
			jumpIncrementSet = true;
		}

		if (position.y > jumpBase && !checkCollisionWithCubes()) // while still in the air
		{
			jumpIncrement *= accnValue;
			if (jumpIncrement >= maxJumpIncrement)
				jumpIncrement = maxJumpIncrement;

			position.y -= jumpIncrement; // fall down

			jumpCounter = jumpMax;
		}
		else if (position.y <= jumpBase || checkCollisionWithCubes()) // collision detection with the ground or cubes
		{
			position.y += jumpIncrement * 2;
			falling = false;
			jumping = false;
			jumpCounter = 0;
			jumpIncrement = baseJumpIncrement;
			jumpIncrementSet = false;
		}

	}

	//boundary death
	if (position.x >= 14.0f)
	{
		position.x = 14.0f;
	}
	if (position.x <= -24.0f)
	{
		position.x = -24.0f;
	}

	if (position.z <= -200.0f)
	{
		position.z = -200.0f;
	}
	if (position.z >= 100.0f)
	{
		position.z = 100.0f;
	}


	checkIfDead();
	checkIfAtEnd();
}


void draw(SDL_Window * window) {
	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);

	GLfloat scale(1.0f); // just to allow easy scaling of complete scene

	glm::mat4 modelview(1.0); // set base position for scene
	mvStack.push(modelview);

	at = position;
	eye = moveForward(at, r, -8.0f);
	eye.y += 6.0;
	mvStack.top() = glm::lookAt(eye, at, up);


	// skybox as single cube using cube map
	glUseProgram(skyboxProgram);
	rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));
	glDepthMask(GL_FALSE); // make sure writing to update depth test is off
	glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
	mvStack.push(glm::mat4(mvRotOnlyMat3));
	glCullFace(GL_FRONT); // drawing inside of cube!
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	glCullFace(GL_BACK); // drawing inside of cube!
	// back to remainder of rendering
	glDepthMask(GL_TRUE); // make sure depth test is on


	glUseProgram(shaderProgram);
	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));


	glm::vec4 tmp = mvStack.top()*lightPos;
	light0.position[0] = tmp.x;
	light0.position[1] = tmp.y;
	light0.position[2] = tmp.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));

	// draw a cube for ground plane
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-5.0f, -0.1f, -100.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 200.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// draw water
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-5.0f, 0.0f, -100.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 50.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material1);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// draw jumping cubes - 0
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[0] = glm::vec3(-5.0f, 1.0f, -50.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[0]);
	cubeScales[0] = glm::vec3(5.0f, 1.0f, 5.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[0]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 1
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[1] = glm::vec3(-5.0f, 1.0f, -60.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[1]);
	cubeScales[1] = glm::vec3(1.0f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[1]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 2
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[2] = glm::vec3(0.0f, 2.0f, -66.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[2]);
	cubeScales[2] = glm::vec3(1.5f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[2]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 3
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[3] = glm::vec3(-4.0f, 1.0f, -75.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[3]);
	cubeScales[3] = glm::vec3(1.5f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[3]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 4
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[4] = glm::vec3(-8.0f, 1.5f, -85.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[4]);
	cubeScales[4] = glm::vec3(1.5f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[4]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 5
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[5] = glm::vec3(-10.0f, 2.0f, -96.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[5]);
	cubeScales[5] = glm::vec3(1.5f, 3.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[5]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 6
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[6] = glm::vec3(-2.0f, 1.0f, -105.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[6]);
	cubeScales[6] = glm::vec3(1.5f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[6]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 7
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[7] = glm::vec3(-2.0f, 1.5f, -120.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[7]);
	cubeScales[7] = glm::vec3(2.0f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[7]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 8
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[8] = glm::vec3(-9.0f, 1.0f, -127.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[8]);
	cubeScales[8] = glm::vec3(2.0f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[8]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 9
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[9] = glm::vec3(-5.0f, 2.0f, -137.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[9]);
	cubeScales[9] = glm::vec3(2.0f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[9]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// 10
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	cubePositions[10] = glm::vec3(-1.0f, 1.0f, -145.0f);
	mvStack.top() = glm::translate(mvStack.top(), cubePositions[10]);
	cubeScales[10] = glm::vec3(2.0f, 2.0f, 1.0f);
	mvStack.top() = glm::scale(mvStack.top(), cubeScales[10]);
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	//fox
	glUseProgram(shaderProgram);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(position.x, position.y, position.z));
	mvStack.top() = glm::rotate(mvStack.top(), float(-r*DEG_TO_RADIAN), glm::vec3(0.0f, 1.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), float(180 * DEG_TO_RADIAN), glm::vec3(1.0f, 0.0f, 0.0f));
	mvStack.top() = glm::rotate(mvStack.top(), float(180 * DEG_TO_RADIAN), glm::vec3(0.0f, 0.0f, 1.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material2);
	rt3d::drawIndexedMesh(meshObjects[1], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// remember to use at least one pop operation per push...
	mvStack.pop(); // initial matrix
	glDepthMask(GL_TRUE);


	SDL_GL_SwapWindow(window); // swap buffers
}


// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
	SDL_Window * hWindow; // window handle
	SDL_GLContext glContext; // OpenGL context handle
	hWindow = setupRC(glContext); // Create window and render context 

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit(1);
	}
	//cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running) {	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {

			if (sdlEvent.type == SDL_QUIT)
				running = false;

			if (sdlEvent.type == SDL_MOUSEMOTION)
			{
				SDL_SetRelativeMouseMode(SDL_TRUE);
				SDL_WarpMouseInWindow(NULL, 800, 600);
				/* If the mouse is moving to the left */
				if (sdlEvent.motion.xrel < 0)
					r -= 3.0f;
				/* If the mouse is moving to the right */
				else if (sdlEvent.motion.xrel > 0)
					r += 3.0f;
			}
		}

		if (sdlEvent.type = SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
			running = false;
		}

		update();
		draw(hWindow); // call the draw function
		bool checkCollision(SDL_Rect a, SDL_Rect b);
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}