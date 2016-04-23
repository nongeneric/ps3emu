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
	CELL_GCM_fpshader_params_texcoord = 0,
	CELL_GCM_fpshader_params_normal = 1,
	CELL_GCM_fpshader_params_texture = 2,
	CELL_GCM_fpshader_params_light = 3,
	CELL_GCM_fpshader_params_color__out = 4,
};

CellGcm_fpshader_params_Table fpshader_params[5] = {
	{3220, -1, -1}, /* index:0, "texcoord" */
	{3221, -1, -1}, /* index:1, "normal" */
	{2048, -1, -1}, /* index:2, "texture" */
	{3256, -1, 0}, /* index:3, "light" */
	{2757, -1, -1}, /* index:4, "color_out" */
};

CellGcm_fpshader_params_ConstOffsetTable fpshader_params_const_offset[2] = {
	{0x10},
	{0xffffffff},
};

#endif //_fpshader_params_H_
