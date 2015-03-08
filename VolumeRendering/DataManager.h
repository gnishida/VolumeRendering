#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <string>

struct CubeIntersectFBO {
	GLuint fbo;
	GLuint texture[2];
};

class DataManager
{
public:
	GLuint fbo;
	GLuint texture;

public:
	DataManager() {}

	void createData(GLsizei gridwidth, GLsizei gridheight, GLsizei griddepth);

	void setDataVolume(float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);

private:
	GLsizei _gridWidth;
	GLsizei _gridHeight;
	GLsizei _gridDepth;

	void createVolumeData(GLsizei width, GLsizei height, GLsizei depth);
};
