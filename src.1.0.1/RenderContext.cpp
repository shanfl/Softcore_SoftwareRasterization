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
#include "RenderContext.h"
#include "softRender.h"
#include "Rasterization.h"
#include <math.h>
#include <list>
#include "mmgr/mmgr.h"
#define MAX_STACK_MATRIX 16
CRenderContext::CRenderContext(const t_contextinfo& context_param)
{
	m_context_param = context_param;
	m_drawSurface=0;
	memset(m_renderop.m_vb_internal,0,VB_MAX_FMT*sizeof(int));
	reset();


}
CRenderContext::~CRenderContext()
{
	m_context_param.device->destroyPBuffer(this);
	for (int i=0;i<VB_MAX_FMT;++i)
	if (m_renderop.m_vb_internal[i])
		delete 		m_renderop.m_vb_internal[i];

	if (	m_zbuffer)
		delete []m_zbuffer;
}
void CRenderContext::create()
{
	if (m_drawSurface)
		return;

	//create color buffer
	m_context_param.device->createPBuffer(this);

	//create z buffer
	if (m_zbuffer )
		delete m_zbuffer;
	m_zbuffer=new float[m_context_param.width*m_context_param.height]; 

	t_matrix m;
	for (int i=0;i<MAX_STACK_MATRIX;++i)
	{
		viewmatrix.push_back(m);
		projmatrix.push_back(m);

		for (int j=0;j<MAX_STAGE;++j)
		{
			texmatrix[j].push_back(m);
			iTex[j]=MAX_STACK_MATRIX-1;
		}
	}
	iView = MAX_STACK_MATRIX-1;
	iProj = MAX_STACK_MATRIX-1;

	m_viewport[0]=0.f;m_viewport[1]=m_context_param.width-1.f;
	m_viewport[2]=m_context_param.height-1.f;m_viewport[3]=0.f;

}
int CRenderContext::reset()
{
	//set all parameters to default
	m_mat_mode=MATRIX_VIEW;
	m_unit = 0;
	m_global_color[0]=m_global_color[1]=m_global_color[2]=m_global_color[3]=1.f;
	//初始时为ccw culling
	m_backfaceCulling=2;
		m_zbuffer=0;
	return 0;
}
void* CRenderContext::lockSurface(int& linewidth)
{
	if (!m_drawSurface)
	{
		myassert(0,"lock null surface!");
		return 0;
	}


	D3DLOCKED_RECT lockinfo;
	memset(&lockinfo,0,sizeof(lockinfo));

	IDirect3DSurface9* surf = m_drawSurface;

	HRESULT res = surf->LockRect(&lockinfo,NULL,D3DLOCK_DISCARD);
	if (res!=S_OK)
	{
		MessageBoxA(NULL,"Can't lock col buf","SOFTCORE",MB_OK);
		surf->Release();
		return 0;
	}

	linewidth = lockinfo.Pitch;
	return lockinfo.pBits;
}
void CRenderContext::unlockSurface()
{
	m_drawSurface->UnlockRect();
}

