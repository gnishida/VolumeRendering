#pragma once

#include <GL/glew.h>

struct CubeIntersectFBO {
	GLuint fbo;
	GLuint texture[2];
};

class VolumeRendering {
public:
	int winWidth;
	int winHeight;
	int gridWidth;
	int gridHeight;
	int gridDepth;

	GLuint cubeVao;
    GLuint quadVao;
	CubeIntersectFBO cubeinterFBO;

	GLuint program_raycubeintersection;
	GLuint program_raycast;

    GLfloat projectionMatrix[16]; 
    GLfloat modelviewMatrix[16];

	GLuint fbo;
	GLuint texture;

public:
	VolumeRendering() {}

	void init(int winWidth, int winHeight, int gridWidth, int gridHeight, int gridDepth);
	void setWindowSize(int width, int height);
	void update();

	//data render functions
	void rayCubeIntersection(CubeIntersectFBO dest);
	void render();

    void resetState(); //reset framebuffer, texture, etc after rendering

	void setDataVolume(float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);
	void createVolumeData(GLsizei width, GLsizei height, GLsizei depth);

};

