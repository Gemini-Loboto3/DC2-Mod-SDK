// extractor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include "DinoCrisis2.h"
#include "Bitmap.h"
#include "hash64.h"
#include "tinyxml2.h"
#include "patch.h"
#include "psound.h"

#include <string>
#include <vector>

// helper class to read exe segments
class PExe
{
public:
	bool Open(LPCSTR name)
	{
		PIMAGE_DOS_HEADER dosHeader;		//Pointer to DOS Header
		PIMAGE_NT_HEADERS ntHeader;			//Pointer to NT Header
		PIMAGE_SECTION_HEADER pSecHeader;	//Section Header or Section Table Header

		CBufferFile f(name);
		if (f.GetBuffer() == nullptr || f.GetSize() == 0)
			return false;
		dosHeader = (PIMAGE_DOS_HEADER)f.GetBuffer();
		// check for valid DOS file
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			return false;

		ntHeader = (PIMAGE_NT_HEADERS)((DWORD)(dosHeader)+(dosHeader->e_lfanew));
		// identify valid signature
		if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
			return false;

		// gather information about segments
		pSecHeader = IMAGE_FIRST_SECTION(ntHeader);
		for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++, pSecHeader++)
		{
			if (_stricmp((char*)pSecHeader->Name, ".data") == 0 ||
				_stricmp((char*)pSecHeader->Name, "data") == 0)
			{
				data = std::vector<BYTE>(pSecHeader->SizeOfRawData);
				memcpy(&data[0], (void*)((DWORD)dosHeader + pSecHeader->PointerToRawData), pSecHeader->SizeOfRawData);
				data_addr = ntHeader->OptionalHeader.ImageBase + pSecHeader->VirtualAddress;
			}
			else if (_stricmp((char*)pSecHeader->Name, ".rdata") == 0 ||
				_stricmp((char*)pSecHeader->Name, "rdata") == 0)
			{
				rdata = std::vector<BYTE>(pSecHeader->SizeOfRawData);
				memcpy(&rdata[0], (void*)((DWORD)dosHeader + pSecHeader->PointerToRawData), pSecHeader->SizeOfRawData);
				rdata_addr = ntHeader->OptionalHeader.ImageBase + pSecHeader->VirtualAddress;
			}
			else if (_stricmp((char*)pSecHeader->Name, ".text") == 0 ||
				_stricmp((char*)pSecHeader->Name, "text") == 0)
			{
				text = std::vector<BYTE>(pSecHeader->SizeOfRawData);
				memcpy(&text[0], (void*)((DWORD)dosHeader + pSecHeader->PointerToRawData), pSecHeader->SizeOfRawData);
				text_addr = ntHeader->OptionalHeader.ImageBase + pSecHeader->VirtualAddress;
			}
		}

		return true;
	}

	std::vector<BYTE> data,
		rdata,
		text;
	DWORD data_addr,
		rdata_addr,
		text_addr;
};

void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

using namespace tinyxml2;

void StringsToXml(std::vector<std::string>& str, LPCSTR out_name)
{
	XMLDocument xml;
	if (str.size() == 0) return;
	xml.SetBOM(true);

	XMLElement* items = xml.NewElement("Strings");
	// extract text
	for (size_t i = 0, si = str.size(); i < si; i++)
	{
		XMLElement* sub = xml.NewElement("Text");
		sub->SetText(str[i].c_str());
		items->InsertEndChild(sub);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);
}

void StringsToXml(std::vector<std::string>& str, LPCSTR out_name, int start, int end)
{
	XMLDocument xml;
	if (str.size() == 0) return;
	xml.SetBOM(true);

	XMLElement* items = xml.NewElement("Strings");
	// extract text
	for (size_t i = start, si = end; i < si; i++)
	{
		XMLElement* sub = xml.NewElement("Text");
		sub->SetText(str[i].c_str());
		items->InsertEndChild(sub);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);
}

