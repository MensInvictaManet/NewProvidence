#pragma once

#include "BasicRenderable3D.h"

#include <tuple>
#include <algorithm>
#include "ShapeSplitPoints.h"

struct SplittableCube : public BasicRenderable3D
{
protected:
	typedef std::tuple<float, float, float, float, float> SC_Point;
	typedef std::tuple<SC_Point, SC_Point, SC_Point, SC_Point> SC_Square;

	static SC_Point MakePoint(float x, float y, float z, float tx = 0.0f, float ty = 0.0f) { return SC_Point(x, y, z, tx, ty); }
	static SC_Point AddPoints(SC_Point& a, SC_Point& b) {
		return SC_Point(std::get<0>(a) + std::get<0>(b), std::get<1>(a) + std::get<1>(b), std::get<2>(a) + std::get<2>(b), std::get<3>(a) + std::get<3>(b), std::get<4>(a) + std::get<4>(b));
	}
	static SC_Point MultiplyPoint(SC_Point& p, float f, bool posOnly = false) {
		if (posOnly) return SC_Point(std::get<0>(p) * f, std::get<1>(p) * f, std::get<2>(p) * f, std::get<3>(p), std::get<4>(p));
		return SC_Point(std::get<0>(p) * f, std::get<1>(p) * f, std::get<2>(p) * f, std::get<3>(p) * f, std::get<4>(p) * f);
	}

	static SC_Point& NormalizePointDistance(SC_Point& p) {
		Vector3<float> distance(std::get<0>(p), std::get<1>(p), std::get<2>(p));
		distance.Normalize();
		std::get<0>(p) = distance.x;
		std::get<1>(p) = distance.y;
		std::get<2>(p) = distance.z;
		return p;
	}

	struct SC_SurfaceSquare
	{
		typedef std::tuple<SC_SurfaceSquare, SC_SurfaceSquare, SC_SurfaceSquare, SC_SurfaceSquare> SC_SplitSquare;

		SC_Square m_Square;
		SC_SplitSquare* m_SplitSquares;

		SC_SurfaceSquare() :
			m_Square(MakePoint(0, 0, 0), MakePoint(0, 0, 0), MakePoint(0, 0, 0), MakePoint(0, 0, 0)),
			m_SplitSquares(nullptr)
		{}

		SC_SurfaceSquare(SC_Square square, SC_SplitSquare* split = nullptr) :
			m_Square(square),
			m_SplitSquares(split)
		{}

		~SC_SurfaceSquare()
		{
			if (m_SplitSquares != nullptr) delete m_SplitSquares;
		}

		void Split(int splitCount, float pointDistance)
		{
			if (splitCount != 0 && m_SplitSquares != nullptr)
			{
				std::get<0>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
				std::get<1>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
				std::get<2>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
				std::get<3>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
				return;
			}

			if (splitCount == 0 && m_SplitSquares != nullptr)
			{
				delete m_SplitSquares;
				m_SplitSquares = nullptr;
				return;
			}

			if (splitCount == 0) return;

			//  If we arrive here, splitCount is greater than 0 and we have no Split Triangles. Create the triangles, and send the split ahead
			auto point1 = MakePoint(std::get<0>(std::get<0>(m_Square)), std::get<1>(std::get<0>(m_Square)), std::get<2>(std::get<0>(m_Square)), std::get<3>(std::get<0>(m_Square)), std::get<4>(std::get<0>(m_Square)));
			auto point2 = MakePoint(std::get<0>(std::get<1>(m_Square)), std::get<1>(std::get<1>(m_Square)), std::get<2>(std::get<1>(m_Square)), std::get<3>(std::get<1>(m_Square)), std::get<4>(std::get<1>(m_Square)));
			auto point3 = MakePoint(std::get<0>(std::get<2>(m_Square)), std::get<1>(std::get<2>(m_Square)), std::get<2>(std::get<2>(m_Square)), std::get<3>(std::get<2>(m_Square)), std::get<4>(std::get<2>(m_Square)));
			auto point4 = MakePoint(std::get<0>(std::get<3>(m_Square)), std::get<1>(std::get<3>(m_Square)), std::get<2>(std::get<3>(m_Square)), std::get<3>(std::get<3>(m_Square)), std::get<4>(std::get<3>(m_Square)));
			auto point12 = AddPoints(point1, point2);
			auto point23 = AddPoints(point2, point3);
			auto point34 = AddPoints(point3, point4);
			auto point41 = AddPoints(point4, point1);
			auto halfPoint12 = MultiplyPoint(point12, 0.5f);
			auto halfPoint23 = MultiplyPoint(point23, 0.5f);
			auto halfPoint34 = MultiplyPoint(point34, 0.5f);
			auto halfPoint41 = MultiplyPoint(point41, 0.5f);
			auto point1234 = AddPoints(point12, point34);
			auto centerPoint = MultiplyPoint(point1234, (1.0f / 4.0f));

			auto midpoint12 = MultiplyPoint(NormalizePointDistance(halfPoint12), pointDistance, true);
			auto midpoint23 = MultiplyPoint(NormalizePointDistance(halfPoint23), pointDistance, true);
			auto midpoint34 = MultiplyPoint(NormalizePointDistance(halfPoint34), pointDistance, true);
			auto midpoint41 = MultiplyPoint(NormalizePointDistance(halfPoint41), pointDistance, true);
			auto midpoint = MultiplyPoint(NormalizePointDistance(centerPoint), pointDistance, true);
			m_SplitSquares = new SC_SplitSquare(SC_Square(point1, midpoint12, midpoint, midpoint41), SC_Square(point2, midpoint23, midpoint, midpoint12), SC_Square(point3, midpoint34, midpoint, midpoint23), SC_Square(point4, midpoint41, midpoint, midpoint34));
			std::get<0>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
			std::get<1>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
			std::get<2>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
			std::get<3>(*m_SplitSquares).Split(splitCount - 1, pointDistance);
		}
	};

