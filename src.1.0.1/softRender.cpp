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

#include "stdafx.h"
#include "childview.h"
#include "softRender.h"
#include <assert.h>
#include "mmsystem.h"
#include "RenderContext.h"
#include "Rasterization.h"
#include "SceneLevel.h"
#include "bitmapWrapper.h"
#include "mmgr/mmgr.h"
#pragma comment(lib,"winmm")
static CSoftrender* _inst = 0;

void myassert(bool res,const char* s,bool forceExit)
{
	if (!res ) 
	{
		if (s)
			::MessageBoxA(NULL,s,"SOFTCORE",MB_OK);
		else
			::MessageBoxA(NULL,"异常错误","SOFTCORE",MB_OK);


		if (forceExit)
			ExitProcess(88);
	}
}
t_primitive::t_primitive()
{
	
	material=0;
	ib=0;
	vb=0;
	numvert=numpolygon=0;
}
t_matrix rigidInverse(t_matrix& mat)
{
	t_matrix matlight = mat;
	matlight[0].w = 0;
	matlight[1].w = 0;
	matlight[2].w = 0;
	t_matrix trans;
	trans.Identity();
	trans[0].w = -mat[0].w;
	trans[1].w = -mat[1].w;
	trans[2].w = -mat[2].w;
	return  matlight.Transpose()*trans;
}
t_entity::t_entity()
{
	m_flags=0;
	m_model_matrix.Identity();
	m_model_inv_matrix.Identity();
}
void t_entity::transform(const idQuat& rotation, const vec3& pos)
{
	idMat4 mat;
	mat.Identity();
	mat[0].w = pos.x;
	mat[1].w = pos.y;
	mat[2].w = pos.z;
	
	m_model_matrix = (rotation.ToMat4() * mat) * m_model_matrix;
	m_model_inv_matrix = rigidInverse(m_model_matrix);
}
CSoftrender*CSoftrender:: instance()
{

	if (!_inst)
		_inst = new CSoftrender;

	return _inst;
}
void CSoftrender::destroy()
{
	if (_inst)
		delete _inst;

	_inst = 0;
}
CSoftrender::CSoftrender()
{
	m_device = 0;
	m_swapChain=0;
	m_depthSurf = 0;
	m_frmSurface=0;
	m_drawSurface=0;
	m_context=0;
	m_frmEnd = m_frmBegin=m_frm=0;

}
CSoftrender::~CSoftrender()
{
	shutdownSystem();
}
int getBPP(D3DFORMAT fmt)
{
	switch(fmt)
	{
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
			return 32;
			break;
		case D3DFMT_R5G6B5:
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A4R4G4B4:
			return 16;
			break;
	}

	return 0;
}
int CSoftrender::initSystem(HWND hWnd,int width,int height)
{
	m_mat_temp = (t_matrix*)_aligned_malloc(sizeof(t_matrix),ALIGNED_BYTE);
	myassert(m_mat_temp!=0,"_aligned malloc异常！");
	int res = 0;
	// Create the D3D object.
	if( NULL == ( m_d3d9 = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	// Set up the structure used to create the D3DDevice. Since we are now
	// using more complex geometry, we will create a device with a zbuffer.
	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Create the D3DDevice
	if( FAILED( m_d3d9->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &m_device ) ) )
	{
		MessageBoxA(hWnd,"Can't Create Device","",MB_OK);
		return E_FAIL;
	}

	DWORD w=0,h=0;
	createSwapWindowBuffer(w,h);

	// Turn on the zbuffer
	m_device->SetRenderState( D3DRS_ZENABLE, TRUE );

	//create primary context
	createContext(width,height);
	activeContext(0);
	idMath::Init(  );
	//init camera
	vec3 eye(0,8,6,1);
	vec3 at(0,8,-1,1);
	vec3 up(0,1,0,0);
	setCamera(eye,at,up);
	setFrustum(60.f,(float)width/(float)height,1.f,1001.f);
	return res;
}
int CSoftrender::activeContext(int n)
{
	CRenderContext* context = getContext(n);
	if (!context)
	{
		myassert(0,"无法激活context! context不存在！");
		return -1;
	}

	m_drawSurface = (IDirect3DSurface9*)context->getPBuffer();
	m_context = context;
	CRasterizer::instance().setContext(m_context);
	return 0;
}
int CSoftrender::createSwapWindowBuffer(DWORD w,DWORD h)
{
	if (!m_device)
		return -1;
	int res = 0;
	D3DPRESENT_PARAMETERS d3dpp; 
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;


	//创建SWAP CHAIN是为了RESIZE窗口时不用重新创建DEVICE，浪费了一个SWAPCHAIN
	m_swapChain=0;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.BackBufferCount=1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	HRESULT nres = m_device->CreateAdditionalSwapChain(&d3dpp,&m_swapChain);
	if (nres!=S_OK)
	{
		MessageBoxA(NULL,"Can't create swap chain buffer","",MB_OK);
		return -1;
	}
	m_swapChain->GetPresentParameters(&d3dpp);
	m_frmParam = d3dpp;
	if (S_OK!=m_swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &m_frmSurface))
	{
		MessageBoxA(NULL,"Can't get frm col buf","",MB_OK);
		return -1;
	}
	m_device->SetRenderTarget(0,m_frmSurface);
	//do need m_device->SetDepthStencilSurface
	//when use additional swap chain, need create additional depthstencil surface.
	//for  easy to destroy when window resizing
	if (S_OK!=m_device->CreateDepthStencilSurface(d3dpp.BackBufferWidth,d3dpp.BackBufferHeight,D3DFMT_D16,D3DMULTISAMPLE_NONE, 0, FALSE, &m_depthSurf, 0))
	{		
		MessageBoxA(NULL,"Can't create additional depth buffer","",MB_OK);
		return -1;

	}
	if (S_OK!=m_device->SetDepthStencilSurface(m_depthSurf))
	{
		MessageBoxA(NULL,"Can't create set additional depth buffer to current target","",MB_OK);
		return -1;
	}


	m_frmSurface->Release();


	return res;
}

