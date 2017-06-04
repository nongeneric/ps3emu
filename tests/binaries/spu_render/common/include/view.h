#ifndef COMMON_VIEW_H
#define COMMON_VIEW_H

struct Perspective{
	float fovyRadians;
	float aspect;
	float zNear;
	float zFar;
	
	inline void Set(float _fovyRadians, float width, float height, float _zNear, float _zFar){
		fovyRadians = _fovyRadians;
		zNear = _zNear;
		zFar = _zFar;
		aspect = width / height;
	}
	inline const Vectormath::Aos::Matrix4 getGLProjectionMatrix(){
		return Vectormath::Aos::Matrix4::perspective(fovyRadians, aspect, zNear, zFar);
	}
} __attribute__ ((aligned(16)));

struct	Orthographic
{
	float			left;
	float			right;
	float			bottom;
	float			top;
	float			zNear;
	float			zFar;
	float			_padding[2];
	inline void Set(float _left, float _right, float _bottom, float _top, float	_zNear, float _zFar){
		left = _left;
		right = _right;
		bottom = _bottom;
		top = _top;
		zNear = _zNear;
		zFar = _zFar;
	}
	inline const Vectormath::Aos::Matrix4 getGLProjectionMatrix(){
		return Vectormath::Aos::Matrix4::orthographic(left,right,bottom,top,zNear,zFar);
	}
}__attribute__ ((aligned(16)));

struct Camera{
	Vectormath::Aos::Point3 cameraPos;
	Vectormath::Aos::Point3 targetPos;
	Vectormath::Aos::Vector3 upVec;
	inline void Set(Vectormath::Aos::Point3 _cameraPos, Vectormath::Aos::Point3 _targetPos, Vectormath::Aos::Vector3 _upVec){
		cameraPos = _cameraPos;
		targetPos = _targetPos;
		upVec = _upVec;
	}
	inline const Vectormath::Aos::Matrix4 getGLViewMatrix(){
		return Vectormath::Aos::Matrix4::lookAt(cameraPos,targetPos,upVec);
	}
}__attribute__ ((aligned(16)));

inline void setViewportGL(CellGcmContextData* thisContext,
						  uint16_t x, uint16_t y, uint16_t width, uint16_t height,
						  float zmin, float zmax, uint16_t surface_height)
{
	// Set Viewport transform
	uint16_t _x;
	uint16_t _y;
	uint16_t _width;
	uint16_t _height;
	float _min;
	float _max;
	float scale[4];
	float offset[4];

	_x = x;
	_y = surface_height - y - height;
	_width = width;
	_height = height;
	_min = zmin;
	_max = zmax;

	scale[0] = _width * 0.5f;
	scale[1] = _height * -0.5f;
	scale[2] = (_max - _min) * 0.5f;
	scale[3] = 0.0f;
	offset[0] = _x + _width * 0.5f;
	offset[1] = _y + _height * 0.5f;
	offset[2] = (_max + _min) * 0.5f;
	offset[3] = 0.0f;

	cell::Gcm::Inline::cellGcmSetViewport(thisContext,_x,_y,_width,_height,_min,_max,scale,offset);
	cell::Gcm::Inline::cellGcmSetScissor(thisContext,_x,_y,_width,_height);
}

inline void setViewport(CellGcmContextData* thisContext,
						int32_t _x, int32_t _y, int32_t _width, int32_t _height,
						float _Zmin, float _Zmax){
	// Set Viewport transform
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	float min;
	float max;

	float scale[4];
	float offset[4];

	x = _x;
	y = _y;
	width = _width;
	height = _height;
	min = _Zmin;
	max = _Zmax;
	scale[0] = width * 0.5f;
	scale[1] = height * 0.5f;
	scale[2] = (max - min) * 0.5f;
	scale[3] = 0.0f;
	offset[0] = x + width * 0.5f;
	offset[1] = y + height * 0.5f;
	offset[2] = (max + min) * 0.5f;
	offset[3] = 0.0f;

	cell::Gcm::Inline::cellGcmSetViewport(thisContext,x,y,width,height,min,max,scale,offset);
	cell::Gcm::Inline::cellGcmSetScissor(thisContext,x,y,width,height);
}

#endif // COMMON_VIEW_H