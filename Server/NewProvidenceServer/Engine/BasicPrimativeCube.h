#pragma once

#include "BasicRenderable3D.h"


struct BasicPrimativeCube : public BasicRenderable3D
{
private:
	float m_HalfWidth;
	float m_HalfHeight;
	float m_HalfDepth;

public:
	BasicPrimativeCube(float width = 10.0f, float height = 10.0f, float depth = 10.0f, bool showLines = true)
	{
		SetWidth(width);
		SetHeight(height);
		SetDepth(depth);

		BasicRenderable3D::SetRotationSpeed(0.0f);
		BasicRenderable3D::SetShowLines(showLines);

		SetupVAO();
	}

	inline void SetValues(float width = 10.0f, float height = 10.0f, float depth = 10.0f, bool showLines = true, float rotationSpeed = 0.0f, GLuint textureID = 0, tdogl::Program* shaderProgram = nullptr) {
		SetWidth(width);
		SetHeight(height);
		SetDepth(depth);

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

	inline void SetDepth(float depth) {
		m_HalfDepth = depth / 2.0f;
	}

	void SetupVAO()
	{
		float vertices[5 * 4 * 6] =
		{
			-m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	0.000f, 0.125f,
			m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	1.000f, 0.125f,
			m_HalfWidth, m_HalfHeight, -m_HalfDepth,	1.000f, 0.001f,
			-m_HalfWidth, m_HalfHeight, -m_HalfDepth,	0.000f, 0.001f,

			m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	0.000f, 0.625f,
			m_HalfWidth, -m_HalfHeight, m_HalfDepth,	1.000f, 0.625f,
			m_HalfWidth, m_HalfHeight, m_HalfDepth,		1.000f, 0.500f,
			m_HalfWidth, m_HalfHeight, -m_HalfDepth,	0.000f, 0.500f,

			m_HalfWidth, -m_HalfHeight, m_HalfDepth,	0.000f, 0.250f,
			-m_HalfWidth, -m_HalfHeight, m_HalfDepth,	1.000f, 0.250f,
			-m_HalfWidth, m_HalfHeight, m_HalfDepth,	1.000f, 0.125f,
			m_HalfWidth, m_HalfHeight, m_HalfDepth,		0.000f, 0.125f,

			-m_HalfWidth, -m_HalfHeight, m_HalfDepth,	0.000f, 0.750f,
			-m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	1.000f, 0.750f,
			-m_HalfWidth, m_HalfHeight, -m_HalfDepth,	1.000f, 0.625f,
			-m_HalfWidth, m_HalfHeight, m_HalfDepth,	0.000f, 0.625f,

			-m_HalfWidth, m_HalfHeight, -m_HalfDepth,	0.000f, 0.375f,
			m_HalfWidth, m_HalfHeight, -m_HalfDepth,	1.000f, 0.375f,
			m_HalfWidth, m_HalfHeight, m_HalfDepth,		1.000f, 0.250f,
			-m_HalfWidth, m_HalfHeight, m_HalfDepth,	0.000f, 0.250f,

			-m_HalfWidth, -m_HalfHeight, m_HalfDepth,	0.000f, 0.500f,
			m_HalfWidth, -m_HalfHeight, m_HalfDepth,	1.000f, 0.500f,
			m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	1.000f, 0.375f,
			-m_HalfWidth, -m_HalfHeight, -m_HalfDepth,	0.000f, 0.375f
		};

		//  Set up the Geometry VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupGeometryVAO(vertices, 5, 4, 6, 3, 2, GL_QUADS);

		float lineVertices[3 * 32] =
		{
			-m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	-m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			-m_HalfWidth,	-m_HalfHeight,	m_HalfDepth,
			m_HalfWidth,	-m_HalfHeight,	m_HalfDepth
		};

		//  Set up the Lines VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupLinesVAO(lineVertices, 3, 16, 3);
	}

	void Render(Vector3<float>& position, Camera& camera)
	{
		glPushMatrix();
			//  Render the object at the position relative to the camera
			BasicRenderable3D::RenderGeometry(position, camera);

			//  Render the lines at the position relative to the camera
			BasicRenderable3D::RenderLines(position, camera);
		glPopMatrix();
	}
};