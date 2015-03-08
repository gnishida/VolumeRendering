#pragma once

#include <GL/glew.h>
#include <vector>
#include <QImage>

struct CubeIntersectFBO {
	GLuint fbo;
	GLuint texture[2];
};

class VolumeRendering {
private:
	int winWidth;
	int winHeight;

	GLuint cubeVao;
    GLuint quadVao;
	CubeIntersectFBO cubeinterFBO;

	GLuint program_raycubeintersection;
	GLuint program_raycast;

	GLuint fbo;
	GLuint texture;

public:
    GLfloat projectionMatrix[16]; 
    GLfloat modelviewMatrix[16];

public:
	VolumeRendering(int winWidth, int winHeight);
	~VolumeRendering();

	void setVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data);
	void setWindowSize(int width, int height);
	void update();

private:
	void rayCubeIntersection(CubeIntersectFBO dest);
	void render();
	void cubeIntersectFBO(GLsizei width, GLsizei height);
};

