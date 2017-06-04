/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2010 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */


#ifdef DOWNSAMPLE_2X_ACCUVIEW_SHIFTED
#define DOWNSAMPLE_2X_QUINCUNX
#define DOWNSAMPLE_2X_QUINCUNX_ALT
#endif

#include "shader_common.h"

void main
(float2 texcoord     : TEXCOORD0, // range 0.0 - 1.0
 out float4 color_out : COLOR
 )
{

#ifdef DOWNSAMPLE_2X_QUINCUNX
	// quincunx
	float2 du = float2(0.0001,0.0);
	float4 color0 = f4tex2D(msaa_sampler0,texcoord + du);
	color_out = color0;
#endif
#ifdef DOWNSAMPLE_2X_QUINCUNX_ALT
	// quincunx_alt
	float2 d1 = float2( -0.0001, -1.0/displayHeight);
	float4 color1 = f4tex2D(msaa_sampler1, texcoord + d1);
	color_out = color1;
#endif
#ifdef DOWNSAMPLE_2X_ACCUVIEW_SHIFTED
	// blend
	color_out = lerp(color0, color1, 0.5);
#endif

}
