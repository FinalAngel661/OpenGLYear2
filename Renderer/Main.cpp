#include "Context.h"
//#include "timer.h"
#include "Render.h"
#include <iostream>
#include <fstream>
#include "glm/ext.hpp"
#include <string>

int main()
{
	context game;
	game.init(800, 600, "Game");
	//game.enableVSync(true);

	//timer time;

	geometry Gio = loadGeometry("Res/Mod/cube.obj");

	vertex triVerts[] =
	{
		{{-.5f,-.5f,0,1}, {0,0,1,1}, {0,0}},
		{{.5f,-.5f,0,1}, {0,0,1,1}, {1,0}},
		{{0,.5f,0,1}, {0,0,1,1}, {0.5,1}}
	};
	unsigned triIndices[] = { 2,1,0 };

	geometry triangle = makeGeometry(triVerts, 3, triIndices, 3);


	const char * basicVert = "#version 410\n"
		"layout (location = 0) in vec4 position;\n"
		"void main() {gl_Position = position;}";

	const char * mvpVert = "#version 430\n"
		"layout (location = 0) in vec4 position;\n"
		"layout (location = 1) in vec4 normal;\n"
		"layout (location = 2) in vec2 uv;\n"
		"out vec2 vUV;\n"
		"out vec3 vNormal;"
		"layout (location = 0) uniform mat4 proj;"
		"layout (location = 1) uniform mat4 view;"
		"layout (location = 2) uniform mat4 model;"
		"void main() { gl_Position = proj * view * model * position; vUV = uv; vNormal = normalize(model * normal).xyz; }";

	const char * basicFrag = "#version 330\n"
		"out vec4 vertColor1;\n"
		"out vec4 vertColor2;\n"
		"out vec4 vertColor3;\n"
		"void main() {vertColor1 = vec4(1.0, 0.0, 0.0, 1.0);\n vertColor2 = vec4(0.0,1.0,0.0,1.0);\n vertColor3 = vec4(0.0,0.0,1.0,1.0);}";

	const char * texFrag = "#version 430\n"
		"in vec2 vUV;\n"
		"in vec3 vNormal;"
		"out vec4 outColor;\n"
		"layout (location = 3) uniform sampler2D albedo;\n"
		"layout (location = 4) uniform vec3 lightDir;"
		"void main() {float diffuse = max(0, dot(vNormal, -lightDir)); outColor = texture(albedo, vUV); outColor = vec4(outColor.x * diffuse, outColor.y * diffuse, outColor.z * diffuse, 1); }";

	shader basicShad = makeShader(basicVert, basicFrag);
	shader mvpShad = makeShader(mvpVert, basicFrag);
	shader texShad = makeShader(mvpVert, texFrag);

	//unsigned char whitePixel[] = {255, 255, 255};
	//texture whiteTexture = makeTexture(1, 1, 3, whitePixel);
	texture pix = loadTexture("Res/UV/Hazama1.png");
	

	glm::mat4 cam_proj = glm::perspective(glm::radians(45.f), 800/600.0f, 0.1f, 1000.0f);
	glm::mat4 cam_view = glm::lookAt(glm::vec3(0,0,-2), glm::vec3(0,0,0), glm::vec3(0,1,0));
	glm::mat4 model = glm::identity < glm::mat4>();

	glm::vec3 lightDir = glm::vec3(-1, 0, 0);

	while (!game.shouldClose())
	{
		game.tick();
		game.clear();

		model = glm::rotate(model, glm::radians(5.f), glm::vec3(0, 1, 0));

		// Draw Logic
		setUniform(texShad, 0, cam_proj);
		setUniform(texShad, 1, cam_view);
		setUniform(texShad, 2, model);
		setUniform(texShad, 3, pix, 0);
		setUniform(texShad, 4, lightDir);
		
		draw(texShad, Gio);

	}

	game.term();


	return 0;
}
