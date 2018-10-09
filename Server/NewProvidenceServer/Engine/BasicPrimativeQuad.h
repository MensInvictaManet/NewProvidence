#pragma once

#include "BasicRenderable3D.h"


struct BasicPrimativeQuad : public BasicRenderable3D
{
private:
	float m_HalfWidth;
	float m_HalfHeight;
	bool m_Billboard;

public:
	BasicPrimativeQuad(float width = 10.0f, float height = 10.0f, bool billboard = false, bool showLines = true)
	{
		SetWidth(width);
		SetHeight(height);
		SetBillboard(billboard);

		BasicRenderable3D::SetRotationSpeed(0.0f);
		BasicRenderable3D::SetShowLines(showLines);

		SetupVAO();
	}

	inline void SetValues(float width = 10.0f, float height = 10.0f, bool billboard = false, bool showLines = true, float rotationSpeed = 0.0f, GLuint textureID = 0, tdogl::Program* shaderProgram = nullptr) {
		SetWidth(width);
		SetHeight(height);
		SetBillboard(billboard);

		BasicRenderable3D::SetTextureID(textureID);
		BasicRenderable3D::SetRotationSpeed(rotationSpeed);
		BasicRenderable3D::SetShaderProgram(shaderProgram);
		BasicRenderable3D::SetShowLines(showLines);

		SetupVAO();
	}

	inline void SetWidth(float width) {
		m_HalfWidth = width / 2.0f;
	}

	inline void SetHeight(float height) {
		m_HalfHeight = height / 2.0f;
	}

	inline void SetBillboard(bool billboard) {
		m_Billboard = billboard;
	}

	void SetupVAO(const Camera* camera = NULL)
	{
		float vertices[5 * 4] =
		{
			-m_HalfWidth,	-m_HalfHeight,	0.0f,	0.000f, 1.000f,
			m_HalfWidth,	-m_HalfHeight,	0.0f,	1.000f, 1.000f,
			m_HalfWidth,	m_HalfHeight,	0.0f,	1.000f, 0.000f,
			-m_HalfWidth,	m_HalfHeight,	0.0f,	0.000f, 0.000f,
		};

		//  Set up the Geometry VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupGeometryVAO(vertices, 5, 4, 1, 3, 2, GL_QUADS);

		float lineVertices[3 * 8] =
		{
			-m_HalfWidth,	-m_HalfHeight,	0.0f,
			m_HalfWidth,	-m_HalfHeight,	0.0f,
			m_HalfWidth,	-m_HalfHeight,	0.0f,
			m_HalfWidth,	m_HalfHeight,	0.0f,
			m_HalfWidth,	m_HalfHeight,	0.0f,
			-m_HalfWidth,	m_HalfHeight,	0.0f,
			-m_HalfWidth,	m_HalfHeight,	0.0f,
			-m_HalfWidth,	-m_HalfHeight,	0.0f,
		};

		//  Set up the Lines VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupLinesVAO(lineVertices, 3, 4, 3);
	}

	void Render(Vector3<float>& position, Camera& camera)
	{
		glPushMatrix();
			//  Render the object at the position relative to the camera
			BasicRenderable3D::RenderGeometry(position, camera, m_Billboard);

			//  Render the lines at the position relative to the camera
			BasicRenderable3D::RenderLines(position, camera);
		glPopMatrix();
	}
};