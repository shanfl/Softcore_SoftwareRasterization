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
#include "SceneLevel.h"
#include "softRender.h"
#include "bitmapWrapper.h"
#include "3dsReader/3ds.h"
#include <map>
#include "mmgr/mmgr.h"
using namespace MAXOBJ;
#define DEBUG_LEVEL 0

//////////////////////////////////////////////////////////////////////////
//  t_mesh
//////////////////////////////////////////////////////////////////////////
t_mesh::t_mesh()
{
	numVert=0;
	numTri=0;
	numIndex=0;
	tris=0;
	vb=0;
	ib=0;
	material = 0;

}
t_mesh::~t_mesh()
{
	if (tris)
		delete []tris;
	if(vb)
		_aligned_free(vb);
	if (ib)
		delete []ib;

}
void t_mesh::createVB(int num)
{
	if (vb)
		_aligned_free(vb);

	vb = (t_drawVert*)_aligned_malloc(sizeof(t_drawVert)* num,ALIGNED_BYTE);
}

int t_mesh::getTriCount()
{
	return numTri;
}
idVec4 g_VECZERO(0,0,0,0);
t_drawVert g_zero={g_VECZERO, g_VECZERO,  g_VECZERO  , g_VECZERO};
const t_drawVert& t_mesh::getTriVert(int n)
{
	if (n<numVert)
		return vb[n];

	return g_zero;
}
unsigned int* t_mesh::getIndexBuffer()
{
	return ib;
}
t_drawVert* t_mesh::getVertexBuffer()
{
	return vb;
}
t_triangle g_tri_zero={0,0,0,g_VECZERO,  0};
t_triangle& t_mesh::getTriangle(int n)
{
	if (n<(int)numTri)
		return tris[n];

	else
		return g_tri_zero;
}


//////////////////////////////////////////////////////////////////////////
//  CScene
//////////////////////////////////////////////////////////////////////////
CScene::CScene()
{
	//TODO: 在HL2中，RENDER SYSTEM在初始化时创建几种常用格式的VB，做为BATCH渲染的内部动态VB
	//在softrender中，按材质排序，对tex mapping时减小cache失效有一定作用，但根据VB格式排序没有意义。
	m_vb=0;
	if (!s_default_img)
		s_default_img= new CBitmapWrapper("../textures/test.png");

	if (!s_default_img->getBuffer())
	{
		delete s_default_img;
		myassert(0,"程序安装不对，必须有textures/目录和目录下的test.png文件，程序退出");
		ExitProcess(88);
	}
}
CScene::~CScene()
{

	clean();
}
CBitmapWrapper* CScene::s_default_img=0;
float g_z = 5;

