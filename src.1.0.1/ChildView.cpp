// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "softcore.h"
#include "ChildView.h"
#include ".\childview.h"
#include "softrender.h"
#include "softrender_def.h"
#include "SceneLevel.h"
#include "mmgr/mmgr.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib,"d3d9")
// CChildView

CChildView::CChildView()
{
	m_render=0;
	m_scene =0;
	m_flags = 0;
	m_yaw=  0.f;
	m_pitch = 0.f;
}

CChildView::~CChildView()
{
	if (m_scene)
		delete m_scene;
	CSoftrender::destroy();
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTON_REFRESH, OnButtonRefresh)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESH, OnUpdateButtonRefresh)
	ON_COMMAND(ID_BUTTON_ANIM, OnViewAnim)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ANIM, OnUpdateViewAnim)

	ON_WM_SIZE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_BUTTON_AFFINE_TEX, OnButtonAffineTex)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_AFFINE_TEX, OnUpdateButtonAffineTex)
	ON_COMMAND(ID_FILE_OPEN3DS, OnFileOpen3ds)
	ON_COMMAND(ID_VIEW_BACKFACECULLING, OnViewBackfaceculling)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BACKFACECULLING, OnUpdateViewBackfaceculling)
	ON_COMMAND(ID_VIEW_GEOM_CW, OnViewGeomCw)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GEOM_CW, OnUpdateViewGeomCw)
	ON_COMMAND(ID_VIEW_GEOM_CCW, OnViewGeomCcw)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GEOM_CCW, OnUpdateViewGeomCcw)
	ON_COMMAND(ID_VIEW_GEOM_NOCULL, OnViewGeomNocull)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GEOM_NOCULL, OnUpdateViewGeomNocull)
	ON_COMMAND(ID_VIEW_GEOM_ALLCULL, OnViewGeomAllcull)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()



// CChildView 消息处理程序

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{

	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // 用于绘制的设备上下文
	
	//create d3d device
	static int _init=0;
	if (!_init)
	{
		_init = 1;

		InitSoftcore();
	}
}
int BUF_WIDTH =400;
int BUF_HEIGHT = 300;

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	lpCreateStruct->cx = BUF_WIDTH;
	lpCreateStruct->cy = BUF_HEIGHT+10;
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;



	return 0;
}
void CChildView::OnInitialUpdate()
{


}
void CChildView::processInput()
{
	const float scale = .5f;

	t_camera & cam = m_render->getCamera();
	vec3 pos = cam.pos;
	vec3 lookat = cam.at;
	int cam_moved=0;

	if (m_flags&VIEW_MOVE_FORWARD)
	{
		cam_moved=1;
		pos = cam.pos + cam.dir * scale;
		lookat = pos + cam.dir;
	}
	if (m_flags&VIEW_MOVE_BACKWARD)
	{
		cam_moved=1;
		pos = cam.pos - cam.dir * scale;
		lookat = pos + cam.dir;
	}
	if (m_flags&VIEW_MOVE_RIGHT)
	{
		cam_moved=1;
		pos = cam.pos + cam.dir_right * scale;
		lookat = cam.at + cam.dir_right* scale;
	}
	if (m_flags&VIEW_MOVE_LEFT)
	{
		cam_moved=1;
		pos = cam.pos -cam.dir_right * scale;
		lookat = cam.at - cam.dir_right * scale;
	}

	if (cam_moved)
	{


		vec3 up(0,1,0,0);
		m_render->setCamera(pos,lookat,up);
	}

}
extern float g_z ;
int CChildView::RunFrame()
{
	if (!m_render || m_render->getState()!=R_OK)
		return -1;
	static float rot=0.f;

	int brot =m_flags&VIEW_ANIM;;
	m_render->frameBegin();

	for (int i=0;m_scene&&i<m_scene->getEntityCount();++i)
	{
		
		t_entity* entity= m_scene->getEntity(i);
		vec3 a (entity->m_aabb[0],entity->m_aabb[1],entity->m_aabb[2],0);
		vec3 b (entity->m_aabb[3],entity->m_aabb[4],entity->m_aabb[5],0);
		vec3 mid = (a+b)* -0.5f;

		if (brot)
		{
		//	rot=  m_render->getFrameInterval()/1000.f;
			rot=0.03f;
			float off=3.14f;
			float z=0;
			idQuat quat(0,sinf(rot+off),0,cosf(rot+off));
			entity->transform(quat,mid);
			mid = mid* -1.f;
			quat.Set(0,0,0,1);
			entity->transform(quat,mid);

		}
		
		m_render->pushEntity(entity);
	}

	//process user input
	processInput();

	//往渲染管道中增加一个光源
	m_render->addLight(m_scene->getLight(0));


	//ok,now start rendering
	m_render->clearScreen(0,0,60,200,255,1.f,0);
	m_render->renderFrame();

	m_render->printFPS();
	m_render->swap();
	m_render->frameEnd();

	return 0;
}
int CChildView::InitSoftcore()
{
	int res=0;
	m_render=CSoftrender::instance();
	if (!m_render)
	{
		MessageBoxA("softcore init failed!");
		return 1;
	}
	m_render->initSystem(this->GetSafeHwnd(),BUF_WIDTH,BUF_HEIGHT);

	//load a level
	m_scene = new CScene;
	m_scene->load("../textures/2.3ds");
	vec4 col(0,0,1,0);
	m_render->setGlobalColor(col);


	m_render->setFrustum(60.f,(float)BUF_WIDTH/(float)BUF_HEIGHT,1.f,1001.f);
	return 0;
}
void CChildView::OnButtonRefresh()
{
	if (!((CsoftcoreApp*)AfxGetApp())->getIdleState())
		((CsoftcoreApp*)AfxGetApp())->setIdleState(true);
	else
		((CsoftcoreApp*)AfxGetApp())->setIdleState(false);
}

