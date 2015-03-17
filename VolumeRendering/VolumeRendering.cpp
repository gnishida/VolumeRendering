#include "VolumeRendering.h"
#include <iostream>
#include "Util.h"

VolumeRendering::VolumeRendering() {
    program = Util::LoadProgram("raycastvs", "raycastfs");

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);

	texture = 0;
	boxVao = 0;
}

VolumeRendering::~VolumeRendering() {
	if (texture > 0) {
		glDeleteTextures(1, &texture);
	}

	if (boxVao > 0) {
		glDeleteVertexArrays(1, &boxVao);
	}
}

/**
 * 指定した幅、高さ、奥行きの3Dテクスチャを生成し、指定した3Dデータをテクスチャにセットする。
 *
 * @param width		幅
 * @param height	高さ
 * @param depth		奥行き
 * @param data		3Dデータ
 */
void VolumeRendering::setVolumeData(GLsizei width, GLsizei height, GLsizei depth, float* data) {
	gridWidth = width;
	gridHeight = height;
	gridDepth = depth;

	if (texture > 0) {
		glDeleteTextures(1, &texture);
	}
	if (boxVao > 0) {
		glDeleteVertexArrays(1, &boxVao);
	}

	// 3Dデータを囲むボックスを生成
	boxVao = Util::CreateBoxVao(width, height, depth);

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
    if (GL_NO_ERROR != glGetError()) {
		std::cout << "Unable to create 3D texture"<< std::endl;
	}
}

/**
 * 画面のピクセルに対応する、キューブの前面／背面の交点を計算し、
 * destに括りついた２つの2Dテクスチャにそれぞれ格納する。
 *
 * @param cameraPos		カメラの位置
 */
void VolumeRendering::render(const QVector3D& cameraPos) {
	if (boxVao == 0) return;

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
    glBindVertexArray(boxVao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0); // 頂点をindexで指定するので、glDrawElementsを使う
	                                                        // また、index数は36個あるので、引数は36。

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}
