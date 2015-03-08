#include "VolumeRendering.h"
#include <iostream>
#include "Utility.h"

void VolumeRendering::init(int winWidth, int winHeight, int gridWidth, int gridHeight, int gridDepth, float* data) {
	this->winWidth = winWidth;
	this->winHeight = winHeight;

	program_raycubeintersection = LoadProgram("rayboxintersectvs", "rayboxintersectfs");
    program_raycast = LoadProgram("raycastvs", "raycastfs");

	createVolumeData(gridWidth, gridHeight, gridDepth, data);

    cubeVao = CreateCubeVao();
    quadVao = CreateQuadVao();
	cubeinterFBO = cubeIntersectFBO(winWidth, winHeight); //raycasting intersection test texture

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);
}

void VolumeRendering::setWindowSize(int width, int height) {
	winWidth = width;
	winHeight = height;
}

void VolumeRendering::update() {
    // 画面サイズなどから、projectionMatrixを計算する
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    float aspect = (float)winWidth/(float)winHeight;
    gluPerspective(60, aspect, 0.1, 1000);
    glGetFloatv (GL_PROJECTION_MATRIX, projectionMatrix);
    glPopMatrix();

	// 回転などから、modelviewMatrixを計算する
	// GLWidget側で計算するので、ここでは不要になったよ！
	/*
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(0, 0, -trans_z[0]);//update position, based on current z translation
    glMultMatrixf( view_rotate );
    glGetFloatv (GL_MODELVIEW_MATRIX, _modelviewMatrix);//get transformation matrix
    glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	*/

    // RayCastのための、キューブの前面／背面の交点を計算する
    rayCubeIntersection(cubeinterFBO);

    // RayCastを実施して、スクリーンに描画する
    render();
}

/**
 * 画面のピクセルに対応する、キューブの前面／背面の交点を計算し、
 * destに括りついた２つの2Dテクスチャにそれぞれ格納する。
 *
 * @param dest		キューブの前面／背面の交点の座標を計算するためのfboと２つの2Dテクスチャ
 */
void VolumeRendering::rayCubeIntersection(CubeIntersectFBO dest) {
	// キューブの前面／背面の交点を計算するGPUシェーダを選択
	glUseProgram(program_raycubeintersection);
    
	// GPUシェーダに、パラメータを渡す
	// シミュレーションをしているキューブが、ワールド座標系の原点を中心として、
	// (-1,-1,-1) - (1,1,1)のサイズである。
	// これに対して、カメラが移動しているので、このキューブを回転、移動しなければいけない。
	// なので、modelviewMatrixとprojectionMatrixをカメラの位置などに基づいて計算し、
	// シェーダに渡している。
	setShaderUniform(getUniformLoc("modelviewMatrix"), (float*)&modelviewMatrix);
    setShaderUniform(getUniformLoc("projectionMatrix"), (float*)&projectionMatrix);
    
	// 画面バッファに、destのfboをバインドすることで、
	// 以降の描画命令は、これに括りついたテクスチャに対して行われるようになる。
	// さらに、ATTACHMENT0、ATTACHMENT1を、それぞれフラグメントシェーダの出力先として指定する。
	// これにより、２つの2Dテクスチャに対して、
	// rayboxintersectfs.glslシェーダ側で、glFragColor[0]、[1]としてアクセスできるようになる。
	glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, buffers); 

	// rayと交差する２つの三角形のうち、カメラから遠いほうは、表面ではなく、背面から
	// rayが当たるため、GL_CULL_FACEしちゃうと、rayが当たってないとして無視されちゃうので、
	// GL_CULL_FACEを無効にする。
	glDisable(GL_CULL_FACE);

	// GL_ONE、GL_ONEを指定してBLENDすることで、画面の各ピクセルに対応する２つの三角形の
	// ワールド座標系での座標を、ORを使って、両方ともうまいこと記録できる。
	// （詳細は、rayboxintersectfs.glslを参照のこと）
    glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// クリアする色(0,0,0,0)を指定し、全画面をクリアする。
	// これにより、画面の各ピクセルに対応する、キューブとの交点は0に初期化された
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

	// rayboxintersectfs.glslシェーダ：
	// 画面上の各ピクセルに対応するrayは、キューブを構成する６面、つまり、１２個の三角形の
	// のうち、２つと交差する。
	// このうち、カメラに近い側の交点のワールド座標系での座標が、glFragColor[0]の対応するテクセルに、
	// 色情報として格納される。
	// つまり、該当テクセルのRGBとして、実際にはXYZが格納される。
	// また、カメラから遠い側の交点のワールド座標系での座標が、glFragColor[1]の対応するテクセルに、
	// 同様に、色情報として格納される。
    glBindVertexArray(cubeVao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0); // 頂点をindexで指定するので、glDrawElementsを使う
	                                                        // また、index数は36個あるので、引数は36。

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