void ListFiles(LPCSTR folder, LPCSTR filter, std::vector<std::string>& names)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char szDir[MAX_PATH];
	sprintf(szDir, "%s\\%s", folder, filter);
	WIN32_FIND_DATA ffd;

	// find the first file in the directory
	hFind = FindFirstFileA(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	// list all mod files in the main folder
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		names.push_back(ffd.cFileName);
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

void jpegtest(u8* data, size_t size, CBitmap& dst);

void test()
{
	char path[MAX_PATH];
	for (int Stage = 0; Stage <= 9; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			CPackageDC dc;

			sprintf_s(path, MAX_PATH, "PC JP\\ST%d%02X.DBS", Stage, Room);
			CBufferFile f(path);

			if (f.GetSize() == 0)continue;

			printf("Extracting %s\n", path);

			size_t pos = 0;
			for (int i = 0;; i++)
			{
				pos += dc.OpenPC(f.GetBuffer() + pos);
				if (pos >= f.GetSize())
					break;
				CBitmap bmp, bm2;
				// dump bg
				//sprintf_s(path, MAX_PATH, "ROOM\\ROOM%d%02X_%02d.jpg", Stage, Room, i);
				//FILE* fl = fopen(path, "wb");
				//fwrite(dc.segment[1], dc.ent[1].size, 1, fl);
				//fclose(fl);
				jpegtest(dc.segment[1], dc.ent[1].size, bmp);
				sprintf_s(path, MAX_PATH, "ROOM\\ROOM%d%02X_%02d.png", Stage, Room, i);
				bmp.SavePng(path);
				// -- dump extra mask --
				int mi = dc.SearchByType(GPC_LZSS2),
					pi = dc.SearchByType(GPC_PALETTE);
				if (mi != -1)
				{
					DC2_ENTRY_GFX* gfx = (DC2_ENTRY_GFX*)&dc.ent[mi];
					u8* img = dc.segment[mi];
					u16* pal = (u16*)dc.segment[pi];
					// convert the mask itself
					CBitmap bmp;
					bmp.Create(gfx->w * 2, gfx->h);
					for (int y = 0, sy = bmp.h; y < sy; y++)
						for (int x = 0, sx = bmp.w; x < sx; x++)
							bmp.setPixel(x, y, Convert_color(pal[*img++]));
					sprintf_s(path, MAX_PATH, "ROOM\\ROOM%d%02X_%02d_maskx.png", Stage, Room, i);
					bmp.SavePng(path);
				}
				// -- dump regular mask --
				int gi = dc.SearchByType(GPC_DATA, 2);
				if (gi == -1) continue;
				u8* temp;
				size_t dsize = Dc2LzssDec(dc.segment[gi], temp, dc.ent[gi].size);
				// generate
				u16* img = (u16*)temp;
				// fix transparency
				for (size_t j = 0; j < dsize / 2; j++) if (!(img[j] & 0x8000)) img[j] = 0;
				bmp.Create(18, dsize / 36);
				bmp.Fill(0);
				for (int y = 0, sy = bmp.h; y < sy; y++)
					for (int x = 0, sx = bmp.w; x < sx; x++)
						bmp.setPixel(x, y, Convert_color(*img++));
				bm2.Create(256, 256);
				bm2.Fill(0);
				// recomposite
				for (int j = 0, js = bmp.h / 18; j < js; j++)
					bm2.Blt(bmp, 1, j * 18 + 1, (j % 16) * 16, (j / 16) * 16, 16, 16);

				sprintf_s(path, MAX_PATH, "ROOM\\ROOM%d%02X_%02d_mask.png", Stage, Room, i);
				bm2.SavePng(path);
				delete[] temp;
			}
		}
	}
}

void DumpMusic()
{
	char path[MAX_PATH];
	std::vector<std::string> list;
	list.push_back("M_TITLE.DAT");
	ListFiles("PC JP", "M*.DAT", list);

	CreateDirectory("music", nullptr);
	FILE* fp;
	fopen_s(&fp, "music\\dupes.txt", "wt");

	X64Hash h;
	for (size_t i = 0, si = list.size(); i < si; i++)
	{
		CPackageDC dc;
		sprintf_s(path, MAX_PATH, "PC JP\\%s", list[i].c_str());
		dc.Open(path);

		for (size_t j = 0, sj = dc.GetCount(); j < sj; j++)
		{
			if (dc.ent[j].type != GPC_MP3)
				continue;

			u8* data = dc.segment[j];
			size_t size = dc.ent[j].size;
			
			//sprintf_s(path, MAX_PATH, "%s_%02d.mp3", list[i].c_str(), j);
			//int res = h.PushHash(data, size, path);
			//if (res != -1)
			//{
				//fprintf(fp, "%s->%s_%02d.mp3\n", h.hashes[res].n.c_str(), list[i].c_str(), j);
				//continue;
			//}

			if (j != dc.ent[j].reserve[0])
				fprintf(fp, "%s %d empty\n", list[i].c_str(), j);

			sprintf_s(path, MAX_PATH, "music\\%s_%02d.mp3", list[i].c_str(), dc.ent[j].reserve[0]);
			FILE* f = fopen(path, "wb");
			fwrite(data, size, 1, f);
			fclose(f);
		}
	}
	fclose(fp);
}