CBitmapWrapper* CScene::findImage(const string& path)
{
	for (size_t i=0;i<m_images.size();++i)
	{
		if (path == string(m_images[i]->getFileName()))
			return m_images[i];
	}

	return 0;
}
struct tVertInfo
{
	//贴图坐标
	int tv;
	//此顶点在mesh vertex buffer中的索引
	int index;
};
int CScene::forDebug()
{
	int triCnt = 2;
	//hack, generate one mesh directly
	t_mesh *mesh= new t_mesh;
	mesh->vb_format=VB_POS_TEX_NORM;
	mesh->tris = new t_triangle[triCnt];
	mesh->numTri = triCnt;
	mesh->vb = (t_drawVert*)_aligned_malloc(sizeof( t_drawVert)*mesh->numTri*3 ,ALIGNED_BYTE);
	mesh->numVert=triCnt*3;
	mesh->ib = new unsigned int[triCnt*3];
	mesh->numIndex = triCnt*3;

	mesh->tris->material_idx = 0;


	float z=-g_z;
	float y=0;
	float x=0;

	mesh->tris[0].p[0]=0;mesh->tris[0].p[1]=1;mesh->tris[0].p[2]=2;

	//设置3顶点
	mesh->vb[0].pos.x = x;mesh->vb[0].pos.y = y+5;mesh->vb[0].pos.z = z;mesh->vb[0].pos.w = 1;
	mesh->vb[1].pos.x = x;mesh->vb[1].pos.y = y-5;mesh->vb[1].pos.z = z;mesh->vb[1].pos.w = 1;
	mesh->vb[2].pos.x = x+10;mesh->vb[2].pos.y = y+5;mesh->vb[2].pos.z = z;mesh->vb[2].pos.w = 1;
	//设置三角形法线
	mesh->tris->norm.Set(0,0,1,z);
	//设置3法线
	mesh->vb[0].norm.Set(0,0,1,1);
	mesh->vb[1].norm.Set(0,0,1,1);
	mesh->vb[2].norm.Set(0,0,1,1);
	//设置2贴图坐标
	mesh->vb[0].tex.Set(0,0,0,1);
	mesh->vb[1].tex.Set(0,1,0,1);
	mesh->vb[2].tex.Set(1,0,0,1);
	//设置index buffer
	mesh->ib[0]=0;mesh->ib[1]=1;mesh->ib[2]=2;

	if (triCnt>1)
	{


		//2nd triangle


		mesh->vb[3].pos.x = x+10;mesh->vb[3].pos.y = y+5;mesh->vb[3].pos.z = z;mesh->vb[3].pos.w = 1;
		mesh->vb[4].pos.x = x;mesh->vb[4].pos.y = y-5;mesh->vb[4].pos.z = z;mesh->vb[4].pos.w = 1;
		mesh->vb[5].pos.x = x+10;mesh->vb[5].pos.y = y-5;mesh->vb[5].pos.z = z;mesh->vb[5].pos.w = 1;

		//设置3法线
		mesh->vb[3].norm.Set(0,0,1,0);
		mesh->vb[4].norm.Set(0,0,1,0);
		mesh->vb[5].norm.Set(0,0,1,0);
		//设置2贴图坐标
		mesh->vb[3].tex.Set(1,0,0,1);
		mesh->vb[4].tex.Set(0,1,0,1);
		mesh->vb[5].tex.Set(1,1,0,1);
		//设置index buffer
		mesh->ib[3]=3;mesh->ib[4]=4;mesh->ib[5]=5;

		(mesh->tris+1)->material_idx = 0;
		(mesh->tris+1)->norm.Set(0,0,1,z);
		mesh->tris[1].p[0]=3;mesh->tris[1].p[1]=4;mesh->tris[1].p[2]=5;

	}
	//add a light
	clight_directional* light = new clight_directional;
	light->diffuse_col.Set(1,1,1,1);
	light->pos.Set(0,0,-1,0);//a directional light
	m_lights.push_back(light);

	t_material *material = new t_material;
	mesh->material=material;
	t_texture* tex = new t_texture;
	CBitmapWrapper*  img = new CBitmapWrapper("../textures/test.PNG");	
	tex->image = img;
	material->tex[0] = tex;

	m_images.push_back(img);
	m_textures.push_back(tex);
	m_materials.push_back(material);
	m_meshs.push_back(mesh);

	t_entity * ent = new t_entity();
	ent->m_flags=ENTITY_MOVEABLE|ENTITY_LOCAL;
	ent->addMesh(mesh);
	ent->m_model_matrix.Identity();
	ent->m_aabb[0]=x;ent->m_aabb[3]=x+10;
	ent->m_aabb[1]=y-5;ent->m_aabb[4]=y+5;
	ent->m_aabb[2]=z;ent->m_aabb[5]=z;
	m_entity.push_back(ent);

	//create some internal vertex buffer
	//	m_vb= CSoftrender::instance()->createVBuffer(3,VB_POS);
	return 0;
}
void addNewVert(int vertIdx,int tvIdx,vector<t_drawVert>& mesh_vb,const t3DObject& obj)
{
	t_drawVert drawV ;
	CVector3& vert =  obj.pVerts[vertIdx];
	CVector2& tvert = obj.pTexVerts[tvIdx];
	CVector3& norm = obj.pNormals[vertIdx];
	drawV.pos.Set(vert.x,vert.y,vert.z,1);
	drawV.tex.Set(tvert.x,tvert.y,0,1);
	drawV.norm.Set(norm.x,norm.y,norm.z,0);
	printDebug("		new vert:%f %f %f\n",vert.x,vert.y,vert.z);
	mesh_vb.push_back(drawV);
}
int CScene::load(const char* s)
{
#if DEBUG_LEVEL
	return forDebug();
#else


	CLoad3DS loader;
	t3DModel model;
	if (!loader.Import3DS(&model,(char*) s))
		return -1;

	//loading successfully
	for (int i=0;i<model.numOfMaterials;++i)
	{
		printDebug("material:%d, name=%s, File=%s,texID=%d\n",i,
			model.pMaterials[i].strName,
			model.pMaterials[i].strFile,
			model.pMaterials[i].texureId);

		t_texture* texture=0;
		if (model.pMaterials[i].strFile[0]!=0)
		{
			strlwr(model.pMaterials[i].strFile);
			string path = "../textures/";
			path+=model.pMaterials[i].strFile;
			CBitmapWrapper*  img  = findImage(path);
			if (!img)
			{
				img = new CBitmapWrapper(path.c_str());

				if (img->getBuffer())
				{
					m_images.push_back(img);
				}
				else
				{
					//loading texture failed, give a default texture
					delete img;
					img = s_default_img;
				}
			}

			texture= new t_texture;
			texture->image = img;
			m_textures.push_back(texture);
			t_material *material = new t_material;
			m_materials.push_back(material);
			material->tex[0] = texture;
		}

	}
	for (int i=0;i<model.numOfObjects;++i)
	{
		//load object
		const t3DObject& obj = *model.pObject[i];
		if (!obj.numOfFaces)
		{
			printDebug("3ds 文件中含有无多边形的物体\n",obj.strName);
			continue;
		}
		
		if (!m_materials.size() || obj.materialID<0 || obj.materialID>=(int)m_materials.size())
		{
			printDebug("3ds 文件中含有无材质的物体\n",obj.strName);
			continue;
		}

		t_mesh* mesh = new t_mesh;
		mesh->material = m_materials[obj.materialID];
		//对应face vert idx和tv idx，假如相同的顶点索引，拥有不同的tv索引，则复制顶点

		multimap<int , tVertInfo> vertMap;
		vector<t_drawVert> mesh_vb;//新生成的vertex buf
		vector<int> mesh_ib;//新生成的index buf
		for (int f=0;f<obj.numOfFaces;++f)
		{
			//add faces
			//printDebug("add face %d\n",f);
			const tFace& face = obj.pFaces[f];

			mesh_ib.push_back(face.vertIndex[0]);
			mesh_ib.push_back(face.vertIndex[1]);
			mesh_ib.push_back(face.vertIndex[2]);
			

		}

		myassert(obj.numOfVerts==obj.numTexVertex,"OBJ读取错误，顶点位置坐标和贴图坐标的数量不相等");
		CVector3* pos=obj.pVerts,*norm = obj.pNormals;
		CVector2* tc = obj.pTexVerts;
		for (int i=0;i<obj.numOfVerts;++i,++pos,++norm,++tc)
		{
			t_drawVert v;
			v.pos.Set(pos->x,pos->y,pos->z,1);
			v.tex.Set(tc->x,1.f-tc->y,0,1);
			v.norm.Set(norm->x,norm->y,norm->z,0);
			mesh_vb.push_back(v);
		}

		//now , copy all verts to mesh
		mesh->createVB(mesh_vb.size()); // new t_drawVert[ mesh_vb.size()];

		mesh->ib = new unsigned int [mesh_ib.size()];
		mesh->numIndex = (int)mesh_ib.size();
		mesh->numVert = (int)mesh_vb.size();
		mesh->numTri = obj.numOfFaces;
		myassert(obj.numOfFaces*3==mesh->numIndex,"3DS 文件读取错误");
		mesh->vb_format = VB_POS_TEX_NORM;
		mesh->tris = new t_triangle[mesh->numTri];
		float bbox[6]={9999,9999,9999,-9999,-9999,-9999};
		for (int i=0;i<mesh->numVert;++i)
		{
			*(mesh->vb+ i )= mesh_vb[i];
			if (mesh_vb[i].pos.x< bbox[0])
				 bbox[0]=mesh_vb[i].pos.x;
			if (mesh_vb[i].pos.y< bbox[1])
				bbox[1]=mesh_vb[i].pos.y;
			if (mesh_vb[i].pos.z< bbox[2])
				bbox[2]=mesh_vb[i].pos.z;
			if (mesh_vb[i].pos.x> bbox[3])
				bbox[3]=mesh_vb[i].pos.x;
			if (mesh_vb[i].pos.x> bbox[4])
				bbox[4]=mesh_vb[i].pos.y;
			if (mesh_vb[i].pos.z> bbox[5])
				bbox[5]=mesh_vb[i].pos.z;

		}
		for (int i=0;i<mesh->numIndex;++i)
		{
			*(mesh->ib+i )= mesh_ib[i];
		}

		for (int i=0;i<mesh->numTri;++i)
		{
			t_triangle* tri = mesh->tris+i;
			tri->material_idx = obj.materialID;
			tri->p[0] = mesh->ib[i*3];
			tri->p[1] = mesh->ib[i*3+1];
			tri->p[2] = mesh->ib[i*3+2];
			vec3 v01 = mesh_vb[tri->p[1]].pos - mesh_vb[tri->p[0]].pos;
			idVec3 point0(mesh_vb[tri->p[0]].pos.x,mesh_vb[tri->p[0]].pos.y,mesh_vb[tri->p[0]].pos.z);
			vec3 v02 = mesh_vb[tri->p[2]].pos - mesh_vb[tri->p[0]].pos;
			idVec3 idv01 (v01.x,v01.y,v01.z);
			idVec3 idv02 (v02.x,v02.y,v02.z);
			idv01.Normalize();
			idv02.Normalize();
			idv01 = idv01.Cross(idv02);
			idv01.NormalizeFast();
			tri->norm.Set(idv01.x,idv01.y,idv01.z,(idv01*point0));
			
		}
		m_meshs.push_back(mesh);
		
		t_entity * ent = new t_entity();
		ent->m_flags=ENTITY_MOVEABLE|ENTITY_LOCAL;
		ent->addMesh(mesh);
		ent->m_model_matrix.Identity();
		memcpy(ent->m_aabb,bbox,sizeof(float)*6);
		m_entity.push_back(ent);
	}


	//add a light
	clight_directional* light = new clight_directional;
	light->diffuse_col.Set(1,1,1,1);
	light->pos.Set(0,-1,-1,0);//a directional light
	light->pos.Normalize();
	m_lights.push_back(light);


	return 0;
#endif//debug
}

clight_directional* CScene::getLight(int n)
{
	if (n<(int)m_lights.size())
		return m_lights[n];
	else
		return 0;
}
t_entity* CScene::getEntity(int n)
{
	if (n<(int)m_entity.size())
		return m_entity[n];
	else
		return 0;
}
t_mesh* CScene::getMesh(int n)
{
	if (n<(int)m_meshs.size())
		return m_meshs[n];
	else
		return 0;
}
int CScene::clean()
{
	for (size_t i=0;i<m_entity.size();++i)
		delete m_entity[i];

	for (size_t i=0;i<m_meshs.size();++i)
		delete m_meshs[i];

	for (size_t i=0;i<m_materials.size();++i)
		delete m_materials[i];

	for (size_t i=0;i<m_textures.size();++i)
		delete m_textures[i];

	for (size_t i=0;i<m_images.size();++i)
		delete m_images[i];

	for (size_t i=0;i<m_lights.size();++i)
		delete m_lights[i];

	if (m_vb)
		delete m_vb;

	m_entity.clear();
	m_meshs.clear();
	m_materials .clear();
	m_textures.clear();
	m_images.clear();
	m_lights .clear();
	m_vb = 0;
	return 0;
}