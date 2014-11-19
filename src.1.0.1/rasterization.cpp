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
#include "stdafx.h"
#include "Rasterization.h" 
#include "RenderContext.h"
#include "vertexBuffer.h"
#include "softRender.h"
#include "bitmapWrapper.h"
#include "mmgr/mmgr.h"
#define epslon_edge_width 0.1f
#define epslon_tc 0.01f
const vec4 RED_COLOR(0,0,1,0);
const vec4 GREEN_COLOR(0,1,0,0);
#define TEST_RAST 0
#define RAST_LAST 0
#define RAST_ADJUST 0
CRasterizer::CRasterizer()
{
	m_context=0;
	m_run_flags=0;
}
CRasterizer::~CRasterizer()
{

}
CRasterizer& CRasterizer::instance()
{
	static CRasterizer _inst;
	return _inst;
}
void CRasterizer::cleanAll()
{
	for (int i=0;i<2;++i)
	{
		for (int j=0;j<VB_MAX_FMT;++j)
		{
			m_rast_triangles[i][j].clear();
		}
	}
	
}
void CRasterizer::setContext(CRenderContext* context)
{
	m_context = context;
	m_epslon = .1f;
	m_viewport = context->getViewport();
	updateViewport();
	m_linebytes=0;
	myassert(m_surfaceBuf==0,"光栅器未渲染完毕就被重置");
	m_run_flags=0;
	
}
void CRasterizer::updateViewport()
{
	m_iviewport[0]=idMath::FtoiFast(m_viewport[0]);
	m_iviewport[1]=idMath::FtoiFast(m_viewport[1]);
	m_iviewport[2]=idMath::FtoiFast(m_viewport[2]);
	m_iviewport[3]=idMath::FtoiFast(m_viewport[3]);
}
inline void sortp_func(int* sortp,float *y)
{

	//sort screen_p from bottom to top...
	if (y[0]<y[1])
	{
		sortp[0] = 1;
		sortp[1] = 0;
	}

	if (y[*sortp]<y[*(sortp+2)])
	{
		int temp = sortp[0];
		sortp[0]=sortp[2];
		sortp[2] = temp;
	}

	if (y[*(sortp+1)]<y[*(sortp+2)]) 
	{
		int temp = sortp[1];
		sortp[1]=sortp[2];
		sortp[2] = temp;
	}

}
inline bool fcomp( float a, float b, float epslon)
{
	float c=  a-b;
	if (c>-epslon && c<epslon)
		return true;
	else
		return false;
}

bool check_outofrange(vec3* point,float* m_viewport,bool downTriangle)
{
	//viewport culling
	if (!downTriangle)
	{

	
	if (point[1].y>=m_viewport[2] || point[0].y<=m_viewport[3] ||
		(point[0].x>=m_viewport[1] && point[1].x>=m_viewport[1]) || 
		(point[0].x<=m_viewport[0] && point[2].x<=m_viewport[0]) )
		return false;
	}
	else
	{
		if (point[2].y>m_viewport[2] || point[0].y<m_viewport[3] ||
			(point[0].x>=m_viewport[1] && point[2].x>=m_viewport[1]) || 
			(point[1].x<=m_viewport[0] && point[2].x<=m_viewport[0]) )
			return false;
	}

	//检查奇异化
	//if (fcomp(point[0].y,point[2].y,epslon_edge_width) ||
	//	(
	//	fcomp(point[0].x,point[1].x,epslon_edge_width*0.5f) &&
	//	fcomp(point[0].x,point[2].x,epslon_edge_width*0.5f) 
	//	)
	//	)
	//	return false;

	if ( (point[0].z<-1.f &&point[1].z<-1.f &&point[2].z<-1.f) ||(point[0].z>1.f ||point[1].z>1.f ||point[2].z>1.f ))
		return false;
	return true;
}

