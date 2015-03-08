#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags) {
	ui.setupUi(this);

	connect(ui.actionStart, SIGNAL(triggered()), this, SLOT(onStart()));
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

	glWidget = new GLWidget3D();
	setCentralWidget(glWidget);
}

MainWindow::~MainWindow() {
}

void MainWindow::onStart() {
}
