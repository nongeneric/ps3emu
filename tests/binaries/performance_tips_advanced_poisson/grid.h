/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#ifndef __GRID_H__
#define __GRID_H__

class Grid
{
public:
	Grid();
	~ Grid();

	inline void setSpacing(float spacing)
	{
		if (mSpacing != spacing) {
			mSpacing = spacing;
			mBuildGrid = true;
		}
	}

	inline float getSpacing() const
	{
		return mSpacing;
	}

	inline void setHeightMap(float *heightMap)
	{
		mpHeightMap = heightMap;
		mBuildGrid = true;
	}

	inline float getHeightScale() const
	{
		return mHeightScale;
	}

	inline void setHeightScale(float scale)
	{
		mHeightScale = scale;
		mBuildGrid = true;
	}

	inline void setGridLineColor(float r, float g, float b)
	{
		mLineR = r;
		mLineG = g;
		mLineB = b;
		mBuildGrid = true;
	}

	inline void setLineWidth(float width)
	{
		mLineWidth = width;
	}

	inline float getLineWidth() const
	{
		return mLineWidth;
	}

	inline unsigned int getIndexSeparator() const
	{
		return mNumGridLinesX * mNumGridLinesZ;
	}

	void render();
	void setNumGridLines(int nx, int nz);

private:
	void buildGrid();
	int calcNumVerts();
	void addVert(int i, int j, float height);
	void addIndex(int i, int j);
	void addIndexSeparator();
	void initializeVerts();

	// Spacing of the grid
	float mSpacing;

	// Line color red component
	float mLineR;

	// Line color green component
	float mLineG;

	// Line color blue component
	float mLineB;

	// Height map
	float *mpHeightMap;

	// Scaling of the height
	float mHeightScale;

	// Array of verts
	float *mpVerts;

	// Array of index
	unsigned int *mpIndex;

	// Array of colors
	float *mpCols;

	// First empty verts
	float *mpNextVert;

	// First empty index
	unsigned int *mpNextIndex;

	// First empty colors
	float *mpNextCol;

	// Number of verts and index
	int mNumVerts;
	int mNumIndex;

	// Total number of grid lines
	int mNumGridLinesX;
	int mNumGridLinesZ;

	// Line width
	float mLineWidth;

	// Grid needs building
	bool mBuildGrid;

	// a handle of Buffer Object
	unsigned int mVertexBuffer;
	unsigned int mColorBuffer;
};

#endif //__GRID_H__

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
