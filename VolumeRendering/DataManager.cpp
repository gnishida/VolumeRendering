#include "DataManager.h"
#include <iostream>

void DataManager::createData(GLsizei width, GLsizei height, GLsizei depth){
	_gridWidth = width;
	_gridHeight = height;
	_gridDepth = depth;

	data.Velocity = createDuoDataVolume(3);
	data.Density = createDuoDataVolume(1);
	data.Pressure = createDuoDataVolume(1);
	data.Temperature = createDuoDataVolume(1);

	data.Divergence = createSingleDataVolume(3);
}

void DataManager::clearAllData(){
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

void DataManager::setDataVolume(DataVolume datav, float v){
    glBindFramebuffer(GL_FRAMEBUFFER, datav.fbo);
    glClearColor(v, v, v, v);
    glClear(GL_COLOR_BUFFER_BIT);
}

DuoDataVolume DataManager::createDuoDataVolume(int numComponents){
	DuoDataVolume data;
	data.cur = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	data.pre = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	return data;
}

DataVolume DataManager::createSingleDataVolume(int numComponents){
	DataVolume data;
	data = createVolumeData(_gridWidth, _gridHeight, _gridDepth, numComponents);
	return data;
}

//create texture which data will render to
DataVolume DataManager::createVolumeData(GLsizei width, GLsizei height, GLsizei depth, int numComponents){
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

CubeIntersectFBO DataManager::cubeIntersectFBO(GLsizei width, GLsizei height)
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



