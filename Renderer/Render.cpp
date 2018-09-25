#include "Render.h"
#include "glm/ext.hpp"
#include <fstream>
#include <iostream>
#include <string>

#define GLM_SWIZZLE
#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <cstdio>
#include <cassert>
#include "stb_image.h"
#include "tiny_obj_loader.h"

geometry makeGeometry(vertex * verts,
	size_t vertCount, unsigned int * indices, size_t indexCount)
{
	//create an instance of geometry
	geometry newGeo = {};
	newGeo.size = indexCount;

	//generate buffers
	glGenVertexArrays(1, &newGeo.vao);
	glGenBuffers(1, &newGeo.vbo);
	glGenBuffers(1, &newGeo.ibo);


	//bind buffers
	glBindVertexArray(newGeo.vao);
	glBindBuffer(GL_ARRAY_BUFFER, newGeo.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newGeo.ibo);

	//populate buffers
	glBufferData(GL_ARRAY_BUFFER, 
		vertCount * sizeof(vertex), verts, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
		indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	//Describe vertex data
	glEnableVertexAttribArray(0); //Position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	glEnableVertexAttribArray(1); // Normal
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)16);
	glEnableVertexAttribArray(2); // UV
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)32);


	//unbind buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//return geometry
	return newGeo;

}

geometry loadGeometry(const char * imagePath)
{
	geometry retval = { 0,0,0,0 };

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, imagePath);

	size_t isize = shapes[0].mesh.indices.size();
	size_t *indices = new unsigned[isize];

	size_t vsize = isize;
	vertex *verts = new vertex[vsize];

	for (int i = 0; i < isize; ++i)
	{
		indices[i] = i;

		int pi = shapes[0].mesh.indices[i].vertex_index;
		int ni = shapes[0].mesh.indices[i].normal_index;
		int ti = shapes[0].mesh.indices[i].texcoord_index;

		const float *p = &attrib.vertices[pi * 3];
		const float *n = &attrib.normals[ni * 3];
		const float *t = &attrib.texcoords[ti * 2];

		verts[i].pos = { p[0], p[1],p[2],1 };
		verts[i].uv = { t[0],t[1] };
		verts[i].norm = {n[0], n[1], n[2],0};

	}

	retval = makeGeometry(verts, vsize, indices, isize);

	delete[] verts;
	delete[] indices;

	return retval;
}


std::string fileToString(const char *imagePath)
{
	std::ifstream t(imagePath);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}

void freeGeometry(geometry & geo)
{
	glDeleteBuffers(1, &geo.vbo);
	glDeleteBuffers(1, &geo.ibo);
	glDeleteVertexArrays(1, &geo.vao);

	geo = {};
}

shader makeShader(const char * vertSource, const char * fragSource)
{
	//make shader object
	shader newShad = {};
	newShad.program = glCreateProgram();

	//create shader
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

	//compile shader
	glShaderSource(vert, 1, &vertSource, 0);
	glShaderSource(frag, 1, &fragSource, 0);
	glCompileShader(vert);
	glCompileShader(frag);

	GLint vertexCompiled = 0;
	glGetShaderiv(vert, GL_COMPILE_STATUS, &vertexCompiled);
	if (vertexCompiled != GL_TRUE)
	{
		GLsizei logLen = 0;
		GLchar message[1024];
		glGetShaderInfoLog(vert, 1024, &logLen, message);
		std::cout << message << std::endl;
	}

	//attach shader
	glAttachShader(newShad.program, vert);
	glAttachShader(newShad.program, frag);

	//link shaders
	glLinkProgram(newShad.program);

	//delete shaders
	glDeleteShader(vert);
	glDeleteShader(frag);

	//return the shader object

	return newShad;
}

shader loadShader(const char * vertSource, const char * fragSource)
{
	shader retval = { 0 };

	std::string vert = fileToString(vertSource);
	std::string frag = fileToString(fragSource);

	retval = makeShader(vert.c_str(), frag.c_str());

	return retval;
}

void freeShader(shader & shad)
{
	glDeleteProgram(shad.program);
	shad = {};

}

texture makeTexture(unsigned width, unsigned height, unsigned channels, const unsigned char * pixels)
{
	GLenum ogFormat = 0;
	switch (channels)
	{
	case 1: ogFormat = GL_RED; break;
	case 2: ogFormat = GL_RG; break;
	case 3: ogFormat = GL_RGB; break;
	case 4: ogFormat = GL_RGBA; break;
	}

	texture newTex = { 0,width,height,channels };

	glGenTextures(1, &newTex.handle);
	glBindTexture(GL_TEXTURE_2D, newTex.handle);

	glTexImage2D(GL_TEXTURE_2D, 0, ogFormat, width, height, 0, ogFormat, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return newTex;
}

void freeTexture(texture & tex)
{
	glDeleteTextures(1, &tex.handle);
	tex = {};
}

texture loadTexture(const char * imagePath)
{
	int imageWidth, imageHeight, imageFormat;
	imageWidth = imageHeight = imageFormat = -1;
	unsigned char *rawPixelData = nullptr;

	//Load Image
	stbi_set_flip_vertically_on_load(true);
	rawPixelData = stbi_load(imagePath, &imageWidth, &imageHeight, &imageFormat, STBI_default);

	//pass it to OpenGL
	texture newTex = makeTexture(imageWidth, imageHeight, imageFormat, rawPixelData);

	//Destroy any other data
	stbi_image_free(rawPixelData);

	return newTex;
}



void draw(const shader & shad, const geometry & geo)
{
	glUseProgram(shad.program);
	glBindVertexArray(geo.vao);

	glDrawElements(GL_TRIANGLES, geo.size, GL_UNSIGNED_INT, 0);
}

void setUniform(const shader & shad, GLuint location, const glm::mat4 &value)
{
	glProgramUniformMatrix4fv(shad.program, location, 1, GL_FALSE, glm::value_ptr(value));

}

void setUniform(const shader & shad, GLuint location, const texture & value, GLuint textureSlot)
{
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_2D, value.handle);
	glProgramUniform1i(shad.program, location, textureSlot);

}

void setUniform(const shader & shad, GLuint location, const glm::vec3 & value)
{
	glProgramUniform3fv(shad.program, location, 1, glm::value_ptr(value));

}
