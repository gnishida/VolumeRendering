#pragma once
#include "DataManager.h"
#include "Utility.h"

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

class GPGPU
{
public:
	GPGPU();

	void init(int width, int height);
	void restart();
	void update();
	void render();
	void updateData();

	void loadShaderProgram();
	void setMaterial(int material); //0 smoke, 1 fire, 2 smoke&fire
	void setWindowSize(int width, int height);

public:
	DataManager dataman;

	int _winWidth;
	int _winHeight;
	int _material;

	//Vertex Array Objects
    GLuint _cube; //volume cube
    GLuint _quad; //screen quad

	CubeIntersectFBO _cubeinterFBO;

    GLfloat _projectionMatrix[16]; 
    GLfloat _modelviewMatrix[16];

	//data render functions
	void rayCubeIntersection(CubeIntersectFBO dest);
	void renderScene();

	//data update functions
    void updateAdvect(DataVolume velocity, DataVolume source, DataVolume dest, float dissipation);
    void updateBuoyancy(DataVolume velocity, DataVolume temperature, DataVolume density, DataVolume dest);
    void updateImpulse(DataVolume dest, float* position, float value);
    void computeDivergence(DataVolume velocity, DataVolume dest);
    void jacobi(DataVolume pressure, DataVolume divergence, DataVolume dest, float alpha, float rBeta);
	void subtractPressureGradient(DataVolume velocity, DataVolume pressure, DataVolume dest);
	void updateBoundary(DataVolume source, DataVolume dest, float scale);

    void resetState(); //reset framebuffer, texture, etc after rendering
	void swapTexture(DuoDataVolume* data);
};