	SC_Square MakeSquare(SC_Point p1, SC_Point p2, SC_Point p3, SC_Point p4) const {
		return std::make_tuple(p1, p2, p3, p4);
	}

	SC_SurfaceSquare MakeSurfaceSquare(SC_Square square, SC_SurfaceSquare::SC_SplitSquare* split) const {
		return SC_SurfaceSquare(square, split);
	}

private:
	const static int SURFACE_POINTS = 8;
	const static int SURFACE_COUNT = 6;
	const static int SURFACE_EDGES = 4;
	const static int SURFACE_SPLIT_COUNT = 4;
	float m_HalfSize;
	float m_PointDistance;
	int m_SplitCount = 0;

	SC_Point m_PrimarySurfacePoints[SURFACE_POINTS];
	SC_SurfaceSquare m_Surfaces[SURFACE_COUNT];

	unsigned int m_PointCount;
	unsigned int m_LinePointCount;

	GLuint m_LineVAO;
	GLuint m_LineVBO;

public:
	SplittableCube(float size = 10.0f, int splitCount = 0, bool showLines = true, float rotationSpeed = 0.0f)
	{
		SetSize(size);

		LayoutSurfacePoints();
		DeterminePrimarySurfaces();

		SetSplitCount(splitCount);

		BasicRenderable3D::SetRotationSpeed(rotationSpeed);
		BasicRenderable3D::SetShowLines(showLines);

		SetupVAO();
	}

	inline void SetValues(float size = 10.0f, int splitCount = 0, bool showLines = true, float rotationSpeed = 0.0f, GLuint textureID = 0, tdogl::Program* shaderProgram = nullptr) {
		SetSize(size);

		LayoutSurfacePoints();
		DeterminePrimarySurfaces();

		SetSplitCount(splitCount);

		BasicRenderable3D::SetTextureID(textureID);
		BasicRenderable3D::SetRotationSpeed(rotationSpeed);
		BasicRenderable3D::SetShaderProgram(shaderProgram);
		BasicRenderable3D::SetShowLines(showLines);

		SetupVAO();
	}

	inline void SetSize(float size) {
		m_HalfSize = size / 2.0f;
		m_PointDistance = std::sqrt(m_HalfSize * m_HalfSize + m_HalfSize * m_HalfSize + m_HalfSize * m_HalfSize);
	}

	inline void SetSplitCount(int splitCount) {
		if (splitCount != m_SplitCount)
			for (int i = 0; i < SURFACE_COUNT; ++i)
				m_Surfaces[i].Split(splitCount, m_PointDistance);
		m_SplitCount = splitCount;
	}

