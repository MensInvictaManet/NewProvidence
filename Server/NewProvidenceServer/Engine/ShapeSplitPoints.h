# pragma once

unsigned int GetShapePointsAfterSplit(unsigned int origPoints, unsigned int origSurfaces, unsigned int numPartsPerSplit, unsigned int origLinesPerSurface, unsigned int splitCount)
{
	if (origPoints < 4 || origSurfaces < 4 || origLinesPerSurface < 3 || splitCount == 0) return 0;
	auto points = origPoints + (origSurfaces * origLinesPerSurface / 2);
	auto surfaces = origSurfaces * numPartsPerSplit;
	auto lines = surfaces * origLinesPerSurface / 2;
	if (--splitCount > 0) return GetShapePointsAfterSplit(points, surfaces, numPartsPerSplit, origLinesPerSurface, splitCount);
	else return points;
}

unsigned int GetShapeSurfacesAfterSplit(unsigned int origPoints, unsigned int origSurfaces, unsigned int numPartsPerSplit, unsigned int origLinesPerSurface, unsigned int splitCount)
{
	if (origPoints < 4 || origSurfaces < 4 || origLinesPerSurface < 3 || splitCount == 0) return 0;
	auto points = origPoints + (origSurfaces * origLinesPerSurface / 2);
	auto surfaces = origSurfaces * numPartsPerSplit;
	auto lines = surfaces * origLinesPerSurface / 2;
	if (--splitCount > 0) return GetShapeSurfacesAfterSplit(points, surfaces, numPartsPerSplit, origLinesPerSurface, splitCount);
	else return surfaces;
}

unsigned int GetShapeLinesAfterSplit(unsigned int origPoints, unsigned int origSurfaces, unsigned int numPartsPerSplit, unsigned int origLinesPerSurface, unsigned int splitCount)
{
	if (origPoints < 4 || origSurfaces < 4 || origLinesPerSurface < 3 || splitCount == 0) return 0;
	auto points = origPoints + (origSurfaces * origLinesPerSurface / 2);
	auto surfaces = origSurfaces * numPartsPerSplit;
	auto lines = surfaces * origLinesPerSurface / 2;
	if (--splitCount > 0) return GetShapeLinesAfterSplit(points, surfaces, numPartsPerSplit, origLinesPerSurface, splitCount);
	else return lines;
}

/*
d4 (4 points, 4 surfaces, 3 lines per surface) 		= 4 + (4 * 3 / 2) = 10 (surfaces 16, lines 24)
d6 (8 points, 6 surfaces, 4 lines per surface) 		= 8 + (6 * 4 / 2) = 20 (surfaces 24, lines 48)
d8 (6 points, 8 surfaces, 3 lines per surface) 		= 6 + (8 * 3 / 2) = 18 (surfaces 32, lines 48)
d10 (12 points, 10 surfaces, 4 lines per surface) 	= 12 + (10 * 4 / 2) = 32 (surfaces 40, lines 80)
d12 (20 points, 12 surfaces, 5 lines per surface) 	= 20 + (12 * 5 / 2) = 50
d20 (12 points, 20 surfaces, 3 lines per surface) 	= 12 + (20 * 3 / 2) = 42 (surfaces 80, lines 120)

The number of points after a split is X + (Y * Z / 2) where:
- X is number of points originally
- Y is the number of surfaces
- Z is the number of lines per surface

The number of surfaces after a split is X * Y where:
- X is the number of surfaces before the split
- Y is the number of parts each surface splits into

The number of lines after a split is (X * Y / 2) where:
- X is the number of surfaces after a split
- Y is the number of lines per surface

NOTE: These rules only work on shapes which, when split, split each surface into a shape with the same number of sides. i.e. it doesn't work on a d12 shape because pentagons get split into triangles. For now, I'm going to skip over the math needed to do those types of shapes, as they are very much unnecessary.
*/