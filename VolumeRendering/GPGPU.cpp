#include "GPGPU.h"
#include <iostream>

# define M_PI           3.14159265358979323846

//volume info
int gridWidth = 32;//128;
int gridHeight = 32;//128;
int gridDepth = 32;//128;

//camera info
float FOV = 60.0;
float nearZ = 0.1;
float farZ = 200.0;

//default parameter setting, can be changed through GUI
float view_rotate[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
float trans_z[] = { 2 };
float TemperatureDissipation = 0.99f;
float VelocityDissipation = 0.99f;
float DensityDissipation = 0.99f;
float dt = 0.25f;
int jacobiInter = 30;
float buoyancy = 1.0f;
float weight = 0.05f;
float forceRadius = 4.8f; 
float forceTemperature = 10.0f;
float forceDensity = 1.0f;
float forcePos[3] ={ gridWidth / 2.0f, forceRadius / 2.0f, gridDepth / 2.0f}; //bottom center

//default parameters
const float ambientTemperature = 0.0f;
const float dx = 1.0f; //cell size
const float gradientDivergedx = 1.125f / dx;
const float pressureGradientrBeta = 0.1666f; 
const float viscosity = 0; //for water like fluids, smoke and fire are partical like fluid, so viscosity is 0
const float centerFactor = dx * dx / (viscosity * dt);
const float stencilFactor = 1.0f / (6.0f + centerFactor);

GPGPU::GPGPU(){}

void GPGPU::init(int width, int height){
	_winWidth = width;
	_winHeight = height;
	_material = -1;//no material at beginning
    _cube = CreateCubeVao();
    _quad = CreateQuadVao();
	
	loadShaderProgram();

	dataman.createData(gridWidth, gridHeight, gridDepth);
	dataman.setDataVolume(dataman.data.Temperature.cur, ambientTemperature);
	_cubeinterFBO = dataman.cubeIntersectFBO(_winWidth, _winHeight); //raycasting intersection test texture

	glDisable(GL_DEPTH_TEST);
    glEnableVertexAttribArray(0);
}

void GPGPU::setMaterial(int material){
	if(material>-1 || material<2){
		_material = material;
	}
}

void GPGPU::setWindowSize(int width, int height){
	_winWidth = width;
	_winHeight = height;
}

void GPGPU::restart(){
	dataman.clearAllData();
	dataman.setDataVolume(dataman.data.Temperature.cur, ambientTemperature);
}

void GPGPU::update(){
    updateData();
    render();
}

void GPGPU::render(){
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
    renderScene();
}

void GPGPU::updateData(){
    // Backup the viewport dimensions
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glViewport(0, 0, gridWidth, gridHeight);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindVertexArray(_quad); //draw quad

    // 1. update all divergence free components first 3 of Navier-Stokes Equation
    //update advecation(velocity)
    updateAdvect(dataman.data.Velocity.cur, dataman.data.Velocity.cur, dataman.data.Velocity.pre, VelocityDissipation);
    swapTexture(&dataman.data.Velocity);
    
    //temperature and density are also flow along the velocity
    updateAdvect(dataman.data.Velocity.cur, dataman.data.Temperature.cur, dataman.data.Temperature.pre, TemperatureDissipation);
    swapTexture(&dataman.data.Temperature);
    updateAdvect(dataman.data.Velocity.cur, dataman.data.Density.cur, dataman.data.Density.pre, DensityDissipation);
    swapTexture(&dataman.data.Density);
    updateBuoyancy(dataman.data.Velocity.cur, dataman.data.Temperature.cur, dataman.data.Density.cur, dataman.data.Velocity.pre);
    swapTexture(&dataman.data.Velocity);

	//impose force
    updateImpulse(dataman.data.Temperature.cur, forcePos, forceTemperature);
    updateImpulse(dataman.data.Density.cur, forcePos, forceDensity);

    //diffuse (if viscosity is > 0)
	if (viscosity > 0){
		for (int i = 0; i < jacobiInter; ++i){
			jacobi(dataman.data.Velocity.cur, dataman.data.Velocity.cur, dataman.data.Velocity.pre, centerFactor, stencilFactor);
			swapTexture(&dataman.data.Velocity);
		}
	}

    // 2. divergence calculate Pressure
    computeDivergence(dataman.data.Velocity.cur, dataman.data.Divergence);
    dataman.setDataVolume(dataman.data.Pressure.cur, 0); //clear pressure data
    for (int i = 0; i < jacobiInter; ++i) {
        jacobi(dataman.data.Pressure.cur, dataman.data.Divergence, dataman.data.Pressure.pre, -dx * dx, pressureGradientrBeta);
        swapTexture(&dataman.data.Pressure);
		//update boundary
		updateBoundary(dataman.data.Pressure.cur, dataman.data.Pressure.pre, 1);
		swapTexture(&dataman.data.Pressure);
    }

    // 3. Subtract gradient to get final velocity
	//no-slip velocity
	updateBoundary(dataman.data.Velocity.cur, dataman.data.Velocity.pre, -1);
	swapTexture(&dataman.data.Velocity);

    subtractPressureGradient(dataman.data.Velocity.cur, dataman.data.Pressure.cur, dataman.data.Velocity.pre);
    swapTexture(&dataman.data.Velocity);

	//no-slip velocity
	updateBoundary(dataman.data.Velocity.cur, dataman.data.Velocity.pre, -1);
	swapTexture(&dataman.data.Velocity);

    // Restore the stored viewport dimensions
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void GPGPU::loadShaderProgram(){
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

/**
 * 画面のピクセルに対応する、キューブの前面／背面の交点を計算し、
 * destに括りついた２つの2Dテクスチャにそれぞれ格納する。
 *
 * @param dest		キューブの前面／背面の交点の座標を計算するためのfboと２つの2Dテクスチャ
 */
void GPGPU::rayCubeIntersection(CubeIntersectFBO dest){
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

void GPGPU::renderScene(){
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
	setShaderUniform(getUniformLoc("temperature"), 3);
	setShaderUniform(getUniformLoc("material"), _material); 
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
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, dataman.data.Test.texture);

	// 温度データを格納した3Dテクスチャを、テクスチャ３として使用する
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, dataman.data.Temperature.cur.texture); 

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

void GPGPU::updateAdvect(DataVolume velocity, DataVolume source, DataVolume dest, float dissipation){
    glUseProgram(ShaderPrograms.advection);

    //set uniform
    setShaderUniform(getUniformLoc("velocity"), 0);
    setShaderUniform(getUniformLoc("source"), 1);
	setShaderUniform(getUniformLoc("inverseSize"), 1.0/float(gridWidth), 1.0/float(gridHeight), 1.0/float(gridDepth));
    setShaderUniform(getUniformLoc("dt"), dt);
	setShaderUniform(getUniformLoc("rdx"), 1.0f/dx);
    setShaderUniform(getUniformLoc("dissipation"), dissipation);
    
    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, source.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::updateBuoyancy(DataVolume velocity, DataVolume temperature, DataVolume density, DataVolume dest){
    glUseProgram(ShaderPrograms.buoyancy);

	setShaderUniform(getUniformLoc("velocity"), 0);
    setShaderUniform(getUniformLoc("temperature"), 1);
    setShaderUniform(getUniformLoc("density"), 2);
    setShaderUniform(getUniformLoc("ambientTemperature"), ambientTemperature);
    setShaderUniform(getUniformLoc("dt"), dt);
    setShaderUniform(getUniformLoc("sigma"), buoyancy);
    setShaderUniform(getUniformLoc("kappa"), weight);
	setShaderUniform(getUniformLoc("material"), _material);
	setShaderUniform(getUniformLoc("dir"), 0.0, 1.0, 0.0);//force direction

    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, temperature.texture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, density.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::updateImpulse(DataVolume dest, float* position, float value)
{
    glUseProgram(ShaderPrograms.addImpulse);

    setShaderUniform(getUniformLoc("pos"), position[0], position[1], position[2]);
    setShaderUniform(getUniformLoc("radius"), forceRadius);
    setShaderUniform(getUniformLoc("force"), value, value, value);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glEnable(GL_BLEND);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::computeDivergence(DataVolume velocity, DataVolume dest)
{
    glUseProgram(ShaderPrograms.divergence);

    setShaderUniform(getUniformLoc("halfrdx"), 0.5f / dx);
    setShaderUniform(getUniformLoc("velocity"), 0);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::jacobi(DataVolume pressure, DataVolume divergence, DataVolume dest, float alpha, float rBeta)
{
    glUseProgram(ShaderPrograms.jacobi);

    setShaderUniform(getUniformLoc("alpha"), alpha);
    setShaderUniform(getUniformLoc("rBeta"), rBeta);
    setShaderUniform(getUniformLoc("x"), 0);
    setShaderUniform(getUniformLoc("b"), 1);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, pressure.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, divergence.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::subtractPressureGradient(DataVolume velocity, DataVolume pressure, DataVolume dest)
{
    glUseProgram(ShaderPrograms.subtractGradient);

	setShaderUniform(getUniformLoc("halfrdx"), gradientDivergedx);
    setShaderUniform(getUniformLoc("velocity"), 0);
    setShaderUniform(getUniformLoc("pressure"), 1);

    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity.texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, pressure.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
}

void GPGPU::updateBoundary(DataVolume source, DataVolume dest, float scale){
    glUseProgram(ShaderPrograms.boundary);

    //set uniform
    setShaderUniform(getUniformLoc("texture"), 0);
	setShaderUniform(getUniformLoc("scale"), scale);
	setShaderUniform(getUniformLoc("width"), gridWidth);
    setShaderUniform(getUniformLoc("height"), gridHeight);
	setShaderUniform(getUniformLoc("depth"), gridDepth);
    
    glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, source.texture);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, gridDepth);
    resetState();
} 

void GPGPU::resetState()
{
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, 0);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_3D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}

void GPGPU::swapTexture(DuoDataVolume* data)
{
    DataVolume temp = data->cur;
    data->cur = data->pre;
    data->pre = temp;
}