std::string DecodeString(u16* data, size_t* size);
std::string DecodeStringJp(u16* data);

void ExtractExeStrings()
{
	CBufferFile b("D:\\Program Files\\Dino Crisis 2\\dino2.exe");

	u16* text = (u16*)&b.GetBuffer()[0x108fcc];
	std::vector<std::string> str;

	for (int i = 0; i < 459; i++)
	{
		printf("%d\n", i);
		size_t size;
		str.push_back(DecodeString(text, &size));
		text += size;
	}

	StringsToXml(str, "test.xml");
}

void ExtractRoomStrings()
{
	LPCSTR types[] =
	{
		"dat",
		"tex",
		"pal",
		"snd",
		"mp3",
		"dat",
		"tex",
		"dat"
	};
	char path[MAX_PATH];


	for (int Stage = 0; Stage <= 9; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			CPackageDC dc;
			sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X.DAT", Stage, Room);
			dc.Open(path, true);
			if (dc.GetCount() == 0) continue;

			int id = dc.GetIDByAddress(0x5E0000);
			if (id == -1) continue;

			sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X\\", Stage, Room);
			CreateDirectory(path, nullptr);

			FILE* log;
			fopen_s(&log, (std::string(path) + "log.txt").c_str(), "wt");

			for (int i = 0, si = dc.GetCount(); i < si; i++)
			{
				if ((dc.ent[i].type == GPC_TEXTURE || dc.ent[i].type == GPC_LZSS1 ) && dc.ent[i + 1].type == GPC_PALETTE)
				{
					DC2_ENTRY_GFX* gfx = (DC2_ENTRY_GFX*)&dc.ent[i];
					DC2_ENTRY_GFX* pal = (DC2_ENTRY_GFX*)&dc.ent[i + 1];
					u8* d = dc.segment[i];
					u16* p = (u16*)dc.segment[i + 1];
					CBitmap bmp;
					bmp.Fill(0);
					if (pal->w == 256 && pal->h == 1)
					{
						bmp.Create(gfx->w * 2, gfx->h);
						for (int y = 0, sy = gfx->h; y < sy; y++)
							for (int x = 0, sx = bmp.w; x < sx; x++, d++)
								bmp.setPixel(x, y, Convert_color(p[*d]));
					}
					else
					{
						bmp.Create(gfx->w * 4, gfx->h);
						for (int y = 0, sy = gfx->h; y < sy; y++)
						{
							for (int x = 0, sx = bmp.w; x < sx; x += 2, d++)
							{
								bmp.setPixel(x, y, Convert_color(p[*d & 0xf]));
								bmp.setPixel(x + 1, y, Convert_color(p[*d >> 4]));
							}
						}
					}
					sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X\\%02d.png", Stage, Room, i);
					bmp.SavePng(path);
					// skip palettes
					i++;
				}
				else
				{
					fprintf(log, "%02d: %08X\n", i, dc.ent[i].reserve[0]);

					sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X\\%02d.%s", Stage, Room, i, types[dc.ent[i].type]);
					FILE* f = fopen(path, "wb");
					fwrite(dc.segment[i], dc.ent[i].size, 1, f);
					fclose(f);
				}
			}

			fclose(log);
		}
	}
}

#define FONT_W		28
#define FONT_H		28
#define FONT_T		(FONT_W * 16)
#define FONT_L		(FONT_T / FONT_W)
#define FONT_JP		1268
#define FONT_US		64

void convert_glyph(int x, int y, u8* fnt, CBitmap& dst, u32 *pal)
{
	for (int yi = 0; yi < FONT_H; yi++)
	{
		for (int xi = 0; xi < FONT_W; xi += 2, fnt++)
		{
			dst.setPixel(x + xi,     y + yi, pal[*fnt >> 4]);
			dst.setPixel(x + xi + 1, y + yi, pal[*fnt & 0xf]);
		}
	}
}

void extract_core()
{
	CPackageDC dc;
	dc.Open("PC US\\CORE.DAT", true);

	FILE *f = fopen("font_us", "wb");
	fwrite(&dc.segment[2][0], dc.ent[2].size, 1, f);
	fclose(f);

	//CBitmap bmp;
	//bmp.Create(FONT_T, 80 * FONT_H);

	//u32 pal[16];
	//for (int i = 1; i < 16; i++)
	//	pal[i] = RGB_BMP(i * 16, i * 16, i * 16);
	//pal[0] = RGB_BMP(0, 128, 128);

	//u8 *fnt = dc.segment[2];
	//for (int i = 0; i < 1268; i++)
	//{
	//	convert_glyph((i % FONT_L) * FONT_W, (i / FONT_L) * FONT_H, fnt, bmp, pal);
	//	fnt += FONT_W * FONT_H / 2;
	//}
	//bmp.SavePng("font jp.png");
}

