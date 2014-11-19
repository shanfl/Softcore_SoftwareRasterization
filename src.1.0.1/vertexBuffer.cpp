#include "stdafx.h"
#include "softrender_def.h"
#include "vertexBuffer.h"
#include "mmgr/mmgr.h"
/*

VB_UNKNOW=0,
VB_POS =1,//use global color to fill scan line
VB_POS_COL=2,//use vertex color to modulate scan line pixel
VB_POS_NORM=2,//only use vertex lighting
VB_POS_TEX=2,//use tex mapping
VB_POS_TEX_NORM=3,//tex with lighting
VB_POS_TEX_COL=3,//tex with vertex col
VB_POS_NORM_TB=2,//phong shading without texture
VB_POS_TEX_NORM_TB=3,//phong shading with texture&bump
VB_MAX_FMT,
*/
//strip in byte
int g_strip[]=
{
0,//0
4*sizeof(float),//1
8*sizeof(float),//2 pos, col
12*sizeof(float),//3//pos,col,tex
};
CVertexBuffer::CVertexBuffer(int vert_num,VB_FMT fmt)
{
	if (fmt<VB_MAX_FMT &&fmt>0)
		m_strip = g_strip[fmt];
	else
	{
		myassert(0,"非法VB格式");
	}

	m_vert_num=m_size=0;
	if (!vert_num)
		return;


	m_buffer = (char*)_aligned_malloc(m_strip* vert_num,ALIGNED_BYTE);
	m_format =fmt;
	m_vert_num = vert_num;
	m_size = m_strip* vert_num;
}
CVertexBuffer::~CVertexBuffer()
{
	_aligned_free(m_buffer);
}

int CVertexBuffer::resize(int numVert)
{
	if (!m_buffer  || numVert<=m_vert_num  )
	{
		myassert(0,"vb重新调整大小出错");
		
	}
	char* buffer = (char*)_aligned_malloc(m_strip* numVert,ALIGNED_BYTE);
	//TODO: need SIMD memcpy version
	memcpy(buffer,m_buffer,m_strip* m_vert_num);
	_aligned_free(m_buffer);
	m_buffer = buffer;
	int n = m_vert_num;
	m_vert_num = numVert;
	m_size = m_strip* numVert;
	return n;

}