t_matrix MATRIX_IDENTITY (1,0,0,0, 0,1,0,0,0,0,1,0,0,0,0,1);
t_matrix MATRIX_ZERO (1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
int CRenderContext::setGlobalColor(sfByte8* col)
{
	m_global_color [0]=col[0];
	m_global_color [1]=col[1];
	m_global_color [2]=col[2];
	m_global_color [3]=col[3];
	return 0;
}
void CRenderContext::loadIdentity(MATRIX_MODE mode)
{
	setMatrix(mode,MATRIX_IDENTITY);
}

void CRenderContext::setMatrix(MATRIX_MODE mode,const t_matrix& mat)
{
	t_matrix& matrix  = getModeMatrix(mode) ;
	matrix= mat;
}
void CRenderContext::setMatrixMode(MATRIX_MODE mode)
{
	m_mat_mode = mode;
}
void CRenderContext::setActiveTexture(int unit)
{
	m_unit = unit;
	myassert(unit>=0&& unit<MAX_STAGE,"非法texture unit");
}


t_matrix& CRenderContext::getModeMatrix(MATRIX_MODE mode)
{
	switch(mode)
	{
	case MATRIX_VIEW:
		return viewmatrix[iView];
		break;
	case MATRIX_PROJ:
		return  projmatrix[iProj];
		break;
	case MATRIX_TEX:
		{
			vector<t_matrix>& texmm = texmatrix[m_unit];
			return texmm[iTex[m_unit]];//= ;
		}
		break;
	}
	myassert(0,"非法MATRIX MODE ");
	return MATRIX_ZERO;
}

void CRenderContext::setViewport(const int* vp)
{
	m_viewport[0] = (float)vp[0];
	m_viewport[1] = (float)vp[1];
	m_viewport[2] = (float)vp[2];
	m_viewport[3] = (float)vp[3];

	CRasterizer::instance().updateViewport();

}
//notice , it's fovy in opengl, but here it's fov X
void CRenderContext::setPerspective(float fovx,float ratio,float n,float f)
{
	t_matrix& m = getModeMatrix(m_mat_mode);

	getProjectMatrix(m,fovx,ratio,n,f);
}

extern int g_strip[];
void CRenderContext::cleanInternalVB()
{
	memset(m_renderop.m_vb_fill_curpos,0,sizeof(int)*VB_MAX_FMT);
	m_renderop.m_indexbuffer.clear();
	m_renderop.m_primitives.clear();
}
char* CRenderContext::getVBuffer(VB_FMT fmt)
{
	return m_renderop.m_vb_internal[fmt]->m_buffer + 
		m_renderop.m_vb_fill_curpos[fmt];
}
int CRenderContext::appendFillPos(int num,VB_FMT fmt)
{
	m_renderop.m_vb_fill_curpos[fmt]+=num*m_renderop.m_vb_internal[fmt]->m_strip;
	myassert(m_renderop.m_vb_fill_curpos[fmt]<=m_renderop.m_vb_internal[fmt]->m_size,"BUFFER填充异常越界！");
	return m_renderop.m_vb_fill_curpos[fmt];
}
void CRenderContext::resizeRenderVB(int num,VB_FMT fmt)
{
	if (!num)
	{
		myassert(0,"vb size为0");
		return;
	}
	if (!m_renderop.m_vb_internal[fmt])
	{
		m_renderop.m_vb_internal[fmt] = new CVertexBuffer(num,fmt);
		m_renderop.m_vb_fill_curpos[fmt]=0;
		return;
	}
	int size_sum =	m_renderop.m_vb_internal[fmt]->m_size;
	int new_size= g_strip[fmt]*num;
	if (m_renderop.m_vb_fill_curpos[fmt] + new_size >=size_sum)
	{
		//relocate the vb
		m_renderop.m_vb_internal[fmt]->resize(m_renderop.m_vb_internal[fmt]->m_vert_num + 2*num);
		//copy vertex to new vb
		printDebug("context internal vb re-allocated\n");
	}
}

void CRenderContext::addPrimitiveList(t_entity* ent,t_mesh* mesh,t_camera* cam)
{
	t_primitive primitiveobj;
	m_renderop.m_primitives.push_back(primitiveobj);
	
	t_primitive &pri = m_renderop.m_primitives[m_renderop.m_primitives.size()-1];
	pri.vb_start = m_renderop.m_vb_fill_curpos[mesh->vb_format] /g_strip[mesh->vb_format] ;
	pri.ib_start = (int)m_renderop.m_indexbuffer.size();
	pri.material = mesh->material;


	pri.ib = &m_renderop.m_indexbuffer;


	backCullingAndClipAndTransform(ent,mesh,&pri,cam);

}

//通用切割polygon函数
//返回增加的新顶点数，如果有切割应该为2，如果是0，则说明所有顶点全在plane正面
sfINLINE void vecInterpolate(const vec3& v1,const vec3& v2, float t,vec3& result)
{
	result = (v2 - v1)* t + v1;
}
//-1: all back side
//0: be cliped
//1: all front side
int clipPolygon(int numPolygonVert, const t_drawVert*const verts,int totalVert,const unsigned int*const idx,
				vector<int>& idx_list,vector<t_drawVert>& new_verts , t_plane& plane)
{
	const float epslon_onPlane=0;
	vec3 normal(plane.param.x,plane.param.y,plane.param.z,0);
	vector<float> vec_dist;
	//顶点序号，因为要根据正负来判断是否在plane的背面，所以序号从1开始
	int n=0;
	int outside_num = 0;
	while(n<numPolygonVert)
	{
		float d = verts[idx[n]].pos * normal - plane.param.w;
		vec_dist.push_back(d);
		if (d<epslon_onPlane)
		{
			++outside_num;
			idx_list.push_back(-((int)idx[n]+1));
		}
		else
			idx_list.push_back(idx[n]+1);

		++n;
	}
	
	if (!outside_num)//全部在正面
		return 1;
	if (outside_num==3)//全部在背面
		return -1;

	/*
	找到跨越near plane的2个顶点，切割线段，增加新顶点，更改顺序
	增加的顶点放入new_verts中
	同时记录新索引
	*/
	vector<int> new_idxs;
	vector<int>::iterator it= idx_list.begin();
	bool clipFinished = false;
	bool bCliped=false;
	//记录距离的索引。 一个三角形3顶点到面的距离。这里索引只从0－2，而传入的index 是在mesh 中vb的位置。
	int dist_idx1 =0,dist_idx2=0,dist_array_index=0;
	while(!clipFinished)
	{
		int idx1 = *it;
		++it;
		dist_idx1 = dist_array_index;
		dist_idx2 = dist_idx1+1;
		++dist_array_index;
		if (it==idx_list.end())
		{
			//已经到达最后一个顶点，赋予第一个顶点
			it = idx_list.begin();
			clipFinished=true;
			dist_idx2 = 0;
		}
		int idx2 = *it;

		if (idx1>0&& idx2>0)
		{//两个顶点都在plane正面，直接copy
		//	printDebug("%d vert: %f %f %f\n",idx1,verts[idx1-1].pos.x,verts[idx1-1].pos.y,verts[idx1-1].pos.z);
			new_idxs.push_back(idx1-1);
			continue;
		}
		else if (idx1*idx2 <0)
		{
			//需要切割
			int idx1a=idx1,idx2a=idx2;
			//把idx1和idx2都转换为正常0 based index
			if (idx1<0)
			{
				idx1 = -idx1- 1;
				idx2--;
			}
			else
			{
				idx2 = -idx2 -1;
				idx1--;
			}
			float t2 = fabsf(vec_dist[dist_idx1]) /fabsf((vec_dist[dist_idx2] - vec_dist[dist_idx1]));
			t_drawVert new_vert;

			vecInterpolate(verts[idx1].pos,verts[idx2].pos,t2,new_vert.pos);
			vecInterpolate(verts[idx1].tex,verts[idx2].tex,t2,new_vert.tex);
			vecInterpolate(verts[idx1].col,verts[idx2].col,t2,new_vert.col);
			vecInterpolate(verts[idx1].norm,verts[idx2].norm,t2,new_vert.norm);
			vecInterpolate(verts[idx1].T,verts[idx2].T,t2,new_vert.T);
			vecInterpolate(verts[idx1].B,verts[idx2].B,t2,new_vert.B);
			if (idx1a<0)
			{
			//	printDebug("new vert: %f %f %f\n",new_vert.pos.x,new_vert.pos.y,new_vert.pos.z);
				new_idxs.push_back(new_verts.size()+ totalVert);//新增加顶点的索引
			}
			else //idx2<0
			{
			//	printDebug("%d vert: %f %f %f\n",idx1,verts[idx1].pos.x,verts[idx1].pos.y,verts[idx1].pos.z);
				new_idxs.push_back(idx1);
				
			//	printDebug("new vert: %f %f %f\n",new_vert.pos.x,new_vert.pos.y,new_vert.pos.z);
				new_idxs.push_back(new_verts.size()+totalVert);
			}
		
			new_verts.push_back(new_vert);
			bCliped = true;
		}
		else
		{
			//都在plane背面，不用切割
		}

	}

	//返回新的索引列表
	idx_list = new_idxs;

	if(bCliped)
		return 0;

	if (!new_idxs.size())//都在背面，直接返回－1
	{
		myassert(0,"剪切三角形异常，都在背面");
	}
	else
		myassert(0,"剪切三角形异常，都在正面");

	return 1;
}
/*
back face culling
near plane clipping
vertex transform
vertex lighting calculate
tangent space calculate
*/
int CRenderContext::backCullingAndClipAndTransform(t_entity* ent,t_mesh* mesh,t_primitive* pri,t_camera* cam)
{
	//back face 可以在光栅化时进行，在world space中的culling只是减少vertex级别的运算开销
	//在surface不提供预计算的法线时，world space中的backface culling也消耗时间
	//所有计算在object space中进行，省却了逐顶点转换到world space过程
	
	t_plane nearplane;
	vec3 eyepos  = cam->pos;
	/*
	计算object space的camera pos 和dir，以及near plane平面方程
	假如entity转换矩阵为 view_matrix * object_matrix
	object local matrix为  object_matrix.inverse * view_matrix.inverse
	*/
	t_matrix obj_inv;

	if (ent->m_flags&ENTITY_LOCAL)
	{
		obj_inv = ent->m_model_inv_matrix ;//* cam->view_invert;
	}
	else
	{
		//obj_inv = cam->view_invert;
		obj_inv.Identity();
	}

	vec3 dir;
	MatrixRotVector(obj_inv, cam->dir,dir);
	dir.w = 0;
	eyepos = obj_inv * cam->pos;
	vec3 vertOnPlane = eyepos + (dir* cam->near_dist);
	nearplane.param.Set(dir.x,dir.y,dir.z, dir*vertOnPlane );


	vec3 near_normal( nearplane.param.x,
		nearplane.param.y,
		nearplane.param.z,0);

	//记录index buffer起点
	size_t idxbuf_start = m_renderop.m_indexbuffer.size();

	//整个mesh 的顶点被near plane切割后，产生的所有新顶点
	vector<t_drawVert> newvert_list;

	int culled=0;
	for (int i=0;i<mesh->getTriCount();++i)
	{
		const t_triangle& tri = mesh->getTriangle(i);

		//back face culling in the world space
		if (m_backfaceCulling)
		{
			vec3 normal ;
			float dist=0;
			normal	= tri.norm;
			normal.w = 0;

			float dd = normal* eyepos -tri.norm.w;
			if ( dd<0)
			{
				//printDebug("backculling dist=%f\n",dd);
				++culled;
				continue;
			}
		}

	//	printDebug("tri:%d\n",i);
		//near plane clipping
		//找到所有横跨near plane的三角形
		vector<int> vert_idx;

		//被near plane分割，现在检查是1个点在near plane里面还是2个点在near plane里
		//注意原始三角形的顶点顺序，新生成的三角形顶点也必须按照原始顺序，否则正反面颠倒
		int n=clipPolygon(3,mesh->vb,mesh->numVert,tri.p,vert_idx,newvert_list,nearplane);
		
		if (n>0)//完全在正面
		{
			//add index
			m_renderop.m_indexbuffer.push_back(pri->vb_start + tri.p[0]);
			m_renderop.m_indexbuffer.push_back(pri->vb_start + tri.p[1]);
			m_renderop.m_indexbuffer.push_back(pri->vb_start + tri.p[2]);
			//add vertex buffer
		}
		else if (n<0)
		{
			//在plane背面直接省略
			continue;
		}
		else
		{
			if (vert_idx.size()==3)
			{
				//被near plane重新切割成一个小三角形，重新计算2个新顶点
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[0]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[1]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[2]);
			}
			else if (vert_idx.size()==4)
			{//一个点在外面，near plane内部的三角形被切割成四边形，重新分割四边形为2个三角形
				//0,1,2; 0,2,3; ...
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[0]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[1]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[2]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[0]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[2]);
				m_renderop.m_indexbuffer.push_back(pri->vb_start + vert_idx[3]);

			}
			else
				myassert(0,"切割三角形异常！");
		}

	}