int CRasterizer::pushTriangle(const t_primitive& pri)
{
	if (!m_context)
		return -1;

	vector<unsigned int>& ib = (*pri.ib);
	//
	float*p[3],*c[3],*t[3]; float y[3];
	vec4 point[3];vec4 col[3];vec2 tex[3];
	for (int i=pri.ib_start;i<pri.ib_start+pri.numpolygon*3;i+=3)
	{
#if RAST_LAST
		if (pri.numpolygon>1 )
			i = pri.ib_start +3*(pri.numpolygon-1) ;
#endif
			
		//draw a triangle
		unsigned int ib0=ib[i],ib1=ib[i+1],ib2=ib[i+2];

		//check if it's a edge
		if (ib0==ib1 || ib1==ib2 || ib0==ib2)
			continue;
		
		p[0] = (float*)(pri.vb->m_buffer + pri.vb->m_strip* ib0);
		c[0] = p[0]+VEC3FLOAT;
		p[1] = (float*)(pri.vb->m_buffer + pri.vb->m_strip* ib1);
		c[1] = p[1]+VEC3FLOAT;
		p[2] = (float*)(pri.vb->m_buffer + pri.vb->m_strip* ib2);
		c[2] = p[2]+VEC3FLOAT;


		//从上到下排序
		y[0]=p[0][1];
		y[1]=p[1][1];
		y[2]=p[2][1];
		int sort_p[3]={0,1,2};
		sortp_func(sort_p,y);
		//排序完成

		for (int k=0;k<3;++k)
		{
			int idx = sort_p[k];
			point[k].Set(p[idx][0],p[idx][1],p[idx][2],p[idx][3]);
			col[k].Set(c[idx][0],c[idx][1],c[idx][2],c[idx][3]);
		}
		//是否有纹理坐标
		if (pri.vb->m_format==VB_POS_TEX_NORM)
		{
			t[0] = c[0]+VEC4FLOAT;
			t[1] = c[1]+VEC4FLOAT;
			t[2] = c[2]+VEC4FLOAT;
			for (int k=0;k<3;++k)
			{
				int idx = sort_p[k];
				tex[k].Set(t[idx][0],t[idx][1],t[idx][2],t[idx][3]);
			}
		}


		//确立2个平底三角形，
		//push to list
		t_rast_triangle tri;
		tri.pri  = &pri;

		if (fcomp(point[2].y,point[1].y,m_epslon))
		{

			tri.point[0] = point[0];
			tri.col[0] = col[0];
			

			//顶点左右排序
			float x1 = point[1].x;
			float x2 = point[2].x;
			int ix1=1,ix2=2;
			if (x1>x2)
			{
				ix1=2;ix2=1;
			}

			
			tri.point[ix1] = point[1];
			tri.col[ix1] = col[1];
			tri.point[ix2] = point[2];
			tri.col[ix2]=col[2];

			//是否需要加入纹理坐标
			if (pri.vb->m_format==VB_POS_TEX_NORM)
			{
				tri.tex[0] = tex[0];
				tri.tex[ix1] = tex[1];
				tri.tex[ix2] = tex[2];

			}
			if (	check_outofrange(tri.point, m_viewport,false) )
				m_rast_triangles[0][pri.vb->m_format].push_back(tri);
		}
		else if (fcomp(point[0].y,point[1].y,m_epslon))
		{


			float x0 = point[0].x;
			float x1 = point[1].x;

			int ix0=0,ix1=1;
			if (x0>x1)
			{
				ix0=1;ix1=0;
			}
			tri.point[ix0]=point[0];
			tri.col[ix0]=col[0];
			tri.point[ix1]=point[1];
			tri.col[ix1]=col[1];
			tri.point[2]= point[2];
			tri.col[2] = col[2];

			//是否需要加入纹理坐标
			if (pri.vb->m_format==VB_POS_TEX_NORM)
			{
				tri.tex[ix0] = tex[0];
				tri.tex[ix1] = tex[1];
				tri.tex[2] = tex[2];
			}

			if (	check_outofrange(tri.point, m_viewport,true) )
				m_rast_triangles[1][pri.vb->m_format].push_back(tri);
		}
		else
		{
			/*slipt to 2 triangle
                         y1-y0
               ratio =   ------
                         y0-y2


			               x0-x2
			 x_new = x0 +  ------ * (y1-y0)
			               y0-y2

			这里还需要计算z_new
			z_new 与 dzdy 成线性关系，因为z已经是其次坐标归一化后的数值，本身已经是1/w 的线性函数
			w_new 与 dwdy成线性关系，因为顶点vertex.w 记录的是1/w
			normal, col,u,v,TB 除以w,与dmdy成线性关系 m=n,c,t,v,t,b

			                   (z0 - z2)
           z_new = z0 +    --------------  * (y1-y0)
			                   ( y0 - y2)

		    c_new/w_new = c0/w0 + (c0/w0-c2/w2)*ratio
			
			*/

			float ratio = (point[1].y - point[0].y)/(point[0].y - point[2].y);
			vec4 point_new = point[0] + (point[0]-point[2])*ratio;
			point_new.y = point[1].y;
			vec4 c_new =  col[0] * point[0].w + (col[0]*point[0].w - col[2]*point[2].w) * ratio;
			c_new/=point_new.w;
			vec2 t_new =  tex[0] * point[0].w + (tex[0]*point[0].w - tex[2]*point[2].w) * ratio;
            t_new/=point_new.w;
			//up tri



			//////////////////////////////////////////////////////////////////////////
			// 正三角
			int orgidx=1,newidx=2;
			if (point_new.x < point[1].x)
			{
				orgidx = 2;newidx =1;
			}
#if TEST_RAST
			col[0]=col[1]=col[2] = RED_COLOR;
#endif
			tri.point[0] = point[0];
			tri.col[0] = col[0];
			tri.point[orgidx] = point[1];
			tri.point[newidx] = point_new;
			tri.col[orgidx] = col[1];
			tri.col[newidx] = c_new;

			//是否需要加入纹理坐标
			if (pri.vb->m_format==VB_POS_TEX_NORM)
			{
				tri.tex[0] = tex[0];
				tri.tex[orgidx] = tex[1];
				tri.tex[newidx] = t_new;
			}
			if (	check_outofrange(tri.point, m_viewport,false) )
				m_rast_triangles[0][pri.vb->m_format].push_back(tri);

			//////////////////////////////////////////////////////////////////////////
			//倒三角
			orgidx=0,newidx=1;
			if (point_new.x<point[1].x)
			{
				orgidx = 1;newidx =0;
			}
#if TEST_RAST
			col[0]=col[1]=col[2] = GREEN_COLOR;
#endif

			tri.point[orgidx] = point[1];
			tri.point[newidx] = point_new;
			tri.col[orgidx] = col[1];
			tri.col[newidx] = c_new;
			tri.point[2] = point[2];
			tri.col[2] = col[2];

			//是否需要加入纹理坐标
			if (pri.vb->m_format==VB_POS_TEX_NORM)
			{
				tri.tex[orgidx] = tex[1];
				tri.tex[newidx] =t_new;
				tri.tex[2] = tex[2];
			}
			if (	check_outofrange(tri.point, m_viewport,true) )
				m_rast_triangles[1][pri.vb->m_format].push_back(tri);
		}

	}

	return 0;
}

