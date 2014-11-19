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
#include "atlimage.h"
#include "bitmapWrapper.h"

#include "mmgr/mmgr.h"


CBitmapWrapper::CBitmapWrapper(const char* filename)
{
	image = new ATL::CImage;
	HRESULT res = image->Load(filename);
	if (res!=S_OK)
	{
		myassert(0,"调入贴图失败");
		if (filename)
			myassert(0,filename);

		delete image;
		image =0;
	}
}
CBitmapWrapper::~CBitmapWrapper()
{
	if (image) 
		delete image;
}
const char* CBitmapWrapper::getFileName()
{
	static string strNull="null string";
	if (!m_filename.empty()) 
		return m_filename.c_str();
	else
		return strNull.c_str();
}
  unsigned char* CBitmapWrapper::getBuffer()
 {
	 if (image) 
		return (unsigned char*)image->GetBits( );
	 else
		 return 0;
 }

  int CBitmapWrapper::getBPP()
 {

	 if (image) 
	 {
		 myassert(image->GetBPP()>16,"位图不支持8和16位");
		return image->GetBPP();
	 }
	 else
		 return 0;
 }
  int CBitmapWrapper::getWidth()
 {	 
	 if (image) 
		return image->GetWidth();
	else
		return 0;
 }
  int CBitmapWrapper::getHeight()
 {
	 if (image) 
		return image->GetHeight();
	else
		 return 0;
 }
  int CBitmapWrapper::getPitch()
 {	
	 if (image) 
		return image->GetPitch();
	else
		return 0;
 }