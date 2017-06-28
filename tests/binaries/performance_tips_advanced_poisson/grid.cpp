/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include "FWDebug.h"
#include "grid.h"
#include "FWGLInclude.h"

Grid::Grid() :
	mSpacing(10.f),
	mLineR(0.5f),
	mLineG(0.5f),
	mLineB(0.5f),
	mpHeightMap(NULL),
	mHeightScale(1.0f),
	mpVerts(NULL),
	mpIndex(NULL),
	mpCols(NULL),
	mNumVerts(0),
	mNumIndex(0),
	mNumGridLinesX(10),
	mNumGridLinesZ(10),
	mLineWidth(1.f),
	mBuildGrid(true),
	mVertexBuffer(0),
	mColorBuffer(0)
{
}

Grid::~Grid()
{
	if (mpVerts) {
		delete[] mpVerts;
	}

	if (mpCols) {
		delete[] mpCols;
	}
}

void Grid::render()
{
	if (mBuildGrid) {
		buildGrid();
	}

	glLineWidth(mLineWidth);
	glPrimitiveRestartIndexNV(getIndexSeparator());

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glVertexPointer(3, GL_FLOAT, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
	glColorPointer(4, GL_FLOAT, 0, NULL);

	glDrawElements(GL_LINE_STRIP, mNumIndex, GL_UNSIGNED_INT, mpIndex);
}

void Grid::setNumGridLines(int numGridLinesX, int numGridLinesZ)
{
	mNumGridLinesX = numGridLinesX;
	mNumGridLinesZ = numGridLinesZ;
	initializeVerts();
}

void Grid::initializeVerts()
{
	// allocate space for verts
	if (mpVerts) {
		delete[] mpVerts;
	}

	if (mpCols) {
		delete[] mpCols;
	}

	if (mpIndex) {
		delete[] mpCols;
	}

	mNumVerts = mNumGridLinesX * mNumGridLinesZ;//calcNumVerts();
	mNumIndex = 0;
	mNumIndex += mNumGridLinesX * (mNumGridLinesZ + 1);
	mNumIndex += mNumGridLinesZ * (mNumGridLinesX + 1);

	mpVerts = new float[mNumVerts * 3];
	mpCols = new float[mNumVerts * 4];
	mpIndex = new unsigned int[mNumIndex];
}

void Grid::addVert(int i, int j, float height)
{
	mpNextVert[0] = (i - (mNumGridLinesX - 1) * 0.5f) * mSpacing;
	mpNextVert[2] = (j - (mNumGridLinesZ - 1) * 0.5f) * mSpacing;
	mpNextVert[1] = height;
	mpNextVert += 3;
	mpNextCol[0] = mLineR;
	mpNextCol[1] = mLineG;
	mpNextCol[2] = mLineB;
	mpNextCol[3] = 1.f;
	mpNextCol += 4;
}

void Grid::addIndex(int i, int j)
{
	*mpNextIndex = i * mNumGridLinesZ + j;
	mpNextIndex++;
}

void Grid::addIndexSeparator()
{
	*mpNextIndex = getIndexSeparator();
	mpNextIndex++;
}

void Grid::buildGrid()
{
	int i, j;

	mpNextVert = mpVerts;
	mpNextCol = mpCols;
	mpNextIndex = mpIndex;

	// grid lines
	for (i = 0; i < mNumGridLinesX; i++) {
		for (j = 0; j < mNumGridLinesZ; j++) {
			addVert(i, j, *(mpHeightMap + i * mNumGridLinesX + j) * mHeightScale);
		}
	}
	// index list
	for (i = 0; i < mNumGridLinesX; i++) {
		for (j = 0; j < mNumGridLinesZ; j++) {
			addIndex(i, j);
		}
		addIndexSeparator();
	}
	for (j = 0; j < mNumGridLinesZ; j++) {
		for (i = 0; i < mNumGridLinesX; i++) {
			addIndex(i, j);
		}
		addIndexSeparator();
	}

	// check number of verts
	int numVertsWritten __attribute__((unused)) = (((long)mpNextVert - (long)mpVerts) / (sizeof(float) * 3));
	FWASSERT(numVertsWritten == mNumVerts);

	// set up for line rendering
	if (!mVertexBuffer) {
		glGenBuffers(1, &mVertexBuffer);
		glGenBuffers(1, &mColorBuffer);
	}

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glDepthMask(1);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_PRIMITIVE_RESTART_NV);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER,
				 mNumVerts * 3 * sizeof(float), mpVerts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
	glBufferData(GL_ARRAY_BUFFER,
				 mNumVerts * 4 * sizeof(float), mpCols, GL_STATIC_DRAW);

	mBuildGrid = false;
}

int Grid::calcNumVerts()
{
	int numVerts = 0;
	numVerts += mNumGridLinesZ * 2 * (mNumGridLinesX - 1);
	numVerts += mNumGridLinesX * 2 * (mNumGridLinesZ - 1);

	return numVerts;
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
