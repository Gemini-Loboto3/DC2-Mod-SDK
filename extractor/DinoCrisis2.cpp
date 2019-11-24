#include <stdafx.h>
#include "DinoCrisis2.h"

void UnswizzleGfx(DC2_ENTRY_GFX* gfx_p, u8* buf_p)
{
	// create temp copy buffer
	u8* buffer = new u8[gfx_p->size];
	memcpy(buffer, buf_p, gfx_p->size);

	u8* bcopy = buffer;
	int tw = gfx_p->w / 32;	// horizontal counter
	int bw = tw * 64;		// tim scanline size
	// rearrange swizzled texture
	for (int yi = 0; yi < gfx_p->h; yi += 32)
	{
		for (int xi = 0; xi < tw; xi++)
		{
			// get to current dest scanline chunk
			u8* scanline = &buf_p[yi * bw + xi * 64];
			// blit 32 scanlines
			for (int j = 0; j < 32; j++)
			{
				memcpy(scanline, bcopy, 64);
				bcopy += 64;			// next swizzled chunk
				scanline += bw;		// next pixel scanline
			}
		}
	}

	// remove temp copy
	delete[] buffer;
}

/*
Decompress data from Dino Crisis 1 & 2
	- src: Compressed source data
	- dst: Empty buffer which will hold the decompressed
		data. This is handled as a reference, so when
		returning the pointer will be changed to reflect
		the new buffer location.
	- src_size: Byte size of the compressed buffer.
Returns: dst size.
*/
u32 Dc2LzssDec(u8* src, u8*& dst, u32 src_size)
{
	// bit flag for decompression
	// (when its value is 1 the decompressor will read a new flag)
	int flag = 1;
	// allocate a buffer big enough for holding the decompressed data
	dst = new u8[src_size * 8];
	// temp buffer for killing the decompression loop
	u8* end = &src[src_size];
	// to be used when the decompression is complete
	u8* dec = dst;

	while (src < end)
	{
		// read flag and add extra bit for fancy math
		if (flag == 1)
			flag = *src++ | 0x100;
		// read next byte
		u8 ch = *src++;
		// process literal
		if (flag & 1)
			* dec++ = ch;
		// compressed sequence
		else
		{
			// read additional byte
			u8 t = *src++;
			// calculate jump and size
			int jump = ((t & 0xF) << 8) | ch;
			int size = (t >> 4) + 2;
			// seek to decompression buffer and copy to dest
			u8* src_dec = &dec[-jump];
			for (int i = 0; i < size; i++)
				* dec++ = *src_dec++;
		}
		// next flag
		flag >>= 1;
	}

	// return decompressed size
	return (u32)(dec - dst);
}

////////////////////////////////////
CPackageDC::CPackageDC()
{
}

CPackageDC::~CPackageDC()
{
	Reset();
}

void CPackageDC::Reset()
{
	ent.clear();
	while (!segment.empty()) { delete[] segment.back(); segment.pop_back(); }
}

void CPackageDC::Open(LPCTSTR filename, bool is_pc)
{
	CBufferFile b(filename);

	if (b.GetSize() == 0) return;

	if(is_pc) OpenPC((u8*)b.GetBuffer());
	else Open((u8*)b.GetBuffer());
}