void make_psf()
{
	std::vector<std::string> list;
	ListFiles("audio", "*.dat", list);

	CBufferFile driver;
	driver.Open("driver.psx");

	u8* vb = &driver.data[0xf0800];
	u8* gian = &driver.data[0xe8800];
	u8* track = &driver.data[0xe9800];
	DWORD* ptr = (DWORD*)&driver.data[0xe87fc];

	char path[MAX_PATH];
	FILE* f = nullptr;
	for (size_t i = 0, si = list.size(); i < si; i++)
	{
		sprintf_s(path, MAX_PATH, "audio\\%s", list[i].c_str());
		CPackageDC pak;
		pak.Open(path, false);

		DC2_ENTRY_GENERIC* egian = nullptr;
		DC2_ENTRY_GENERIC* etrack = nullptr;
		DC2_ENTRY_GENERIC* ebody = nullptr;

		std::string name = std::string(list[i].c_str(), strrchr(list[i].c_str(), '.'));

		for (int j = 0, count = 0; j < (int)pak.GetCount(); j++)
		{
			switch (pak.ent[j].type)
			{
			case GET_SNDH:	// gian
				egian = &pak.ent[j];
				*ptr = egian->reserve[0];
				memcpy(gian, &pak.segment[j][0], egian->size);
				break;
			case GET_SNDB:	// body
				ebody = &pak.ent[j];
				memcpy(vb, &pak.segment[j][0], ebody->size);
				break;
			case GET_SNDE:	// tracker
				etrack = &pak.ent[j];
				memcpy(track, &pak.segment[j][0], etrack->size);
				sprintf_s(path, MAX_PATH, "audio\\%s_%i.exe", name.c_str(), count++);
				fopen_s(&f, path, "wb");
				fwrite(driver.data, driver.size, 1, f);
				fclose(f);

				sprintf_s(path, MAX_PATH, "exe2psf audio\\%s_%i.exe", name.c_str(), count - 1);
				system(path);
				break;
			}
		}
	}
}

typedef struct WAV_HEADER
{
	u32                 RIFF;           // RIFF Header
	unsigned long       ChunkSize;      // RIFF Chunk Size  
	u32                 WAVE;           // WAVE Header      
	u32                 fmt;            // FMT header       
	unsigned long       Subchunk1Size;  // Size of the fmt chunk                                
	unsigned short      AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	unsigned short      NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
	unsigned long       SamplesPerSec;  // Sampling Frequency in Hz                             
	unsigned long       bytesPerSec;    // bytes per second 
	unsigned short      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
	unsigned short      bitsPerSample;  // Number of bits per sample      
	u32                 Subchunk2ID;    // "data"  string   
	unsigned long       Subchunk2Size;  // Sampled data length
} wav_hdr;

void find_loop(LPCSTR wav)
{
	CBufferFile f;
	f.Open(wav);
	WAV_HEADER* h = (WAV_HEADER*)f.data;

	s16* data = (s16*)&h[1];
	for(size_t i = 0; i < h->Subchunk2Size - (44100 * 2 * 2); i += 44100 * 2 * 2, data++)
	{
		s16 *ahead = &data[4];
		int count = 0, pos = -1;
		for (int j = 0; j < 8000 * 2; j++)
		{
			if (memcmp(&ahead[j], data, 1024) == 0)
			{
				if (count == 0)
					pos = data - (s16*)f.data;
				count++;
			}
			else break;
		}
		if (count > 0)
			printf("%X %d\n", pos, count);
	}
}

int dc2_strlen(WORD* str)
{
	int i = 0;
	while ((*str++ & 0xf000) != 0xF000) i++;

	return i + 1;
}

struct SOME_STR
{
	DWORD field_0;
	DWORD field_4;
	char field_8;
	char field_9;
	__int16 field_A;
};

struct FILE_DATA
{
	__int16 field_0;
	__int16 rdt;
	__int16 bss;
	__int16 field_6;
	__int16 field_8;
	__int16 field_A;
	__int16 field_C;
	__int16 field_E;
	DWORD door_name;
	char* field_14;
};

struct Lookup
{
	DWORD ptr;
	std::vector<DWORD> match;
	std::string string;
};

