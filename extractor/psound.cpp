#include "psound.h"

#define MAGIC(a,b,c,d)		((a)|((b)<<8)|((c)<<16)|((d)<<24))

uint16_t _svm_ptable[]=
{
	4096, 4110, 4125, 4140, 4155, 4170, 4185, 4200,
	4216, 4231, 4246, 4261, 4277, 4292, 4308, 4323,
	4339, 4355, 4371, 4386, 4402, 4418, 4434, 4450,
	4466, 4482, 4499, 4515, 4531, 4548, 4564, 4581,
	4597, 4614, 4630, 4647, 4664, 4681, 4698, 4715,
	4732, 4749, 4766, 4783, 4801, 4818, 4835, 4853,
	4870, 4888, 4906, 4924, 4941, 4959, 4977, 4995,
	5013, 5031, 5050, 5068, 5086, 5105, 5123, 5142,
	5160, 5179, 5198, 5216, 5235, 5254, 5273, 5292,
	5311, 5331, 5350, 5369, 5389, 5408, 5428, 5447,
	5467, 5487, 5507, 5527, 5547, 5567, 5587, 5607,
	5627, 5648, 5668, 5688, 5709, 5730, 5750, 5771,
	5792, 5813, 5834, 5855, 5876, 5898, 5919, 5940,
	5962, 5983, 6005, 6027, 6049, 6070, 6092, 6114,
	6137, 6159, 6181, 6203, 6226, 6248, 6271, 6294,
	6316, 6339, 6362, 6385, 6408, 6431, 6455, 6478,
	6501, 6525, 6549, 6572, 6596, 6620, 6644, 6668,
	6692, 6716, 6741, 6765, 6789, 6814, 6839, 6863,
	6888, 6913, 6938, 6963, 6988, 7014, 7039, 7064,
	7090, 7116, 7141, 7167, 7193, 7219, 7245, 7271,
	7298, 7324, 7351, 7377, 7404, 7431, 7458, 7485,
	7512, 7539, 7566, 7593, 7621, 7648, 7676, 7704,
	7732, 7760, 7788, 7816, 7844, 7873, 7901, 7930,
	7958, 7987, 8016, 8045, 8074, 8103, 8133, 8162,
	8192
};

uint16_t SsPitchFromNote(short note, short fine, uint8_t center, uint8_t shift)
{
	uint32_t pitch;
	short calc, type;
	int add, sfine;

	sfine=fine+shift;
	if(sfine<0) sfine+=7;
	sfine>>=3;

	add=0;
	if(sfine>15)
	{
		add=1;
		sfine-=16;
	}
	
	calc=add+(note-(center-60));//((center + 60) - note) + add;
	pitch=_svm_ptable[16*(calc%12)+(short)sfine];
	type=calc/12-5;

	// regular shift
	if(type>0) return pitch << type;
	// negative shift
	if(type<0) return pitch >> -type;

	return pitch;
}

////////////////////////////////////////////////////////////
CVab::CVab(void)
{
}

CVab::~CVab(void)
{
	RemoveAll();
}

int CVab::OpenVab(uint8_t* vh, uint8_t* vb)
{
	VabHdr *h=(VabHdr*)vh;

	if(h->form!=MAGIC('V','A','B','p') && h->form!=MAGIC('p','B','A','V')) return 0;

	attr1=h->attr1;
	attr2=h->attr2;
	id=h->id;
	mvol=h->mvol;
	pan=h->pan;
	ver=h->ver;

	RemoveAll();

	ProgAtr* prg=(ProgAtr*)&h[1];
	VagAtr* atr=(VagAtr*)&prg[128];

	for(int i=0; i<h->ps; i++)
	{
		CVabProgram p;
		p.Open(&prg[i],atr);
		prog.push_back(p);
		atr+=16;
	}

	int freq=SET_SAMPLENOTE(44100);

	uint16_t *ptr=(uint16_t*)atr;
	int pos=0;
	for (int i = 0; i < h->vs; i++)
	{
		CVag* v = new CVag;
		int size = ptr[i + 1] * 8;
		pos += ptr[i] * 8;
		v->SetDataHeaderless(&vb[pos], size);
		vag_data.push_back(v);
		//_tprintf(_T("Vag pos %x, size %d, ptr %x\n"),pos,size,ptr[i]);
	}

	return 1;
}