int CSoftrender::createPBuffer(CRenderContext* context)
{
	if (context->getPBuffer())
	{
		myassert(0,"context已经有surface了");
	}
	IDirect3DSurface9* drawSurface=0;
	const t_contextinfo& m_context_param = *context->getParam();
	if (S_OK!=m_device->CreateOffscreenPlainSurface(
		m_context_param.width,
		m_context_param.height,
		m_context_param.frminfo.BackBufferFormat,
		D3DPOOL_DEFAULT,
		&drawSurface,NULL) )
	{
		MessageBoxA(NULL,"Can't Create Drawing Surface","",MB_OK);
		return E_FAIL;
	}

	//create successfully
	context->setPBuffer((void*)drawSurface);
	return 0;
}
int CSoftrender::destroyPBuffer(CRenderContext* context)
{
	if (context->getPBuffer())
	{
		UINT cnt = ((IDirect3DSurface9*)context->getPBuffer())->Release();
		myassert(!cnt,"无法卸载pbuffer!");
	}
	context->setPBuffer(0);

	return 0;
}
CVertexBuffer*  CSoftrender::createVBuffer(int num, VB_FMT fmt)
{
	CVertexBuffer* buf = new CVertexBuffer(num,fmt);
	return buf;
}
int CSoftrender::releaseSwapChain()
{
	if (!m_device)
		return -1;
	int res = 0;

	if (m_depthSurf)
	{
		UINT cnt = m_depthSurf->Release();
		assert(!cnt);
	}

	if (m_swapChain)
	{
		UINT cnt = m_swapChain->Release();
		assert(!cnt);
	}

	return res;
}
int CSoftrender::releaseContext()
{
	for (size_t i=0;i<m_context_mng.size();++i)
		delete m_context_mng[i];

	return 0;
}
int CSoftrender::shutdownSystem()
{
	int res = 0;
	if (CScene::s_default_img)
	{
		delete CScene::s_default_img;
		CScene::s_default_img=0;
	}

	releaseContext();
	releaseSwapChain();

	if( m_device != NULL )
		m_device->Release();

	if( m_d3d9 != NULL )
		m_d3d9->Release();

	_aligned_free(m_mat_temp);
	return res;
}
//这里使用了显卡d3d driver提供的fillcolor 函数，用于加速清屏
//使用ddraw或者其他access directly buffer时，需要按象素填充
int CSoftrender::clearScreen(int flags,int r,int g,int b,int a,float z,DWORD stencil)
{
	if (!m_swapChain)
		return -1;

	//D3DLOCKED_RECT lockinfo;
	//memset(&lockinfo,0,sizeof(lockinfo));

	//IDirect3DSurface9* surf = m_drawSurface;

	//HRESULT res = surf->LockRect(&lockinfo,NULL,D3DLOCK_DISCARD);
	//if (res!=S_OK)
	//{
	//	MessageBoxA(NULL,"Can't lock col buf","",MB_OK);
	//	surf->Release();
	//	return -3;
	//}

	//DWORD col =0;
	//switch(m_frmParam.BackBufferFormat)
	//{
	//case D3DFMT_X8R8G8B8:
	//case D3DFMT_A8R8G8B8:
	//	col =D3DCOLOR_ARGB(a,r,g,b);
	//	break;
	//case D3DFMT_R5G6B5:
	//	col = ( (((r&0xff)>>3) << 11) |(((g&0xff)>>2) << 5) | ((b&0xff)>>3) );
	//	break;
	//case D3DFMT_X4R4G4B4:
	//case D3DFMT_A4R4G4B4:
	//	col = ( (((a&0xff)>>4) << 12) | (((r&0xff)>>4) << 8) |(((g&0xff)>>4) << 4) | ((b&0xff)>>4) );
	//	break;
	//}
	//int colbit = lockinfo.Pitch/m_frmParam.BackBufferWidth;
	////pitch = bytes in one row, size= pitch* number of row
	//int size = lockinfo.Pitch*m_frmParam.BackBufferHeight ;

	//res  = surf->UnlockRect();

	if (!m_drawSurface)
	{
		myassert(0,"context为空！");
		return -1;
	}

	HRESULT res =m_device->ColorFill(m_drawSurface,0,D3DCOLOR_ARGB(a,r,g,b));
	if (res!=S_OK)
	{
		myassert(0,"无法fill color");
	}
	
	/*使用Z BUFFER FILL优化，可以减少这一步。
	当前frame count为奇数时 z buffer数值为0－1， 当前frame count为偶数时，zbuffer 数值范围为 0,-1
	*/
	float* zbuf = m_context->getZBuffer();
	for (int i=0;i<m_context->getParam()->width*m_context->getParam()->height;++i,++zbuf)
		*zbuf =0;
	return 0;
}
int CSoftrender::drawText(const char* str, int x,int y)
{
	if (!m_drawSurface)
		return -1;

	HDC hdc ;
	HRESULT res = m_drawSurface->GetDC(&hdc);
	if (res!=S_OK)
	{
		MessageBoxA(NULL,"Can't unlock col buf","",MB_OK);
		return -4;
	}
	RECT rect;
	rect.left = x;
	rect.right =(LONG)(strlen(str) * 10 +x);
	rect.top = y;
	rect.bottom = y+20;
	int len = (int)strlen(str);
	DrawTextA(hdc,str,len,&rect,DT_CENTER);
	m_drawSurface->ReleaseDC(hdc);

	return 0;
}
int CSoftrender::createContext(int w,int h)
{
	if (!m_device)
		return -1;
	t_contextinfo info;
	info.bpp = getBPP(m_frmParam.BackBufferFormat);
	myassert(info.bpp!=0,"非法象素颜色格式！异常退出",true);
	info.width = w;
	info.height = h;
	info.frminfo = m_frmParam;
	info.win_width = m_frmParam.BackBufferWidth;
	info.win_height = m_frmParam.BackBufferHeight;
	info.device = this;
	CRenderContext* context = new CRenderContext(info);
	context->create();
	m_context_mng.push_back(context);
	return (int)m_context_mng.size()-1;
}
void CSoftrender::setContext(int n)
{
	if (n<(int)m_context_mng.size())
	{
		m_context = m_context_mng[n];
		CRasterizer::instance().setContext(m_context );
	}
}
CRenderContext* CSoftrender::getContext(int n)
{
	if (n<(int)m_context_mng.size())
		return m_context_mng[n];

	return 0;
}
void CSoftrender::setViewport(const int* vp)
{
	m_context->setViewport(vp);
}
void CSoftrender::getViewport(int* vp)
{
	float* view = m_context->getViewport();
	vp[0] = (int)view[0];
	vp[1] = (int)view[1];
	vp[2] = (int)view[2];
	vp[3] = (int)view[3];
}
int CSoftrender::setGlobalColor(vec4 col)
{
	sfByte8 c[4]={col[3]*255, col[0]*255,col[1]*255,col[2]*255};
	m_context->setGlobalColor(c);
	return 0;
}
int CSoftrender::pushEntity( t_entity* entity)
{
	if (!entity)
		return -1;

	m_entity.push_back(entity);
	return 0;
}
int CSoftrender::setDynamicVB(CVertexBuffer* vb)
{
	m_dyn_vb = vb;
	return 0;
}
void CSoftrender::addLight(clight_directional* light)
{
	if (light)
		m_lights.push_back(light);
}
int CSoftrender::renderFrame()
{
	if (!m_swapChain)
		return -1;

	updateCamera();
	//
	//backfaceCulling();
	//
	frustumCulling();
	//
	processLighting();
	//
	processEntity();

	//
	rasterizer();


	return 0;
}

