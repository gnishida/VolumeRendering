#pragma once

#include <GL/glew.h>
#include <vector>
#include <QImage>
#include <QVector3D>

class VolumeRendering {
private:
	int winWidth;
	int winHeight;

	GLuint cubeVao;
    GLuint quadVao;

	GLuint program;

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
	void update(const QVector3D& cameraPos);

private:
	void render(const QVector3D& cameraPos);
};

