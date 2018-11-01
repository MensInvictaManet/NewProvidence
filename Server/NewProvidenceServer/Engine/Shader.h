/*
 tdogl::Shader
 
 Copyright 2012 Thomas Dalling - http://tomdalling.com/
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#pragma once

#include <GL/glew.h>
#include <string>

namespace tdogl {

    /**
     Represents a compiled OpenGL shader.
     */
    class Shader { 
    public:
        
        /**
         Creates a shader from a text file.
         
         @param filePath    The path to the text file containing the shader source.
         @param shaderType  Same as the argument to glCreateShader. For example GL_VERTEX_SHADER
                            or GL_FRAGMENT_SHADER.
         
         @throws std::exception if an error occurs.
         */
        static Shader shaderFromFile(const std::string& filePath, GLenum shaderType);
        
        
        /**
         Creates a shader from a string of shader source code.
         
         @param shaderCode  The source code for the shader.
         @param shaderType  Same as the argument to glCreateShader. For example GL_VERTEX_SHADER
                            or GL_FRAGMENT_SHADER.
         
         @throws std::exception if an error occurs.
         */
        Shader(const std::string& shaderCode, GLenum shaderType);
        
        
        /**
         @result The shader's object ID, as returned from glCreateShader
         */
        GLuint object() const;
        
        // tdogl::Shader objects can be copied and assigned because they are reference counted
        // like a shared pointer
        Shader(const Shader& other);
        Shader& operator =(const Shader& other);
        ~Shader();
        
    private:
        GLuint _object			= 0;
        unsigned* _refCount		= nullptr;
        
        void _retain();
        void _release();
    };
    
}


#include <stdexcept>
#include <fstream>
#include <string>
#include <cassert>
#include <sstream>

using namespace tdogl;

Shader::Shader(const std::string& shaderCode, GLenum shaderType) :
	_object(0),
	_refCount(NULL)
{
	//create the shader object
	_object = glCreateShader(shaderType);
	if (_object == 0)
		throw std::runtime_error("glCreateShader failed");

	//set the source code
	const char* code = shaderCode.c_str();
	glShaderSource(_object, 1, (const GLchar**)&code, NULL);

	//compile
	glCompileShader(_object);

	//throw exception if compile error occurred
	GLint status;
	glGetShaderiv(_object, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		std::string msg("Compile failure in shader:\n");

		GLint infoLogLength;
		glGetShaderiv(_object, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* strInfoLog = new char[infoLogLength + 1];
		glGetShaderInfoLog(_object, infoLogLength, NULL, strInfoLog);
		msg += strInfoLog;
		delete[] strInfoLog;

		glDeleteShader(_object); _object = 0;
		throw std::runtime_error(msg);
	}

	_refCount = new unsigned;
	*_refCount = 1;
}

Shader::Shader(const Shader& other) :
	_object(other._object),
	_refCount(other._refCount)
{
	_retain();
}

Shader::~Shader() {
	//_refCount will be NULL if constructor failed and threw an exception
	if (_refCount) _release();
}

GLuint Shader::object() const {
	return _object;
}

Shader& Shader::operator = (const Shader& other) {
	_release();
	_object = other._object;
	_refCount = other._refCount;
	_retain();
	return *this;
}

Shader Shader::shaderFromFile(const std::string& filePath, GLenum shaderType) {
	//open file
	std::ifstream f;
	f.open(filePath.c_str(), std::ios::in | std::ios::binary);
	if (!f.is_open()) {
		throw std::runtime_error(std::string("Failed to open file: ") + filePath);
	}

	//read whole file into stringstream buffer
	std::stringstream buffer;
	buffer << f.rdbuf();

	//return new shader
	Shader shader(buffer.str(), shaderType);
	return shader;
}

void Shader::_retain() {
	assert(_refCount);
	*_refCount += 1;
}

void Shader::_release() {
	assert(_refCount && *_refCount > 0);
	*_refCount -= 1;
	if (*_refCount == 0) {
		glDeleteShader(_object); _object = 0;
		delete _refCount; _refCount = NULL;
	}
}