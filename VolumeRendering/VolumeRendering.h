#pragma once

#include <GL/glew.h>
#include <vector>
#include <QImage>
#include <QVector3D>

class VolumeRendering {
private:
	int gridWidth;
	int gridHeight;
	int gridDepth;

	GLuint program;

	GLuint texture;
	GLuint boxVao;

public:
    GLfloat projectionMatrix[16]; 
    GLfloat modelviewMatrix[16];

public:
	VolumeRendering();
	~VolumeRendering();

	void setVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data);
	void render(const QVector3D& cameraPos);
};