//渲染所有triangles，hotspot!
//
int CRasterizer::flush()
{

	m_surfaceBuf = m_context->lockSurface(m_linebytes);
	m_zbuf = m_context->getZBuffer();
	m_run_flags|=RAST_SURFACE_LOCKED;

	//render all vertex buffer
	//TODO: 使用多线程同时绘制三角形
	drawTriangle_colTex(m_rast_triangles[0][VB_POS],true);
	drawTriangle_colTex(m_rast_triangles[1][VB_POS],false);
	//
	drawTriangle_colTex(m_rast_triangles[0][VB_POS_NORM],true);
	drawTriangle_colTex(m_rast_triangles[1][VB_POS_NORM],false);
//printDebug("rast uptri=%d,down tri=%d\n",m_rast_triangles[0][VB_POS_TEX_NORM].size(),m_rast_triangles[1][VB_POS_TEX_NORM].size());
	drawTriangle_colTex(m_rast_triangles[0][VB_POS_TEX_NORM],true);
	drawTriangle_colTex(m_rast_triangles[1][VB_POS_TEX_NORM],false);

	
	m_context->unlockSurface();

	cleanAll();
	m_surfaceBuf=0;
	m_run_flags&=(~RAST_SURFACE_LOCKED);
	return 0;
}

//opengl窗口是bottom=0, top = height-1，顶点排序是按照从大到小，也就是三角形从上到下绘制，y依次递减，这里是底边为平的三角形，y1=y2
/*
绘制如下正三角形, uptri=false

  x0 ,y0
    /\
   /  \
  /    \
  ------
x1,y1  x2,y2


*/