void Search_instance(std::vector<u8>& text_seg, DWORD seg_ptr, DWORD ptr, std::vector<DWORD> &match)
{
	u8* start = &text_seg[0],
		*pos = &text_seg[20];
	u8* end = &pos[text_seg.size() - 4 - 20];

	for (; pos < end; pos++)
	{
		if (*(DWORD*)pos == ptr)	// found a matching pointer, do more investigation
		{
			if(pos[-1] == 0x68 ||		// push
				pos[-4] == 0xC7)		// mov [x], offset ptr
				match.push_back((DWORD)(pos - start) + seg_ptr);
		}
	}
}

void Search_instance2(std::vector<u8>& data_seg, DWORD data_ptr, DWORD ptr, std::vector<DWORD>& match)
{
	u8* start = &data_seg[0],
		* pos = &data_seg[20];
	u8* end = &pos[data_seg.size() - 4 - 20];

	for (; pos < end; pos += 4)
		if (*(DWORD*)pos == ptr)
			match.push_back((DWORD)(pos - start) + data_ptr);
}

void StringsToXml(std::vector<Lookup>&str, LPCSTR out_name)
{
	XMLDocument xml;
	if (str.size() == 0) return;
	xml.SetBOM(true);

	XMLElement* items = xml.NewElement("Strings");
	// extract text
	for (size_t i = 0, si = str.size(); i < si; i++)
	{
		if (str[i].match.size() == 0) continue;

		XMLElement* ent = xml.NewElement("Entry");
		XMLElement* sub = xml.NewElement("Text");
		sub->SetText(str[i].string.c_str());
		// append the pointers
		for (size_t j = 0, sj = str[i].match.size(); j < sj; j++)
		{
			XMLElement* ptr = xml.NewElement("Ptr");
			ptr->SetAttribute("p", (unsigned int)str[i].match[j]);
			ent->InsertEndChild(ptr);
		}
		ent->InsertEndChild(sub);
		items->InsertEndChild(ent);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);
}

void Find_text_instances(PExe& exe, DWORD text_ptr)
{
	std::vector<DWORD> ptr_list;

	// gather all pointers for now
	WORD* text = (WORD*)&exe.data[text_ptr - exe.data_addr],
		* t = text;
	std::vector<std::string> str;
	FILE* f = fopen("strings_jp.txt", "wt");
	std::vector<Lookup> lut;
	for (int i = 0; i < 474; i++)
	{
		Lookup l;
		l.string = DecodeStringJp(text);
		l.ptr = text_ptr + (text - t) * 2;
		lut.push_back(l);
		//DWORD ptr = ;
		fprintf(f, "%03d %06X: %s\n", i, l.ptr, l.string.c_str());
		//ptr_list.push_back(ptr);
		text += dc2_strlen(text);
	}
	fclose(f);

	for (size_t i = 0, si = lut.size(); i < si; i++)
	{
		Search_instance(exe.text, exe.text_addr, lut[i].ptr, lut[i].match);
		Search_instance2(exe.data, exe.data_addr, lut[i].ptr, lut[i].match);
	}

	// dump to special xml
	StringsToXml(lut, "system.xml");

	//DWORD* ptr = (DWORD*)&exe.rdata[0x704620 - exe.rdata_addr];
	//for (int i = 0; i < 96; i++)
	//{
	//	std::string s = DecodeStringJp((WORD*)&exe.data[ptr[i] - exe.data_addr]);
	//	fprintf(f, "%s\n", s.c_str());
	//}

	
}

void Dump_eng_text(PExe& exe)
{
	WORD* text = (WORD*)&exe.data[0x723FC8 - exe.data_addr],
		* t = text;
	std::vector<std::string> str;
	FILE* f = fopen("strings.txt", "wt");
	std::vector<Lookup> lut;
	for (int i = 0; i < 460; i++)
	{
		Lookup l;
		l.string = DecodeString(text, nullptr);
		l.ptr = 0x723FC8 + (text - t) * 2;
		//str.push_back(s);
		fprintf(f, "%03d %06X: %s\n", i, 0x723FC8 + (text - t) * 2, l.string.c_str());
		text += dc2_strlen(text);
	}
	fclose(f);

	//

	//std::vector<std::string> str;
	str.clear();
	DWORD* ptr = (DWORD*)&exe.rdata[0x704628 - exe.rdata_addr];
	for (int i = 0; i < 96; i++)
	{
		std::string s = DecodeStringJp((WORD*)&exe.data[ptr[i] - exe.data_addr]);
		str.push_back(s);
	}
	StringsToXml(str, "eng\\files.xml", 0, str.size());

	str.clear();
	SOME_STR* item = (SOME_STR*)&exe.rdata[0x704260 - exe.rdata_addr];
	for (int i = 0; i < 80; i++)
	{
		std::string s = DecodeStringJp((WORD*)&exe.data[item[i].field_0 - exe.data_addr]);
		str.push_back(s);
		s = DecodeStringJp((WORD*)&exe.data[item[i].field_4 - exe.data_addr]);
		str.push_back(s);
	}
	StringsToXml(str, "eng\\items.xml", 0, str.size());

	str.clear();
	FILE_DATA *st = (FILE_DATA*)&exe.rdata[0x703608 - exe.rdata_addr];
	for (int i = 0; i < 88; i++)
	{
		std::string s = DecodeStringJp((WORD*)&exe.data[st[i].door_name - exe.data_addr]);
		str.push_back(s);
	}
	StringsToXml(str, "eng\\doors.xml", 0, str.size());
}