int CVab::AddVag(CVag *vag)
{
	CVag *tvag=new CVag;
	tvag->Copy(vag);
	vag_data.push_back(tvag);
	return (int)vag_data.size();
}

void CVab::RemoveAll()
{
	prog.clear();
	while(!vag_data.empty())
	{
		delete vag_data.back();
		vag_data.pop_back();
	}
}

////////////////////////////////////////////////////////////
void CVabProgram::Open(ProgAtr *prog, VagAtr *tones)
{
	int p;
	this->attr=prog->attr;
	this->mode=prog->mode;
	this->mpan=prog->mpan;
	this->mvol=prog->mvol;
	this->prior=prog->prior;
	this->tones=prog->tones;

	tone.clear();
	for(int i=0; i<prog->tones; i++)
	{
		tone.push_back(tones[i]);
		p=SsPitchFromNote(tones[i].min,tones[i].shift,tones[i].center,tones[i].shift);
		pitch.push_back(p);
		freq.push_back((p*44100)>>12);
	}
}

////////////////////////////////////////////////////////////
void CVag::Reset()
{
	if(data)
	{
		delete[] data;
		data=NULL;
		size=0;
	}
};

void CVag::Copy(CVag *vag)
{
	if(!vag->GetData() || vag->GetSize()==0) return;
	SetData(vag->GetData(),vag->GetSize());
}

void CVag::SetData(uint8_t* _data, int _size)
{
	Reset();
	data = new uint8_t[_size];
	size = _size;
	memcpy(data, _data, _size);
}

void CVag::SetDataHeaderless(uint8_t *_data, int _size)
{
	Reset();
	data=new uint8_t[_size];
	size=_size;
	memcpy(data,_data,_size);
}

short bezier(short a, short b, double perc)
{
	short diff = b - a;
	return (short)((double)a + ((double)diff * perc));
}

short linear(short a, short b, double perc)
{
	return (short)((double)a * perc + (double)b * (1. - perc));
}

void PsxSample::Decode(uint8_t* src, size_t size)
{
	const int itpl_size = 2048;
	if (size == 0)
		return;

	// skip leading zeroes for spu init
	src += 16;
	size -= 16;

	loop_start = -1;
	loop_end = -1;
	loop_releases = 0;
	sample_release = -1;

	this->size = size / 16 * 28;
	data = new short[this->size + itpl_size];
	memset(data, 0, (this->size + itpl_size) * 2);
	short* dst = data,
		* start = dst;

	int predict_nr, shift_factor, flags, d, s;
	double s_1 = 0.0;
	double s_2 = 0.0;

	static double f[5][2] = { { 0.0, 0.0 },
					{   60.0 / 64.0,  0.0 },
					{  115.0 / 64.0, -52.0 / 64.0 },
					{   98.0 / 64.0, -55.0 / 64.0 },
					{  122.0 / 64.0, -60.0 / 64.0 } };

	double samples[28];

	while (size)
	{
		predict_nr = *src++;
		shift_factor = predict_nr & 0xf;
		predict_nr >>= 4;
		flags = *src++;

		if (flags == 7)
		{
			this->size -= 28 * 2;
			loop_start = -1;
			loop_end = -1;
			return;
		}
		else
		{
			if ((flags & 1) /*&& loop_end == -1*/)	// loop end
				loop_end = dst - start + 28;
			if ((flags & 2) == 0)
			{
				if ((flags & 1))
				{
					loop_releases = 1;
					sample_release = dst - start;
				}
			}
			if ((flags & 4) /*&& loop_start == -1*/)	// loop start
				loop_start = dst - start;
		}

		for (int i = 0; i < 28; i += 2)
		{

			d = *src++;
			s = (d & 0xf) << 12;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i] = (double)(s >> shift_factor);
			s = (d & 0xf0) << 8;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i + 1] = (double)(s >> shift_factor);
		}

		for (int i = 0; i < 28; i++)
		{
			samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
			s_2 = s_1;
			s_1 = samples[i];
			d = (int)(samples[i] + 0.5);

			if (d > SHRT_MAX) d = SHRT_MAX;
			if (d < SHRT_MIN) d = SHRT_MIN;

			*dst++ = d;
		}

		size -= 16;
	}

	// do we need any form of gaussian interpolation?
	if (loop_start != -1 && loop_end != -1)
	{
		short buff[itpl_size];

		// interpolate
#if 0			// replace tail loop with crossfaded sound
		int start = loop_start;
		int end = loop_end - itpl_size;
		for (int i = 0; i < itpl_size; i++, start++, end++)
		{
			double ratio = (double)i / (double)itpl_size;
			buff[i] = bezier(data[start], data[end], ratio);
		}

		memcpy(&data[loop_end - itpl_size], buff, sizeof(buff));
#else			// add a crossfaded loop tail and keep the original end intact
		int start = loop_start;
		int end = loop_end - itpl_size;
		for (int i = 0; i < itpl_size; i++, start++, end++)
		{
			double ratio = (double)i / (double)itpl_size;
			buff[i] = linear(data[start], data[end], ratio);
		}

		memcpy(&data[loop_end], buff, sizeof(buff));

		// move the loop points forward into the interpolated section
		loop_start += itpl_size;
		loop_end += itpl_size;
		this->size += itpl_size;
#endif
	}
}

