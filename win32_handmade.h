#if !defined(WIN32_HANDMADE_H)
struct Win32SoundStruct {
	int toneHz;
	int SamplesPerSecond;
	int  BytesPerSample;
	int SoundBufferSize;
	uint32 GlobalSampleIndex;
	int16 ToneVolume;
	real32 SinDegreeValue;//When output one sample , the SinDegreeValue increases by 2.0*Pi*/WavePeriod, therefore in one wavePeriod, the SinIncrease will increase by 2*Pi, and the sin value increase will also change periodically 
	int LatencySampleCount;
	int WavePeriod;
};


struct win32_offscreen_buffer
{
	// NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};




struct win32_window_dimension
{
	int Width;
	int Height;
};


#define WIN32_HANDMADE_H
#endif
