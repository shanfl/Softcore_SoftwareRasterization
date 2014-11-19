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
//光栅化模块

#ifndef _RASTERIZATION
#define _RASTERIZATION

#include "softrender_def.h"
class CRenderContext;

//光栅器屏幕像素属性
class t_rasterizier_point 
{
public:
	//这里缺省了构造和拷贝构造，直接按bit copy
	//t_rasterizier_point ();
	//t_rasterizier_point (t_rasterizier_point& );

	//屏幕像素垂直差值
	sfINLINE void interpolateVerticalCol();//gouraud shading without texture
	sfINLINE void interpolateVerticalTex();//gouraud shading with texture
	sfINLINE void interpolateVerticalColTex();//gouraud shading with texture
	sfINLINE void interpolateVerticalTBNTex();//phong shading with texture

	//vertex point
	vec4 pl,pr,dpl,dpr;
	//tagent space
	vec3 T,B,N;
	//vertex color, 这里是已经/w 的数值
	vec4 cl,cr,dcl,dcr;
	vec4 tl,tr,dtl,dtr;
	//perspective correction texture mapping & project texture,也已经是 /w 过的数值

	//这里省略多重贴图
	//	vec4 tl[MAX_STAGE],tr[MAX_STAGE],dtl[MAX_STAGE],dtr[MAX_STAGE];
};

/*////////////////////////////////////////////////////////////////////////
 采用oo方法应该抽象行为变化，把绘制
tex triangle, 
color triangle, 
tex+col triangle, 
phong triangle, 
bump phong triangle
等行为抽象为不同的类
但因为光栅化完全以速度优先，
实用扫描线函数都应使用SIMD汇编代码实现
此处光栅器为了实现简化，使用不同独立函数实现不同shading时的扫描线算法
//////////////////////////////////////////////////////////////////////////*/
class CRasterizer
{
	CRasterizer();
	virtual ~CRasterizer();
	CRenderContext* m_context;
	
public:
	//内部buffer
	//TODO,每种vb format各自拥有一个buffer
	struct t_rast_triangle
	{
		//已经被切割过的3顶点
		vec4 point[3];
		vec4 col[3];
		vec2 tex[3];
		vec3 norm[3];
		vec3 T[3],B[3];
		const t_primitive* pri;		
	};



	//TODO: 使用以下格式使数据128bit对齐，使得插值计算可以由SIMD完成
	/*
	struct t_rast_triangle
	{
	//已经被切割过的3顶点
	vec3 point[3];
	vec3 norm[3];
	vec2 uv[3];
	vec4 col[3];
	}
	t_rast_triangle* m_tirs = (t_rast_triangle*)aligned_alloc(sizeof( t_rast_triangle)* num, 16);
	t_primitive* m_primitives = new t_primitive[num];
	*/
	static CRasterizer& instance();

	//设置当前context
	void setContext(CRenderContext* );

	//当外部变化context viewport时，必须更新rasterizier的viewport
	void updateViewport();

	//生成最后渲染的triangle list，把普通三角形切割成下平底三角形和上平顶三角形
	int pushTriangle(const t_primitive&);

	//!光栅化三角形
	int drawTriangle_colTex(vector<t_rast_triangle>& ,bool );


	/*
	screen space中需要垂直切割的三角形由 _xclip 函数绘制
	为了简化，只是按要插值顶点属性的多少操作划分绘制函数，这样不用在每一行
	判断哪些顶点属性需要插值，减少动态分支
	利用多态或函数指针亦可
	*/

	//!绘制单色三角形，
	template<class T>  void rastTri_constCol(int iy0,int iy1,sfByte8* buf,t_rasterizier_point attribute,T col);
	template<class T>  void rastTri_constCol_xclip(int iy0,int iy1,int minX,int maxX,sfByte8* buf,t_rasterizier_point& attribute,T col);
	//!diffuse lighting+texture triangle
	template<class T>  void rastTri_constColTex(int iy0,int iy1,sfByte8* buf,ZBUF_TYPE* zbuf,
		t_rasterizier_point attribute,T col,t_material* material);
	template<class T>  void rastTri_constColTex_xclip(int iy0,int iy1,int minX,int maxX,sfByte8* buf,ZBUF_TYPE* zbuf,
		t_rasterizier_point& attribute,T col,t_material* material);


	//!绘制固定颜色扫描线
	template<class T> sfINLINE  int scanLine_constCol_Affine(T* buf,int xleft,int xright,T col,t_rasterizier_point& p);
	//!affine linear color filling
	template<class T> sfINLINE  int  scanLine_constCol(T* buf,int xleft,int xright,T col,t_rasterizier_point& p);


	//!绘制光照+贴图扫描线
	template<class T>  sfINLINE int scanLine_ColTex(T* buf,ZBUF_TYPE* zbuf,int xleft,int xright,T col,t_rasterizier_point& p,t_material* material);
	//!affine texture mapping
	template<class T>  sfINLINE int scanLine_ColTex_Affine(T* buf,int xleft,int xright,T col,t_rasterizier_point& p,t_material* material);

	/*
	pixel shading operations
	这里只给出了diffuse texture* diffuse color
	和constant color的pixel operation
	*/
	template<class T> sfINLINE void pixel_shader_constCol(const vec4& col,T* c);
	//!支持16和32位色的全特化版本
	template<> sfINLINE void pixel_shader_constCol (const vec4& col,sfCol16* c);
	template<> sfINLINE void pixel_shader_constCol (const vec4& col,sfCol32* c);
	
	template<class T> sfINLINE T pixel_shader_modulateTexCol(const T col, const vec4& c);
	//!支持16和32位色的全特化版本
	template <> sfINLINE sfCol16 pixel_shader_modulateTexCol(const sfCol16 col, const vec4& c);
	template <> sfINLINE sfCol32 pixel_shader_modulateTexCol(const sfCol32 col, const vec4& c);


	//!2d texture sampler
	template<class T> sfINLINE void tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,sfByte8* texbuf,T& col,t_texture* tex);
	//!支持16和32位色的全特化版本
	template<> sfINLINE void tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,
		sfByte8* texbuf,sfCol16& col,t_texture* tex);
	template<> sfINLINE void tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,
		sfByte8* texbuf,sfCol32& col,t_texture* tex);

	//!绘制所有三角形，同时清空所有临时渲染内容
	int flush();
	//清除所有光栅器中的临时渲染内容
	void cleanAll();
	//
	int getFlags()const{return m_run_flags;};
	void setFlags(int flags){m_run_flags = flags;};
protected:

	//上平顶和下平底三角形，一种格式一个array,每针清除，和动态填充
	vector<t_rast_triangle> m_rast_triangles[2][VB_MAX_FMT];
	void* m_surfaceBuf;
	int m_run_flags;
	float m_epslon;
	//l,r,t,b
	float* m_viewport;
	int m_iviewport[4];
	int m_linebytes;
	//指向context z buffer的指针
	ZBUF_TYPE* m_zbuf;

};
#endif