typedef struct WAV_HEADER
{
	uint32_t RIFF;           // RIFF Header
	uint32_t ChunkSize;      // RIFF Chunk Size
	uint32_t WAVE;           // WAVE Header
	uint32_t fmt;            // FMT header
	uint32_t Subchunk1Size;  // Size of the fmt chunk
	uint16_t AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
	uint16_t NumOfChan;      // Number of channels 1=Mono 2=Stereo
	uint32_t SamplesPerSec;  // Sampling Frequency in Hz
	uint32_t bytesPerSec;    // bytes per second 
	uint16_t blockAlign;     // 2=16-bit mono, 4=16-bit stereo
	uint16_t bitsPerSample;  // Number of bits per sample
} wav_hdr;

typedef struct WAV_DATA
{
	uint32_t Subchunk2ID;    // "data" string
	uint32_t Subchunk2Size;  // Sampled data length
} wav_data;

typedef struct WAV_LOOP
{
	uint32_t LOOP,
		size;
	uint32_t loop_start,
		loop_end;
} WAV_LOOP;

void PsxSample::Write(const char* filename, int freq)
{
	FILE* fp;
	fopen_s(&fp, filename, "wb+");
	if (fp)
	{
		Write(fp, freq);
		fclose(fp);
	}
}

void PsxSample::Write(FILE *fp, int freq)
{
	wav_hdr head;
	head.RIFF = MAGIC('R', 'I', 'F', 'F');
	head.ChunkSize = size * 2 + sizeof(head) + sizeof(wav_data);
	if (loop_start != -1)
		head.ChunkSize += sizeof(WAV_LOOP);
	head.WAVE = MAGIC('W', 'A', 'V', 'E');
	head.fmt = MAGIC('f', 'm', 't', ' ');
	head.Subchunk1Size = 16;
	head.AudioFormat = 1;
	head.NumOfChan = 1;
	head.SamplesPerSec = freq;
	head.bytesPerSec = freq * 2;
	head.blockAlign = 2;
	head.bitsPerSample = 16;

	fwrite(&head, sizeof(head), 1, fp);

	if (loop_start != -1)
	{
		WAV_LOOP l;
		l.LOOP = MAGIC('l', 'o', 'o', 'p');
		l.size = 8;
		l.loop_start = loop_start;
		l.loop_end = loop_end;
		fwrite(&l, sizeof(l), 1, fp);
	}

	wav_data wav;
	wav.Subchunk2ID = MAGIC('d', 'a', 't', 'a');
	wav.Subchunk2Size = size * 2 /*+ 8*/;
	fwrite(&wav, sizeof(wav), 1, fp);
	fwrite(data, size * 2, 1, fp);
}
