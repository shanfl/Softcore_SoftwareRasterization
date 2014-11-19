/********************************************************************
Copyright (C) 2008 Ò¦ÓÂ- All Rights Reserved
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
//¹Ø¿¨¹ÜÀí
#ifndef _SCENE_LEVEL
#define _SCENE_LEVEL
#include "softRender_def.h"
#include <vector>
using namespace std;
class CVertexBuffer;
class CBitmapWrapper;
class CScene
{
public:
	CScene();
	~CScene();

	int load(const char* );

	t_entity* getEntity(int n);
	t_mesh* getMesh(int n);
	int getEntityCount()const {return (int)m_entity.size();};
	int getMeshCount()const {return (int)m_meshs.size();};
	clight_directional* getLight(int n);
	int getLightCount()const{return (int)m_lights.size();};
	int clean();
	CBitmapWrapper* findImage(const string& path);

	static CBitmapWrapper* s_default_img;
protected:

	int forDebug();
	vector< t_mesh*> m_meshs;
	vector<t_material*> m_materials;
	vector<t_texture*> m_textures;
	vector<CBitmapWrapper*> m_images;
	vector<t_entity*> m_entity;
	vector<clight_directional*> m_lights;

	//dynamic vb
	CVertexBuffer* m_vb;
};
#endif