void VolumeRendering::render() {
	// raycastするGPUシェーダを選択
	// rayCubeIntersectionと違い、modelviewMatrixやprojectionMatrixを使わないことに注意！
	// デフォルトのままなので、つまり、(-1,-1)-(1,1)のOrtho射影を使用する。
	// そして、(-1,-1)-(1,1)の正方形を描画するので、結局、画面全体に対して、
	// 各ピクセルに対応してfragmentシェーダが起動されることになる。
	// 各ピクセルに対応するrayと、キューブとの交点は、既にテクスチャとして計算済みなので、
	// 後はそれを使って、ray castして色を計算すれば良い。
	// なので、modelviewMatrixやprojectionMatrixを使う必要がないのだ。
	// ちなみに、vertexシェーダ(raycastvs.glsl)でも、頂点座標をそのままフラグメントシェーダに
	// 渡していることが分かる。しかも、xy座標だけ。2Dで十分なので、z座標は使ってない。
	// このことは、CreateQuadVao()関数を見ても分かる。xy座標しかセットしていない。
	glUseProgram(program_raycast);

	setShaderUniform(getUniformLoc("raystart"), 0);
    setShaderUniform(getUniformLoc("raystop"), 1);
	setShaderUniform(getUniformLoc("density"), 2);
	setShaderUniform(getUniformLoc("width"), winWidth); 
	setShaderUniform(getUniformLoc("height"), winHeight); 

	// フレームバッファとして０をバインドすることで、
	// これ以降の描画は、実際のスクリーンに対して行われる。
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// rayとの前面の交点を格納したテクスチャを、テクスチャ０として使用する
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, cubeinterFBO.texture[0]);

	// rayとの背面の交点を格納したテクスチャを、テクスチャ１として使用する	
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, cubeinterFBO.texture[1]);

	// 密度データを格納した3Dテクスチャを、テクスチャ２として使用する
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, texture);

	// クリアする色をセットする
    glClearColor(0, 0, 0, 0);

	// フレームバッファをクリアする
	// （スクリーンが出力先バッファとして指定されているので、スクリーンがクリアされる）
    glClear(GL_COLOR_BUFFER_BIT);

	// Ray castingのため、alphaブレンディングを使用するよう、Blendオプションを設定する
	glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// raycastfs.glslシェーダ：
	// １：画面上の各ピクセルに対応するrayについて、前面の交点座標(enter)をテクスチャ０から、
	// 　　背面の交点座標(leave)をテクスチャ１から取得する。
	// ２：rayの方向ベクトル(ray)を計算し、毎ステップで移動するベクトルを計算する(step)。
	// ３：現在座標(pos=enter)から、rayの方向に、stepベクトルずつ進みながら、
	// 　　- テクスチャ２から、密度(sampleDens)を取得。
	//　　 - アルファ(alpha)を更新。
	//　　 - 光源方向へ、lightDirベクトルずつ進みながら、光量(lalpha)を計算する。
	//　　 - 光量から色を決定し、色(color)を更新。
	//　　 - アルファが0に十分小さくなる、もしくは、背面の交点に達すれば、終了
	// ４：アルファ、色をglFragColorにセットする。
    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 頂点座標の配列をそのまま使うので、glDrawArraysを使う
	                                       // 頂点が4つあるので、引数は4。
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

/**
 * datavに括り付けられた3Dテクスチャの各テクセルの値を、vにセットする。
 * datavには、fboが括り付けられており、OpenGLに対してfboを指定することで、
 * 対応する3Dテクスチャに対して操作が出来るようになっている。
 *
 * @param datav		3Dテクスチャを含むデータ
 * @param v			セットする値
 */
void VolumeRendering::setDataVolume(float v) {
	// datavのfboをバインドすることで、以降の描画命令は、
	// このfboに括り付けられたテクスチャに対して行われる。
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

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
CubeIntersectFBO VolumeRendering::cubeIntersectFBO(GLsizei width, GLsizei height) {
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

		// 生成した2Dテクスチャを、fboに括り付ける。
		// 以後、OpenGLに対しては、fboを指定することで、この2Dテクスチャにアクセスできるようになる。
		// （１つのfboに、前面用の2Dテクスチャと、背面用の2Dテクスチャを括り付ける）
        if (i == 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        } else {
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
void VolumeRendering::createVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data) {
	//the FBO
	glGenFramebuffers(1, &this->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

	// 3Dテクスチャを生成
	glGenTextures(1, &this->texture);

	// 生成した3Dテクスチャのパラメータを設定する
	glBindTexture(GL_TEXTURE_3D, this->texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 3Dテクスチャ用のメモリを確保する
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, width, height, depth, 0, GL_RED, GL_FLOAT, data);
    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to create 3D texture"<<std::endl;}

	// 生成した3Dテクスチャを、fboに括り付ける。
	// 以後、OpenGLに対しては、fboを指定することで、この3Dテクスチャにアクセスできるようになる。
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->texture, 0);

    if(GL_NO_ERROR != glGetError()){std::cout<<"Unable to bind texture to fbo"<<std::endl;}

	// 出力先をスクリーンに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