/*
绘制如下倒三角形, uptri=true

x0,y0  --------------  x1,y1
       \           /
        \         /
         \       /
          \     /
           \   /
            \ /
             V
            x2,y2
*/
int CRasterizer::drawTriangle_colTex(vector<t_rast_triangle>& tris,bool uptri)
{
	if (!(m_run_flags&RAST_SURFACE_LOCKED))
		return 0 ;
	for (size_t i=0;i<tris.size();++i)
	{
		t_rast_triangle& tri = tris[i];

		//draw scan line use 1 const color
		//光栅化规则，整数坐标是一个象素的中点，（0，0）的象素范围为(-0.5,-0.5)---(0.5,0.5)
		//这样就需要用ceil进行取整数，而不能是float to integer取整
		float  y0 ,y1;
		if (uptri)
		{
			y0 = idMath::FtoiFast(tri.point[0].y);
			y1 = idMath::FtoiFast(tri.point[1].y);
			//y0 = idMath::Ceil(tri.point[0].y);
			//y1 = idMath::Ceil(tri.point[1].y);
		}
		else
		{
			y0 = idMath::FtoiFast(tri.point[0].y);
			y1 = idMath::FtoiFast(tri.point[2].y);
			//y0 = idMath::Ceil(tri.point[0].y);
			//y1 = idMath::Ceil(tri.point[2].y);
		}


		//屏幕线性差值
		float height;
		t_rasterizier_point attribute;

		if (uptri)
		{
			height = tri.point[0].y-tri.point[1].y;
			attribute.pl = tri.point[0];
			attribute.pr = tri.point[0];
			attribute.pl.y=y0;
			attribute.pr.y=y0;
			attribute.dpl = (tri.point[0]-tri.point[1])/height;
			attribute.dpr = (tri.point[0]-tri.point[2])/height;
			attribute.cl = tri.col[0]*tri.point[0].w;
			attribute.cr = attribute.cl;
			attribute.dcl = (attribute.cl - tri.col[1]*tri.point[1].w)/height;
			attribute.dcr = (attribute.cr - tri.col[2]*tri.point[2].w)/height;
			attribute.tl = tri.tex[0]*tri.point[0].w;
			attribute.tr = attribute.tl;
			attribute.dtl = (attribute.tl - tri.tex[1]*tri.point[1].w)/height;
			attribute.dtr = (attribute.tr - tri.tex[2]*tri.point[2].w)/height;
		}
		else
		{
			height = tri.point[0].y-tri.point[2].y;
			//vertex position
			attribute.pl = tri.point[0];
			attribute.pr = tri.point[1];
			attribute.pl.y=y0;
			attribute.pr.y=y0;
			attribute.dpl = (tri.point[0]-tri.point[2])/height;
			attribute.dpr = (tri.point[1]-tri.point[2])/height;
			//vertex color
			attribute.cl = tri.col[0]*tri.point[0].w;
			attribute.cr = tri.col[1]*tri.point[1].w;
			attribute.dcl = (attribute.cl  - tri.col[2]*tri.point[2].w)/height;
			attribute.dcr = (attribute.cr - tri.col[2]*tri.point[2].w)/height;
			//texture coordination
			attribute.tl = tri.tex[0]*tri.point[0].w;
			attribute.tr = tri.tex[1]*tri.point[1].w;
			attribute.dtl = (attribute.tl - tri.tex[2]*tri.point[2].w)/height;
			attribute.dtr = (attribute.tr - tri.tex[2]*tri.point[2].w)/height;

		}

		int noDownTriangleClip=0;
		//clipping top Y
		if (y0>m_viewport[2])
		{
			y0=m_viewport[2];

		//做微调，当斜率很大时，因为Y被取整，X需要重新做调整，
		//当被屏幕上沿儿接切时，一样要重新计算xleft和xright
		//got new left&right side x
			noDownTriangleClip=1;
		}
		float h;
		h = y0 - tri.point[0].y;
		attribute.pl+= attribute.dpl*h;
		attribute.pr+= attribute.dpr*h;
		attribute.cl+= attribute.dcl*h;
		attribute.cr+= attribute.dcr*h;
		attribute.tl+= attribute.dtl*h;
		attribute.tr+= attribute.dtr*h;

		//为x clip做准备
		int maxX = m_iviewport[1];
		int minX = m_iviewport[0];
		//1:min x clipping; 2:max x clipping
		int downTriangleClip = 0;

		int _releaseVer=1; 
#ifdef _DEBUG1111
		_releaseVer=0;
#endif

		if (!uptri &&!noDownTriangleClip )
		{
			/*倒三角
			调整三角形上沿，因为y取整的原因，对上沿两端的顶点进行了调整，
			这里针对x方向的调整，再调整一回，使得宽度符合原始浮点数的宽度
			比如如下三角形,"@@@@@"是由于取整y而多出的部分

                                      *
                               ********
                        ***************
          @@@@@************************
			      *********************
				        ***************
						      *********
							        ***

			针对这种情况，需要截掉 @@@@@，采用带x剪切的扫描绘制函数
			*/
			
			float deltaLeft = tri.point[0].x - attribute.pl.x  ;
			float deltaRight = attribute.pr.x - tri.point[1].x;
			float len = attribute.pr.x - attribute.pl.x;
			if (deltaLeft >=1 && tri.point[0].x>m_viewport[0]&&tri.point[0].x<m_viewport[1] )
			{
				//调整clip min X 
				float t = deltaLeft/len;
				minX = idMath::FtoiFast(min(tri.point[0].x,tri.point[2].x));
				downTriangleClip=1;
			//	printDebug("倒三角形上边左边越界%f\n",deltaLeft);

			}
			if (deltaRight>=1 && tri.point[1].x<m_viewport[1]&&tri.point[1].x>m_viewport[0])
			{
				//调整clip max X 
				float t = deltaRight/len;
				maxX= idMath::FtoiFast(max(tri.point[1].x,tri.point[2].x));
				downTriangleClip=2;
			//	printDebug("倒三角形上边右边越界%f\n",deltaRight);

			}
		}
		//clipping bottom Y
		if (y1<m_viewport[3])
		{
			y1 = m_viewport[3];
		}
		else
			y1+=1.f;


		int iy0 = idMath::Ftoi(y0);
		int iy1 = idMath::Ftoi(y1);

		sfByte8* col =  m_context->getGlobalColor();

		//当前屏幕象素颜色位数
		sfByte8* buf = (sfByte8* )m_surfaceBuf + (m_context->getParam()->height-iy0-1)*m_linebytes;							
		
		//check if not need x clipping
		if (tri.point[0].x<m_viewport[1] &&tri.point[0].x>m_viewport[0] &&
			tri.point[1].x<m_viewport[1] &&tri.point[1].x>m_viewport[0] &&
			tri.point[2].x<m_viewport[1] &&tri.point[2].x>m_viewport[0] && 
			!downTriangleClip)
		{

			if (m_context->getParam()->bpp==16)//RGB format: R5G6B5
			{
				//从上到下，上面的Y大

				sfCol16 col16 = ( (col[1]>>3) <<11 )|( (col[2]>>2)<<5 )|(col[3]>>3);

				if(tri.pri->vb->m_format==VB_POS_TEX_NORM)
					rastTri_constColTex(iy0,iy1,buf,m_zbuf,attribute,col16,tri.pri->material);
				else
					rastTri_constCol(iy0,iy1,buf,attribute,col16);
			}
			else if (m_context->getParam()->bpp==32)//RGB format: A8R8G8B8
			{
				//从上到下，上面的Y大
				sfCol32 col32 = COLORREF(RGB(col[3],col[2],col[1])) | 0xff000000;
				if(tri.pri->vb->m_format==VB_POS_TEX_NORM)
					rastTri_constColTex(iy0,iy1,buf,m_zbuf,attribute,col32,tri.pri->material);
				else
					rastTri_constCol(iy0,iy1,buf,attribute,col32);
			}
		}
		else
		{

			//x clipping
			if (m_context->getParam()->bpp==16)//RGB format: R5G6B5
			{
				//从上到下，上面的Y大

				sfCol16 col16 = ( (col[1]>>3) <<11 )|( (col[2]>>2)<<5 )|(col[3]>>3);

				if (tri.pri->vb->m_format==VB_POS_TEX_NORM)
					rastTri_constColTex_xclip(iy0,iy1,minX,maxX,buf,m_zbuf,attribute,col16,tri.pri->material);
				else
					rastTri_constCol_xclip(iy0,iy1,minX,maxX,buf,attribute,col16);
			}
			else if (m_context->getParam()->bpp==32)//RGB format: A8R8G8B8
			{
				//从上到下，上面的Y大
				sfCol32 col32 = COLORREF(RGB(col[3],col[2],col[1])) | 0xff000000;
				if (tri.pri->vb->m_format==VB_POS_TEX_NORM)
					rastTri_constColTex_xclip(iy0,iy1,minX,maxX,buf,m_zbuf,attribute,col32,tri.pri->material);
				else
					rastTri_constCol_xclip(iy0,iy1,minX,maxX,buf,attribute,col32);

			}
		}

	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////
//从128bit浮点生成32 bit 或者16bit RGBA颜色 
//采用函数模板全特化，只处理16和32位色。不处理8bit调色板
//一般而言，程序都应该运行在16bit
template<class T> sfINLINE void CRasterizer::pixel_shader_constCol(const vec4& col,T* c)
{
 	myassert(0,"无法得到颜色，使用了非UINT16和UINT32类型的颜色");
}

//TODO: use SIMD to optimize
template<> sfINLINE void CRasterizer::pixel_shader_constCol (const vec4& col,sfCol16* c )
{	
	*c  = idMath::FtoiFast(col[0]*31) <<11 |idMath::FtoiFast(col[1]*63) <<5 |idMath::FtoiFast(col[2]*31) ;
	
}
template<> sfINLINE void CRasterizer::pixel_shader_constCol (const vec4& col,sfCol32* c)
{
	
	*c = idMath::FtoiFast(col[3]*255) <<24 |idMath::FtoiFast(col[0]*255) <<16 |idMath::FtoiFast(col[1]*255) <<8 |idMath::FtoiFast(col[2]*255);
	
}



//屏幕像素垂直差值
//因为是从top到bottom绘制，y的数值是从大到小，所以deltaY == -1
//TODO: use SIMD to optimize
sfINLINE void t_rasterizier_point::interpolateVerticalCol()
{
	//position
	pl-= dpl; // pl += (-1)*dpl
	pr-= dpr;
	//color attribute
	cl-= dcl;
	cr-= dcr;
	//tex attribute
	tl -= dtl;
	tr -= dtr;
}
 
//TODO: use SIMD to optimize
//通用固定颜色填充函数，支持16bit和32bit
template<class T> sfINLINE  int  CRasterizer::scanLine_constCol(T* buf,int xleft,int xright,T col,t_rasterizier_point& p)
{
	float inv_len =1/(p.pr.x - p.pl.x);
	float t;
	vec4 posDelta=p.pr - p.pl;
	vec4 colDelta = p.cr - p.cl;
	for (int i=0;i<xright-xleft;++i,++buf)
	{
		t = (i+xleft-p.pl.x)*inv_len;
		vec4 v = p.pl + posDelta * t;
		float w = p.pl.w + posDelta.w*t;
		vec4 c = (p.cl+colDelta*t) / w;
		pixel_shader_constCol(c,&col);

		
		*buf = col;
	}
	return xright-xleft;
}
template<class T> sfINLINE  int  CRasterizer::scanLine_constCol_Affine(T* buf,int xleft,int xright,T col,t_rasterizier_point& p)
{
	float inv_len =1/(p.pr.x - p.pl.x);
	float t;
	vec4 posDelta=p.pr - p.pl;
	vec4 cl = p.cl/p.pl.w;
	vec4 colDelta = cl - p.cl/p.pl.w;
	for (int i=0;i<xright-xleft;++i,++buf)
	{
		t = (i+xleft-p.pl.x)*inv_len;
		vec4 v = p.pl + posDelta * t;
		vec4 c = cl+colDelta*t;
		pixel_shader_constCol(c,&col);

		*buf = col;
	}
	return xright-xleft;
}

//////////////////////////////////////////////////////////////////////////
//texture mapping rasterizier
//纹理采样器，负责根据贴图坐标寻址2D贴图图素
template<class T> sfINLINE void CRasterizer::tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,sfByte8* texbuf,T& col,t_texture* tex)
{
	myassert(0,"未知颜色格式纹理采样");
}
#define TEXSAMPLE_CLAMP 1
template<> sfINLINE void CRasterizer::tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,sfByte8* texbuf,sfCol32& col,t_texture* tex)
{
	float u = tc[0];
	float v = tc[1];
#if TEXSAMPLE_CLAMP
	if (v<0)
		v=1;
	else if (v>1)
		v=1;
	if (u<0)
		u=0;
	else if (u>1)
		u=1;
#else//REPEAT
	if (v<0)
	{
		if ( v<-epslon_tc)
			v=v-floorf(v);
		else
			v=0;
	}
	else if (v>1)
	{
		if ( v>1+epslon_tc)
			v=v-floorf(v);
		else
			v=1;
	}
	if (u<0)
	{
		if ( u<-epslon_tc)
			u=u-floorf(u);
		else
			u=0;
	}
	else if (u>1)
	{
		if ( u>1+epslon_tc)
			u=u-floorf(u);
		else
			u=1;
	}
#endif
	int tx = idMath::FtoiFast(w*u);
	int ty = idMath::FtoiFast(h*v);

	int off = ty*pitch + tx*bpp;
	sfByte8 b= *(texbuf + off);
	sfByte8 g= *(texbuf + off+1);
	sfByte8 r= *(texbuf + off+2);
	col = (r<<16) | (g<<8)|(b);

}
template<> sfINLINE void CRasterizer::tex2DSampler(const vec4& tc,int w,int h,int pitch,int bpp,
									  sfByte8* texbuf,sfCol16& col,t_texture* tex)
{
	//clamp mode
	//	float u = tc[0]-floorf(tc[0]);
	//	float v = tc[1]-floorf(tc[1]);
	float u = tc[0];
	if (u<0) u=0;
	else if (u>1) u=1;
	float v = tc[1];
	if (v<0)
		v=0;
	else if (v>1)
		v=1;
	int tx = idMath::FtoiFast(w*u);
	int ty = idMath::FtoiFast(h*v);

	int off = ty*pitch + tx*bpp;
	sfByte8 b= *(texbuf + off);
	sfByte8 g= *(texbuf + off+1);
	sfByte8 r= *(texbuf + off+2);
	col = (r>>3)<<11 | (g>>2)<<5 | (b>>3);


}
sfINLINE void clampf(vec4& c)
{
	if (c.x<0) c.x =0;
	else if (c.x>1.f) c.x=1.f;
	if (c.y<0) c.y =0;
	else if (c.y>1.f) c.y=1.f;
	if (c.z<0) c.z =0;
	else if (c.z>1.f) c.z=1.f;
	if (c.w<0) c.w=0;
	else if (c.w>1.f) c.w =1.f;

}
//tex * vertex lighting pixel operator
template<class T> T CRasterizer::pixel_shader_modulateTexCol(const T col, const vec4& c)
{

}
//对应16和32bit 全特化
template <> sfCol16 CRasterizer::pixel_shader_modulateTexCol(const sfCol16 col, const vec4& c)
{
	int b = idMath::FtoiFast( (col&31) * c[0] ) ;
	int g =  idMath::FtoiFast( ((col&2016)>>5) * c[1] );
	int r =  idMath::FtoiFast( ( (col&0xf800)>>11) * c[2] ) ;
	return  (r<<11)|(g<<5)|b;
}
template <> sfCol32 CRasterizer::pixel_shader_modulateTexCol(const sfCol32 col, const vec4& c)
{
	int b = idMath::FtoiFast( ( (col&0xff)) * c[0] ) ;
	int g =  idMath::FtoiFast( ( (col&0x00ff00)>>8) * c[1] );
	int r =  idMath::FtoiFast( ( (col&0xff0000)>>16) * c[2] ) ;
	return  (r<<16)|(g<<8)|(b);
}
template<class T>   int  CRasterizer::scanLine_ColTex_Affine(T* buf,int xleft,int xright,T col,t_rasterizier_point& p,t_material* material)
{
	t_texture* tex = material->tex[0];
	CBitmapWrapper* bitmap = tex->image;
	float inv_len =1/(p.pr.x - p.pl.x);
	float t;
	vec4 posDelta=p.pr - p.pl;
	vec4 colDelta = p.cr/p.pr.w - p.cl/p.pl.w;
	vec4 colLeft = p.cl/p.pr.w;
	vec4 texDelta = p.tr/p.pr.w - p.tl/p.pl.w;
	vec4 texLeft = p.tl/p.pl.w;

	int bpp = bitmap->getBPP()/8;
	int width = bitmap->getWidth()-1;
	int height = bitmap->getHeight()-1;
	int pitch = bitmap->getPitch();
	unsigned char* texbuf = bitmap->getBuffer();

	for (int i=0;i<xright-xleft;++i,++buf)
	{
		t = (i+xleft-p.pl.x)*inv_len;
		vec4 v = p.pl + posDelta * t;
		float w = p.pl.w + posDelta.w*t;
		vec4 c = (colLeft+colDelta*t);

		vec4 tc = (texLeft + texDelta*t);
		//TODO: 为了速度和简化，省略multitexture的计算
		tex2DSampler(tc,width,height,pitch,bpp,texbuf,col,tex);
		//modulate tex& lighting
		T final_c = pixel_shader_modulateTexCol(col,c);
		*buf = final_c;

	}
	return xright - xleft;
}

