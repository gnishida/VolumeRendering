#include "VolumeRendering.h"
#include <iostream>
#include "Util.h"

VolumeRendering::VolumeRendering() {
    program = Util::LoadProgram("raycastvs", "raycastfs");

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);

	fbo = 0;
	texture = 0;
	cubeVao = 0;
}

VolumeRendering::~VolumeRendering() {
	if (fbo > 0) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &texture);
	}

	if (cubeVao > 0) {
		glDeleteVertexArrays(1, &cubeVao);
	}
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
void VolumeRendering::setVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data) {
	gridWidth = width;
	gridHeight = height;
	gridDepth = depth;

	if (fbo > 0) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &texture);
	}

	// 3Dデータを囲むボックスを生成
    cubeVao = Util::CreateBoxVao(width, height, depth);

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

/**
 * 画面のピクセルに対応する、キューブの前面／背面の交点を計算し、
 * destに括りついた２つの2Dテクスチャにそれぞれ格納する。
 *
 * @param dest		キューブの前面／背面の交点の座標を計算するためのfboと２つの2Dテクスチャ
 */
void VolumeRendering::render(const QVector3D& cameraPos) {
	if (cubeVao == 0) return;

	// キューブの前面／背面の交点を計算するGPUシェーダを選択
	glUseProgram(program);
    
	// GPUシェーダに、パラメータを渡す
	// シミュレーションをしているキューブが、ワールド座標系の原点を中心として、
	// (-1,-1,-1) - (1,1,1)のサイズである。
	// これに対して、カメラが移動しているので、このキューブを回転、移動しなければいけない。
	// なので、modelviewMatrixとprojectionMatrixをカメラの位置などに基づいて計算し、
	// シェーダに渡している。
	glUniformMatrix4fv(glGetUniformLocation(program, "modelviewMatrix"), 1, 0, (float*)&modelviewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "projectionMatrix"), 1, 0, (float*)&projectionMatrix);
	glUniform3f(glGetUniformLocation(program, "gridSize"), gridWidth, gridHeight, gridDepth);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x(), cameraPos.y(), cameraPos.z());
    glUniform1i(glGetUniformLocation(program, "density"), 0);

	// フレームバッファとして０をバインドすることで、
	// これ以降の描画は、実際のスクリーンに対して行われる。
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 密度データを格納した3Dテクスチャを、テクスチャ２として使用する
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, texture);

	// rayと交差する２つの三角形のうち、カメラから遠いほうは、表面ではなく、背面から
	// rayが当たるため、GL_CULL_FACEしちゃうと、rayが当たってないとして無視されちゃうので、
	// GL_CULL_FACEを無効にする。
	glDisable(GL_CULL_FACE);

	// GL_ONE、GL_ONEを指定してBLENDすることで、画面の各ピクセルに対応する２つの三角形の
	// ワールド座標系での座標を、ORを使って、両方ともうまいこと記録できる。
	// （詳細は、rayboxintersectfs.glslを参照のこと）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
