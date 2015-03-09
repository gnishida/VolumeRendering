#pragma once

#include <GL/glew.h>
#include <string>

class Util {
protected:
	Util() {}

public:
	static int LoadShader(char* filename, std::string& text);
	static GLuint LoadProgram(const char* vsKey, const char* fsKey);

	static GLuint CreateCubeVao();
	static GLuint CreateQuadVao();

	static void loadVTK(char* filename, int& width, int& height, int& depth, float** data);
};
