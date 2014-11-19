/*
modify by 姚勇 , 2008-7-16
作者未知，已修正致命内存错误
download from CSDN ...
*/
#include "stdafx.h"
#include "../softrender_def.h"
#include "3ds.h"
#include <Windows.h>
#include "../mmgr/mmgr.h"
namespace MAXOBJ
{
	t3DModel::~t3DModel()
	{
		for (int i=0;i<(int)pObject.size();++i)
		{
			delete pObject[i];
		}
	}
	t3DObject::~t3DObject()
	{
		delete []pFaces;
		delete []pVerts;
		delete []pNormals;
		delete []pTexVerts;
	}
	t3DObject::t3DObject()
	{
		memset(this,0,sizeof(t3DObject));
	}
//  construture initialize tChunk data
CLoad3DS::CLoad3DS()
{
//	m_CurrentChunk = new tChunk;				// alloc memory for current chunk
//	m_TempChunk = new tChunk;					// alloc memory for temp chunk
}

// read a 3ds file, read it,then free memory
bool CLoad3DS::Import3DS(t3DModel *pModel, char *strFileName)
{
	char strMessage[255] = {0};
	// open a 3ds file
	m_FilePointer = fopen(strFileName, "rb");

	// make sure file pointer correct
	if(!m_FilePointer) 
	{
		myassert(NULL, "无法打开3DS文件!");
		myassert(NULL, strFileName);
		return false;
	}

	// judge whether the file is a 3ds file, 3ds file the first chunk is PRIMARY
	tChunk* chunk  = new tChunk;
	ReadChunk(chunk);

	// make sure it is 3ds file
	if (chunk->ID != PRIMARY)
	{
		myassert(NULL,"Unable to load PRIMARY chuck from file");
			delete chunk;
		return false;
	}

	// read data from file,ProcessNextChunk()is a recursion function
	ProcessNextChunk(pModel, chunk);

	//after reading 3ds file, compute the normal of vertex
	ComputeNormals(pModel);

	//release memory
	delete chunk;
	CleanUp();
	return true;
}


// release memory and close file
void CLoad3DS::CleanUp()
{

	fclose(m_FilePointer);						// close file pointer
//	delete m_CurrentChunk;						// release current chunk
//	delete m_TempChunk;							// release tempt chunk
}



// read main part of 3ds file 
void CLoad3DS::ProcessNextChunk(t3DModel *pModel, tChunk *pPreviousChunk)
{
	t3DObject newObject ;					// add object link table
	tMaterialInfo newTexture = {0};				// add material link table
	unsigned int version = 0;					// save file version
	tChunk* m_CurrentChunk = new tChunk;				// alloc new memory for new chunk		

	// before read a chunk,judge the chunk ID, if it is need to read, go on,else jump over
	// read child chunk until the definite length
	while (pPreviousChunk->bytesRead < pPreviousChunk->length)
	{
		// read next chunk
		ReadChunk(m_CurrentChunk);

		// judge chunk ID
		switch (m_CurrentChunk->ID)
		{
		case VERSION:							// file version
			
			// in this chunk,there is an unsigned short int used for saving the file version 		
			// read file version, and all the byte count into bytesRead variable
			m_CurrentChunk->bytesRead += fread(&version, 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);

			// if file version if bigger than 3, give out a warning.
			if (version > 0x03)
				myassert(NULL, "This 3DS file is over version 3 so it may load incorrectly");
			break;

		case OBJECTINFO:
			ProcessNextChunk(pModel, m_CurrentChunk);
			break;

		case MATERIAL:							// material information

			// add count of material
			pModel->numOfMaterials++;

			// add a empty texture in texture link table
			pModel->pMaterials.push_back(newTexture);

			// go to material read in function
			ProcessNextMaterialChunk(pModel, m_CurrentChunk);
			break;

		case OBJECT:							// object name
			{

						
			// the chunk if head of object information, it contains the object name

			// add count of object

			pModel->numOfObjects++;
		
			// add a new object into object link table
			t3DObject* newObject = new t3DObject;
			pModel->pObject.push_back(newObject);
			
			//Init object and its data struct
			memset((pModel->pObject[pModel->numOfObjects - 1]), 0, sizeof(t3DObject));

			// get and save object name, then add the read bytes count
			m_CurrentChunk->bytesRead += GetString(pModel->pObject[pModel->numOfObjects - 1]->strName);
			
			// read other object information
			ProcessNextObjectChunk(pModel, (pModel->pObject[pModel->numOfObjects - 1]), m_CurrentChunk);
			}
			break;

		case EDITKEYFRAME:

			// jump key frame chunk, add read bytes count
			fseek(m_FilePointer, (long)(m_CurrentChunk->length - m_CurrentChunk->bytesRead), SEEK_CUR);
			m_CurrentChunk->bytesRead = m_CurrentChunk->length;
			break;

		default: 
			
			// jump unnecessary chunk, add the read byte count
			fseek(m_FilePointer, (long)(m_CurrentChunk->length - m_CurrentChunk->bytesRead), SEEK_CUR);
			m_CurrentChunk->bytesRead = m_CurrentChunk->length;
			break;
		}

		//add byte count read from the last chunk
		pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
	}

	// release current chunk 
	delete m_CurrentChunk;
	m_CurrentChunk = pPreviousChunk;
}


// deal with all object information
void CLoad3DS::ProcessNextObjectChunk(t3DModel *pModel, t3DObject *pObject, tChunk *pPreviousChunk)
{
	// alloc memory for new chunk
	tChunk* m_CurrentChunk = new tChunk;

	//read chunk until the tail of the chunk
	while (pPreviousChunk->bytesRead < pPreviousChunk->length)
	{
		//read next chunk
		ReadChunk(m_CurrentChunk);

		//set chunk into different kinds of chunk
		switch (m_CurrentChunk->ID)
		{
		case OBJECT_MESH:		
			// use recursion function
			ProcessNextObjectChunk(pModel, pObject, m_CurrentChunk);
			break;

		case OBJECT_VERTICES:				// read object vertex
			ReadVertices(pObject, m_CurrentChunk);
			break;

		case OBJECT_FACES:					// read object face
			ReadVertexIndices(pObject, m_CurrentChunk);
			break;

		case OBJECT_MATERIAL:				//read object material information
			
			// the chunk contain object material name, can be a color, can be a texture. and texture face
			// read material object name 
			ReadObjectMaterial(pModel, pObject, m_CurrentChunk);			
			break;

		case OBJECT_UV:						
			// read texture coordinate
			ReadUVCoordinates(pObject, m_CurrentChunk);
			break;

		default:  

			// jump no use chunk
			fseek(m_FilePointer, (long)(m_CurrentChunk->length - m_CurrentChunk->bytesRead), SEEK_CUR);
			m_CurrentChunk->bytesRead = m_CurrentChunk->length;
			break;
		}

		// add byte count read form the last chunk
		pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
	}

	// release current chunk, set current chunk as previous one
	delete m_CurrentChunk;
	m_CurrentChunk = pPreviousChunk;
}

//  deal with all material information
void CLoad3DS::ProcessNextMaterialChunk(t3DModel *pModel, tChunk *pPreviousChunk)
{
	// alloc memory for current chunk
	tChunk* m_CurrentChunk = new tChunk;

	// read these chunks until the tail of the chunk
	while (pPreviousChunk->bytesRead < pPreviousChunk->length)
	{
		// read next chunk
		ReadChunk(m_CurrentChunk);

		// judge the chunk that had been read
		switch (m_CurrentChunk->ID)
		{
		case MATNAME:							// material name			
			// read material name
			m_CurrentChunk->bytesRead += fread(pModel->pMaterials[pModel->numOfMaterials - 1].strName, 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
			break;

		case MATDIFFUSE:						// read object's color
			ReadColorChunk(&(pModel->pMaterials[pModel->numOfMaterials - 1]), m_CurrentChunk);
			break;
		
		case MATMAP:							// head of texture information
			
			// go into next material chunk information
			ProcessNextMaterialChunk(pModel, m_CurrentChunk);
			break;

		case MATMAPFILE:						// material file name

			// read material file name
			m_CurrentChunk->bytesRead += fread(pModel->pMaterials[pModel->numOfMaterials - 1].strFile, 1, m_CurrentChunk->length - m_CurrentChunk->bytesRead, m_FilePointer);
			break;
		
		default:  

			// jump no use chunk
			fseek(m_FilePointer, (long)(m_CurrentChunk->length - m_CurrentChunk->bytesRead), SEEK_CUR);
			m_CurrentChunk->bytesRead = m_CurrentChunk->length;
			break;
		}

		// add byte count read from the last chunk
		pPreviousChunk->bytesRead += m_CurrentChunk->bytesRead;
	}

	// delete current chunk, and set it to the previous one
	delete m_CurrentChunk;
	m_CurrentChunk = pPreviousChunk;
}

// read chunk ID and its length
void CLoad3DS::ReadChunk(tChunk *pChunk)
{
	//read chunk ID,occupy two bytes. chunk ID tell what is contained in the chunk
	pChunk->bytesRead = fread(&pChunk->ID, 1, 2, m_FilePointer);

	// read length, use for byte
	pChunk->bytesRead += fread(&pChunk->length, 1, 4, m_FilePointer);
}

//  read a string
int CLoad3DS::GetString(char *pBuffer)
{
	int index = 0;

	// read one byte data
	fread(pBuffer, 1, 1, m_FilePointer);

	while (*(pBuffer + index++) != 0) {

		// read one char until NULL
		fread(pBuffer + index, 1, 1, m_FilePointer);
	}

	// return string length
	return strlen(pBuffer) + 1;
}


//  read RGB color
void CLoad3DS::ReadColorChunk(tMaterialInfo *pMaterial, tChunk *pChunk)
{
	// read color chunk information
	tChunk *m_TempChunk = new tChunk;
	ReadChunk(m_TempChunk);

	// read in RGB color
	m_TempChunk->bytesRead += fread(pMaterial->color, 1, m_TempChunk->length - m_TempChunk->bytesRead, m_FilePointer);

	// add read in byte count
	pChunk->bytesRead += m_TempChunk->bytesRead;

	delete m_TempChunk;
}

//  read vertex index
void CLoad3DS::ReadVertexIndices(t3DObject *pObject, tChunk *pPreviousChunk)
{
	unsigned short index = 0;					// use for reading current face index

	// use for reading face count of the objec
	pPreviousChunk->bytesRead += fread(&pObject->numOfFaces, 1, 2, m_FilePointer);

	// malloc face space,and init their structure
	pObject->pFaces = new tFace [pObject->numOfFaces];
	memset(pObject->pFaces, 0, sizeof(tFace) * pObject->numOfFaces);

	// visit all faces of the object
	for(int i = 0; i < pObject->numOfFaces; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			// read the first vertex of the current face 
			pPreviousChunk->bytesRead += fread(&index, 1, sizeof(index), m_FilePointer);

			if(j < 3)
			{
				// save index into face structure
				pObject->pFaces[i].vertIndex[j] = index;
			}
		}
	}
}


// read object UV coordinate
void CLoad3DS::ReadUVCoordinates(t3DObject *pObject, tChunk *pPreviousChunk)
{
	// read count of UV coordinate, then read data

	// read UV coordinate 
	pPreviousChunk->bytesRead += fread(&pObject->numTexVertex, 1, 2, m_FilePointer);

	// alloc memory for UV coordinate
	pObject->pTexVerts = new CVector2 [pObject->numTexVertex];

	// read texture coordinate
	pPreviousChunk->bytesRead += fread(pObject->pTexVerts, 1, pPreviousChunk->length - pPreviousChunk->bytesRead, m_FilePointer);
}

//  read object vertex
void CLoad3DS::ReadVertices(t3DObject *pObject, tChunk *pPreviousChunk)
{
	// read vertex count
	pPreviousChunk->bytesRead += fread(&(pObject->numOfVerts), 1, 2, m_FilePointer);

	// alloc memory and init
	pObject->pVerts = new CVector3 [pObject->numOfVerts];
	memset(pObject->pVerts, 0, sizeof(CVector3) * pObject->numOfVerts);

	// read vertex
	pPreviousChunk->bytesRead += fread(pObject->pVerts, 1, pPreviousChunk->length - pPreviousChunk->bytesRead, m_FilePointer);

	// now all vertex had been read
	// as Z axis of 3D Studio Max model is up direction, so change y axis and z axis
	// change z axis direction

	// visit all vertex
	for(int i = 0; i < pObject->numOfVerts; i++)
	{
		// save y value
		float fTempY = pObject->pVerts[i].y;

		// change y value to z value
		pObject->pVerts[i].y = pObject->pVerts[i].z;

		// change z value to negative y value
		pObject->pVerts[i].z = -fTempY;
	}
}


//  read object material name
void CLoad3DS::ReadObjectMaterial(t3DModel *pModel, t3DObject *pObject, tChunk *pPreviousChunk)
{
	char strMaterial[255] = {0};			// use for saving material name
	// material or color, or object texture, or luminance,or radiation information

	// read current object material name
	pPreviousChunk->bytesRead += GetString(strMaterial);

	//visit all texture
	for(int i = 0; i < pModel->numOfMaterials; i++)
	{
		if(strcmp(strMaterial, pModel->pMaterials[i].strName) == 0)
		{
			// set material ID
			pObject->materialID = i;

			// Judge texture mapping,if length of strFile if bigger than one,it is texture mapping
			if(strlen(pModel->pMaterials[i].strFile) > 0) {

				// set object texture mapping flag
				pObject->bHasTexture = true;
			}	
			break;
		}
		else
		{
			// if object have no texture，set ID -1
			pObject->materialID = -1;
		}
	}
	fseek(m_FilePointer, (long)(pPreviousChunk->length - pPreviousChunk->bytesRead), SEEK_CUR);
	pPreviousChunk->bytesRead = pPreviousChunk->length;
}			
//  Compute vertex normal
void CLoad3DS::ComputeNormals(t3DModel *pModel)
{
	CVector3 vVector1, vVector2, vNormal, vPoly[3];

	// if no object in model, return
	if(pModel->numOfObjects <= 0)
		return;

	// visit all objects
	for(int index = 0; index < pModel->numOfObjects; index++)
	{
		//get current object
		t3DObject *pObject = (pModel->pObject[index]);

		// malloc memory
		CVector3 *pNormals		= new CVector3 [pObject->numOfFaces];
		CVector3 *pTempNormals	= new CVector3 [pObject->numOfFaces];
		pObject->pNormals		= new CVector3 [pObject->numOfVerts];

		// visit all object faces 
		for(int i=0; i < pObject->numOfFaces; i++)
		{												
			vPoly[0] = pObject->pVerts[pObject->pFaces[i].vertIndex[0]];
			vPoly[1] = pObject->pVerts[pObject->pFaces[i].vertIndex[1]];
			vPoly[2] = pObject->pVerts[pObject->pFaces[i].vertIndex[2]];

			// compute face normal

			vVector1 = (vPoly[0] -  vPoly[2]);		// get polygon vector
			vVector2 = (vPoly[2] - vPoly[1]);		

			vNormal  = vVector1.Cross(vVector2);		// cross two vector
			pTempNormals[i] = vNormal;					
			vNormal  =vNormal.Normalize();

			pNormals[i] = vNormal;						// add vector to vector table
		}

		//  compute vertex normal
		CVector3 vSum(0.0, 0.0, 0.0);
		CVector3 vZero = vSum;
		int shared=0;
		// visit all vertex
		for (int i = 0; i < pObject->numOfVerts; i++)			
		{
			for (int j = 0; j < pObject->numOfFaces; j++)	// visit all triangle face
			{												// judge whether vertex is shared with other face 
				if (pObject->pFaces[j].vertIndex[0] == i || 
					pObject->pFaces[j].vertIndex[1] == i || 
					pObject->pFaces[j].vertIndex[2] == i)
				{
					vSum = (vSum + pTempNormals[j]);
					shared++;								
				}
			}      
			
			pObject->pNormals[i] = (vSum / float(-shared));

			// normalize the last vertex
			pObject->pNormals[i] = pObject->pNormals[i].Normalize();

			vSum = vZero;								
			shared = 0;										
		}
	
		//release memory
		delete [] pTempNormals;
		delete [] pNormals;
	}
}
}