#include "patch.h"

void Generate_room_patches()
{
	char path[MAX_PATH];
	for (int Stage = 0; Stage <= 9; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			CPackageDC jp, us;
			CBufferFile f;

			sprintf_s(path, MAX_PATH, "PC JP\\ST%d%02X.DAT", Stage, Room);
			if (!f.Open(path)) continue;
			jp.OpenPC(f.GetBuffer());

			sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X.DAT", Stage, Room);
			if (!f.Open(path)) continue;
			us.OpenPC(f.GetBuffer());

			int id_us = us.GetIDByAddress(0x5e0000);
			int id_jp = jp.GetIDByAddress(0x5e0000);

			CMemStream patch;
			CompareFiles((DWORD*)&jp.segment[id_jp][0], (DWORD*)&us.segment[id_us][0], jp.ent[id_jp].size / 4, us.ent[id_us].size / 4, patch);
			sprintf_s(path, MAX_PATH, "patch\\ST%d%02X.d2p", Stage, Room);
			FILE* fp = fopen(path, "wb");
			fwrite(patch.Data(), patch.str.size, 1, fp);
			fclose(fp);

			sprintf_s(path, MAX_PATH, "patch\\ST%d%02X.rdt", Stage, Room);
			fp = fopen(path, "wb");
			fwrite(&jp.segment[id_jp][0], jp.ent[id_jp].size, 1, fp);
			fclose(fp);
		}
	}
}

typedef struct SAMPLE_DATA
{
	u8 id,
		loops,
		vol,
		pan;
	u32 freq;
} SAMPLE_DATA;

typedef struct SAMPLE_PTR
{
	u32 ptr,
		size;
} SAMPLE_PTR;

void extract_enemy(LPCSTR name)
{
	CPackageDC dc;
	dc.Open(name, true);
	
	auto wav_id = dc.SearchByType(GPC_SOUND);
	auto wav_dat = dc.segment[wav_id];
	auto wav_inf = dc.ent[wav_id];

	SAMPLE_DATA* smp = (SAMPLE_DATA*)wav_dat;
	SAMPLE_PTR* ptr = (SAMPLE_PTR*)&smp[32];
	for (int i = 0; i < 32; i++)
	{
		if (ptr[i].ptr == 0) continue;

		char path[MAX_PATH];
		sprintf_s(path, MAX_PATH, "test\\%02d.wav", i);

		FILE* fp = fopen(path, "wb");
		fwrite(wav_dat + ptr[i].ptr, ptr[i].size, 1, fp);
		fclose(fp);
	}
}

void DumpRoom(LPCSTR name)
{
	CPackageDC dc;
	dc.Open(name, true);
}

WORD WORD_ARRAY_8008fda0[] =
{
	32768, 34716, 36780, 38967, 41285, 43740, 46340, 49096, 52015, 55108, 58385, 61857
};

WORD WORD_ARRAY_8008fdb8[] =
{
	32768, 32782, 32797, 32812, 32827, 32842, 32856, 32871, 32886, 32901, 32916, 32931, 32945, 32960, 32975, 32990,
	33005, 33020, 33035, 33050, 33065, 33080, 33094, 33109, 33124, 33139, 33154, 33169, 33184, 33199, 33214, 33229,
	33244, 33259, 33274, 33289, 33304, 33319, 33334, 33349, 33364, 33379, 33394, 33410, 33425, 33440, 33455, 33470,
	33485, 33500, 33515, 33530, 33546, 33561, 33576, 33591, 33606, 33621, 33636, 33652, 33667, 33682, 33697, 33712,
	33728, 33743, 33758, 33773, 33789, 33804, 33819, 33834, 33850, 33865, 33880, 33896, 33911, 33926, 33941, 33957,
	33972, 33987, 34003, 34018, 34033, 34049, 34064, 34080, 34095, 34110, 34126, 34141, 34157, 34172, 34187, 34203,
	34218, 34234, 34249, 34265, 34280, 34296, 34311, 34327, 34342, 34358, 34373, 34389, 34404, 34420, 34435, 34451,
	34466, 34482, 34497, 34513, 34528, 34544, 34560, 34575, 34591, 34606, 34622, 34638, 34653, 34669, 34685, 34700
};

