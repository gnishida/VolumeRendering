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

struct DuoDataVolume {
	DataVolume cur; 
	DataVolume pre;
};

struct RenderData {
	//Duo for swap
	DuoDataVolume Velocity;
    DuoDataVolume Density;
    DuoDataVolume Pressure;
    DuoDataVolume Temperature;

    //single data
    DataVolume Divergence;

	DataVolume Test;
};

class DataManager
{
public:
	DataManager(){};

	void createData(GLsizei gridwidth, GLsizei gridheight, GLsizei griddepth);
	void clearAllData();
	RenderData data;

	//set a default value to whole texture
	void setDataVolume(DataVolume datav, float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);

private:
	GLsizei _gridWidth;
	GLsizei _gridHeight;
	GLsizei _gridDepth;

	DuoDataVolume createDuoDataVolume(int numComponents);
	DataVolume createSingleDataVolume(int numComponents);
	DataVolume createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
	DataVolume createCloudVolumeData(GLsizei width, GLsizei height, GLsizei depth);
};
