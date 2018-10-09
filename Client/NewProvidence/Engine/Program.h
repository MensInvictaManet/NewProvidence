/*
 tdogl::Program
 
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

#include "Shader.h"
#include <vector>
#include <glm/glm.hpp>

namespace tdogl {

    /**
     Represents an OpenGL program made by linking shaders.
     */
    class Program { 
    public:
        /**
         Creates a program by linking a list of tdogl::Shader objects
         
         @param shaders  The shaders to link together to make the program
         
         @throws std::exception if an error occurs.
         
         @see tdogl::Shader
         */
        Program(const std::vector<Shader>& shaders);
        ~Program();
        
        
        /**
         @result The program's object ID, as returned from glCreateProgram
         */
        GLuint object() const;

        void use() const;

        bool isInUse() const;

        void stopUsing() const;
        
        /**
         @result The attribute index for the given name, as returned from glGetAttribLocation.
         */
        GLint attrib(const GLchar* attribName) const;
        
        
        /**
         @result The uniform index for the given name, as returned from glGetUniformLocation.
         */
        GLint uniform(const GLchar* uniformName) const;

        /**
         Setters for attribute and uniform variables.

         These are convenience methods for the glVertexAttrib* and glUniform* functions.
         */
#define _TDOGL_PROGRAM_ATTRIB_N_UNIFORM_SETTERS(OGL_TYPE) \
        void setAttrib(const GLchar* attribName, OGL_TYPE v0); \
        void setAttrib(const GLchar* attribName, OGL_TYPE v0, OGL_TYPE v1); \
        void setAttrib(const GLchar* attribName, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2); \
        void setAttrib(const GLchar* attribName, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3); \
\
        void setAttrib1v(const GLchar* attribName, const OGL_TYPE* v); \
        void setAttrib2v(const GLchar* attribName, const OGL_TYPE* v); \
        void setAttrib3v(const GLchar* attribName, const OGL_TYPE* v); \
        void setAttrib4v(const GLchar* attribName, const OGL_TYPE* v); \
\
        void setUniform(const GLchar* uniformName, OGL_TYPE v0); \
        void setUniform(const GLchar* uniformName, OGL_TYPE v0, OGL_TYPE v1); \
        void setUniform(const GLchar* uniformName, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2); \
        void setUniform(const GLchar* uniformName, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3); \
\
        void setUniform1v(const GLchar* uniformName, const OGL_TYPE* v, GLsizei count=1); \
        void setUniform2v(const GLchar* uniformName, const OGL_TYPE* v, GLsizei count=1); \
        void setUniform3v(const GLchar* uniformName, const OGL_TYPE* v, GLsizei count=1); \
        void setUniform4v(const GLchar* uniformName, const OGL_TYPE* v, GLsizei count=1); \

        _TDOGL_PROGRAM_ATTRIB_N_UNIFORM_SETTERS(GLfloat)
        _TDOGL_PROGRAM_ATTRIB_N_UNIFORM_SETTERS(GLdouble)
        _TDOGL_PROGRAM_ATTRIB_N_UNIFORM_SETTERS(GLint)
        _TDOGL_PROGRAM_ATTRIB_N_UNIFORM_SETTERS(GLuint)

        void setUniformMatrix2(const GLchar* uniformName, const GLfloat* v, GLsizei count=1, GLboolean transpose=GL_FALSE);
        void setUniformMatrix3(const GLchar* uniformName, const GLfloat* v, GLsizei count=1, GLboolean transpose=GL_FALSE);
        void setUniformMatrix4(const GLchar* uniformName, const GLfloat* v, GLsizei count=1, GLboolean transpose=GL_FALSE);
        void setUniform(const GLchar* uniformName, const glm::mat2& m, GLboolean transpose=GL_FALSE);
        void setUniform(const GLchar* uniformName, const glm::mat3& m, GLboolean transpose=GL_FALSE);
        void setUniform(const GLchar* uniformName, const glm::mat4& m, GLboolean transpose=GL_FALSE);
        void setUniform(const GLchar* uniformName, const glm::vec3& v);
        void setUniform(const GLchar* uniformName, const glm::vec4& v);

        
    private:
        GLuint _object;
        
        //copying disabled
        Program(const Program&);
        const Program& operator=(const Program&);
    };

}

#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

using namespace tdogl;

