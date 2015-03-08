#include "DataManager.h"
#include <iostream>

void DataManager::createData(GLsizei width, GLsizei height, GLsizei depth){
	_gridWidth = width;
	_gridHeight = height;
	_gridDepth = depth;

	data = createCloudVolumeData(_gridWidth, _gridHeight, _gridDepth);
}

/**
 * 全ての3Dテクスチャの値を0にリセットする。
 */
void DataManager::clearAllData(){
	setDataVolume(data, 0);
}

/**
 * datavに括り付けられた3Dテクスチャの各テクセルの値を、vにセットする。
 * datavには、fboが括り付けられており、OpenGLに対してfboを指定することで、
 * 対応する3Dテクスチャに対して操作が出来るようになっている。
 *
 * @param datav		3Dテクスチャを含むデータ
 * @param v			セットする値
 */
void DataManager::setDataVolume(DataVolume datav, float v){
	// datavのfboをバインドすることで、以降の描画命令は、
	// このfboに括り付けられたテクスチャに対して行われる。
    glBindFramebuffer(GL_FRAMEBUFFER, datav.fbo);

	// クリアする色をセットする
    glClearColor(v, v, v, v);

	// datavのfboに括り付けられた3Dテクスチャの内容を全て、指定した値にセットする
    glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * RayTracingに必要となる、前面／背面の交点計算用のfbo、および、それに対応する2Dテクスチャを生成する。
 * fboに2Dテクスチャを括り付けることで、以後は、OpenGLに対してfboを介して2Dテクスチャにアクセスできるようになる。
 *
 * @param width		画面の幅
 * @param height	画面の高さ
 * @return			fboと対応する2Dテクスチャ
 */
CubeIntersectFBO DataManager::cubeIntersectFBO(GLsizei width, GLsizei height)
{
    CubeIntersectFBO cubefbo;

	// fboを生成する
    glGenFramebuffers(1, &cubefbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, cubefbo.fbo);

    for (int i = 0; i < 2; ++i) {
		// 2Dテクスチャを生成する
        GLuint textureId;
        glGenTextures(1, &textureId);

		// 生成したテクスチャのパラメータを設定する
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        cubefbo.texture[i] = textureId;

		// 生成したテクスチャ用に、メモリを確保する
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0); 
               
        if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to create 2D texture"<<std::endl;}

		// 以下の３行、なくても動作するみたい。不要のようだ。。。。
        /*GLuint colorbuffer;
        glGenRenderbuffers(1, &colorbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);*/

		// 生成した2Dテクスチャを、fboに括り付ける。
		// 以後、OpenGLに対しては、fboを指定することで、この2Dテクスチャにアクセスできるようになる。
		// （１つのfboに、前面用の2Dテクスチャと、背面用の2Dテクスチャを括り付ける）
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

/**
 * FBOと、対応する、指定した幅、高さ、奥行きの3Dテクスチャを生成し、
 * 雲みたいなやつをセットする。
 *
 * @param width				幅
 * @param height			高さ
 * @param depth				奥行き
 * @param numComponents		コンポーネントの数 (各テクセルがscalarなら1、RGBベクトルなら3）
 * @return					FBOと対応する3Dテクスチャを返却する。
 */
DataVolume DataManager::createCloudVolumeData(GLsizei width, GLsizei height, GLsizei depth){
	// 雲データの作成
	float sigma = (float)width * 0.3;
	float* data = new float[width * height * depth];
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			for (int z = 0; z < depth; ++z) {
				float dist = (x - width * 0.5) * (x - width * 0.5)  + (y - height * 0.5) * (y - height * 0.5) + (z - depth * 0.5) * (z - depth * 0.5);
				
				if (dist < sigma * sigma) {
					data[z * width * height + y * width + x] = expf(-dist/2.0/sigma/sigma);
				} else {
					data[z * width * height + y * width + x] = 0.0f;
				}
			}
		}
	}

	//the FBO
	GLuint fboId;
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	// 3Dテクスチャを生成
	GLuint textureId;
	glGenTextures(1, &textureId);

	// 生成した3Dテクスチャのパラメータを設定する
	glBindTexture(GL_TEXTURE_3D, textureId);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 3Dテクスチャ用のメモリを確保する
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, width, height, depth, 0, GL_RED, GL_FLOAT, data);
    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to create 3D texture"<<std::endl;}

	delete [] data;

	// 生成した3Dテクスチャを、fboに括り付ける。
	// 以後、OpenGLに対しては、fboを指定することで、この3Dテクスチャにアクセスできるようになる。
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureId, 0);

    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to bind texture to fbo"<<std::endl;}

    DataVolume dataVolume = {fboId, textureId};

	// 以下の初期化は、不要だった。
	// こいつのせいで、せっかくセットしたテクスチャが全部消えちゃっていた。
    //init the texture as black color(value 0)
    //glClearColor(0, 0, 0, 0);
    //glClear(GL_COLOR_BUFFER_BIT);

	// 出力先をスクリーンに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return dataVolume;
}

