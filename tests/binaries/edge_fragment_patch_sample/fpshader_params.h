#ifndef _fpshader_params_H_
#define _fpshader_params_H_

typedef struct {
	int res;
	int resindex;
	int ecindex;
} CellGcm_fpshader_params_Table;

typedef struct {
	unsigned int offset;
} CellGcm_fpshader_params_ConstOffsetTable;

enum CellGcm_fpshader_params_Enum {
	CELL_GCM_fpshader_params_fragment_texcoord = 0,
	CELL_GCM_fpshader_params_fragment_tanSpaceLightVec = 1,
	CELL_GCM_fpshader_params_fragment_tanSpaceViewVec = 2,
	CELL_GCM_fpshader_params_colorMap = 3,
	CELL_GCM_fpshader_params_bumpGlossMap = 4,
	CELL_GCM_fpshader_params_facing = 5,
	CELL_GCM_fpshader_params_lightRgb = 6,
	CELL_GCM_fpshader_params_main_color = 7,
};

CellGcm_fpshader_params_Table fpshader_params[8] = {
	{3220, -1, -1}, /* index:0, "fragment.texcoord" */
	{3221, -1, -1}, /* index:1, "fragment.tanSpaceLightVec" */
	{3222, -1, -1}, /* index:2, "fragment.tanSpaceViewVec" */
	{2048, -1, -1}, /* index:3, "colorMap" */
	{2049, -1, -1}, /* index:4, "bumpGlossMap" */
	{2199, -1, -1}, /* index:5, "facing" */
	{3256, -1, 0}, /* index:6, "lightRgb" */
	{2757, -1, -1}, /* index:7, "main.color" */
};

CellGcm_fpshader_params_ConstOffsetTable fpshader_params_const_offset[3] = {
	{0x160},
	{0xe0},
	{0xffffffff},
};

#endif //_fpshader_params_H_