#define TEST_ZBUF 1
template<class T>   int  CRasterizer::scanLine_ColTex(T* buf,ZBUF_TYPE* zbuf,int xleft,int xright,T col,t_rasterizier_point& p,t_material* material)
{
	t_texture* tex = material->tex[0];
	CBitmapWrapper* bitmap = tex->image;
	float inv_len =1/(p.pr.x - p.pl.x);
	float t;
	vec4 posDelta=p.pr - p.pl;
	vec4 colDelta = (p.cr - p.cl);
	vec4 texDelta = (p.tr - p.tl);

	int bpp = bitmap->getBPP()/8;
	int width = bitmap->getWidth()-1;
	int height = bitmap->getHeight()-1;
	int pitch = bitmap->getPitch();
	unsigned char* texbuf = bitmap->getBuffer();
	vec4 cw  = p.cl;
	vec4 tw = p.tl;
	for (int i=0;i<xright-xleft;++i,++buf,++zbuf)
	{
		t = (i+xleft-p.pl.x)*inv_len;
		vec4 v = p.pl + posDelta * t;
		//z test, 1/w buffer, mean 1/Zcamera buffer, when z1>z2, should write
		float w = v.w ;


#if TEST_ZBUF
		if (*zbuf > w)
			continue;

		*zbuf = w;
#endif
		//this is not good, should use dc/dx, not a multiple operator
		vec4 c = (p.cl + colDelta*t)/w;
		vec4 tc = (p.tl + texDelta*t)/w;

		//TODO: 为了速度和简化，省略multitexture的计算
		tex2DSampler(tc,width,height,pitch,bpp,texbuf,col,tex);

		//modulate tex& lighting
		clampf(c);
		T final_c = pixel_shader_modulateTexCol(col,c);
		*buf = final_c;

		

	}
	return xright - xleft;
}




