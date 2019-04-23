#if !defined(HAND_MADE_HERO_H)

/* ========================================================================
$File: $
$Date: $
$Revision: $
$Creator: Casey Muratori $
$Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
======================================================================== */

/*
TODO(casey): Services that the platform layer provides to the game
*/

/*
NOTE(casey): Services that the game provides to the platform layer.
(this may expand in the future - sound on separate thread, etc.)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO(casey): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
#include<stdint.h>
#include<math.h>
#define internal static 
#define local_persist static 
#define global_variable static
#define PI 3.1415926
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))
#define KiloBytes(Value) (Value*1024)
#define MegaBytes(Value) (KiloBytes(Value)*1024LL)
#define GigaBytes(Value) (MegaBytes(Value)*1024LL)
#define TeraBytes(Value) (GigaBytes(Value)*1024LL)
#define Assert(EXPRESSION)  if(!(EXPRESSION)){*(int *)0 = 0;}

typedef float real32;
typedef double real64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;
typedef uint8_t uint8;
typedef int16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;




struct game_memroy {
	void * TransientMemory;
	uint64 TransientMemorySize;
	void * PermanentMemory;
	uint64 PermanentMemorySize;
};


struct game_state {
	bool32 IsInitialized;
	int32 ToneHz;
	int32 BlueOffSet;
	int32 GreenOffSet;

};
struct game_off_screen_buffer {
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};


struct game_sound_buffer {
	
	int SamplesPerSecond;
	int16* Samples;
	int SamplesCount;
};

struct game_button_state {
	int HalfTransitionCount;
	bool32  EndedDown;
};

struct game_controller_input {
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;
	union {
		game_button_state Buttons[12];
		struct {
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;
			game_button_state ActionUp;
			game_button_state ActionDown; 
			game_button_state ActionLeft;
			game_button_state ActionRight;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
			game_button_state Back;
			game_button_state Start;
			// NOTE(Lei Wang): All buttons must be added above this line
			game_button_state Terminator;
		};

	};
	
};


struct game_input {
	game_controller_input Controllers[5];
};

internal  void GameUpdateAndRender(game_state* State,game_input * Input, game_off_screen_buffer * Buffer, game_sound_buffer * SoundBuffer);
internal  void  GameSoundOutput(game_sound_buffer * buffer);
internal  int32 TruncateTo32Bits(int64 Value) {
	Assert(Value <= 0xffffffff);
	return (int32)Value;
}


struct file_result {

	int32  BytesRead;
	void * FileMemory;
};

file_result DebugReadEntireFile( const  char* lpFileName);
int32 DebugWriteFile( const char* lpFileName, void * FileMemory, int32 BytesToWrite);

void DebugOutput();
#define HAND_MADE_HERO_H
#endif 