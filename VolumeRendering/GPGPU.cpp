#include "GPGPU.h"
#include <iostream>

#define M_PI           3.14159265358979323846

//volume info
int gridWidth = 128;
int gridHeight = 128;
int gridDepth = 128;

//camera info
float FOV = 60.0;
float nearZ = 0.1;
float farZ = 200.0;

void GPGPU::init(int width, int height) {
	_winWidth = width;
	_winHeight = height;
    _cube = CreateCubeVao();
    _quad = CreateQuadVao();
	
	loadShaderProgram();

	dataman.createData(gridWidth, gridHeight, gridDepth);
	_cubeinterFBO = dataman.cubeIntersectFBO(_winWidth, _winHeight); //raycasting intersection test texture

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);
}

void GPGPU::setWindowSize(int width, int height) {
	_winWidth = width;
	_winHeight = height;
}

void GPGPU::update() {
    // 画面サイズなどから、projectionMatrixを計算する
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    float aspect = (float)_winWidth/(float)_winHeight;
    gluPerspective(FOV, aspect, nearZ, farZ);
    glGetFloatv (GL_PROJECTION_MATRIX, _projectionMatrix);
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
    rayCubeIntersection(_cubeinterFBO);

	glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Perform the raycast to get fianl image
    render();
}

void GPGPU::loadShaderProgram() {
    ShaderPrograms.raycubeintersection = LoadProgram("rayboxintersectvs", "rayboxintersectfs");
    ShaderPrograms.raycast = LoadProgram("raycastvs", "raycastfs");
}

/**
 * 画面のピクセルに対応する、キューブの前面／背面の交点を計算し、
 * destに括りついた２つの2Dテクスチャにそれぞれ格納する。
 *
 * @param dest		キューブの前面／背面の交点の座標を計算するためのfboと２つの2Dテクスチャ
 */
void GPGPU::rayCubeIntersection(CubeIntersectFBO dest) {
	// キューブの前面／背面の交点を計算するGPUシェーダを選択
	glUseProgram(ShaderPrograms.raycubeintersection);
    
	// GPUシェーダに、パラメータを渡す
	// シミュレーションをしているキューブが、ワールド座標系の原点を中心として、
	// (-1,-1,-1) - (1,1,1)のサイズである。
	// これに対して、カメラが移動しているので、このキューブを回転、移動しなければいけない。
	// なので、modelviewMatrixとprojectionMatrixをカメラの位置などに基づいて計算し、
	// シェーダに渡している。
	setShaderUniform(getUniformLoc("modelviewMatrix"), (float*)&_modelviewMatrix);
    setShaderUniform(getUniformLoc("projectionMatrix"), (float*)&_projectionMatrix);
    
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
    glBindVertexArray(_cube);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0); // 頂点をindexで指定するので、glDrawElementsを使う
	                                                        // また、index数は36個あるので、引数は36。

	resetState();
}

void GPGPU::render() {
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
	glUseProgram(ShaderPrograms.raycast);

	setShaderUniform(getUniformLoc("raystart"), 0);
    setShaderUniform(getUniformLoc("raystop"), 1);
	setShaderUniform(getUniformLoc("density"), 2);
	setShaderUniform(getUniformLoc("width"), _winWidth); 
	setShaderUniform(getUniformLoc("height"), _winHeight); 

	// フレームバッファとして０をバインドすることで、
	// これ以降の描画は、実際のスクリーンに対して行われる。
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// rayとの前面の交点を格納したテクスチャを、テクスチャ０として使用する
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, _cubeinterFBO.texture[0]);

	// rayとの背面の交点を格納したテクスチャを、テクスチャ１として使用する	
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, _cubeinterFBO.texture[1]);

	// 密度データを格納した3Dテクスチャを、テクスチャ２として使用する
	//glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, dataman.data.Density.cur.texture);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, dataman.texture);

	// クリアする色をセットする
    glClearColor(0, 0, 0, 0);

	// フレームバッファをクリアする
	// （スクリーンが出力先バッファとして指定されているので、スクリーンがクリアされる）
    glClear(GL_COLOR_BUFFER_BIT);

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
    glBindVertexArray(_quad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // 頂点座標の配列をそのまま使うので、glDrawArraysを使う
	                                       // 頂点が4つあるので、引数は4。
	
	resetState();
}

void GPGPU::resetState() {
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

