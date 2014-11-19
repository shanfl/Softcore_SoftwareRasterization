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

//渲染上下文代表所有显卡要处理的内部数据和状态
//如vertex buffer, index buffer, vertex format, back buffer，matrix stack等
//render context可以相当于显卡功能模块
#ifndef _RENDER_CONTEXT
#define _RENDER_CONTEXT
#include "stdafx.h"
#include "d3d9.h"
#include "softrender_def.h"
class CSoftrender;
#include <stack>
#include <vector>
using namespace std;
struct t_contextinfo
{
	//显示分辨率。假如显示分辨率<实际窗口尺寸，则象素填充时，一个象素相当窗口中m_win_width/m_width 个大小
	int width,height;
	//实际windows尺寸
	int win_height,win_width;
	//颜色数值,bit per pixel
	int bpp;
	//windows info
	D3DPRESENT_PARAMETERS frminfo;
	//render device
	CSoftrender* device;
};
struct t_internal_renderop
{
	//不同VB格式的BUFFER
	CVertexBuffer* m_vb_internal[VB_MAX_FMT];
	//当前填充到哪里了
	int m_vb_fill_curpos[VB_MAX_FMT];
	
	//要绘制的图元
	vector<t_primitive> m_primitives;
	vector<unsigned int> m_indexbuffer;
};

class CSoftrender;

class CRenderContext
{
public :
	CRenderContext(const t_contextinfo& context_param);
	virtual ~CRenderContext();
	void create();
	const t_contextinfo* getParam()const {return &m_context_param;};
	//off screen RENDERING SURFACE
	//平台相关，抽象为虚函数。这里是用direct3d9 surface实现的渲染buffer
	//在其他平台可以直接使用DirectDraw，gdi, directly access buffer 机制
	virtual void setPBuffer(void * surface){m_drawSurface = (IDirect3DSurface9*)surface;};
	virtual void* getPBuffer(){return (void*)m_drawSurface;};
	//MATRIX FUNCTIONS
	void loadIdentity(MATRIX_MODE mode);
	void setMatrix(MATRIX_MODE mode,const t_matrix& mat);
	void setMatrixMode(MATRIX_MODE mode);
	void setActiveTexture(int unit);
	t_matrix& getModeMatrix(MATRIX_MODE);
	//重置context
	int reset();
	//设置projection matrix
	void setPerspective(float fovx,float ratio,float n,float f);
	

	//////////////////////////////////////////////////////////////////////////
	//往context内部BUFFER中填充primitive数据
	//////////////////////////////////////////////////////////////////////////
	void addPrimitiveList(t_entity* ent,t_mesh* mesh ,t_camera* cam);

	//重新改变内部VB大小
	void resizeRenderVB(int num,VB_FMT fmt);
	//清楚内部vb，这里的vb是经过转换的顶点坐标，结果是screen space的位置
	void cleanInternalVB();
	//得到内部vb
	char* getVBuffer(VB_FMT);
	//增加当前填充位置
	int appendFillPos(int num,VB_FMT);

	//转换entity 顶点到screen space，x,y是屏幕象素坐标，同时保留device space的z和w
	int tranformAndFillPos(t_entity* ent,int numVert,t_drawVert* vb,VB_FMT vb_format);
	int tranformAndFillPosNorm(t_entity* ent,int numVert,t_drawVert* vb,VB_FMT vb_format);
	//把所有triangle放入光栅器
	int pushTriangleListToRasterizer();
	//锁住surface开始写屏 ，平台相关
	virtual void* lockSurface(int& linewidth);
	virtual void unlockSurface();
	//得到surface 尺寸
	int getSurfaceWidth()const{return m_context_param.width;};
	int getSurfaceHeight()const{return m_context_param.height;};
	//得到viewport浮点数组指针,just for convenience
	float* getViewport(){return m_viewport;};
	void setViewport(const int * );

	//设置一个全局渲染颜色
	int setGlobalColor(sfByte8*);
	sfByte8* getGlobalColor(){return m_global_color;};
	ZBUF_TYPE* getZBuffer()const{return m_zbuffer;};
	void setBackfaceCulling(int cull){m_backfaceCulling=cull;};
	int getBackfaceCulling()const{return m_backfaceCulling;};
protected:
	
	//用户数据转换到渲染BUFFER，为了只进行一次内存copy，把back face culling, near plane triangle clipping，
	//和vertex transform 放入了一个函数。这样可以直接从用户的mesh vb数据一次性转换到context内部渲染VERTEX BUFFER中
	int backCullingAndClipAndTransform(t_entity* ent, t_mesh* mesh ,t_primitive* primitive,t_camera* cam);

	//context param
	t_contextinfo m_context_param;
	
	//offscreen buffer
	IDirect3DSurface9* m_drawSurface;

	//model matrix,view matrix, projection matrix, texture matrixs, all kinds of matrix stack, 
	vector<t_matrix> viewmatrix;
	vector<t_matrix> projmatrix;
	vector<t_matrix> texmatrix[MAX_STAGE];

	int iView,iProj,iTex[MAX_STAGE];
	MATRIX_MODE m_mat_mode;
	int m_unit;
	//view port, x,y, w,h
	float m_viewport[4];
	
	//global color
	sfByte8 m_global_color[4];
	//0, no culling, 1, CW culling, 2, CCW culling
	int m_backfaceCulling;

	//运行时的内部vb数据，所有转换后的多边形顶点最后都填入这个buffer
	//internal renderop也记录了当前填入buffer的位置，以及要绘制的primitive信息
	t_internal_renderop m_renderop;
	ZBUF_TYPE *m_zbuffer;

private:
	CRenderContext();
};


#endif