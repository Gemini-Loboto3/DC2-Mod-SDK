#pragma once
#include <vector>

#define VAG_MAGIC	"pGAV"
#define VAB_MAGIC	"pBAV"

#define SET_SAMPLENOTE(freq)	((60+((44100/(freq)-1))*12)<<8)
#define GETFREQ(pitch)			((pitch*44100)>>12)

typedef struct tagEdhAttr
{
	uint32_t unk00:8;				// flags
	uint32_t prog:7;
	uint32_t unk01:5;
	uint32_t tone:4;
	uint32_t voice:5;
	uint32_t unk02:3;				// count of the following elements
} EdhAttr;

typedef struct tagVagHeader
{
	uint32_t magic;					// always "VAGp"
	uint32_t version;				// 0=1.8, 0x20=2.0
	uint32_t pad1;					// reserved
	uint32_t datasize;				// big endian
	uint32_t frequency;				// big endian
	uint32_t pad2[3];				// reserved
	char name[16];				// used by "Sound Delicatessen"
} VagHdr;

/* VAB Bank Headdings */
typedef struct VabHdr
{
	uint32_t form;		/* always 'VABp' */
	int ver;		/* VAB file version number */
	int id;			/* VAB id */
	uint32_t fsize;		/* VAB file size */
	uint16_t reserved0;	/* system reserved */
	uint16_t ps;			/* # of the programs in this bank */
	uint16_t ts;			/* # of the tones in this bank */
	uint16_t vs;			/* # of the vags in this bank */
	uint8_t mvol;		/* master volume for this bank */
	uint8_t pan;			/* master panning for this bank */
	uint8_t attr1;		/* bank attributes1 */
	uint8_t attr2;		/* bank attributes2 */
	uint32_t reserved1;	/* system reserved */
} VabHdr;	/* 32 byte */

// Program Headdings
typedef struct tagProgramAttributes
{
	uint8_t tones;					// # of tones
	uint8_t mvol;					// program volume
	uint8_t prior;					// program priority
	uint8_t mode;					// program mode
	uint8_t mpan;					// program pan
	char reserved0;				// system reserved
	short attr;					// program attribute
	uint32_t reserved[2];			// system reserved
} ProgAtr;			// 16 bytes

// VAG Tone Headdings
typedef struct tagVagAttributes
{
	uint8_t prior;					// tone priority
	uint8_t mode;					// play mode
	uint8_t vol;					// tone volume
	uint8_t pan;					// tone panning
	uint8_t center;				// center note
	uint8_t shift;					// center note fine tune
	uint8_t min;					// minimum note limit
	uint8_t max;					// maximum note limit
	uint8_t vibW;					// vibrate depth
	uint8_t vibT;					// vibrate duration
	uint8_t porW;					// portamento depth
	uint8_t porT;					// portamento duration
	uint8_t pbmin;					// under pitch bend max
	uint8_t pbmax;					// upper pitch bend max
	uint8_t reserved1;				// system reserved
	uint8_t reserved2;				// system reserved
	uint16_t adsr1;				// adsr1
	uint16_t adsr2;				// adsr2
	short prog;					// parent program
	short vag;					// vag reference
	short reserved[4];			// system reserved
} VagAtr;			// 32 bytes

#define KEY_NOTE	60
#define KEY_FINE	0

uint16_t SsPitchFromNote(short note, short fine, uint8_t center, uint8_t shift);

// VPK data
typedef struct tagTone
{
	uint8_t voll;
	uint8_t volr;		// panning uses both, otherwise only left is used
	uint16_t pitch;
	uint16_t adsr[2];
	uint32_t addr;
} TONE;

//class CVabTone
//{
//public:
//};

class CVabProgram : public ProgAtr
{
public:
	void Open(ProgAtr *prog, VagAtr *tones);

	std::vector<VagAtr> tone;
	std::vector<int> freq, pitch;
};

class CVag
{
public:
	CVag(void){ data=NULL; size=0; }
	~CVag(void){ Reset(); }

	void operator=(const CVag& vag){};

	void Reset();
	void Copy(CVag *vag);
	uint8_t* GetData(){return data;};
	int GetSize(){return size;};
	void SetData(uint8_t *_data, int _size);
	void SetDataHeaderless(uint8_t *_data, int _size);

public:
	uint8_t *data;
	int size;
};

class CVab
{
public:
	CVab(void);
	~CVab(void);

	int AddVag(CVag *vag);
	void RemoveAll();

	int OpenVab(const char* vh, const char* vb);
	int OpenVab(uint8_t* vh, uint8_t* vb);

	__inline CVag* GetVag(int index) { return vag_data[index]; }
	__inline int   GetVagCount() { return (int)vag_data.size(); }

	__inline CVabProgram* GetVabProg(int index) { return &prog[index]; }
	__inline int          GetProgCount() { return (int)prog.size(); }

	__inline void CopyVabAttr(CVab *c)
	{
		ver=c->ver, id=c->id;
		mvol=c->mvol, pan=c->pan;
		attr1=c->attr1, attr2=c->attr2;

		prog=c->prog;
	}

	__inline void operator = (CVab *c)
	{
		for(int i=0; i<(int)c->GetVagCount(); i++)
			AddVag(c->GetVag(i));
		// vab attributes
		CopyVabAttr(c);
	}

private:
	std::vector<CVabProgram> prog;
	std::vector<CVag*> vag_data;

public:
	int ver;		/* VAB file version number */
	int id;			/* VAB id */
	uint8_t mvol;		/* master volume for this bank */
	uint8_t pan;			/* master panning for this bank */
	uint8_t attr1;		/* bank attributes1 */
	uint8_t attr2;		/* bank attributes2 */
};

class PsxSample
{
public:
	PsxSample() : data(nullptr),
		size(0),
		loop_start(-1),
		loop_end(-1),
		loop_releases(0),
		sample_release(-1)
	{}
	~PsxSample()
	{
		if (data)
		{
			delete[] data;
			data = nullptr;
		}
	}

	void Decode(uint8_t* src, size_t size);
	void Write(const char* filename, int freq);
	void Write(FILE *fp, int freq);

	short* data = nullptr;
	size_t size = 0;

	int loop_start = 0,
		loop_end = 0,
		loop_releases = 0,
		sample_release = 0;
};

class CWbk
{
	std::vector<PsxSample> smpl;

	void Write(const char* name);
};
