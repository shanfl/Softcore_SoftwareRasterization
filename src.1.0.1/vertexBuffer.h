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

//用于context内部渲染顶点缓冲对象
#ifndef _VERTEXBUFFER
#define _VERTEXBUFFER
#include "softrender_def.h"
//用于不同光栅化管道

class CVertexBuffer
{
public:
	CVertexBuffer(int ,VB_FMT);
	~CVertexBuffer();

	//扩展vb容量，返回原始大小
	int resize(int numVert);

	//should 16bytes aligned,
	//just use microsoft's _aligned_malloc (size,16) and _aligned_free  is ok
	char* m_buffer;
	VB_FMT m_format;
	int m_strip;
	int m_vert_num;
	int m_size;
private:
	CVertexBuffer();
};
class CIndexBuffer
{

};
#endif