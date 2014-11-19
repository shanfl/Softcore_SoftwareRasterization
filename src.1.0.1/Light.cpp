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
#include "softrender_def.h"
void clight_directional::caculateCol(const vec3 &pos,const vec3 &norm,const t_vlight& vl,vec4 &col)
{
	//为了简化这里都当作方向光
	float dot = vl.light_objpos*norm;
	if (dot>0)
	{
		col.Set(0,0,0,0);
		return;
	}
	dot=-dot;
	vec4 result = diffuse_col* dot;
	col[0] = result.x;col[1] = result.y;col[2] = result.z;col[3] = diffuse_col.w;
}