template<class T> void CRasterizer::rastTri_constCol(int iy0,int iy1,sfByte8* buf,t_rasterizier_point attribute,T col)
{
		int affine=m_run_flags&RAST_AFFINE;
	for (int iy = iy0;iy>=iy1;--iy,buf+=m_linebytes)
	{
		int ix0 = idMath::FtoiFast(attribute.pl.x);
		int ix1 = idMath::FtoiFast(attribute.pr.x);
		T* screenbuf = (T*)buf;
		screenbuf +=ix0;
		if (!affine)
			scanLine_constCol(screenbuf ,ix0,ix1,col,attribute);
		else
			scanLine_constCol_Affine(screenbuf,ix0,ix1,col,attribute);
		attribute.interpolateVerticalCol();
	}
}
 
template<class T>   void  CRasterizer::rastTri_constCol_xclip(int iy0,int iy1,int minX,int maxX,sfByte8* buf,t_rasterizier_point& attribute,T col)
{
	int affine=m_run_flags&RAST_AFFINE;
	for (int iy = iy0;iy>=iy1;--iy,buf+=m_linebytes)
	{
		int ix0 = idMath::FtoiFast(attribute.pl.x);
		if (ix0<minX)
			ix0=minX;
		int ix1 = idMath::FtoiFast(attribute.pr.x);
		if (ix1>maxX)
			ix1=maxX;
		T* screenbuf = (T*)buf;
		screenbuf+=ix0;
		if (!affine)
			scanLine_constCol(screenbuf ,ix0,ix1,col,attribute);
		else
			scanLine_constCol_Affine(screenbuf,ix0,ix1,col,attribute);
		attribute.interpolateVerticalCol();
	}
}

