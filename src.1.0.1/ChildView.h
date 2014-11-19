// ChildView.h : CChildView 类的接口
//


#pragma once


// CChildView 窗口
class CSoftrender;
class CScene;
enum
{
	VIEW_ANIM = 1<<0,
	VIEW_FIT = 1<<1,
	VIEW_AFFINE=1<<2,
	VIEW_MOVE_FORWARD=1<<3,
	VIEW_MOVE_BACKWARD=1<<4,
	VIEW_MOVE_LEFT=1<<5,
	VIEW_MOVE_RIGHT=1<<6,
	VIEW_LBUTTON_DOWN=1<<7,
	
};
class CChildView : public CWnd
{
// 构造
public:
	CChildView();

// 属性
public:

// 操作
public:

// 重写
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	void OnInitialUpdate();
// 实现
public:
	virtual ~CChildView();
	int InitSoftcore();
	int RunFrame();
	// 生成的消息映射函数
protected:
	void processInput();
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

	CSoftrender* m_render;
	CScene* m_scene;
	int m_flags;
	float m_yaw,m_pitch;
	CPoint m_click_pos;
	
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonRefresh();
	afx_msg void OnUpdateButtonRefresh(CCmdUI *pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnViewAnim();
	afx_msg void OnUpdateViewAnim(CCmdUI *pCmdUI);
	afx_msg void OnButtonAffineTex();
	afx_msg void OnUpdateButtonAffineTex(CCmdUI *pCmdUI);
	afx_msg void OnFileOpen3ds();
	afx_msg void OnViewBackfaceculling();
	afx_msg void OnUpdateViewBackfaceculling(CCmdUI *pCmdUI);
	afx_msg void OnViewGeomCw();
	afx_msg void OnUpdateViewGeomCw(CCmdUI *pCmdUI);
	afx_msg void OnViewGeomCcw();
	afx_msg void OnUpdateViewGeomCcw(CCmdUI *pCmdUI);
	afx_msg void OnViewGeomNocull();
	afx_msg void OnUpdateViewGeomNocull(CCmdUI *pCmdUI);

	afx_msg void OnViewGeomAllcull();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};

