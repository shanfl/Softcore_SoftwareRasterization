/*
modify by 姚勇 , 2008-7-16
作者未知，已修正致命内存错误
download from CSDN ...
已知问题：不同贴图坐标和法线下的顶点复制规则错误
此类库只能用于教学示范，不可用于商业软件
*/
#ifndef _3DS_H
#define _3DS_H
#include "3DS_DATA_STRUCT.h"
namespace MAXOBJ
{

//  Primary Chunk in header of the file
#define PRIMARY       0x4D4D

//  Main Chunks
#define OBJECTINFO    0x3D3D				// mesh object version
#define VERSION       0x0002				// .3ds file version
#define EDITKEYFRAME  0xB000				// all key fram header information


#define MATERIAL	  0xAFFF				// texture information
#define OBJECT		  0x4000				// face and vertex information

#define MATNAME       0xA000				// material name
#define MATDIFFUSE    0xA020				// color
#define MATMAP        0xA200				// new material header
#define MATMAPFILE    0xA300				// texture file name

#define OBJECT_MESH   0x4100				// new mesh object

//  OBJECT_MESH
#define OBJECT_VERTICES     0x4110			// object vertex
#define OBJECT_FACES		0x4120			// object face
#define OBJECT_MATERIAL		0x4130			// object material
#define OBJECT_UV			0x4140			// object uv coordinate

struct tIndices 
{							
	unsigned short a, b, c, bVisible;	
};

// chunk information structure
struct tChunk
{
	unsigned short int ID;					// chunk id		
	unsigned int length;					// length of chunk
	unsigned int bytesRead;					// byte count need to read
};

// CLoad3DS
class CLoad3DS
{
public:
	CLoad3DS();								// init variable
	//import from 3ds file
	bool Import3DS(t3DModel *pModel, char *strFileName);
	// close file and release memory
	void CleanUp();

private:
	// read in a string
	int GetString(char *);
	// read next chunk
	void ReadChunk(tChunk *);
	// process next chunk
	void ProcessNextChunk(t3DModel *pModel, tChunk *);
	// process next object chunk
	void ProcessNextObjectChunk(t3DModel *pModel, t3DObject *pObject, tChunk *);
	// process next material chunk
	void ProcessNextMaterialChunk(t3DModel *pModel, tChunk *);
	// read object RGB value
	void ReadColorChunk(tMaterialInfo *pMaterial, tChunk *pChunk);
	// read object vertex information
	void ReadVertices(t3DObject *pObject, tChunk *);
	// read object face information
	void ReadVertexIndices(t3DObject *pObject, tChunk *);
	// read object texture coordinate
	void ReadUVCoordinates(t3DObject *pObject, tChunk *);
	// read object material name
	void ReadObjectMaterial(t3DModel *pModel, t3DObject *pObject, tChunk *pPreviousChunk);
	// compute object normal
	void ComputeNormals(t3DModel *pModel);
	// file pointer
	FILE *m_FilePointer;

//	tChunk *m_CurrentChunk;
	tChunk *m_TempChunk;
};

}
#endif