void CChildView::OnUpdateButtonRefresh(CCmdUI *pCmdUI)
{
	if (!((CsoftcoreApp*)AfxGetApp())->getIdleState())
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	//should resize d3d's buffer
	CSoftrender::instance()->releaseSwapChain();
	CSoftrender::instance()->createSwapWindowBuffer((DWORD)cx,(DWORD)cy);

	OutputDebugStringA("re-create buffer\n");
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{


	CWnd::OnRButtonDown(nFlags, point);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_flags|=VIEW_LBUTTON_DOWN;
	m_click_pos = point;

	CWnd::OnLButtonDown(nFlags, point);
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{

	m_flags&=~VIEW_LBUTTON_DOWN;
	CWnd::OnLButtonUp(nFlags, point);

}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_flags&VIEW_LBUTTON_DOWN)
	{
		const float scale = 0.01f;

		float offx = (float)(point.x - m_click_pos.x) * scale;
		float offy = (float)(point.y - m_click_pos.y) * scale;

		m_click_pos = point;

		m_yaw += offx/__PI;
		m_pitch+=offy/__PI;
		
		
		//首先计算镜头指向的位置，根据yaw和pitch角度，计算一个旋转矩阵
		float sinYaw = sinf(m_yaw*0.5f);
		float cosYaw = cosf(m_yaw*0.5f);
		idQuat qYaw(0,sinYaw,0,cosYaw);
		idQuat qPitch(sinf(m_pitch*0.5f),0,0,cosf(m_pitch*0.5f));
		idVec3 camdir(0,0,-1);
		camdir = (qPitch*qYaw )* camdir;
		vec4 dir(camdir.x,camdir.y,camdir.z,0);
		vec4 at = m_render->getCamera().pos + dir;
		vec4 up(0,1,0,0);
		m_render->setCamera(m_render->getCamera().pos ,at,up);
	}
	CWnd::OnMouseMove(nFlags, point);
}

void CChildView::OnViewAnim()
{
	if (m_flags&VIEW_ANIM)
		m_flags&=~VIEW_ANIM;
	else
		m_flags|=VIEW_ANIM;
}

void CChildView::OnUpdateViewAnim(CCmdUI *pCmdUI)
{
	if (m_flags&VIEW_ANIM)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CChildView::OnButtonAffineTex()
{
	if (m_flags&VIEW_AFFINE)
	{
		m_flags&=~VIEW_AFFINE;
	}
	else
	{
		m_flags|=VIEW_AFFINE;
	}
	
	CSoftrender::instance()->setRasterizierAffine(m_flags&VIEW_AFFINE);
}

void CChildView::OnUpdateButtonAffineTex(CCmdUI *pCmdUI)
{
	if (m_flags&VIEW_AFFINE)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CChildView::OnFileOpen3ds()
{
//open a 3ds file
	char buf[256];buf[255]=0;
	GetCurrentDirectoryA(255,buf);
	CFileDialog fdlg(TRUE,".3ds","",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"3D Studio Files (*.3ds)|*.3ds||");
	if (IDOK!=fdlg.DoModal())
		return ;

	SetCurrentDirectoryA(buf);
	
	CString path = fdlg.GetPathName();
	//import to Scene obj
	m_scene->clean();
	if (m_scene->load(path))
	{
		myassert(0,"调入场景失败");
	}
}

void CChildView::OnViewBackfaceculling()
{
	int flags =CSoftrender::instance()->getRasterizierFlags();
	if (flags&CONTEXT_GEOM_BACKFACE_CULLING)
	{
		flags&=~CONTEXT_GEOM_BACKFACE_CULLING;
	}
	else
	{
		flags|=CONTEXT_GEOM_BACKFACE_CULLING;

	}
	CSoftrender::instance()->setRasterizierFlags(flags);
}

void CChildView::OnUpdateViewBackfaceculling(CCmdUI *pCmdUI)
{
	int flags =CSoftrender::instance()->getRasterizierFlags();
	if (flags&CONTEXT_GEOM_BACKFACE_CULLING)
	{
		pCmdUI->SetCheck(1);
	}
	else
	{
		pCmdUI->SetCheck(0);

	}
}

void CChildView::OnViewGeomCw()
{
	// TODO: Add your command handler code here
}

void CChildView::OnUpdateViewGeomCw(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CChildView::OnViewGeomCcw()
{
	// TODO: Add your command handler code here
}

void CChildView::OnUpdateViewGeomCcw(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CChildView::OnViewGeomNocull()
{
	CSoftrender::instance()->setCulling(0);
}

void CChildView::OnUpdateViewGeomNocull(CCmdUI *pCmdUI)
{
	if (CSoftrender::instance()->getCulling()==0)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}


void CChildView::OnViewGeomAllcull()
{
	// TODO: Add your command handler code here
}

void CChildView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

	switch(nChar)
	{
	case 0x57://W
		m_flags|=VIEW_MOVE_FORWARD;
		break;
	case 0x53://S
		m_flags|=VIEW_MOVE_BACKWARD;
		break;
	case 0x41://A
		m_flags|=VIEW_MOVE_LEFT;
		break;
	case 0x44://D
		m_flags|=VIEW_MOVE_RIGHT;
		break;

	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CChildView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch(nChar)
	{
	case 0x57://W
		m_flags&=~VIEW_MOVE_FORWARD;
		break;
	case 0x53://S
		m_flags&=~VIEW_MOVE_BACKWARD;
		break;
	case 0x41://A
		m_flags&=~VIEW_MOVE_LEFT;
		break;
	case 0x44://D
		m_flags&=~VIEW_MOVE_RIGHT;
		break;

	}
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnRButtonUp(nFlags, point);
}