u32 _spu_note2pitch(int param_1, int param_2, int param_3, int param_4)

{
	short sVar1;
	int iVar2;
	u32 uVar3;
	short sVar4;

	iVar2 = (int)(((param_3 + ((param_4 + param_2 & 0xffffU) >> 7)) - param_1) * 0x10000) >> 0x10;
	sVar1 = (short)(iVar2 / 0xc);
	sVar4 = sVar1 + -2;
	iVar2 = iVar2 % 0xc;
	if (iVar2 * 0x10000 < 0) {
		iVar2 = iVar2 + 0xc;
		sVar4 = sVar1 + -3;
	}
	if (sVar4 < 0) {
		uVar3 = -(int)sVar4;
		uVar3 = (u32)(((int)((u32) * (u16*)((int)WORD_ARRAY_8008fda0 + ((iVar2 << 0x10) >> 0xf)) *
			(u32)WORD_ARRAY_8008fdb8[param_4 + param_2 & 0x7f]) >> 0x10) +
			(1 << (uVar3 - 1 & 0x1f))) >> (uVar3 & 0x1f);
	}
	else {
		uVar3 = 0x3fff;
	}
	return uVar3 & 0xffff;
}

int sort_cmp(std::pair<DWORD, int> &a, std::pair<DWORD, int> &b)
{
	return a.first < b.first;
}

void extract_psx_sfx(LPCSTR name, LPCSTR folder)
{
	CPackageDC dc;
	dc.Open(name, false);

	if (dc.GetCount() == 0)
		return;

	auto id_vh = dc.SearchByType(GET_SNDH);		// VAG header 'Gian'
	auto id_vb = dc.SearchByType(GET_SNDB);		// VAG body

	if (id_vh == -1 || id_vb == -1)
		return;

	BYTE* vag = dc.segment[id_vb];
	BYTE* gia = dc.segment[id_vh];
	DC2_ENTRY_VAB *vb = (DC2_ENTRY_VAB*)&dc.ent[id_vb];

	GIAN_ENTRY* gh = (GIAN_ENTRY*)&gia[0x50];
	char path[MAX_PATH];
	SAMPLE_DATA samples[32];
	memset(samples, 0, sizeof(samples));

	CreateDirectoryA(folder, nullptr);
	int count = gia[6] * 16;

	if (count > 32)
		return;

	// gather VAG pointers
	std::vector<std::pair<DWORD, int>> ptr;
	for (int i = 0, cnt = 1; i < count; i++)
	{
		// find a previously used value
		bool found = false;
		for (int j = 0; j < i; j++)
		{
			if (j == i) continue;
			if (gh[i].addr == gh[j].addr)
			{
				found = true;
				break;
			}
		}

		// push if it's a new one
		if (!found)
			ptr.push_back(std::pair<DWORD, int>(gh[i].addr * 8 - vb->addr, cnt++));
	}
	// add size as last fake pointer and sort
	ptr.push_back(std::pair<DWORD, int>(vb->size, -1));
	std::sort(ptr.begin(), ptr.end(), sort_cmp);

	for (int i = 0; i < count; i++)
	{
		if (gh[i].vol == 0)	// void entry
			continue;

		samples[i].vol = gh[i].vol;
		samples[i].pan = gh[i].pan;
		samples[i].loops = 0;
		// address to id
		for (size_t j = 0, sj = ptr.size() - 1; j < sj; j++)
		{
			if (ptr[j].first == (gh[i].addr * 8 - vb->addr))
			{
				samples[i].id = ptr[j].second;
				break;
			}
		}
		
		// note to frequency
		static u8 add[] = { 0, 1, 0xff, 0xfe, 0xfd, 0xff, 0xfe, 0xfd, 1, 0, 0, 0, 0xfd, 0, 0, 0, 0xfe, 0, 0, 0 };
		gh[i].note |= add[(gh[i].sample_note / 608) % 10] << 8;
		auto snote = gh[i].sample_note << 8;
		samples[i].freq = _spu_note2pitch(snote >> 8, snote & 0xff, gh[i].note >> 8, gh[i].note & 0xff) * 44100 / 0x1000;
	}

	DWORD wav_ptr[32];
	memset(wav_ptr, 0, sizeof(wav_ptr));

	FILE* fp;
	sprintf_s(path, MAX_PATH, "%s\\snd.wbk", folder);
	fopen_s(&fp, path, "wb");
	if (fp)
	{
		fseek(fp, sizeof(samples) + sizeof(wav_ptr), SEEK_SET);

		for (size_t i = 0, si = ptr.size() - 1; i < si; i++)
		{
			wav_ptr[ptr[i].second] = ftell(fp);
			PsxSample sample;
			sample.Decode(&vag[ptr[i].first], ptr[i + 1].first - ptr[i].first);
			sample.Write(fp, 22050);

			//sprintf_s(path, MAX_PATH, "%s\\%02d.wav", folder, ptr[i].second);
			//sample.Write(path, 22050);
		}

		fseek(fp, 0, SEEK_SET);
		fwrite(samples, sizeof(samples), 1, fp);
		fwrite(wav_ptr, sizeof(wav_ptr), 1, fp);
		fclose(fp);
	}
}

