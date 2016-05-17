/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2006 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

struct v2fConnector
{
	float2 texCoord  : TEXCOORD0;
};

struct f2fConnector
{
	float4 COL : COLOR;
};

f2fConnector main
(
	v2fConnector v2f,
	uniform texobj2D texture
)
{
	f2fConnector f2f;
	f2f.COL = tex2D( texture, v2f.texCoord );
	return f2f;
}