//clip triangle
	//把near plane转换为entity local space，然后切割mesh中的三角形，
	//新生成的顶点和triangle，放入一个临时变量，当进行transform时一起处理，
	//在near plane之后的三角形索引一律切掉（索引设置成0，0，0），
	//切除的顶点在transform时不进行转换处理，使用一个数组记录不需要转换顶点的索引
	//


int CSoftrender::processEntity()
{
	checkEverything();
	for (size_t i=0;i<m_entity.size();++i)
	{		
		t_entity* ent = m_entity[i];

		for (int j=0;j<m_entity[i]->getMeshCount();++j)
		{
			t_mesh* mesh = m_entity[i]->getMesh(j);
			
			m_context->addPrimitiveList(ent,mesh,&m_camera);
		}
	}
	return 0;
}

int CSoftrender::getState()
{
	int state = 0;
	if (!m_context)
		return R_NO_CONTEXT;

	return R_OK;
}
int CSoftrender::frameBegin()
{
	if (!m_context)
		return -1;
	m_frmEnd=0;
	m_frmBegin=1;
	m_context->cleanInternalVB();
	m_entity.clear();
	m_lights.clear();
	m_frm ++;
	return 0;
}
int CSoftrender::frameEnd()
{

	m_frmEnd=1;
	m_frmBegin=0;
	return 0;
}
t_camera& CSoftrender::getCamera() 
{
	return m_camera;
}
void CSoftrender::setCamera(const vec3& eyepos,const vec3& lookat,const vec3& up)
{
	m_camera.pos = eyepos;
	m_camera.at = lookat;
	m_camera.dir = lookat - eyepos;
	m_camera.dir.Normalize();
	/*从眼睛位置，看向目标的位置，以及世界坐标朝上的方向，得到world matrix
	现在假设相机在世界坐标中眼睛的位置，沿相机镜头指向的方向，架设一根轴，穿过相机。这是相机坐标系的Z轴
	相机快门一般在相机正上方，沿着按动快门的方向架设一根轴。这是相机坐标系的Y轴。这样横穿相机机身左右方向的轴，就是X轴
	求出camera本地坐标系在世界坐标系的三根轴X,Y,Z，就可以得到把顶点从世界坐标系转换到视角坐标系（也叫相机坐标系）的转换矩阵。
	现在，穿过镜头的轴已经知道，就是eyepos - lookat 。这样结合世界坐标的up向量，就可以用差乘得到另外两根轴。
	
	这三根轴组成一个坐标系，既view space（视角坐标系）
	*/

	//取负数是因为当view matrix为单位阵时，缺省相机朝向是view space的负z轴方向
	idVec3 Z (-m_camera.dir.x,-m_camera.dir.y,-m_camera.dir.z);
	Z.Normalize();
	idVec3 upDir (up.x,up.y,up.z);
	idVec3 X= upDir.Cross(Z);
	X.Normalize();
	m_camera.dir_right.Set( X.x,X.y,X.z,0);
	idVec3 Y = Z.Cross(X);
	/*
	利用在世界坐标的三根正交轴，可以得到一个矩阵T，它的含义是当世界坐标系中三根轴的向量X, Y, Z ，经过T
	的转换后，可以成为(1,0,0),(0,1,0),(0,0,1) . 打个比方，空间有3根轴，是X,Y,Z。 当我们的眼睛移动到eyepos，并且脑袋的角度
	按照相机的角度来观察这3根轴，3根轴在眼睛里就变成了(1,0,0),(0,1,0),(0,0,1)
	如何求出T，可以用下面的方法
	首先，我们找出把世界坐标系的三根坐标轴 (1,0,0),(0,1,0),(0,0,1)，转换为X,Y,Z 的矩阵。 这个矩阵很好求，直接把三根轴代入矩阵的3列
	X.x,Y.x,Z.x 0 
	X.y,Y.y,Z.y 0  =  T1
	X.z,Y.z,Z.z 0
    0   0   0   1

	可以看到
	X.x,Y.x,Z.x 0     1        X.x
	X.y,Y.y,Z.y 0  × 0   ＝   X.y
	X.z,Y.z,Z.z 0     0        X.z
	0   0   0   1     1        1

	然后把眼睛从世界原点移动到相机位置
	X.x,Y.x,Z.x eyepos.x 
	X.y,Y.y,Z.y eyepos.y  =  T2
	X.z,Y.z,Z.z eyepos.z
	0  0  0  1

	这样我们得到一个矩阵，可以把世界坐标轴转换为相机坐标轴，而我们需要的T，
	实际是把世界坐标系中的相机把相机移动回原点，然后把相机三根轴，转为(1,0,0),(0,1,0),(0,0,1)。
	这个操作实际是把刚才的步骤反过来，先移动回相机，再旋转三根轴到(1,0,0),(0,1,0),(0,0,1)

	由线性代数可知这个转换就是T2的逆 ，由此T＝T2的逆
	T = (T2)inverse

	这里假设T2是刚体阵，所以求逆很简单，先平移，再乘以T1的转置

	*/

	
	t_matrix Mt ;
	Mt.Identity();
	Mt[0].w = -eyepos.x;
	Mt[1].w = -eyepos.y;
	Mt[2].w = -eyepos.z;


	t_matrix Mr;//Mr = T1.inverse
	Mr.Identity();
	Mr[0].x = X.x;Mr[1].x = Y.x;Mr[2].x = Z.x;
	Mr[0].y = X.y;Mr[1].y = Y.y;Mr[2].y = Z.y;
	Mr[0].z = X.z;Mr[1].z = Y.z;Mr[2].z = Z.z;

	//OPENGL 矩阵乘法为左乘
	m_camera.mat = Mr*Mt;
	m_camera.view_invert = m_camera.mat.InverseFast();
}
void getProjectMatrix(t_matrix& m,float fovx,float ratio,float n,float f)
{
	float r,l,t,b;
	r = tanf(fovx/180*__PI /2.f);
	l = -r;
	t = r/ratio;
	b= -t;

	m[0][0] = 2*n/(r-l);
	m[0][1] = 0;
	m[0][2] = (r+l)/(r-l);
	m[0][3] =0;

	m[1][0] = 0;
	m[1][1] = 2*n/(t-b);
	m[1][2] = (t+b)/(t-b);
	m[1][3] =0;

	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = -(f+n)/(f-n);
	m[2][3] =-2*f*n/(f-n);

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = -1;
	m[3][3] =0;
}
void CSoftrender::setFrustum(float fovx,float ratio,float n,float f)
{
	m_camera.fovx  = fovx;
	m_camera.ratio = ratio;
	m_camera.farplane = f;
	m_camera.near_dist = n;

	getProjectMatrix(m_camera.projmat,fovx,ratio,n,f);

}
void CSoftrender::updateCamera()
{
	if (!m_context)
		return;

	//setup frustum 
	m_context->setMatrix(MATRIX_VIEW,m_camera.mat);
	m_context->setMatrix(MATRIX_PROJ,m_camera.projmat);
}
int CSoftrender::frustumCulling()
{
	return 0;
}