	void LayoutSurfacePoints()
	{
		m_PrimarySurfacePoints[0] = MakePoint(-m_HalfSize, -m_HalfSize, -m_HalfSize);
		m_PrimarySurfacePoints[1] = MakePoint(m_HalfSize, -m_HalfSize, -m_HalfSize);
		m_PrimarySurfacePoints[2] = MakePoint(m_HalfSize, m_HalfSize, -m_HalfSize);
		m_PrimarySurfacePoints[3] = MakePoint(-m_HalfSize, m_HalfSize, -m_HalfSize);
		m_PrimarySurfacePoints[4] = MakePoint(-m_HalfSize, -m_HalfSize, m_HalfSize);
		m_PrimarySurfacePoints[5] = MakePoint(m_HalfSize, -m_HalfSize, m_HalfSize);
		m_PrimarySurfacePoints[6] = MakePoint(m_HalfSize, m_HalfSize, m_HalfSize);
		m_PrimarySurfacePoints[7] = MakePoint(-m_HalfSize, m_HalfSize, m_HalfSize);
	}

	void DeterminePrimarySurfaces()
	{
		//  Create the texture coordinates for each of the original points being laid out for the primary surfaces
		auto point0_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.125f);
		auto point0_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.125f);
		auto point0_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.001f);
		auto point0_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.001f);

		auto point1_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.625f);
		auto point1_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.625f);
		auto point1_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.500f);
		auto point1_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.500f);

		auto point2_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.250f);
		auto point2_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.250f);
		auto point2_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.125f);
		auto point2_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.125f);

		auto point3_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.750f);
		auto point3_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.750f);
		auto point3_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.625f);
		auto point3_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.625f);

		auto point4_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.375f);
		auto point4_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.375f);
		auto point4_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.250f);
		auto point4_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.250f);

		auto point5_0 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.500f);
		auto point5_1 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.500f);
		auto point5_2 = MakePoint(0.0f, 0.0f, 0.0f, 1.000f, 0.375f);
		auto point5_3 = MakePoint(0.0f, 0.0f, 0.0f, 0.000f, 0.375f);

		//  Add the texture coordinates in, so that we can apply a texture if needed
		m_Surfaces[0] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[0], point0_0), AddPoints(m_PrimarySurfacePoints[1], point0_1), AddPoints(m_PrimarySurfacePoints[2], point0_2), AddPoints(m_PrimarySurfacePoints[3], point0_3)), nullptr);
		m_Surfaces[1] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[1], point1_0), AddPoints(m_PrimarySurfacePoints[5], point1_1), AddPoints(m_PrimarySurfacePoints[6], point1_2), AddPoints(m_PrimarySurfacePoints[2], point1_3)), nullptr);
		m_Surfaces[2] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[5], point2_0), AddPoints(m_PrimarySurfacePoints[4], point2_1), AddPoints(m_PrimarySurfacePoints[7], point2_2), AddPoints(m_PrimarySurfacePoints[6], point2_3)), nullptr);
		m_Surfaces[3] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[4], point3_0), AddPoints(m_PrimarySurfacePoints[0], point3_1), AddPoints(m_PrimarySurfacePoints[3], point3_2), AddPoints(m_PrimarySurfacePoints[7], point3_3)), nullptr);
		m_Surfaces[4] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[3], point4_0), AddPoints(m_PrimarySurfacePoints[2], point4_1), AddPoints(m_PrimarySurfacePoints[6], point4_2), AddPoints(m_PrimarySurfacePoints[7], point4_3)), nullptr);
		m_Surfaces[5] = MakeSurfaceSquare(MakeSquare(AddPoints(m_PrimarySurfacePoints[4], point5_0), AddPoints(m_PrimarySurfacePoints[5], point5_1), AddPoints(m_PrimarySurfacePoints[1], point5_2), AddPoints(m_PrimarySurfacePoints[0], point5_3)), nullptr);
	}

	void SetupVAO()
	{
		//// NOTE: In the future, use the actual number of points instead of adding duplicate points in
		//auto pointCount = GetShapePointsAfterSplit(SURFACE_POINTS, SURFACE_COUNT, 4, 4, m_SplitCount);
		//float* vertices = new float[pointCount * 3];

		//  Set up a vertex array large enough to hold every point of every surface, even duplicates
		m_PointCount = (m_SplitCount > 0) ? (GetShapeSurfacesAfterSplit(SURFACE_POINTS, SURFACE_COUNT, SURFACE_SPLIT_COUNT, SURFACE_EDGES, m_SplitCount) * SURFACE_EDGES) : (SURFACE_COUNT * SURFACE_EDGES);
		float* vertices = new float[m_PointCount * 5];
		memset(vertices, 0, sizeof(float) * m_PointCount * 5);

		// Using each primary surface, add all geometry vertices
		for (int i = 0; i < SURFACE_COUNT; ++i) AddVerticesForGeometry(m_Surfaces[i], vertices, (m_PointCount * 5 / SURFACE_COUNT), i * (m_PointCount * 5 / SURFACE_COUNT));

		//  Set up the Geometry VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupGeometryVAO(vertices, 5, 4, m_PointCount / SURFACE_EDGES, 3, 2, GL_QUADS);

		delete[] vertices;

		m_LinePointCount = m_PointCount * 2;
		vertices = new float[m_LinePointCount * 3];
		memset(vertices, 0, sizeof(float) * m_LinePointCount * 3);

		// Using each primary surface, add all line vertices
		for (int i = 0; i < SURFACE_COUNT; ++i) AddVerticesForLines(m_Surfaces[i], vertices, (m_LinePointCount * 3 / SURFACE_COUNT), i * (m_LinePointCount * 3 / SURFACE_COUNT));

		//  Set up the Lines VAO in the Basic Renderable 3D subclass
		BasicRenderable3D::SetupLinesVAO(vertices, 3, m_PointCount, 3);

		delete[] vertices;
	}

	void AddVerticesForGeometry(SC_SurfaceSquare& surfaceSquare, float* vertexArray, unsigned int memoryBlock, unsigned int memoryIndex)
	{
		//  vertexArray is size [m_PointCount * 5] because each point has five floats (x, y, z, tx, ty)
		//  memoryBlock is how many values we should be writing
		//  memoryIndex is where we begin to write, based on which surface we are

		if (surfaceSquare.m_SplitSquares == nullptr)
		{
			vertexArray[memoryIndex +  0] = std::get<0>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  1] = std::get<1>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  2] = std::get<2>(std::get<0>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  3] = std::get<3>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  4] = std::get<4>(std::get<0>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  5] = std::get<0>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  6] = std::get<1>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  7] = std::get<2>(std::get<1>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  8] = std::get<3>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  9] = std::get<4>(std::get<1>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 10] = std::get<0>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 11] = std::get<1>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 12] = std::get<2>(std::get<2>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 13] = std::get<3>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 14] = std::get<4>(std::get<2>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 15] = std::get<0>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 16] = std::get<1>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 17] = std::get<2>(std::get<3>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 18] = std::get<3>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 19] = std::get<4>(std::get<3>(surfaceSquare.m_Square));
		}
		else
		{
			auto newMemoryBlock = memoryBlock / 4;
			AddVerticesForGeometry(std::get<0>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 0);
			AddVerticesForGeometry(std::get<1>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 1);
			AddVerticesForGeometry(std::get<2>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 2);
			AddVerticesForGeometry(std::get<3>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 3);
		}
	}

	void AddVerticesForLines(SC_SurfaceSquare& surfaceSquare, float* vertexArray, unsigned int memoryBlock, unsigned int memoryIndex)
	{
		//  vertexArray is size [m_LinePointCount * 3] because each point has three floats (x, y, z)
		//  memoryBlock is how many values we should be writing
		//  memoryIndex is where we begin to write, based on which surface we are

		if (surfaceSquare.m_SplitSquares == nullptr)
		{
			vertexArray[memoryIndex +  0] = std::get<0>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  1] = std::get<1>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  2] = std::get<2>(std::get<0>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  3] = std::get<0>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  4] = std::get<1>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  5] = std::get<2>(std::get<1>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  6] = std::get<0>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  7] = std::get<1>(std::get<1>(surfaceSquare.m_Square));	vertexArray[memoryIndex +  8] = std::get<2>(std::get<1>(surfaceSquare.m_Square));
			vertexArray[memoryIndex +  9] = std::get<0>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 10] = std::get<1>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 11] = std::get<2>(std::get<2>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 12] = std::get<0>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 13] = std::get<1>(std::get<2>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 14] = std::get<2>(std::get<2>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 15] = std::get<0>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 16] = std::get<1>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 17] = std::get<2>(std::get<3>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 18] = std::get<0>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 19] = std::get<1>(std::get<3>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 20] = std::get<2>(std::get<3>(surfaceSquare.m_Square));
			vertexArray[memoryIndex + 21] = std::get<0>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 22] = std::get<1>(std::get<0>(surfaceSquare.m_Square));	vertexArray[memoryIndex + 23] = std::get<2>(std::get<0>(surfaceSquare.m_Square));
		}
		else
		{
			auto newMemoryBlock = memoryBlock / 4;
			AddVerticesForLines(std::get<0>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 0);
			AddVerticesForLines(std::get<1>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 1);
			AddVerticesForLines(std::get<2>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 2);
			AddVerticesForLines(std::get<3>(*surfaceSquare.m_SplitSquares), vertexArray, newMemoryBlock, memoryIndex + newMemoryBlock * 3);
		}
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