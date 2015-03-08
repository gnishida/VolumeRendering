#pragma once

#include <GL/glew.h>
#include <GL/glut.h>
#include <string>

//utilities for shader program
int LoadShader(char* filename, std::string& text);
GLuint LoadProgram(const char* vsKey, const char* fsKey);
GLint getUniformLoc(const char* name);	
void setShaderUniform(GLint location, int v);
//void setShaderUniform(GLint location, float v);
//void setShaderUniform(GLint location, float x, float y);
//void setShaderUniform(GLint location, float x, float y, float z);
void setShaderUniform(GLint location, float* v);

GLuint CreateCubeVao();
GLuint CreateQuadVao();
