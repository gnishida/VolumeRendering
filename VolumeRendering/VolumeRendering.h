#pragma once

#include <GL/glew.h>

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
};

class VolumeRendering {
private:
	GLsizei _gridWidth;
	GLsizei _gridHeight;
	GLsizei _gridDepth;
	RenderData data;

public:
	VolumeRendering() {}

	void createData(GLsizei gridwidth, GLsizei gridheight, GLsizei griddepth);
	void clearAllData();
	void setDataVolume(DataVolume datav, float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);

private:
	DuoDataVolume createDuoDataVolume(int numComponents);
	DataVolume createSingleDataVolume(int numComponents);
	DataVolume createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
};