size_t CPackageDC::Open(u8* data)
{
	// clear lists
	Reset();

	// temp buffers
	u8* seek, * entry_d = data;

	// size values
	u32* check = (u32*)& data[16];
	int entry_size = 16;
	pack_type = Type_DC1;
	// determine if entries use 16 bytes (DC1/2) or 32 bytes (DC2)
	if (check[0] == 0 && check[1] == 0 && check[2] == 0 && check[3] == 0)
	{
		pack_type = Type_DC2;
		entry_size = 32;
	}

	size_t pos = 2048;
	// process all the entries and cache all the data to the lists
	for (int i = 0, si = 2048 / entry_size;; i++, entry_d += entry_size)
	{
		// assign current entry
		DC2_ENTRY_GENERIC* entry = (DC2_ENTRY_GENERIC*)entry_d;
		// temp caching buffer
		u8* buffer = NULL;
		// copy padded size for sector seek
		int ssize = align(entry->size, 2048);
		// set to current data segment
		seek = &data[pos];

		switch (entry->type)
		{
		case GET_TEXTURE:	// stripped TIM pixel: deswizzle and add to segment list
			// allocate a sector padded buffer
			buffer = new u8[align(entry->size, 2048)];
			// clear buffer and copy pixel data
			ZeroMemory(buffer, align(entry->size, 2048));
			memcpy(buffer, seek, entry->size);
			// permanently pad size
			entry->size = align(entry->size, 2048);
			// unswizzle graphics
			UnswizzleGfx((DC2_ENTRY_GFX*)entry, buffer);
			break;
		case GET_LZSS0:		// compressed data: decompress
			entry->size = Dc2LzssDec(seek, buffer, entry->size);
			break;
		case GET_LZSS1:		// compressed TIM pixel: decompress and deswizzle
		{
			u8* temp;
			// decompress
			entry->size = Dc2LzssDec(seek, temp, entry->size);
			// allocate a sector padded buffer
			buffer = new u8[align(entry->size, 2048)];
			// clear buffer and copy decompressed pixel
			ZeroMemory(buffer, align(entry->size, 2048));
			memcpy(buffer, temp, entry->size);
			// we don't need the old buffer anymore
			delete[] temp;
			// permanently pad size
			entry->size = align(entry->size, 2048);
			// unswizzle graphics
			UnswizzleGfx((DC2_ENTRY_GFX*)entry, buffer);
		}
		break;
		case GET_DATA:		// generic literal data
		case GET_PALETTE:	// stripped TIM clut
		case GET_SNDH:		// VAG header 'Gian'
		case GET_SNDB:		// VAG body
		case GET_SNDE:		// configuration for sound samples?
		case GET_UNK:		// unknown type, no idea what this does
			// allocate buffer and copy data into it
			buffer = new u8[entry->size];
			memcpy(buffer, seek, entry->size);
			break;
		default:
			i = si;			// prevent any further processing
			break;
		}

		// for break condition
		// needs to be here in order to prevent incorrect caching
		if (i == si) break;

		// add current entry to the entry list
		ent.push_back(*entry);
		// add cached buffer to the segment list
		segment.push_back(buffer);

		// seek to the next sector containing data
		pos += ssize;
	}

	return pos;
}

size_t CPackageDC::OpenPC(u8* data)
{
	// clear lists
	Reset();

	// temp buffers
	u8* seek, * entry_d = data;

	// size values
	u32* check = (u32*)&data[16];
	int entry_size = 16;
	pack_type = Type_DC1;
	// determine if entries use 16 bytes (DC1/2) or 32 bytes (DC2)
	if (check[0] == 0 && check[1] == 0 && check[2] == 0 && check[3] == 0)
	{
		pack_type = Type_DC2;
		entry_size = 32;
	}

	size_t pos = 2048;
	// process all the entries and cache all the data to the lists
	for (int i = 0, si = 2048 / entry_size;; i++, entry_d += entry_size)
	{
		// assign current entry
		DC2_ENTRY_GENERIC* entry = (DC2_ENTRY_GENERIC*)entry_d;
		// temp caching buffer
		u8* buffer = NULL;
		// copy padded size for sector seek
		int ssize = align(entry->size, 2048);
		// set to current data segment
		seek = &data[pos];

		switch (entry->type)
		{
		case GPC_TEXTURE:	// stripped TIM pixel: deswizzle and add to segment list
			// allocate a sector padded buffer
			buffer = new u8[align(entry->size, 2048)];
			// clear buffer and copy pixel data
			ZeroMemory(buffer, align(entry->size, 2048));
			memcpy(buffer, seek, entry->size);
			// permanently pad size
			entry->size = align(entry->size, 2048);
			// unswizzle graphics
			UnswizzleGfx((DC2_ENTRY_GFX*)entry, buffer);
			break;
		case GPC_LZSS0:		// compressed data: decompress
		case GPC_LZSS2:
			entry->size = Dc2LzssDec(seek, buffer, entry->size);
			break;
		case GPC_LZSS1:		// compressed TIM pixel: decompress and deswizzle
		{
			u8* temp;
			// decompress
			entry->size = Dc2LzssDec(seek, temp, entry->size);
			// allocate a sector padded buffer
			buffer = new u8[align(entry->size, 2048)];
			// clear buffer and copy decompressed pixel
			ZeroMemory(buffer, align(entry->size, 2048));
			memcpy(buffer, temp, entry->size);
			// we don't need the old buffer anymore
			delete[] temp;
			// permanently pad size
			entry->size = align(entry->size, 2048);
			// unswizzle graphics
			UnswizzleGfx((DC2_ENTRY_GFX*)entry, buffer);
		}
		break;
		case GPC_SOUND:
		case GPC_MP3:
		case GPC_DATA:		// generic literal data
		case GPC_PALETTE:	// stripped TIM clut
		case GPC_UNK:		// this seems to be used just by the font
			// allocate buffer and copy data into it
			buffer = new u8[entry->size];
			memcpy(buffer, seek, entry->size);
			break;
		default:
			i = si;			// prevent any further processing
			break;
		}

		// for break condition
		// needs to be here in order to prevent incorrect caching
		if (i == si) break;

		// add current entry to the entry list
		ent.push_back(*entry);
		// add cached buffer to the segment list
		segment.push_back(buffer);

		// seek to the next sector containing data
		pos += ssize;
	}

	return pos;
}

