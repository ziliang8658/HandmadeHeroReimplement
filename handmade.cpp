#include"handmade.h"
internal  void  RenderWierdGradient(game_off_screen_buffer * Buffer, int BlueOffset, int GreenOffset) {
	uint8 *Row = (uint8 *)Buffer->Memory;
	uint32 *Pixel = (uint32*)Row;
	for (int Y = 0;
		Y < Buffer->Height;
		++Y)
	{
		for (int X = 0;
			X < Buffer->Width;
			++X)
		{
			uint8 Blue = (uint8)(X + BlueOffset);
			uint8 Green = (uint8)(Y + GreenOffset);
			*Pixel++ = ((Green << 8) | Blue);
		}
	}
}

 



internal  void GameUpdateAndRender(game_state* GameState, game_input * Input,game_off_screen_buffer * Buffer, game_sound_buffer * SoundBuffer) {
	if (!GameState->IsInitialized) {
		GameState->IsInitialized = true;
		GameState->BlueOffSet = 0;
		GameState->GreenOffSet = 0;
		GameState->ToneHz = 0;
		file_result Result = {};
		Result = DebugReadEntireFile(__FILE__);
		DebugWriteFile("output.txt", Result.FileMemory, Result.BytesRead);

	}
	for (int16 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input * Controller = &Input->Controllers[ControllerIndex];
		if (Controller->IsAnalog > 0) {
			GameState->BlueOffSet += (int32)Controller->StickAverageY;
			GameState->GreenOffSet += (int32)Controller->StickAverageY;
			GameState->ToneHz = 256 + (int)(128.0f*(Controller->StickAverageY));

		}
		else {
			game_button_state *LeftButtonState = &Controller->MoveLeft;
			if (LeftButtonState->EndedDown) {
				GameState->GreenOffSet -= 10;
				DebugOutput();
			}
			game_button_state* RightButtonState = &Controller->MoveRight;
			if (RightButtonState->EndedDown) {
				GameState->GreenOffSet += 10;
				GameState->ToneHz= 10;
			

			}


		}

	}
	GameState->BlueOffSet++;
	RenderWierdGradient(Buffer, GameState->BlueOffSet, GameState->GreenOffSet);
	GameSoundOutput(SoundBuffer);
}




internal void GameSoundOutput(game_sound_buffer * buffer) {
	int16* Sample = buffer->Samples;
	int16 SampleValue = 0;
	uint16 ToneHz = 256;
	uint16 ToneVoume = 1000;
	int WavePeriod = buffer->SamplesPerSecond / ToneHz;
	local_persist real32 tSine = 0;
	for (int SampleIndex = 0; SampleIndex < buffer->SamplesCount; SampleIndex++) {
	
		real32 SinValue = sinf(tSine);
		SampleValue = (int16)(SinValue*ToneVoume);
		*Sample++ = SampleValue;
		*Sample++ = SampleValue;
		tSine+= 2.0f*PI*1.0f / (real32)WavePeriod;
		
	}

}