void CSoftrender::checkEverything()
{
	if (m_frmEnd || !m_frmBegin)
	{
		myassert(0,"调用顺序错误，主循环必须调用frameBegin()之后才开始渲染，所有渲染工作完成后必须执行frameEnd()");
	}
}

int CSoftrender::processLighting()
{
	//light is in the world space
	//这里为了简便直接让所有物体收到所有光照，而省却light sphere culling过程
	for (size_t i=0;i<m_lights.size();++i)
	{
		for (size_t j=0;j<m_entity.size();++j)
		{
			//每一帧都更新
			m_entity[j]->lights.clear();
			m_entity[j]->lights.push_back(m_lights[i]);
		}
	}
	return 0;
}

int CSoftrender::rasterizer()
{
	m_context->pushTriangleListToRasterizer();
	return CRasterizer::instance().flush();
	
}
int CSoftrender::swap()
{
	IDirect3DSurface9* dstsurf=0;
	HRESULT res=S_OK;
	m_swapChain->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&dstsurf);
	res = m_device->StretchRect(m_drawSurface,0,dstsurf,0,D3DTEXF_NONE);
	dstsurf->Release();
	if (res!=S_OK)
	{
		MessageBoxA(NULL,"Can't copy to frame col buf","",MB_OK);
	}
	m_swapChain->Present(NULL,NULL,NULL,NULL,NULL);
	return 0;
}

