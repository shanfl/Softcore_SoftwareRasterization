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

//核心渲染功能类
#ifndef _SOFTRENDER
#define _SOFTRENDER
#include <d3d9.h>
#include "softrender_def.h"
#include "vertexBuffer.h"
#include <vector>

class CVertexBuffer;
using namespace std;
class CRenderContext;
class CSoftrender
{
public:
	static CSoftrender* instance();
	static void destroy();
	
public:
	int getState();
	int initSystem(HWND hWnd,int w,int h);
	int shutdownSystem();

	//realtime utility
	int clearScreen(int flags,int r,int g,int b,int a,float z,DWORD stencil);
	int renderFrame();
	int swap();
	int frameBegin();
	int frameEnd();

	//util
	int createContext(int w,int h);
	int releaseContext();
	CRenderContext* getContext(int n);
	void setContext(int n);
	int createPBuffer(CRenderContext*);
	int destroyPBuffer(CRenderContext*);
	int activeContext(int n);

	//transform pipeline,notice: use right-hand coordination
	//设置相机位置朝向和视锥平截体
	void setCamera(const vec3& eyepos,const vec3& lookat,const vec3& up);
	t_camera& getCamera() ;
	//镜头广角度，屏幕长宽比例，远平面距离，近平面距离
	void setFrustum(float fovx,float ratio,float n,float f);
	//每帧更新相机的矩阵
	void updateCamera();
	//viewport update
	void setViewport(const int* vp);
	void getViewport(int* vp);

	//geometry pipeline
	int pushTriangle(const t_triangle& tri);
	int pushEntity(t_entity* entity);
	//用于transform 顶点的结果存放,一个vertex format一个vb既可
	int setDynamicVB(CVertexBuffer* vb);
	//transform完毕后压入渲染管道，等待最后光栅化
	int pushDynmaicVB(CVertexBuffer* vb);
	//culling in world space use entity's aabb or bounding sphere 
	int frustumCulling();

	//add a light
	int setLight(int idx,vec4 pos,vec4 col);
	//add a light 
	void addLight(clight_directional*);
	//转换灯光
	int processLighting();
	//顶点转换，包括pos, normal, T,B, 把所有需要渲染的primitive压入context
	int processEntity();
	//begin rasterizer
	int rasterizer();
	

	//render state setup
	int setTexture();
	int setRenderState();
	int setTextureStage();
	//设置rgba的颜色数值,r,g,b,a的顺序
	int setGlobalColor(vec4);

	//primitive rendering
	int drawPrimitive();
	int drawTriangle();

	//resource manangement
	int createSwapWindowBuffer(DWORD w,DWORD h);
	int releaseSwapChain();
	CVertexBuffer* createVBuffer(int, VB_FMT);

	//draw utility
	//设置仿射转换插值
	void setRasterizierAffine(int );
	//get affine status
	int getRasterizierAffine();
	//set rasterizier flags
	void setRasterizierFlags(int flags);
	//get rasterizier flags
	int  getRasterizierFlags();

	void setCulling(int cull);
	int getCulling()const;
	int drawText(const char* str, int x,int y);
	int drawPoint(dpoint* p);
	int printFPS();
	unsigned int getFrameInterval(){return m_deltaTime;};
	unsigned int getFrameCount()const{return m_frm;};
private:
	CSoftrender();
	~CSoftrender();
protected:
	//util
	void checkEverything();

protected:
	IDirect3D9* m_d3d9;
	IDirect3DDevice9* m_device;
	IDirect3DSwapChain9* m_swapChain;
	IDirect3DSurface9* m_frmSurface,*m_depthSurf,*m_drawSurface;
	D3DPRESENT_PARAMETERS m_frmParam; 

	//当前渲染context
	CRenderContext* m_context;
	vector<CRenderContext*> m_context_mng;
	CVertexBuffer* m_dyn_vb;

	//渲染容器，每帧清空
	vector<t_entity*> m_entity;
	//临时变量
	t_matrix* m_mat_temp;
	//计数器
	unsigned int m_frm;
	//在一个渲染循环结束后，必须执行frameEnd()
	unsigned int m_frmBegin,m_frmEnd;
	//current valid light
	vector<clight_directional*> m_lights;
	t_camera m_camera;
	unsigned int  m_deltaTime;

};
#endif