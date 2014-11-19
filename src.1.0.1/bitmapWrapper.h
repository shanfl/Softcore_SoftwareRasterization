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
//简单的位图封装
//使用CImage 调入jpg,png,bmp位图
#ifndef __BITMAPWRAPPER
#define __BITMAPWRAPPER
#include "softrender_def.h"
namespace ATL
{
	class CImage;
}
class CBitmapWrapper
{
public :

	CBitmapWrapper(const char* filename);
	~CBitmapWrapper();
	 unsigned char* getBuffer();
	   int getBPP();
	   int getWidth();
	   int getHeight();
	   int getPitch();
	   
	   const char* getFileName();
public:
	
private:
	ATL::CImage* image; 
	CBitmapWrapper();
	string m_filename;
};
#endif