#include <iostream>
#include "GLWidget3D.h"
#include "MainWindow.h"
#include <GL/GLU.h>
#include <QRgb>
#include "Util.h"

#define SQR(x)	((x) * (x))

GLWidget3D::GLWidget3D() {
	camera.setRotatonSensitivity(0.003);
	camera.setZoomSensitivity(0.01);
}

/**
 * This event handler is called when the mouse press events occur.
 */
void GLWidget3D::mousePressEvent(QMouseEvent *e) {
	camera.mouseDown(e->x(), e->y());
}

/**
 * This event handler is called when the mouse release events occur.
 */
void GLWidget3D::mouseReleaseEvent(QMouseEvent *e) {
	camera.mouseUp();
	updateGL();
}

/**
 * This event handler is called when the mouse move events occur.
 */
void GLWidget3D::mouseMoveEvent(QMouseEvent *e) {
	if (e->buttons() & Qt::LeftButton) {
		camera.rotate(e->x(), e->y());
	} else if (e->buttons() & Qt::RightButton) {
		camera.zoom(e->x(), e->y());
	}

	updateGL();
}

/**
 * This function is called once before the first call to paintGL() or resizeGL().
 */
void GLWidget3D::initializeGL() {
	GLenum err = glewInit();
	if (GLEW_OK != err){// Problem: glewInit failed, something is seriously wrong.
		std::cout << "Error: " << glewGetErrorString(err);
	}

#if 1
	float* data;
	int width, height, depth;
	//Util::loadVTK("vfhead-smooth-small.vtk", width, height, depth, &data);
	//Util::loadVTK("delta-vorticitymag.vtk", width, height, depth, &data);
	Util::loadVTK("bonsai.vtk", width, height, depth, &data);
#endif

#if 0
	// 球の形の3Dデータを作成する
	int width = 128;
	int height = 128;
	int depth = 128;
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
#endif

	// Volume Renderingを初期化
	vr = new VolumeRendering(this->width(), this->height());
	vr->setVolumeData(width, height, depth, data);

	delete [] data;
}

/**
 * This function is called whenever the widget has been resized.
 */
void GLWidget3D::resizeGL(int width, int height) {
	height = height?height:1;

    glViewport(0, 0, width, height);
    
	// Use orthographic projection
    glMatrixMode(GL_PROJECTION);    
    glLoadIdentity();               
    gluOrtho2D(-1, 1, -1, 1);       
    glMatrixMode(GL_MODELVIEW);     
    glLoadIdentity(); 

	vr->setWindowSize(width, height);
}

/**
 * This function is called whenever the widget needs to be painted.
 */
void GLWidget3D::paintGL() {
	//glMatrixMode(GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -camera.z);
	glMultMatrixd(camera.rt);
	glGetFloatv(GL_MODELVIEW_MATRIX, vr->modelviewMatrix);

	vr->update();	
}

QVector2D GLWidget3D::mouseTo2D(int x,int y) {
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];

	// retrieve the matrices
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	float z;
	glReadPixels(x, (float)viewport[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
	
	// unproject the image plane coordinate to the model space
	GLdouble posX, posY, posZ;
	gluUnProject(x, (float)viewport[3] - y, z, modelview, projection, viewport, &posX, &posY, &posZ);

	return QVector2D(posX, posY);
}

void GLWidget3D::timerEvent(QTimerEvent* e) {
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	//updateGL();
}