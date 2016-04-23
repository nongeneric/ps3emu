#ifndef _vpshader_params_H_
#define _vpshader_params_H_

typedef struct {
	int res;
	int resindex;
	int dvindex;
} CellGcm_vpshader_params_Table;

typedef struct {
	float defaultvalue[4];
} CellGcm_vpshader_params_DefaultValueTable;

enum CellGcm_vpshader_params_Enum {
	CELL_GCM_vpshader_params_position = 0,
	CELL_GCM_vpshader_params_texcoord = 1,
	CELL_GCM_vpshader_params_normal = 2,
	CELL_GCM_vpshader_params_modelViewProj = 3,
	CELL_GCM_vpshader_params_modelViewProj_0 = 4,
	CELL_GCM_vpshader_params_modelViewProj_1 = 5,
	CELL_GCM_vpshader_params_modelViewProj_2 = 6,
	CELL_GCM_vpshader_params_modelViewProj_3 = 7,
	CELL_GCM_vpshader_params_modelView = 8,
	CELL_GCM_vpshader_params_modelView_0 = 9,
	CELL_GCM_vpshader_params_modelView_1 = 10,
	CELL_GCM_vpshader_params_modelView_2 = 11,
	CELL_GCM_vpshader_params_modelView_3 = 12,
	CELL_GCM_vpshader_params_oPosition = 13,
	CELL_GCM_vpshader_params_oTexCoord = 14,
	CELL_GCM_vpshader_params_oNormal = 15,
};

CellGcm_vpshader_params_Table vpshader_params[16] = {
	{2113, -1, -1}, /* index:0, "position" */
	{2121, -1, -1}, /* index:1, "texcoord" */
	{2122, -1, -1}, /* index:2, "normal" */
	{2178, 256, -1}, /* index:3, "modelViewProj" */
	{2178, 256, 0}, /* index:4, "modelViewProj[0]" */
	{2178, 257, 1}, /* index:5, "modelViewProj[1]" */
	{2178, 258, 2}, /* index:6, "modelViewProj[2]" */
	{2178, 259, 3}, /* index:7, "modelViewProj[3]" */
	{2178, 260, -1}, /* index:8, "modelView" */
	{2178, 260, -1}, /* index:9, "modelView[0]" */
	{2178, 261, -1}, /* index:10, "modelView[1]" */
	{2178, 262, -1}, /* index:11, "modelView[2]" */
	{2178, 263, -1}, /* index:12, "modelView[3]" */
	{2243, -1, -1}, /* index:13, "oPosition" */
	{3220, -1, -1}, /* index:14, "oTexCoord" */
	{3221, -1, -1}, /* index:15, "oNormal" */
};

CellGcm_vpshader_params_DefaultValueTable vpshader_params_default_value[4] = {
	{{1.000000f, 0.000000f, 0.000000f, 0.000000f}},
	{{0.000000f, 1.000000f, 0.000000f, 0.000000f}},
	{{0.000000f, 0.000000f, 1.000000f, 0.000000f}},
	{{0.000000f, 0.000000f, 0.000000f, 0.000000f}},
};

#endif //_vpshader_params_H_