//	printDebug("backface culled=%d\n",culled);
	//现在所有三角形索引和新顶点都生成完毕，创建渲染顶点缓冲
	//总顶点数量
	pri->numvert = mesh->numVert + (int)newvert_list.size();
	//总三角形数量
	pri->numpolygon= (int)(m_renderop.m_indexbuffer.size() - idxbuf_start)/3;
	//append vb
	resizeRenderVB(pri->numvert ,mesh->vb_format);

	pri->vb = m_renderop.m_vb_internal[mesh->vb_format];


	switch(pri->vb->m_format)
	{
	case VB_POS:
		tranformAndFillPos(ent,mesh->numVert,mesh->vb,mesh->vb_format);
		if(newvert_list.size())//转换near clipping新增加的顶点
			tranformAndFillPos(ent,(int)newvert_list.size(),&newvert_list[0],mesh->vb_format);//&newvert_list[0]把STL 数组指针传入，但不同STL实现结果未知，只在VC7&8
		break;
		//			case VB_POS_COL:
	case VB_POS_NORM:
	case VB_POS_TEX_NORM:
		tranformAndFillPosNorm(ent,mesh->numVert,mesh->vb,mesh->vb_format);
		if(newvert_list.size())//转换near clipping新增加的顶点
			tranformAndFillPosNorm(ent,(int)newvert_list.size(),&newvert_list[0],mesh->vb_format);
		break;

	case VB_POS_TEX_NORM_TB://phong shading, 计算出tagent space的灯光储存在vertex color和其他tex coord中
		//tranformPosNormTB(mesh,*m_mat_temp);
		break;
	default:
		break;
	}

	return 0;
}
int CRenderContext::tranformAndFillPos(t_entity* ent,int numVert,t_drawVert* vb,VB_FMT vb_format)
{
	const t_matrix& entity_mat = ent->m_model_matrix;
	
	t_matrix matfinal= getModeMatrix(MATRIX_PROJ)*getModeMatrix(MATRIX_VIEW) * entity_mat;		

	//只转换position，之所以把不同VB FORMAT转换函数分开，是为了cache friendly
	float* __vb  = (float*)getVBuffer(vb_format);
	//TODO: SIMD & Multithread
	float w = m_context_param.width*0.5f;
	float h = m_context_param.height*0.5f;

	for (int i=0;i<numVert;++i)
	{
		t_drawVert* v = vb + i;
		vec3 db = matfinal*v->pos ;
		float inv_w = 1.f/db.w;
		__vb[0] = db.x * inv_w;__vb[1] = db.y*inv_w;__vb[2] = db.z*inv_w;__vb[3] = inv_w;
		__vb[0] =  (__vb[0] +1.f)*w;
		__vb[1] =  (__vb[1] +1.f)*h;
		__vb+=4;//add directly
	}
	appendFillPos(numVert,vb_format);
	return 0;
}
int CRenderContext::tranformAndFillPosNorm(t_entity* ent,int numVert,t_drawVert* vb,VB_FMT vb_format)
{
	const t_matrix& entity_mat = ent->m_model_matrix;

	t_matrix matfinal;
	if (ent->m_flags&ENTITY_LOCAL)//假如物体没有本地矩阵，则物体坐标都是世界坐标系
		matfinal= getModeMatrix(MATRIX_PROJ)*getModeMatrix(MATRIX_VIEW) * entity_mat;		
	else
		matfinal= getModeMatrix(MATRIX_PROJ)*getModeMatrix(MATRIX_VIEW);

	//只转换position，之所以把不同VB FORMAT转换函数分开，是为了cache friendly
	float* __vb  = (float*)getVBuffer(vb_format);
	float* __col = __vb+VEC3FLOAT;
	

	int strip = VEC3FLOAT + 4;
	vec4 tctemp(0,0,0,1);
	float* __tc = &tctemp.x;
	int tex_strip = 0;

	if (//only tex
		vb_format==VB_POS_TEX_COL||//tex  /w vertex color
		vb_format==VB_POS_TEX_NORM||//gouraud texture
		vb_format==VB_POS_TEX_NORM_TB)//phong texture
	{
		tex_strip = strip+4;
		__tc =__col+4; 
		strip+=4;//需要多加一个tc的偏移量
	}

	

	//TODO: SIMD & Multithread
	float w = m_context_param.width*0.5f;
	float h = m_context_param.height*0.5f;

	//这里需要object matrix的逆，这里省略entity缩放的情况，只计算刚体matrix rotation部分的逆==矩阵的转置

	vector<t_vlight> light_object;
	t_matrix matlight = entity_mat;
	matlight[0].w = 0;
	matlight[1].w = 0;
	matlight[2].w = 0;
	t_matrix trans;
	trans.Identity();
	trans[0].w = -entity_mat[0].w;
	trans[1].w = -entity_mat[1].w;
	trans[2].w = -entity_mat[2].w;
	matlight = matlight.Transpose()*trans;


	//求出light pos在object space的位置
	for (size_t l = 0;l<ent->lights.size();++l)
	{
		clight_directional*  light = ent->lights[l];
		t_vlight vl ;
		vl.light_objpos = matlight* light->pos;
		light_object.push_back(vl);
	}

	vec4 color;

	for (int i=0;i<numVert;++i)
	{
		t_drawVert* v = vb + i;
		vec3 testv = getModeMatrix(MATRIX_VIEW) * entity_mat * v->pos;
		vec3 db = matfinal*v->pos ;
		float inv_w = 1.f/db.w;
		__vb[0] = db.x * inv_w;
		__vb[1] = db.y*inv_w;
		__vb[2] = db.z*inv_w;
		__vb[3] = inv_w;

		__vb[0] =  (__vb[0] +1.f)*w;
		__vb[1] =  (__vb[1] +1.f)*h;

		__col[0] = __col[1] = __col[2] = __col[3] = 0;
		//计算光照，如果是点光源，增加衰减项目，如果是spot light，计算区域
		for (size_t l = 0;l<ent->lights.size();++l)
		{
			clight_directional*  light = ent->lights[l];
			light->caculateCol(v->pos,v->norm,light_object[l],color);
			__col[0]+=color.x;__col[1]+=color.y;__col[2]+=color.z;__col[3]+=color.w;
		}

		//clamp
		__col[0] = (__col[0]>1.f)? 1.f:__col[0];
		__col[1] = (__col[1]>1.f)? 1.f:__col[1];
		__col[2] = (__col[2]>1.f)? 1.f:__col[2];
		__col[3] = (__col[3]>1.f)? 1.f:__col[3];

		//tex coord copy or transform
		__tc[0] = v->tex.x;
		__tc[1] = v->tex.y;
		//__tc[2] = v->tex.z;
		//__tc[3] = v->tex.w;

		//省略了project texture
		//tc[0]/=tc[3];
		//tc[1]/=tc[3];
		//省略了texture matrix转换
		//t_matrix texmat = getModeMatrix(MATRIX_TEX);
		//vec4 tc = texmat * __tc;
		//__tc[0]=tc.x;__tc[1]=tc.y;__tc[2]=tc.z;__tc[3]=tc.w;
		
		__vb+=strip;//add directly
		__col+=strip;
		__tc+=tex_strip;
	}
	appendFillPos(numVert,vb_format);
	return 0;
}

int CRenderContext::pushTriangleListToRasterizer()
{
	CRasterizer& rast = CRasterizer::instance();
	for (size_t i=0;i<m_renderop.m_primitives.size();++i)
		rast.pushTriangle(m_renderop.m_primitives[i]);
	return 0;
}
