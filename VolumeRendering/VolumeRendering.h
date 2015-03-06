#pragma once

#include <GL/glew.h>

static struct {
    GLuint advection;
    GLuint buoyancy;
    GLuint addImpulse;
    GLuint jacobi;
    GLuint divergence;
    GLuint subtractGradient;
	GLuint boundary;
    GLuint raycubeintersection;
    GLuint raycast;
} ShaderPrograms;

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
	int _winWidth;
	int _winHeight;
    GLuint _cube; //volume cube
    GLuint _quad; //screen quad
	CubeIntersectFBO _cubeinterFBO;

    GLfloat _projectionMatrix[16]; 
    GLfloat _modelviewMatrix[16];

	GLsizei _gridWidth;
	GLsizei _gridHeight;
	GLsizei _gridDepth;
	RenderData data;

public:
	VolumeRendering() {}

	void init(int width, int height);
	void loadShaderProgram();
	void render();
	void rayCubeIntersection(CubeIntersectFBO dest);
	void renderScene();
	void resetState();
	void createData(GLsizei gridwidth, GLsizei gridheight, GLsizei griddepth);
	void clearAllData();
	void setDataVolume(DataVolume datav, float value);
	CubeIntersectFBO cubeIntersectFBO(GLsizei width, GLsizei height);

private:
	DuoDataVolume createDuoDataVolume(int numComponents);
	DataVolume createSingleDataVolume(int numComponents);
	DataVolume createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
};

