#include "VolumeRendering.h"
#include <iostream>
#include "Utility.h"

float view_rotate[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

void VolumeRendering::init(int width, int height){
	_winWidth = width;
	_winHeight = height;
    _cube = CreateCubeVao();
    _quad = CreateQuadVao();
	
	loadShaderProgram();

	createData(128, 128, 128);
	//setDataVolume(data.Temperature.cur, ambientTemperature);
	_cubeinterFBO = cubeIntersectFBO(_winWidth, _winHeight); //raycasting intersection test texture

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);
}

void VolumeRendering::loadShaderProgram(){
	ShaderPrograms.advection = LoadProgram("fluidvs", "fluidgs", "advectfs");
    ShaderPrograms.buoyancy = LoadProgram("fluidvs", "fluidgs", "buoyancyfs");
    ShaderPrograms.addImpulse = LoadProgram("fluidvs", "fluidgs", "forcefs");
    ShaderPrograms.jacobi = LoadProgram("fluidvs", "fluidgs", "jacobifs");
    ShaderPrograms.divergence = LoadProgram("fluidvs", "fluidgs", "divergencefs");
    ShaderPrograms.subtractGradient = LoadProgram("fluidvs", "fluidgs", "subgradientfs");
	ShaderPrograms.boundary = LoadProgram("fluidvs", "fluidgs", "boundaryfs");
    ShaderPrograms.raycubeintersection = LoadProgram("rayboxintersectvs", "", "rayboxintersectfs");
    ShaderPrograms.raycast = LoadProgram("raycastvs", "", "raycastfs");
}

void VolumeRendering::render(){
    //get projection and model view matrix
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    float aspect = (float)_winWidth/(float)_winHeight;
    gluPerspective(60, aspect, 0.1, 1000);
    glGetFloatv (GL_PROJECTION_MATRIX, _projectionMatrix);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(0, 0, -2);//update position, based on current z translation
    glMultMatrixf( view_rotate );
    glGetFloatv (GL_MODELVIEW_MATRIX, _modelviewMatrix);//get transformation matrix
    glPopMatrix();

	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_CULL_FACE);
    // Update the ray start & stop surfaces:
    rayCubeIntersection(_cubeinterFBO);
	glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Perform the raycast to get fianl image
    renderScene();
}

void VolumeRendering::rayCubeIntersection(CubeIntersectFBO dest){
	glUseProgram(ShaderPrograms.raycubeintersection);
    
	setShaderUniform(getUniformLoc("modelviewMatrix"), (float*)&_modelviewMatrix);
    setShaderUniform(getUniformLoc("projectionMatrix"), (float*)&_projectionMatrix);
    
	glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glEnable(GL_BLEND);
    GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, buffers); 
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(_cube);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	resetState();
}

void VolumeRendering::renderScene(){
	glUseProgram(ShaderPrograms.raycast);

	setShaderUniform(getUniformLoc("raystart"), 0);
    setShaderUniform(getUniformLoc("raystop"), 1);
	setShaderUniform(getUniformLoc("density"), 2);
	setShaderUniform(getUniformLoc("temperature"), 3);
	setShaderUniform(getUniformLoc("material"), _material); 
	setShaderUniform(getUniformLoc("width"), _winWidth); 
	setShaderUniform(getUniformLoc("height"), _winHeight); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, _cubeinterFBO.texture[0]);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, _cubeinterFBO.texture[1]);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, data.Density.cur.texture);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, data.Temperature.cur.texture); 
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	resetState();
}

void VolumeRendering::resetState()
{
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, 0);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

void VolumeRendering::createData(GLsizei width, GLsizei height, GLsizei depth){
	_gridWidth = width;
	_gridHeight = height;
	_gridDepth = depth;

	data.Velocity = createDuoDataVolume(3);
	data.Density = createDuoDataVolume(1);
	data.Pressure = createDuoDataVolume(1);
	data.Temperature = createDuoDataVolume(1);

	data.Divergence = createSingleDataVolume(3);
}

void VolumeRendering::clearAllData(){
	setDataVolume(data.Velocity.cur, 0);
	setDataVolume(data.Velocity.pre, 0);
	setDataVolume(data.Density.cur, 0);
	setDataVolume(data.Density.pre, 0);
	setDataVolume(data.Pressure.cur, 0);
	setDataVolume(data.Pressure.pre, 0);
	setDataVolume(data.Temperature.cur, 0);
	setDataVolume(data.Temperature.pre, 0);

	setDataVolume(data.Divergence, 0);
	setDataVolume(data.Divergence, 0);
}

void VolumeRendering::setDataVolume(DataVolume datav, float v){
    glBindFramebuffer(GL_FRAMEBUFFER, datav.fbo);
    glClearColor(v, v, v, v);
    glClear(GL_COLOR_BUFFER_BIT);
}

DuoDataVolume VolumeRendering::createDuoDataVolume(int numComponents){
	DuoDataVolume data;
	data.cur = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	data.pre = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	return data;
}

DataVolume VolumeRendering::createSingleDataVolume(int numComponents){
	DataVolume data;
	data = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	return data;
}

//create texture which data will render to
DataVolume VolumeRendering::createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents){
	//the FBO
	GLuint fboId;
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	//create the 3D texture
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_3D, textureId);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (numComponents) {
        case 1:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, width, height, depth, 0, GL_RED, GL_HALF_FLOAT, 0);
            break;
        case 2:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RG16F, width, height, depth, 0, GL_RG, GL_HALF_FLOAT, 0);
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, width, height, depth, 0, GL_RGB, GL_HALF_FLOAT, 0);
            break;
        case 4:
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, width, height, depth, 0, GL_RGBA, GL_HALF_FLOAT, 0);
            break;
    }

    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to create 3D texture"<<std::endl;}

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureId, 0);

    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to bind texture to fbo"<<std::endl;}

    DataVolume dataVolume = {fboId, textureId};

    //init the texture as black color(value 0)
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return dataVolume;
}

CubeIntersectFBO VolumeRendering::cubeIntersectFBO(GLsizei width, GLsizei height)
{
    CubeIntersectFBO cubefbo;
    glGenFramebuffers(1, &cubefbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, cubefbo.fbo);

    for (int i = 0; i < 2; ++i) {

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        cubefbo.texture[i] = textureId;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0); 
               
        if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to create 2D texture"<<std::endl;}

        GLuint colorbuffer;
        glGenRenderbuffers(1, &colorbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
        if(i==0){
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        }
        if(i==1){
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textureId, 0);
        }
    }
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){std::cout<<"can't render to texture frontTexCoord"<<std::endl;}

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return cubefbo;
}