Program::Program(const std::vector<Shader>& shaders) :
	_object(0)
{
	if (shaders.size() <= 0)
		throw std::runtime_error("No shaders were provided to create the program");

	//create the program object
	_object = glCreateProgram();
	if (_object == 0)
		throw std::runtime_error("glCreateProgram failed");

	//attach all the shaders
	for (unsigned i = 0; i < shaders.size(); ++i)
		glAttachShader(_object, shaders[i].object());

	//link the shaders together
	glLinkProgram(_object);

	//detach all the shaders
	for (unsigned i = 0; i < shaders.size(); ++i)
		glDetachShader(_object, shaders[i].object());

	//throw exception if linking failed
	GLint status;
	glGetProgramiv(_object, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		std::string msg("Program linking failure: ");

		GLint infoLogLength;
		glGetProgramiv(_object, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* strInfoLog = new char[infoLogLength + 1];
		glGetProgramInfoLog(_object, infoLogLength, NULL, strInfoLog);
		msg += strInfoLog;
		delete[] strInfoLog;

		glDeleteProgram(_object); _object = 0;
		throw std::runtime_error(msg);
	}
}

Program::~Program() {
	//might be 0 if ctor fails by throwing exception
	if (_object != 0) glDeleteProgram(_object);
}

GLuint Program::object() const {
	return _object;
}

void Program::use() const {
	glUseProgram(_object);
}

bool Program::isInUse() const {
	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	return (currentProgram == (GLint)_object);
}

void Program::stopUsing() const {
	assert(isInUse());
	glUseProgram(0);
}

GLint Program::attrib(const GLchar* attribName) const {
	if (!attribName)
		throw std::runtime_error("attribName was NULL");

	GLint attrib = glGetAttribLocation(_object, attribName);
	if (attrib == -1)
		throw std::runtime_error(std::string("Program attribute not found: ") + attribName);

	return attrib;
}

GLint Program::uniform(const GLchar* uniformName) const {
	if (!uniformName)
		throw std::runtime_error("uniformName was NULL");

	GLint uniform = glGetUniformLocation(_object, uniformName);
	if (uniform == -1)
		throw std::runtime_error(std::string("Program uniform not found: ") + uniformName);

	return uniform;
}

#define ATTRIB_N_UNIFORM_SETTERS(OGL_TYPE, TYPE_PREFIX, TYPE_SUFFIX) \
\
    void Program::setAttrib(const GLchar* name, OGL_TYPE v0) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 1 ## TYPE_SUFFIX (attrib(name), v0); } \
    void Program::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 2 ## TYPE_SUFFIX (attrib(name), v0, v1); } \
    void Program::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 3 ## TYPE_SUFFIX (attrib(name), v0, v1, v2); } \
    void Program::setAttrib(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 4 ## TYPE_SUFFIX (attrib(name), v0, v1, v2, v3); } \
\
    void Program::setAttrib1v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 1 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void Program::setAttrib2v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 2 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void Program::setAttrib3v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 3 ## TYPE_SUFFIX ## v (attrib(name), v); } \
    void Program::setAttrib4v(const GLchar* name, const OGL_TYPE* v) \
        { assert(isInUse()); glVertexAttrib ## TYPE_PREFIX ## 4 ## TYPE_SUFFIX ## v (attrib(name), v); } \
\
    void Program::setUniform(const GLchar* name, OGL_TYPE v0) \
        { assert(isInUse()); glUniform1 ## TYPE_SUFFIX (uniform(name), v0); } \
    void Program::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1) \
        { assert(isInUse()); glUniform2 ## TYPE_SUFFIX (uniform(name), v0, v1); } \
    void Program::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2) \
        { assert(isInUse()); glUniform3 ## TYPE_SUFFIX (uniform(name), v0, v1, v2); } \
    void Program::setUniform(const GLchar* name, OGL_TYPE v0, OGL_TYPE v1, OGL_TYPE v2, OGL_TYPE v3) \
        { assert(isInUse()); glUniform4 ## TYPE_SUFFIX (uniform(name), v0, v1, v2, v3); } \
\
    void Program::setUniform1v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform1 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void Program::setUniform2v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform2 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void Program::setUniform3v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform3 ## TYPE_SUFFIX ## v (uniform(name), count, v); } \
    void Program::setUniform4v(const GLchar* name, const OGL_TYPE* v, GLsizei count) \
        { assert(isInUse()); glUniform4 ## TYPE_SUFFIX ## v (uniform(name), count, v); }

ATTRIB_N_UNIFORM_SETTERS(GLfloat, , f);
ATTRIB_N_UNIFORM_SETTERS(GLdouble, , d);
ATTRIB_N_UNIFORM_SETTERS(GLint, I, i);
ATTRIB_N_UNIFORM_SETTERS(GLuint, I, ui);

void Program::setUniformMatrix2(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix2fv(uniform(name), count, transpose, v);
}

void Program::setUniformMatrix3(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix3fv(uniform(name), count, transpose, v);
}

void Program::setUniformMatrix4(const GLchar* name, const GLfloat* v, GLsizei count, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix4fv(uniform(name), count, transpose, v);
}

void Program::setUniform(const GLchar* name, const glm::mat2& m, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix2fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void Program::setUniform(const GLchar* name, const glm::mat3& m, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix3fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void Program::setUniform(const GLchar* name, const glm::mat4& m, GLboolean transpose) {
	assert(isInUse());
	glUniformMatrix4fv(uniform(name), 1, transpose, glm::value_ptr(m));
}

void Program::setUniform(const GLchar* uniformName, const glm::vec3& v) {
	setUniform3v(uniformName, glm::value_ptr(v));
}

void Program::setUniform(const GLchar* uniformName, const glm::vec4& v) {
	setUniform4v(uniformName, glm::value_ptr(v));
}