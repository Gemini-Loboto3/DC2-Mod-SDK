﻿#include "stdafx.h"
#include <string>

static const wchar_t encode[] =
{
	L" @@@@.§…“”｢｣～←→@"			// 00
	L"@@@@@①②③④⑤@@ＳＯＲＴ"		// 10
	L"@!@#@%&'()@+,-·/"			// 20
	L"0123456789:;<=>?"			// 30
	L"@ABCDEFGHIJKLMNO"			// 40
	L"PQRSTUVWXYZ[\\]^@"		// 50
	L"`abcdefghijklmno"			// 60
	L"pqrstuvwxyz《@》￥@"		// 70
	L"々一あいうぇえおかがきぎくぐけげ"
	L"こごさざしじすずせぜそぞただちっ"
	L"つづてでとどなにぬねのはばぱひび"
	L"ふぶへべほぼまみむめもゃやゅゆょ"
	L"よらりるれろわをんァアィイゥウェ"
	L"エォオカガキギクグケゲコゴサザシ"
	L"ジスズセゼソタダチッツテデトドナ"
	L"ニヌネノハバパビピフブプヘベペホ"
	L"ボポマミムメモャヤュユョラリルレ"
	L"ロワンヴヶ"
};

size_t u8_wc_toutf8(char *dest, uint32_t ch);

std::string DecodeString(u16 *data, size_t *size)
{
	std::string str;
	char temp[32];
	u16* start = data;

	while (1)
	{
		u16 c = *data++;

		switch (c & 0xf000)
		{
		case 0x1000:
			sprintf_s(temp, sizeof(temp), "{color %d}", c & 0xF);
			str += temp;
			break;
		case 0x2000:
			str += "{pic}";
			break;
		case 0x4000:
			str += "\\n";
			break;
		case 0x5000:	// non-instant text scrolling
			str += "{clear}";
			break;
		case 0x6000:
			str += "{pause}";
			break;
		case 0x7000:
			sprintf_s(temp, sizeof(temp), "{delay %d}", c);
			str += temp;
			break;
		case 0x8000:
		case 0x9000:
		case 0xA000:
		case 0xB000:
		case 0xC000:
		case 0xD000:
		case 0xE000:
			sprintf_s(temp, sizeof(temp), "{0x%04X}", c);
			str += temp;
			break;
		case 0xF000:
			return str;
		default:
			if (encode[c & 0x7f] == L'@')
			{
				sprintf_s(temp, sizeof(temp), "{0x%04X}", c);
				str += temp;
			}
			else
			{
				u8_wc_toutf8(temp, encode[c & 0x7f]);
				str += temp;
			}
		}
	}

	return str;
}

std::string DecodeStringJp(u16* data)
{
	std::string str;
	char temp[32];
	u16* start = data;

	while (1)
	{
		u16 c = *data++;

		switch (c & 0xf000)
		{
		case 0x1000:
			sprintf_s(temp, sizeof(temp), "{color %d}", c & 0xF);
			str += temp;
			break;
		case 0x2000:
			str += "{pic}";
			break;
		case 0x4000:
			str += "\\n";
			break;
		case 0x5000:	// non-instant text scrolling
			str += "{clear}";
			break;
		case 0x6000:
			str += "{pause}";
			break;
		case 0x7000:
			sprintf_s(temp, sizeof(temp), "{delay %d}", c);
			str += temp;
			break;
		case 0x8000:
		case 0x9000:
		case 0xA000:
		case 0xB000:
		case 0xC000:
		case 0xD000:
		case 0xE000:
			sprintf_s(temp, sizeof(temp), "{0x%04X}", c);
			str += temp;
			break;
		case 0xF000:
			return str;
		default:
			c &= 0xfff;
			if(c >= _countof(encode))
			{
				sprintf_s(temp, sizeof(temp), "{0x%04X}", c);
				str += temp;
			}
			else
			{
				u8_wc_toutf8(temp, encode[c]);
				str += temp;
			}
		}
	}

	return str;
}

static LPCWSTR encode_eu =
{
	L"  @@@@@@@@@@012345"	// 00
	L"6789:;,\"!?@ABCDEFG"	// 12
	L"HIJKLMNOPQRSTUVWXY"	// 24
	L"Z(/)'-·abcdefghijk"	// 36
	L"lmnopqrstuvwxyzÄäÖ"	// 48
	L"öÜüßÀàÂâÈèÉéÊêÏïÎî"	// 5A
	L"ÔôÙùÛûÇçＳＴＡＲ“.…‒–+"	// 6C
	L"=&@@@@ÑñËë°ªÁáÍíÓó"	// 7E
	L"Úú¿¡ÌìÒò$*„‟`æ@@@@"	// 90
};

