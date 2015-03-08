#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <string>

struct CubeIntersectFBO {
	GLuint fbo;
	GLuint texture[2];
};

//structure of rendering volume
struct DataVolume {
	GLuint fbo;
	GLuint texture;
};

class DataManager
{
public:
	DataManager(){};

	void createData(GLsizei gridwidth, GLsizei gridheight, GLsizei griddepth);
	void clearAllData();
	DataVolume data;

	//set a default value to whole texture
	void setDataVolume(DataVolume datav, float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);

private:
	GLsizei _gridWidth;
	GLsizei _gridHeight;
	GLsizei _gridDepth;

	DataVolume createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
	DataVolume createCloudVolumeData(GLsizei width, GLsizei height, GLsizei depth);
};
