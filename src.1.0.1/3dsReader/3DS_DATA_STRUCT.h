/*
modify by 姚勇 , 2008-7-16
作者未知，已修正致命内存错误
download from CSDN ...
*/
#pragma once
#include <math.h>
#include <vector>
using namespace std;
namespace MAXOBJ
{


class CVector3
{
public:

	// default construction
	CVector3() {}

	// user construction
	CVector3(float X, float Y, float Z) 
	{ 
		x = X; y = Y; z = Z;
	}

	void operator=(CVector3 vVector)
	{
		// return value
		x = vVector.x;
		y = vVector.y;
		z = vVector.z;
	}
	// define iperator + 
	CVector3 operator+(CVector3 vVector)
	{
		// return value
		return CVector3(vVector.x + x, vVector.y + y, vVector.z + z);
	}

	// define operatro "-" 
	CVector3 operator-(CVector3 vVector)
	{
		// return result
		return CVector3(x - vVector.x, y - vVector.y, z - vVector.z);
	}

	// define operator "*" with a number
	CVector3 operator*(float num)
	{
		// return result
		return CVector3(x * num, y * num, z * num);
	}

	// define operator "/" with a number
	CVector3 operator/(float num)
	{
		// return result
		return CVector3(x / num, y / num, z / num);
	}
	CVector3 Cross(CVector3 vVector2)
	{
		CVector3 vNormal;	

		// compute vertical vector
		vNormal.x = ((y * vVector2.z) - (z * vVector2.y));
		vNormal.y = ((z * vVector2.x) - (x * vVector2.z));
		vNormal.z = ((x * vVector2.y) - (y * vVector2.x));

		// return result
		return vNormal;										 
	}
	//  compute vector length
	float Magnitude()
	{
		return (float)sqrt( (x * x) +(y * y) +(z * z) );
	}


	//  unit a vector
	CVector3 Normalize()
	{
		float magnitude = Magnitude();				
		return CVector3(x/magnitude,y/magnitude,z/magnitude);										
	}
public:
	float x, y, z;						
};
class CVector2 
{
public:
	float x, y;
};

// face structure
struct tFace
{
	int vertIndex[3];			// vertex index
	int coordIndex[3];			// texture coordinate index
};

//  material information structure
struct tMaterialInfo
{
	char			strName[255];			// texture name
	char			strFile[255];			// if exist texture,it is texture name
	unsigned char   color[3];				// object RGB color
	int				texureId;				// texture ID
	float			uTile;					// u repeat
	float			vTile;					// v repeat
	float			uOffset;			    // u offset
	float			vOffset;				// v offset
} ;

//  object information structure
struct t3DObject 
{
	t3DObject();
	~t3DObject();
	int  numOfVerts;			// vertex count
	int  numOfFaces;			// face count
	int  numTexVertex;			// texture count
	int  materialID;			// texture ID
	bool bHasTexture;			// texture flag
	char strName[255];			// object name
	CVector3  *pVerts;			// object vertex
	CVector3  *pNormals;		// object normal
	CVector2  *pTexVerts;		// texture UV coordinate
	tFace *pFaces;				// object face information
};

//  model information structure
struct t3DModel 
{
	t3DModel()
	{
		numOfObjects = 0;
		numOfMaterials = 0;
	}
	~t3DModel();

	int numOfObjects;					// object count
	int numOfMaterials;					// material count
	vector<tMaterialInfo> pMaterials;	// material link table information
	/*
	modify by puzzy, 2008-7-21
	it really sucks, the coder of original version does not know any C++&STL at all...
	here is the shit in the original file....
	//vector<t3DObject> pObject;			// object link table information
	*/
	vector<t3DObject*> pObject;			// object link table information
};
}