template<class T>  void CRasterizer::rastTri_constColTex(int iy0,int iy1,sfByte8* buf,ZBUF_TYPE* zbuf,
											t_rasterizier_point attribute,T col,t_material* material)
{
	int affine=m_run_flags&RAST_AFFINE;
	int nline=0;
	for (int iy = iy0;iy>=iy1;--iy,buf+=m_linebytes,++nline)
	{
		int ix0 = idMath::FtoiFast(attribute.pl.x);
		int ix1 = idMath::FtoiFast(attribute.pr.x);
		ZBUF_TYPE* zbuf = m_zbuf + (m_context->getParam()->height-iy-1)*m_context->getParam()->width;
		T* screenbuf = (T*)buf;
		screenbuf +=ix0;
		if (affine)
			scanLine_ColTex_Affine(screenbuf ,ix0,ix1,col,attribute,material);
		else
			scanLine_ColTex(screenbuf ,zbuf+ix0,ix0,ix1,col,attribute,material);
		
		attribute.interpolateVerticalCol();
	}
}
template<class T>  void CRasterizer::rastTri_constColTex_xclip(int iy0,int iy1,int minX,int maxX,sfByte8* buf,ZBUF_TYPE* zbuf,
												  t_rasterizier_point& attribute,T col,t_material* material)
{
	int affine=m_run_flags&RAST_AFFINE;
	
	for (int iy = iy0;iy>=iy1;--iy,buf+=m_linebytes)
	{
		ZBUF_TYPE* zbuf = m_zbuf + (m_context->getParam()->height-iy-1)*m_context->getParam()->width;
		int ix0 = idMath::FtoiFast(attribute.pl.x);
		if (ix0<minX && iy==iy0)
			ix0=minX;
		else if (ix0<m_iviewport[0])
			ix0= m_iviewport[0];
		int ix1 = idMath::FtoiFast(attribute.pr.x);
		if (ix1>maxX && iy==iy0)
			ix1=maxX;
		else if (ix1>m_iviewport[1])
			ix1 = m_iviewport[1];

		T* screenbuf = (T*)buf;
		screenbuf+=ix0;
		if (affine) 
			scanLine_ColTex_Affine(screenbuf ,ix0,ix1,col,attribute,material);
		else
			scanLine_ColTex(screenbuf ,zbuf+ix0,ix0,ix1,col,attribute,material);

		attribute.interpolateVerticalCol();
	}
}