size_t u8_wc_toutf8(char *dest, uint32_t ch)
{
	if (ch < 0x80) {
		dest[0] = (char)ch;
		dest[1] = '\0';
		return 1;
	}
	if (ch < 0x800) {
		dest[0] = (ch >> 6) | 0xC0;
		dest[1] = (ch & 0x3F) | 0x80;
		dest[2] = '\0';
		return 2;
	}
	if (ch < 0x10000) {
		dest[0] = (ch >> 12) | 0xE0;
		dest[1] = ((ch >> 6) & 0x3F) | 0x80;
		dest[2] = (ch & 0x3F) | 0x80;
		dest[3] = '\0';
		return 3;
	}
	if (ch < 0x110000) {
		dest[0] = (ch >> 18) | 0xF0;
		dest[1] = ((ch >> 12) & 0x3F) | 0x80;
		dest[2] = ((ch >> 6) & 0x3F) | 0x80;
		dest[3] = (ch & 0x3F) | 0x80;
		dest[4] = '\0';
		return 4;
	}
	return 0;
}

std::string DecodeStringEU(u8 *data)
{
	std::string str;
	char temp[32];

	while (1)
	{
		u8 c = *data++;

		switch (c)
		{
		case 0xee:	// upper ranges
			str += encode[*data++ + 0xea] & 0xff;
			break;
		case 0xef:
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf4:
		case 0xf5:	// ??
			str += "{p}";
			break;
		case 0xf6:	// ??
			break;
		case 0xf7:	// short EOS for items
			return str;
		case 0xf8:
			sprintf_s(temp, sizeof(temp), "{string %d}", *data++);
			str += temp;
			break;
		case 0xf9:
			str += "{color ";
			str += *data + '0';
			str += "}";
			data++;
			break;
		case 0xfa:	// non-instant text scrolling
			sprintf_s(temp, sizeof(temp), "{scroll %d}", *data++);
			str += temp;
			break;
		case 0xfb:
			sprintf_s(temp, sizeof(temp), "{branch %d}", *data++);
			str += temp;
			break;
		case 0xfc:
			str += "\\n";
			break;
		case 0xfd:
			sprintf_s(temp, sizeof(temp), "{clear %d}", *data++ * 60 / 50);
			str += temp;
			break;
		case 0xfe:
			if (*data != 0)
			{
				sprintf_s(temp, sizeof(temp), "{timed %d}", *data * 60 / 50);
				str += temp;
			}
			return str;
		case 0xff: // ??
			break;
		default:
			u8_wc_toutf8(temp, encode_eu[c]);
			str += temp;
		}
	}

	return str;
}

std::string DecodeStringDS(u8* data)
{
	char temp[32];
	char ucs32[5];
	std::string str;

	while (1)
	{
		int ch = *data++;

		switch (ch)
		{
		case 0:
			return str;
		case 1:
			str += "\\n";
			break;
		case 3:
			sprintf_s(temp, 32, "{scroll %d}", *data++);
			str += temp;
			break;
		case 4:
			sprintf_s(temp, 32, "{color %d}", *data++);
			str += temp;
			break;
		case 5:
			sprintf_s(temp, 32, "{string %d}", *data++);
			str += temp;
			break;
		case 6:
			if (data[1] == 2)
			{
				sprintf_s(temp, 32, "{clear %d}", *data++);
				data++;
				str += temp;
				continue;
			}
			else if (*data != 0)
			{
				sprintf_s(temp, 32, "{timed %d}", *data++);
				str += temp;
			}
			break;
			//return str;
		case 7:
			sprintf_s(temp, 32, "{branch %d %d %d}", data[0], data[1], data[2]);
			str += temp;
			data += 3;
			//str += "{branch 0 1 0}";
			break;
		case 0x11:	// long characters
			switch (*data)
			{
			case 1:
				u8_wc_toutf8(ucs32, 0xff33);	// S
				str += ucs32;
				break;
			case 0x81:
				u8_wc_toutf8(ucs32, 0xff34);	// T
				str += ucs32;
				break;
			case 0x83:
				u8_wc_toutf8(ucs32, 0xff21);	// A
				str += ucs32;
				break;
			case 0x86:
				u8_wc_toutf8(ucs32, 0xff32);	// R
				str += ucs32;
				break;
			default:
				printf("What the deuce? %X\n", *data);
			}
			data++;
			break;
		case 0x12:	// more long characters
			if (*data == 0x36) str += '\'';
			else
				printf("What the re-deuce? %X\n", *data);
			data++;
			break;
		default:
			if (ch == 0x85)
				str += "...";
			else if (ch < 128)
				str += ch;
			else if (ch < 0xC0)
			{
				str += 0xc2;
				str += ch;
			}
			else
			{
				str += 0xc3;
				str += (ch - 0x40);
			}
		}
	}

	return str;
}