void extract_psx_sfx()
{
	extract_psx_sfx("PSX\\CORE.DAT", "PSX\\CORE");
	extract_psx_sfx("PSX\\CORE00.DAT", "PSX\\CORE00");
	extract_psx_sfx("PSX\\CORE01.DAT", "PSX\\CORE01");
	extract_psx_sfx("PSX\\CORE02.DAT", "PSX\\CORE02");
	extract_psx_sfx("PSX\\CORE03.DAT", "PSX\\CORE03");
	extract_psx_sfx("PSX\\CORE04.DAT", "PSX\\CORE04");
	extract_psx_sfx("PSX\\CORE05.DAT", "PSX\\CORE05");
	extract_psx_sfx("PSX\\CORE06.DAT", "PSX\\CORE06");
	extract_psx_sfx("PSX\\CORE07.DAT", "PSX\\CORE07");
	extract_psx_sfx("PSX\\CORE08.DAT", "PSX\\CORE08");
	extract_psx_sfx("PSX\\CORE09.DAT", "PSX\\CORE09");
	extract_psx_sfx("PSX\\CORE10.DAT", "PSX\\CORE10");
	extract_psx_sfx("PSX\\CORE11.DAT", "PSX\\CORE11");
	extract_psx_sfx("PSX\\CORE12.DAT", "PSX\\CORE12");
	extract_psx_sfx("PSX\\TITLE.DAT", "PSX\\TITLE");
	extract_psx_sfx("PSX\\TITLE2.DAT", "PSX\\TITLE2");

	char path[MAX_PATH], fold[MAX_PATH];
	for (int i = 0; i <= 9; i++)
	{
		for (int j = 0; j < 48; j++)
		{
			sprintf_s(path, MAX_PATH, "PSX\\ST%d%02X.DAT", i, j);
			sprintf_s(fold, MAX_PATH, "PSX\\ST%d%02X", i, j);
			extract_psx_sfx(path, fold);
		}
	}
}

void tests()
{
	CPackageDC dc;
	dc.Open("PC JP\\ST205.DAT", true);

	char path[MAX_PATH];
	for (int i = 0; i < dc.GetCount(); i++)
	{
		sprintf_s(path, MAX_PATH, "test\\%02d.dat", i);
		FILE* fp;
		fopen_s(&fp, path, "wb");
		fwrite(dc.segment[i], dc.ent[i].size, 1, fp);
		fclose(fp);
	}
}

int main()
{
	Generate_room_patches();
	ExtractRoomStrings();

	tests();
	//extract_psx_sfx("PSX\\ST003.DAT", "PSX\\ST003");
	extract_psx_sfx();

	//extract_enemy("PC JP\\ST003.DAT");

	//extract_enemy("D:\\Program Files\\Dino Crisis 2 SN\\Data\\CORE01.DAT");

	//

	//PExe exe, eng;
	//exe.Open("D:\\Program Files\\Dino Crisis 2 SN\\Dino2.exe");
	//eng.Open("D:\\Program Files\\Dino Crisis 2 ENG\\Dino2.exe");

	//Dump_eng_text(eng);
	//Find_text_instances(exe, 0x723DCC);

	//find_loop("001_MS 0401 1.wav");
	//test();
	//extract_core();
	//ExtractExeStrings();

	DumpMusic();
	//make_psf();
}

