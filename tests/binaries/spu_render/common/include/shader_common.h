#ifndef COMMON_SHADER_COMMON_H
#define COMMON_SHADER_COMMON_H

#define DIRECTIONAL_LIGHT_DIR directional_light_dir
#define DIRECTIONAL_LIGHT_COLOR directional_light_color
#define SPOT_LIGHT_POSITION_0 spot_light_position0
#define SPOT_LIGHT_POSITION_1 spot_light_position1
#define SPOT_LIGHT_COLOR_0 spot_light_color0
#define SPOT_LIGHT_COLOR_1 spot_light_color1


#ifdef __CGC__
uniform float4x4 modelViewProj : C0;
uniform float4x4 modelView : C4;

// FragmentShader parameter
#define AS_PARAMNAME(x) x
uniform float4	AS_PARAMNAME(DIRECTIONAL_LIGHT_DIR); // normalized
uniform float4	AS_PARAMNAME(DIRECTIONAL_LIGHT_COLOR); // normalized
uniform float4	AS_PARAMNAME(SPOT_LIGHT_POSITION_0);
uniform float4	AS_PARAMNAME(SPOT_LIGHT_POSITION_1);
uniform float4	AS_PARAMNAME(SPOT_LIGHT_COLOR_0);
uniform float4	AS_PARAMNAME(SPOT_LIGHT_COLOR_1);
#undef AS_PARAMNAME

// Texure parameter
uniform sampler2D color_sampler : TEXUNIT0;
uniform sampler2D msaa_sampler0 : TEXUNIT0;
uniform sampler2D msaa_sampler1 : TEXUNIT1;

// Global constants
const float displayHeight = 720.0f;
const float displayWidth = 1280.0f;
const float2 Display_div = {1.0f/1280.0f,1.0f/720.0f};

// Uniform values
uniform float4 I_a = {0.3f, 0.3f, 0.3f, 0.0f};
uniform float4 I_d = {0.3f, 0.3f, 0.3f, 0.0f};
uniform float4 K_a = {1.0f, 1.0f, 1.0f, 1.0f};
uniform float4 K_d = {1.0f, 1.0f, 1.0f, 1.0f};

#else // For PPU/SPU code
enum{
	VERTEXSHADER_MODEL_VIEW_PROJ_CONSTANT_INDEX = 0,
	VERTEXSHADER_MODEL_VIEW_CONSTANT_INDEX = 4,
	FRAGMENT_SHADER_COLOR_SAMPLER_INDEX=0,
	MSAA_SAMPLER0 = 0,
	MSAA_SAMPLER1 = 1
};

#define AS_CSTRING(macro) STR(macro) // See "The C Preprocessor - 3.4 Stringification"
#define STR(s) #s
#define DIRECTIONAL_LIGHT_DIR_NAME AS_CSTRING(DIRECTIONAL_LIGHT_DIR)
#define DIRECTIONAL_LIGHT_COLOR_NAME AS_CSTRING(DIRECTIONAL_LIGHT_COLOR)
#define SPOT_LIGHT_POSITION_0_NAME AS_CSTRING(SPOT_LIGHT_POSITION_0)
#define SPOT_LIGHT_POSITION_1_NAME AS_CSTRING(SPOT_LIGHT_POSITION_1)
#define SPOT_LIGHT_COLOR_0_NAME AS_CSTRING(SPOT_LIGHT_COLOR_0)
#define SPOT_LIGHT_COLOR_1_NAME AS_CSTRING(SPOT_LIGHT_COLOR_1)

#define SHADER_BINARY_ALIGNMENT 128
#endif // __CGC__

#endif // COMMON_SHADER_COMMON_H