#pragma once
#include "DataManager.h"
#include "Utility.h"

static struct {
    GLuint raycubeintersection;
    GLuint raycast;
} ShaderPrograms;

class GPGPU
{
public:
	GPGPU();

	void init(int width, int height);
	void update();

	void loadShaderProgram();
	void setWindowSize(int width, int height);

public:
	DataManager dataman;

	int _winWidth;
	int _winHeight;

	//Vertex Array Objects
    GLuint _cube; //volume cube
    GLuint _quad; //screen quad

	CubeIntersectFBO _cubeinterFBO;

    GLfloat _projectionMatrix[16]; 
    GLfloat _modelviewMatrix[16];

	//data render functions
	void rayCubeIntersection(CubeIntersectFBO dest);
	void render();

    void resetState(); //reset framebuffer, texture, etc after rendering
};

