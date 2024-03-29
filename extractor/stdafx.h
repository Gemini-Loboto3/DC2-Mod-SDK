// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once
#define TESTDLL_EXPORTS

#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "filehandle.h"
//#include "console.h"

// TODO: reference additional headers your program requires here
#ifdef TESTDLL_EXPORTS
#define TESTDLL_API __declspec(dllexport)
#else
#define TESTDLL_API __declspec(dllimport)
#endif

#include <typos.h>

#define MAGIC(a,b,c,d)		(a | (b<<8) | (c<<16) | (d<<24))
#define Align(val,align)	((((val-1)/align)+1)*align)

static __inline int align(int val, int align) { return Align(val, align); }

class CBufferFile
{
public:
	CBufferFile() { data = NULL; size = 0; }
	CBufferFile(LPCSTR filename)
	{
		data = NULL;
		size = 0;
		Open(filename);
	}
	~CBufferFile() { if (data) delete[] data; }

	bool Open(LPCSTR filename)
	{
		CFile f(filename);
		if (!f.IsOpen())
		{
			return 0;
		}

		size = f.GetSize();

		data = new u8[size];
		f.Read(data, size);
		return 1;
	}

	__inline size_t GetSize()   { return size; }
	__inline u8    *GetBuffer() { return data; }

	u8 *data;
	size_t size;
};
