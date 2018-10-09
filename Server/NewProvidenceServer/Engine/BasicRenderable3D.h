#pragma once

#include <SDL_opengl.h>
#include <GL/glew.h>
#include "Program.h"
#include "Vector3.h"
#include "TimeSlice.h"
#include "Color.h"
#include "GLMCamera.h"

struct BasicRenderable3D
{
private:
	GLuint m_GeometryVAO;
	GLuint m_GeometryVBO;
	GLuint m_LinesVAO;
	GLuint m_LinesVBO;

	GLuint m_TextureID = 0;
	tdogl::Program* m_ShaderProgram = nullptr;
	float m_RotationSpeed = 0.0f;
	float m_RotationStartTime = 0.0f;
	bool m_ShowLines = false;
	Color m_GeometryColor;
	Color m_LinesColor;
	unsigned int m_RenderType = GL_TRIANGLES;

	unsigned int m_GeometryVertexCount = 0;
	unsigned int m_LinesVertexCount = 0;

public:
	BasicRenderable3D() :
		m_GeometryColor(1.0f, 1.0f, 1.0f, 1.0f),
		m_LinesColor(1.0f, 1.0f, 1.0f, 1.0f)
	{}

	inline void SetTextureID(GLuint textureID) {
		m_TextureID = textureID;
	}

	inline GLuint GetTextureID() {
		return m_TextureID;
	}

	inline void SetShaderProgram(tdogl::Program* program) {
		m_ShaderProgram = program;
	}

	inline void SetRotationSpeed(const float rotationSpeed) {
		m_RotationSpeed = rotationSpeed;
		m_RotationStartTime = gameSecondsF;
	}

	inline float GetRotation() {
		return (gameSecondsF - m_RotationStartTime) * m_RotationSpeed;
	}

	inline void SetShowLines(bool showLines) {
		m_ShowLines = showLines;
	}

	inline float GetShowLines() {
		return m_ShowLines;
	}

	inline void SetGeometryColor(const Color& renderColor) {
		m_GeometryColor = renderColor;
	}

	inline const Color& GetGeometryColor() {
		return m_GeometryColor;
	}

	inline void SetLinesColor(const Color& renderColor) {
		m_LinesColor = renderColor;
	}

	inline const Color& GetLinesColor() {
		return m_LinesColor;
	}

	void SetupGeometryVAO(float* vertices, unsigned int floatsPerVertex, unsigned int verticesPerShape, unsigned int shapesPerObject, unsigned int floatsForPosition, unsigned int floatsForTextureMap, unsigned int renderType)
	{
		m_GeometryVertexCount = verticesPerShape * shapesPerObject;
		m_RenderType = renderType;

		//  Generate and Bind the geometry vertex array
		glGenVertexArrays(1, &m_GeometryVAO);
		glBindVertexArray(m_GeometryVAO);

		//  Generate the OpenGL vertex buffer for geometry, and bind it
		glGenBuffers(1, &m_GeometryVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_GeometryVBO);

		//  Set the vertex buffer data information and the vertex attribute pointer within
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * floatsPerVertex * verticesPerShape * shapesPerObject, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(GLuint(0), floatsForPosition, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), NULL);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(GLuint(1), floatsForTextureMap, GL_FLOAT, GL_TRUE, floatsPerVertex * sizeof(float), (const GLvoid*)(floatsForPosition * sizeof(float)));

