// extractor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include "DinoCrisis2.h"
#include "Bitmap.h"
#include "hash64.h"
#include "tinyxml2.h"

#include <string>
#include <vector>

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

			size_t pos = 0;
			for (int i = 0;; i++)
			{
				pos += dc.OpenPC(f.GetBuffer() + pos);
				if (pos >= f.GetSize())
					break;
				CBitmap bmp, bm2;
				// dump bg
				sprintf_s(path, MAX_PATH, "ROOM\\ROOM%d%02X_%02d.jpg", Stage, Room, i);
				FILE* fl = fopen(path, "wb");
				fwrite(dc.segment[1], dc.ent[1].size, 1, fl);
				fclose(fl);
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
				//UnswizzleGfx(, dsize);
				fl = fopen("test.bin", "wb");
				fwrite(temp, dsize, 1, fl);
				fclose(fl);
				// generate
				u16* img = (u16*)temp;
				// fix transparency
				for (int j = 0; j < dsize / 2; j++) if (!(img[j] & 0x8000)) img[j] = 0;
				bmp.Create(18, dsize / 36);
				for (int y = 0, sy = bmp.h; y < sy; y++)
					for (int x = 0, sx = bmp.w; x < sx; x++)
						bmp.setPixel(x, y, Convert_color(*img++));
				bm2.Create(256, 320);
				// recomposite
				for (int j = 0, js = bmp.h / 18; j < js; j++)
					bm2.Blt(bmp, 0, j * 18, (j % 18) * 18, (j / 18) * 18, 18, 18);

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
	ListFiles("PC JP", "M*.DAT", list);

	CreateDirectory("music", nullptr);

	X64Hash h;
	for (size_t i = 0, si = list.size(); i < si; i++)
	{
		CPackageDC dc;
		sprintf_s(path, MAX_PATH, "PC JP\\%s", list[i].c_str());
		dc.Open(path);

		for (size_t j = 0, sj = dc.GetCount(); j < sj; j++)
		{
			u8* data = dc.segment[j];
			size_t size = dc.ent[j].size;
			if (h.PushHash(data, size, ""))
				continue;
			sprintf_s(path, MAX_PATH, "music\\%02d_%02d.mp3", i, j);
			FILE* f = fopen(path, "wb");
			fwrite(data, size, 1, f);
			fclose(f);
		}
	}
}

std::string DecodeString(u16* data, size_t* size);

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

			for (int i = 0, si = dc.GetCount(); i < si; i++)
			{
				if ((dc.ent[i].type == GPC_TEXTURE || dc.ent[i].type == GPC_LZSS1 ) && dc.ent[i + 1].type == GPC_PALETTE)
				{
					DC2_ENTRY_GFX* gfx = (DC2_ENTRY_GFX*)&dc.ent[i];
					DC2_ENTRY_GFX* pal = (DC2_ENTRY_GFX*)&dc.ent[i + 1];
					u8* d = dc.segment[i];
					u16* p = (u16*)dc.segment[i + 1];
					CBitmap bmp;
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
					sprintf_s(path, MAX_PATH, "PC US\\ST%d%02X\\%02d.%s", Stage, Room, i, types[dc.ent[i].type]);
					FILE* f = fopen(path, "wb");
					fwrite(dc.segment[i], dc.ent[i].size, 1, f);
					fclose(f);
				}
			}
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
	dc.Open("PC JP\\CORE.DAT", true);

	CBitmap bmp;
	bmp.Create(FONT_T, 80 * FONT_H);

	u32 pal[16];
	for (int i = 1; i < 16; i++)
		pal[i] = RGB_BMP(i * 16, i * 16, i * 16);
	pal[0] = RGB_BMP(0, 128, 128);

	u8 *fnt = dc.segment[2];
	for (int i = 0; i < 1268; i++)
	{
		convert_glyph((i % FONT_L) * FONT_W, (i / FONT_L) * FONT_H, fnt, bmp, pal);
		fnt += FONT_W * FONT_H / 2;
	}
	bmp.SavePng("font jp.png");
}

int main()
{
	test();
	extract_core();
	ExtractRoomStrings();
	ExtractExeStrings();

	DumpMusic();
}