int CSoftrender::printFPS()
{
	static DWORD t = timeGetTime();
	static DWORD frm = 0;
	++frm;

	static char strFPS[256]={"FPS:"};
	const int maxFrm=100;
	DWORD curTime = timeGetTime();
	DWORD deltaT =(curTime-t);
	static DWORD lastTime = curTime;
	m_deltaTime = curTime-lastTime;
	lastTime =  curTime;
	if (deltaT > 500)
	{
		float fps = frm/( deltaT/1000.f);
		frm=0;
		t = timeGetTime();
		sprintf(strFPS,"FPS:%5.1f",fps);
	}

	drawText(strFPS,100,10);
	return 0;
}
int CSoftrender::drawPoint(dpoint* p)
{
	int res = 0;
	return res;
}

void  CSoftrender::setRasterizierFlags(int flags)
{
	CRasterizer::instance().setFlags(flags);
}
int  CSoftrender::getRasterizierFlags()
{
	return CRasterizer::instance().getFlags();
}
void CSoftrender::setRasterizierAffine(int affine)
{
	if (!affine)
	{
		CRasterizer::instance().setFlags(
			CRasterizer::instance().getFlags()&~RAST_AFFINE);
	}
	else
	{
		CRasterizer::instance().setFlags(
			CRasterizer::instance().getFlags()|RAST_AFFINE);
	}
}
int CSoftrender::getRasterizierAffine()
{
	return CRasterizer::instance().getFlags()&RAST_AFFINE;
}

void CSoftrender::setCulling(int cull)
{	
	if (m_context)
		m_context->setBackfaceCulling(cull);

}
int CSoftrender::getCulling()const
{
	if (m_context)
		return m_context->getBackfaceCulling();

	return 0;
}