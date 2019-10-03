#pragma once
#include <stdafx.h>
#include <vector>

enum GEntryType
{
	GET_DATA,		// generic literal data
	GET_TEXTURE,	// stripped TIM pixel
	GET_PALETTE,	// stripped TIM clut
	GET_SNDH,		// VAG header 'Gian'
	GET_SNDB,		// VAG body
	GET_SNDE,		// configuration for sound samples?
	GET_UNK,
	GET_LZSS0,
	GET_LZSS1		// compressed texture
};

/////////////////////////////////
// Dino crisis package system  //
/////////////////////////////////

// standard entry in packed headers
typedef struct tagDc2GenericEntry
{
	u32 type;		// check generic entry type enumation
	u32 size;		// real size to read
	u32 reserve[6];	// round entry to 32 bytes
} DC2_ENTRY_GENERIC;

// alternate structure for 4 word entries
typedef struct tagDc2GenericEntry16
{
	u32 type;		// check generic entry type enumation
	u32 size;		// real size to read
	u32 reserve[2];	// round entry to 16 bytes
} DC2_ENTRY_GENERIC16;

// works for both TEXTURE and PALETTE types
typedef struct tagDc2EntryGfx
{
	u32 type;
	u32 size;
	u16 x, y;		// framebuffer coordinates
	u16 w, h;		// framebuffer size
} DC2_ENTRY_GFX;

/////////////////////////////////
// GIAN - Dino crisis sounds   //
/////////////////////////////////
typedef struct tagGianEntry
{
	u8 reverb;		// only bit 1<<3 is checked
	u8 unk1;
	u8 unk2;
	u8 sample_note;	// *8 to obtain the actual value
	u16 note;
	u16 note_copy;	// unused, contains the same value as note>>8
	u16 reserved;	// unused
	u16 adsr1;		// always 0x80FF
	u16 adsr2;
	u16 addr;		// entry base-addr*8 to obtain the sample address
} GIAN_ENTRY;

/////////////////////////////////
// DCM - Dino crisis models	   //
/////////////////////////////////
typedef struct tagDcmMapping
{
	s16 xyz[3];
	u8 super, child;
	u32 tri_ptr;
	u32 quad_ptr;
	u16 tri_cnt;
	u16 quad_cnt;
} DCM_MAPPING;

typedef struct tagDcmHeader
{
	u32 vertex_ptr;		// vectors
	u32 normal_ptr;		// normals
	u32 tri_ptr;		// first entry of the triangle data		[ram mirror]
	u32 quad_ptr;		// first entry of the rect data			[ram mirror]
	u16 tri_cnt;		// total of triangles
	u16 quad_cnt;		// total of quadrilaterals
	u32 obj_cnt;		// total of objects in the model
	u16 tpage, clut;
	DCM_MAPPING map[1];	// mapping data
} DCM_HEADER;

typedef struct tagDcm1Header
{
	u32 unk_ptr;
	u32 unk_ptr1[6];
	u32 vertex_ptr;		// vectors
	u32 normal_ptr;		// normals
	u32 tri_ptr;		// first entry of the triangle data		[ram mirror]
	u32 quad_ptr;		// first entry of the rect data			[ram mirror]
	u16 tri_cnt;		// total of triangles
	u16 quad_cnt;		// total of quadrilaterals
	u32 obj_cnt;		// total of objects in the model
	DCM_MAPPING map[1];	// mapping data
} DCM1_HEADER;

typedef struct tagDcmTriMap
{
	u16 vertex[3];
	u8 uv[3][2];
} DCM_TRIMAP;

typedef struct tagDcm1TriMap
{
	u16 vertex[3];
	u8 uv0[2];
	u8 uv1[2];
	u16 clut;
	u8 uv2[2];
	u16 tpage;
} DCM1_TRIMAP;

typedef struct tagDcmRectMap
{
	u16 vertex[4];
	u8 uv[4][2];
} DCM_QUADMAP;

typedef struct tagDcm1RectMap
{
	u16 vertex[4];
	u8 uv0[2];
	u16 clut;
	u8 uv1[2];
	u16 tpage;
	u8 uv2[2];
	u8 uv3[2];
} DCM1_QUADMAP;

//////////////////////////////////
// Functions and classes		//
//////////////////////////////////
u32 Dc2LzssDec(u8* src, u8* &dst, u32 src_size);
void UnswizzleGfx(DC2_ENTRY_GFX *gfx_p, u8 *buf_p);

class CPackageDC
{
public:
	CPackageDC();
	~CPackageDC();

	void Reset();

	void Open(LPCTSTR filename);
	void Open(u8* data);

	void ExtractRaw(LPCTSTR dest);

	int GetIDByAddress(u32 address);

	std::vector<DC2_ENTRY_GENERIC> ent;
	std::vector<u8*> segment;
	int pack_type;

	enum
	{
		Type_DC1,
		Type_DC2
	};
};
