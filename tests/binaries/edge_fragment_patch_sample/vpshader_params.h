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
	CELL_GCM_vpshader_params_vertex_position = 0,
	CELL_GCM_vpshader_params_vertex_normal = 1,
	CELL_GCM_vpshader_params_vertex_tanFlip = 2,
	CELL_GCM_vpshader_params_vertex_texcoord = 3,
	CELL_GCM_vpshader_params_mvp = 4,
	CELL_GCM_vpshader_params_mvp_0 = 5,
	CELL_GCM_vpshader_params_mvp_1 = 6,
	CELL_GCM_vpshader_params_mvp_2 = 7,
	CELL_GCM_vpshader_params_mvp_3 = 8,
	CELL_GCM_vpshader_params_lightPos = 9,
	CELL_GCM_vpshader_params_eyePos = 10,
	CELL_GCM_vpshader_params_main_position = 11,
	CELL_GCM_vpshader_params_main_texcoord = 12,
	CELL_GCM_vpshader_params_main_tanSpaceLightVec = 13,
	CELL_GCM_vpshader_params_main_tanSpaceViewVec = 14,
	CELL_GCM_vpshader_params_main_objSpaceLightVec = 15,
};

CellGcm_vpshader_params_Table vpshader_params[16] = {
	{2113, -1, -1}, /* index:0, "vertex.position" */
	{2115, -1, -1}, /* index:1, "vertex.normal" */
	{2127, -1, -1}, /* index:2, "vertex.tanFlip" */
	{2121, -1, -1}, /* index:3, "vertex.texcoord" */
	{2178, 0, -1}, /* index:4, "mvp" */
	{2178, 0, -1}, /* index:5, "mvp[0]" */
	{2178, 1, -1}, /* index:6, "mvp[1]" */
	{2178, 2, -1}, /* index:7, "mvp[2]" */
	{2178, 3, -1}, /* index:8, "mvp[3]" */
	{2178, 4, -1}, /* index:9, "lightPos" */
	{2178, 5, -1}, /* index:10, "eyePos" */
	{2243, -1, -1}, /* index:11, "main.position" */
	{3220, -1, -1}, /* index:12, "main.texcoord" */
	{3221, -1, -1}, /* index:13, "main.tanSpaceLightVec" */
	{3222, -1, -1}, /* index:14, "main.tanSpaceViewVec" */
	{3223, -1, -1}, /* index:15, "main.objSpaceLightVec" */
};

CellGcm_vpshader_params_DefaultValueTable vpshader_params_default_value[0] = {
};

#endif //_vpshader_params_H_
