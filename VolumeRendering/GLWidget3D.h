#pragma once

#include <QGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include "Camera.h"
#include <QVector3D>
#include <vector>

using namespace std;

class MainWindow;

class GLWidget3D : public QGLWidget {
private:
	Camera camera;
	QPoint lastPos;


public:
	GLWidget3D();
	void drawScene();
	QVector2D mouseTo2D(int x,int y);

protected:
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();    
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
};

