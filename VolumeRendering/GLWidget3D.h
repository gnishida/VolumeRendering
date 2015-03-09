#pragma once

#include <GL/glew.h>
#include <QGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include "Camera.h"
#include <QVector2D>
#include <vector>
#include <QBasicTimer>
#include "VolumeRendering.h"

using namespace std;

class MainWindow;

class GLWidget3D : public QGLWidget {
private:
	Camera camera;
	QPoint lastPos;
	VolumeRendering* vr;
	QBasicTimer timer;

public:
	GLWidget3D();
	QVector2D mouseTo2D(int x,int y);
	void loadVTK(char* filename);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();    
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
};