		glBindVertexArray(0);
	}

	void SetupLinesVAO(float* vertices, unsigned int floatsPerVertex, unsigned int lineCount, unsigned int floatsForPosition)
	{
		m_LinesVertexCount = 2 * lineCount;

		//  Generate and Bind the geometry vertex array
		glGenVertexArrays(1, &m_LinesVAO);
		glBindVertexArray(m_LinesVAO);

		//  Generate the OpenGL vertex buffer for geometry, and bind it
		glGenBuffers(1, &m_LinesVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_LinesVBO);

		//  Set the vertex buffer data information and the vertex attribute pointer within
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * floatsPerVertex * m_LinesVertexCount, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(GLuint(0), floatsForPosition, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), NULL);

		glBindVertexArray(0);
	}

	void RenderGeometry(Vector3<float>& position, Camera& camera, bool billboard = false)
	{
		if (m_ShowLines == true) return;
		if (m_GeometryVertexCount == 0) return;

		//  Set the base color (in case we aren't overriding with a shader program)
		glColor4f(m_GeometryColor.R, m_GeometryColor.G, m_GeometryColor.B, m_GeometryColor.A);

		//  Move to the given position and rotate the object based on it's rotation speed and rotation start time
		glTranslatef(position.x, position.y, position.z);
		//glRotatef(GetRotation(), 0.0f, 1.0f, 0.0f);

		//  Set up and activate any currently set shader program
		if (m_ShaderProgram != nullptr)
		{
			m_ShaderProgram->use();
			m_ShaderProgram->setUniform("camera", camera.matrix());
			if (billboard)
			{
				assert(glm::vec3(position.x, position.y, position.z) != camera.position());
				glm::vec3 direction = glm::normalize(glm::vec3(position.x, position.y, position.z) - camera.position());
				float deltaX = position.x - camera.position().x;
				float deltaY = position.y - camera.position().y;
				float deltaZ = position.z - camera.position().z;
				float radXZ = glm::atan(-deltaX, -deltaZ);
				float radYZ = glm::atan(deltaY, glm::abs(deltaZ));
				float radXY = glm::atan(deltaX, deltaY);

				//glm::mat4 matrix = glm::translate(glm::mat4(), glm::vec3(position.x, position.y, position.z));
				//matrix = glm::rotate(matrix, rad, glm::vec3(1, 0, 0));
				glm::mat4 matrix = glm::translate(glm::mat4(), glm::vec3(position.x, position.y, position.z));
				matrix = glm::rotate(matrix, radXZ, glm::vec3(0, 1, 0));
				matrix = glm::rotate(matrix, radYZ, glm::vec3(1, 0, 0));
				//matrix = glm::rotate(matrix, radXY, glm::vec3(0, 0, -1));
				//matrix = glm::rotate(matrix, radYZ, glm::vec3(1, 0, 0));
				//glm::mat4 matrix = glm::lookAt(glm::vec3(position.x, position.y, position.z), camera.position(), glm::vec3(0.0f, 1.0f, 0.0f));
				

				m_ShaderProgram->setUniform("model", matrix);
			}
			else m_ShaderProgram->setUniform("model", glm::rotate(glm::translate(glm::mat4(), glm::vec3(position.x, position.y, position.z)), glm::radians(GetRotation()), glm::vec3(0, 1, 0))); 
			m_ShaderProgram->setUniform("tex", 0);
			m_ShaderProgram->setUniform("overrideColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			m_ShaderProgram->setUniform("colorOverride", false);
		}

		//  Draw the geometry (set into the video card using VAO and VBO
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(m_GeometryVAO);
		glDrawArrays(m_RenderType, 0, m_GeometryVertexCount);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (m_ShaderProgram != nullptr) m_ShaderProgram->stopUsing();
	}

	void RenderLines(Vector3<float>& position, Camera& camera)
	{
		if (m_ShowLines == false) return;
		if (m_LinesVertexCount == 0) return;

		//  Set the base color (in case we aren't overriding with a shader program) and width
		glColor4f(m_LinesColor.R, m_LinesColor.G, m_LinesColor.B, m_LinesColor.A);
		glLineWidth(2);

		//  Move to the given position and rotate the object based on it's rotation speed and rotation start time
		glTranslatef(position.x, position.y, position.z);
		glRotatef(GetRotation(), 0.0f, 1.0f, 0.0f);

		//  Set up and activate any currently set shader program
		if (m_ShaderProgram != nullptr)
		{
			m_ShaderProgram->use();
			m_ShaderProgram->setUniform("camera", camera.matrix());
			m_ShaderProgram->setUniform("model", glm::rotate(glm::translate(glm::mat4(), glm::vec3(position.x, position.y, position.z)), glm::radians(GetRotation()), glm::vec3(0, 1, 0)));
			m_ShaderProgram->setUniform("tex", 0);
			m_ShaderProgram->setUniform("overrideColor", glm::vec4(m_LinesColor.R, m_LinesColor.G, m_LinesColor.B, m_LinesColor.A));
			m_ShaderProgram->setUniform("colorOverride", true);
		}

		//  Draw the lines (set into the video card using VAO and VBO
		glDisable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(m_LinesVAO);
		glDrawArrays(GL_LINES, 0, m_LinesVertexCount);
		glBindVertexArray(0);

		if (m_ShaderProgram != nullptr) m_ShaderProgram->stopUsing();
	}
};