static LPCSTR type_ext[] =
{
	"bin",
	"tex",
	"pal",
	"gnh",
	"gns",
	"gnt",
	"unk",
	"bin",
	"tex"
};

int CPackageDC::GetIDByAddress(u32 address)
{
	//0x80162500
	for (int i = 0; i < (int)segment.size(); i++)
	{
		if (ent[i].reserve[0] == address)
			return i;
	}

	return -1;
}

void CPackageDC::ExtractRaw(LPCTSTR dest)
{
	//GString str, path;
	//CMarkup xml;

	////xml.SetDoc(_T("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"));
	//xml.AddElem(_T("DinoCrisisPackage"));
	//xml.AddAttrib(_T("type"), pack_type);

	//// derive path to xml folder
	//str = dest;
	//int pos = str.ReverseFind(_T('\\'));
	//path.Copy(dest, pos + 1);

	//// extract data and append to xml project
	//for (int i = 0, si = (int)ent.size(); i < si; i++)
	//{
	//	str.Format(_T("%s\\%02d.%s"), (LPCTSTR)path, i, type_ext[ent[i].type]);
	//	FlushFile(str, segment[i], ent[i].size);

	//	str.Format(_T("%02d.%s"), i, type_ext[ent[i].type]);
	//	xml.AddChildElem(_T("Entry"), str);
	//	xml.IntoElem();

	//	xml.AddAttrib(_T("type"), ent[i].type);
	//	// set all human-readable data here
	//	switch (ent[i].type)
	//	{
	//	case GET_TEXTURE:
	//	case GET_PALETTE:
	//	case GET_LZSS1:
	//	{
	//		DC2_ENTRY_GFX* g = (DC2_ENTRY_GFX*)& ent[i];
	//		xml.AddAttrib(_T("x"), g->x);
	//		xml.AddAttrib(_T("y"), g->y);
	//		xml.AddAttrib(_T("w"), g->w);
	//		xml.AddAttrib(_T("h"), g->h);
	//	}
	//	break;
	//	default:
	//		str.Format(_T("0x%X"), ent[i].reserve[0]);
	//		xml.AddAttrib(_T("address"), str);
	//		xml.AddAttrib(_T("size"), ent[i].size);
	//	}
	//	//
	//	xml.OutOfElem();
	//}

	//// flush xml data
	//str = xml.GetDoc();
	//FILE* out = _tfopen(dest, _T("wb+"));
	//WriteBOM(out);
	//WriteUtf8(str, out);
	//fclose(out);
}

void CPackageDC::ExtractRawPC(LPCSTR dest)
{

}
