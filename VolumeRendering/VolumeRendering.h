#pragma once

#include <GL/glew.h>
#include <vector>
#include <QImage>
#include <QVector3D>

class VolumeRendering {
private:
	GLuint cubeVao;
    GLuint quadVao;

	GLuint program;

	GLuint fbo;
	GLuint texture;

public:
    GLfloat projectionMatrix[16]; 
    GLfloat modelviewMatrix[16];

public:
	VolumeRendering();
	~VolumeRendering();

	void setVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data);
	void render(const QVector3D& cameraPos);
};

