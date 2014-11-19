/********************************************************************
Copyright (C) 2008 姚勇- All Rights Reserved
This file is part of Softcore.

Softcore is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Softcore is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Softcore.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

//所有Id Software 软件源代码版权规Id Software公司所有

//////////////////////////////////////////////////////////////////////////
//渲染用核心数据，为了demo目的，保持尽可能简单，并且直接实用idMath 库的向量和矩阵
//同时尽量做到可移植性，在平台相关的部分进行抽象
//////////////////////////////////////////////////////////////////////////
#ifndef _SOFTRENDER_DEF
#define _SOFTRENDER_DEF


#define __PI 3.1415926f
#define MAX_STAGE 2
#define ALIGNED_BYTE 16
#define sfINLINE __forceinline
#define ZBUF_TYPE float

//draw point func param
extern void myassert(bool res,const char* s=0,bool forceExit=false);
extern void printDebug(const char* , ...);

sfINLINE extern bool hasTexCoord(int flags);

#include <vector>
#include <string>
#include "util/precompiled.h"
using namespace std;
class CVertexBuffer;
class CBitmapWrapper;
namespace SOFTCORE
{

	//渲染器状态
	enum
	{
		R_OK=0,
		R_NO_CONTEXT=1<<0,
		R_RENDERING=1<<1,
		R_STOP=1<<2,
		R_SHUTDOWN=1<<3,
	};
	struct dpoint
	{
		int x,y;
		char r,g,b,a;
	};

	//全部保持128bit的整倍数
	typedef idVec4 vec2;
	typedef idVec4 vec3;
	typedef idVec4 vec4;
#define VEC3FLOAT 4
#define VEC2FLOAT 4
#define VEC4FLOAT 4
	typedef idMat4 t_matrix;
	
	typedef unsigned short sfCol16  ;
	typedef unsigned int sfCol32 ;
	typedef unsigned char sfByte8 ;

	
	enum FILTER_MODE
	{
		FILTER_POINT=0,
		FILTER_LINEAR=1,
	};
	enum WRAP_MODE
	{
		WRAP_CLAMP,
		WRAP_REPEAT,
	};
	enum TEX_OP
	{
		TEX_REPLACE,
		TEX_MODULATE,
		TEX_ADD,
		TEX_BLEND,
	};
	struct t_texture
	{
		t_texture()
		{
			image= 0;
			min_filter = max_filter = FILTER_POINT;
			wrapmode[0] = wrapmode[1] = WRAP_REPEAT;
			tex_op = TEX_REPLACE;
			(DWORD)(*(DWORD*)col )= 0xffffffff;
		};
		CBitmapWrapper* image;
		FILTER_MODE min_filter,max_filter;
		int wrapmode[2] ;
		int tex_op;
		unsigned char col[4];
	};
	enum PIXEL_OP
	{
		PIXEL_BLEND = 1<<0,
		PIXEL_ALPHA = 1<<1,
		PIXEL_DEPTH = 1<<2,
	};
	enum MATRIX_MODE
	{
		MATRIX_VIEW,
		MATRIX_PROJ,
		MATRIX_TEX,
	};
	struct t_material
	{
		//blend, alpha test, depth...
		t_material()
		{
			flags = PIXEL_DEPTH;
			for (int i=0;i<MAX_STAGE;++i)
				tex[i]= 0;
			numTex = 0;
			gloss = power = 1.f;
		};
		int flags;
		float gloss;
		float power;
		t_texture* tex[MAX_STAGE];
		int numTex;
	};
	//外部顶点数据
	//这个结构保持16字节的倍数，并且以16字节对齐
	struct t_drawVert
	{
		vec3 pos;
		vec2 tex;
		vec3 norm;
		vec4 col;
		vec3 T,B;
	};
	struct t_triangle
	{
		unsigned int p[3];
		vec3 norm;
		int material_idx;
	};
	//内部vb格式
	enum VB_FMT
	{
		VB_UNKNOW=0,
		VB_POS ,//use global color to fill scan line
		VB_POS_COL,//use vertex color to modulate scan line pixel
		VB_POS_NORM=VB_POS_COL,//only use vertex lighting
		VB_POS_TEX=VB_POS_COL,//use tex mapping
		VB_POS_TEX_NORM,//tex with lighting
		VB_POS_TEX_COL=VB_POS_TEX_NORM,//tex with vertex col
		VB_POS_NORM_TB=VB_POS_NORM,//phong shading without texture
		VB_POS_TEX_NORM_TB = VB_POS_TEX_NORM+1,//phong shading with texture&bump
		VB_MAX_FMT,
	};
	struct t_mesh
	{
		t_mesh();
		~t_mesh();
		void createVB(int num);
		int getTriCount();
		const t_drawVert& getTriVert(int n);
		unsigned int* getIndexBuffer();
		t_drawVert* getVertexBuffer();
		t_triangle& getTriangle(int n);
		int numTri,numVert,numIndex;
		t_triangle* tris;
		//16bytes aligned 
		t_drawVert* vb;
		unsigned int* ib;
		t_material* material;
		//vb格式
		VB_FMT vb_format;
	};
	//light
	enum LIGHT_TYPE
	{
		LIGHT_POINT,
		LIGHT_DIR,
		LIGHT_SPOT,
		LIGHT_AREA,
	};
	struct AABB
	{
		float Pmax[3],Pmin[3];
	};

	struct t_vlight
	{
		vec3 light_objpos,eyepos,eyedir;
	};
	//一个非常简单的光源
	class clight_directional//it's a directional light
	{
	public:
		virtual void caculateCol(const vec3 &pos,const vec3 &norm,const t_vlight& lightparam,vec4 &col);
		vec4 pos;
		vec4 diffuse_col;
		vec4 specular_col;
		vec4 ambient_col;
		LIGHT_TYPE type;
		//for spot light
		vec3 target;
		AABB box;
	};

	class clight_point: public clight_directional
	{
	public:
		 void caculateCol(const vec3 &pos,const vec3 &norm,const t_vlight& lightparam,vec4 col);
	};

	class clight_spot:public clight_directional
	{
		void caculateCol(const vec3 &pos,const vec3 &norm,const t_vlight& lightparam,vec4 col);
	};

	//世界可渲染物体
	enum
	{
		ENTITY_MOVEABLE=1<<0,//can move&rotate
		ENTITY_DISABLE=1<<1,//can't push to render pipeline
		ENTITY_LOCAL=1<<2,//if entity has no local matrix , must transform all its vertex attribute and face normal when rotate&move it
	};
	struct t_entity
	{

	
	public:
		t_entity();
		//MODEL MATRIX
		void transform(const idQuat& quat,const vec3& pos);
		t_matrix m_model_matrix;
		//mesh mng
		void addMesh(t_mesh* mesh)
		{
			m_meshs.push_back(mesh);
		}
		t_mesh* getMesh(int n)
		{
				if (n<(int)m_meshs.size())
					return m_meshs[n];
				else
					return 0;
		
		};
		int getMeshCount()const {return (int)m_meshs.size();};
		//entity's mesh resource
		vector<t_mesh* >m_meshs;
		//other properties
		//照射到的灯光数组
		vector<clight_directional*> lights;
		int m_flags;
		//为了把camera和light从world space 转换到object space
		t_matrix m_model_inv_matrix;
		//AABB
		float m_aabb[6];
	};

	struct t_camera
	{
		//look at
		vec3 pos;
		vec3 at;
		//eular
		vec3 rot;
		vec3 dir,dir_right;
		//cam's view,view's inverse &projmatrix
		t_matrix mat,projmat,view_invert;
		float near_dist,farplane,ratio,fovx;
	};
	struct t_plane
	{
		vec4 param;//Ax+By+Cz = D
	};

	//最终渲染图元
	struct t_primitive
	{
		t_primitive();
		//在context的CVertexBuffer中顶点起始位置
		//primitive type
		int type;//only triangle！
		//在vb中起始位置
		int vb_start;
		//在ib中的起始位置，ib_start,ib_start+1，ib_start+2 三个位置决定了三角形3个顶点索引
		int ib_start;
		//顶点数量
		int numvert,numpolygon;
		//位于哪个vb
		CVertexBuffer* vb;
		//
		vector<unsigned int>* ib;
		//当前primitive所用的texture
		t_material* material;
	};
	enum//光栅器运行状态
	{
		RAST_SURFACE_LOCKED=1<<0,
		RAST_AFFINE=1<<1,
		CONTEXT_GEOM_BACKFACE_CULLING=1<<2,//camera space中的几何剔除
		RAST_BACKFACE_CULLING=1<<3,//光栅化时的背面剔除
	};

	sfINLINE void MatrixRotVector(const t_matrix& matrix, const vec3& vec,vec3& result)
	{
		result.x = matrix[0].x * vec.x +matrix[0].y * vec.y +matrix[0].z * vec.z ;
		result.y = matrix[1].x * vec.x +matrix[1].y * vec.y +matrix[1].z * vec.z ;
		result.z = matrix[2].x * vec.x +matrix[2].y * vec.y +matrix[2].z * vec.z ;
	};

}
using namespace SOFTCORE;
extern void getProjectMatrix(t_matrix& m,float fovx,float ratio,float f,float n);
#include "mmgr/mmgr.h"
#endif