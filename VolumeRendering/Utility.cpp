#include "Utility.h"
#include <fstream>
#include <iostream>

int Util::LoadShader(char* filename, std::string& text) {
    std::ifstream ifs;
    ifs.open(filename, std::ios::in);

    std::string line;
    while (ifs.good()) {
        getline(ifs, line);

        text += line + "\n";
    }

    return 0;
}

GLuint Util::LoadProgram(const char* vsfile, const char* fsfile) {
    //load vertex shader
    std::string vsPath = "shader/vs/"+std::string(vsfile)+".glsl";
	std::string vsSourceStr;
    LoadShader((char*)vsPath.c_str(), vsSourceStr);

    //load fragment shader
    std::string fsPath = "shader/fs/"+std::string(fsfile)+".glsl";
    std::string fsSourceStr;
    LoadShader((char*)fsPath.c_str(), fsSourceStr);

    if (vsSourceStr.empty()) std::cout<<"Can't load vertex shader "<<vsfile<<std::endl;
    if (fsfile!="" && fsSourceStr.empty()) std::cout<<"Can't load fragment shader "<<fsfile<<std::endl;

    //convert to const char
    const char* vsSource = vsSourceStr.c_str();
    const char* fsSource = fsSourceStr.c_str();

    GLint compileSuccess;
    GLchar errorLog[256];

    GLuint programHandle = glCreateProgram();

    GLuint vsShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsShader, 1, &vsSource, NULL);
    glCompileShader(vsShader);
    glGetShaderiv(vsShader, GL_COMPILE_STATUS, &compileSuccess);
    if(compileSuccess == GL_FALSE){
        glGetShaderInfoLog(vsShader, sizeof(errorLog), 0, errorLog);
        std::cout<<"Can't compile vertex shader "<<vsfile<<std::endl;
        std::cout<<errorLog<<std::endl;
    }
    glAttachShader(programHandle, vsShader);

    if(fsfile!=""){
        GLuint fsShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fsShader, 1, &fsSource, NULL);
        glCompileShader(fsShader);
        glGetShaderiv(fsShader, GL_COMPILE_STATUS, &compileSuccess);
        if(compileSuccess == GL_FALSE){
            glGetShaderInfoLog(fsShader, sizeof(errorLog), 0, errorLog);
            std::cout<<"Can't compile fragment shader "<<fsfile<<std::endl;
            std::cout<<errorLog<<std::endl;
        }
        glAttachShader(programHandle, fsShader);
    }

    glBindAttribLocation(programHandle, 0, "glVertex");
	glBindAttribLocation(programHandle, 1, "glTexCoord");
    glLinkProgram(programHandle);

    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);

    if (!linkSuccess) {
        glGetProgramInfoLog(programHandle, sizeof(errorLog), 0, errorLog);
        std::cout<<"Link error.\n"<<"vs: "<<vsfile<<"fs: "<<fsfile<<std::endl;
        std::cout<<errorLog<<std::endl;
    }

    return programHandle;

}

//create cubic information
GLuint Util::CreateCubeVao() {
    float positions[] = { 
		-1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f,  1.0f, 
		-1.0f,  1.0f, -1.0f, 
		-1.0f,  1.0f,  1.0f, 
		 1.0f, -1.0f, -1.0f, 
		 1.0f, -1.0f,  1.0f, 
		 1.0f,  1.0f, -1.0f, 
		 1.0f,  1.0f,  1.0f, 
	};
    
    short indices[] = {
        7, 3, 1, 1, 5, 7, 
        0, 2, 6, 6, 4, 0, 
        6, 2, 3, 3, 7, 6, 
        1, 0, 4, 4, 5, 1, 
        3, 2, 0, 0, 1, 3, 
        4, 6, 7, 7, 5, 4, 
    };

    // Create the VAO:
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create the VBO for positions:
    {
        GLuint vbo;
        GLsizeiptr size = sizeof(positions);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);
    }

    // Create the VBO for indices:
    {
        GLuint vbo;
        GLsizeiptr size = sizeof(indices);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }

    // Set up the vertex layout:
    GLsizeiptr stride = 3 * sizeof(positions[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

    return vao;
}

GLuint Util::CreateQuadVao() {
    short positions[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    
    // Create the VAO:
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create the VBO:
    GLuint vbo;
    GLsizeiptr size = sizeof(positions);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

    // Set up the vertex layout:
    GLsizeiptr stride = 2 * sizeof(positions[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, stride, 0);

    return vao;
}
