/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori Retyped By Lei Wang $
   ======================================================================== */
#undef main 
#include "handmade.cpp"
#include "handmade.h"
#include <windows.h>
#include <stdint.h>
#include<tchar.h> 
#include<Xinput.h>
#include<dsound.h>
#include<math.h>
#include<stdio.h>
#include"win32_handmade.h"
#define internal static 
#define local_persist static 
#define global_variable static
#define Pi32 3.14159265359f
#define PI 3.1415926
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))
global_variable int XOffset = 0;
global_variable int YOffset = 0;
global_variable bool32 GlobalRunning;
global_variable LPDIRECTSOUNDBUFFER GlobalPriamryBuffer;
global_variable DSBUFFERDESC DSDBPrimary;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable DSBUFFERDESC DSDBSecondary;
global_variable bool32 IsPlaying = false;

#define X_INPUT_GET_STATE(name)	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE * pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state * XinputGetState_= XInputGetStateStub;
#define XInputGetState XinputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE * pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
	return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_set_state * XinputSetState_ = XInputSetStateStub;
#define XinputSetState XinputSetState_
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void Win32LoadXInput(void) {
	HMODULE XinputLibrary = LoadLibraryA("xinput1_3.dll");

	if (!XinputLibrary)
	{
		XinputLibrary = LoadLibraryA("xinput1_4.dll");
	}
	if (!XinputLibrary) {
		XinputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}
	if(XinputLibrary){
		XInputGetState = (x_input_get_state*)GetProcAddress(XinputLibrary, "XInputGetState");
		if (!XInputGetState)
			XInputGetState = XInputGetStateStub;
		XinputSetState = (x_input_get_state*)GetProcAddress(XinputLibrary, "XInputSetState");
		if (!XinputSetState)
			XInputGetState = XInputSetStateStub;
	}
	else {
		//TODO: Diagnostic
	}

}

internal void Win32ClearBuffer(Win32SoundStruct *SoundOutput) {
	void * Region1;
	DWORD Region1Size;
	void * Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SoundBufferSize, &Region1, &Region1Size, &Region2, &Region2Size,0))) {
		uint8 * RegionSample = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++) {
			*RegionSample++ = 0;
		}
		RegionSample = (uint8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++) {
			*RegionSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}


internal void Win32InitSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
	HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");
	if (DirectSoundLibrary) {
		direct_sound_create * DirectSoundCreate = (direct_sound_create*)GetProcAddress(DirectSoundLibrary,"DirectSoundCreate");
		IDirectSound* DirectSound = 0;
		if (DirectSoundCreate) {
			if (DirectSoundCreate(NULL, &DirectSound, NULL) == DS_OK) {
				WAVEFORMATEX WaveFormat;
				WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
				WaveFormat.nChannels = 2;
				WaveFormat.nSamplesPerSec = SamplesPerSecond;
				WaveFormat.wBitsPerSample = 16;
				WaveFormat.nBlockAlign = WaveFormat.nChannels*WaveFormat.wBitsPerSample / 8;
				WaveFormat.nAvgBytesPerSec = SamplesPerSecond * WaveFormat.nBlockAlign;
				WaveFormat.cbSize = 0;
				if (DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY) == DS_OK) {
					DSDBPrimary.dwSize = sizeof(DSBUFFERDESC);
					DSDBPrimary.dwFlags = DSBCAPS_PRIMARYBUFFER;
					DSDBPrimary.dwBufferBytes = 0;
					if (DirectSound->CreateSoundBuffer(&DSDBPrimary, &GlobalPriamryBuffer, NULL) == DS_OK) {
						OutputDebugStringA("Primary buffer succssfully created");
						GlobalPriamryBuffer->SetFormat(&WaveFormat);
						DSDBSecondary.dwSize = sizeof(DSBUFFERDESC);
						DSDBSecondary.dwBufferBytes = BufferSize;
						DSDBSecondary.dwFlags = 0;
						DSDBSecondary.lpwfxFormat = &WaveFormat;
						if (DirectSound->CreateSoundBuffer(&DSDBSecondary, &GlobalSecondaryBuffer, NULL) == DS_OK) {
							OutputDebugStringA("Secondary buffer created successfully.\n");
						}
						else {
							//TODO: Diagnostic Info
						}
					}
					else {
						//TODO: Diagnostic Info
					}
				}
				else {
					//TODO: Diagnostic Info
				}
			

			}
			else {
				//TODO: Diagnostic Info
			}

		
		}
		else {
			//TODO: Diagnostic Info
		}
	}
}





// TODO(casey): This is a global for now.
global_variable win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    // NOTE(casey): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    // No more DC for us.
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO(casey): Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
                           int WindowWidth, int WindowHeight,
						   win32_offscreen_buffer* Buffer)
{
    // TODO(casey): Aspect ratio correction
    // TODO(casey): Play with stretch modes

    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  */
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}




internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = 0;
	switch (Message)
	{
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_CLOSE:
	{
		// TODO(casey): Handle this with a message to the user?
		GlobalRunning = false;
	} break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_DESTROY:
	{
		// TODO(casey): Handle this as an error - recreate window?
		GlobalRunning = false;
	} break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
			&GlobalBackbuffer);
		EndPaint(Window, &Paint);
	} break;

	default:
	{
		//            OutputDebugStringA("default\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;

	return Result;
	}
}

internal void Win32FillBuffer(DWORD BytesToLock, DWORD WriteBytes, Win32SoundStruct * SoundStruct, game_sound_buffer * buffer) {
	void * FirstLockedAudioPart;
	DWORD  FirstLockedAudioBytes;
	void * SecondLockedAudioPart;
	DWORD  SecondLockedAudioBytes;
	GlobalSecondaryBuffer->Lock(BytesToLock, WriteBytes, &FirstLockedAudioPart, &FirstLockedAudioBytes, &SecondLockedAudioPart, &SecondLockedAudioBytes, 0);
	int16* Region1Sample = (int16*)FirstLockedAudioPart;
	DWORD Region1SampleCount = FirstLockedAudioBytes / SoundStruct->BytesPerSample;
	int16* SampleSource = (int16*)buffer->Samples;
	for (DWORD Index = 0; Index < Region1SampleCount; Index++) {
		*Region1Sample++ = *SampleSource++;
		*Region1Sample++ = *SampleSource++;
		++SoundStruct->GlobalSampleIndex;

	}
	int16* Region2Sample = (int16*)SecondLockedAudioPart;
	DWORD Region2SampleCount = SecondLockedAudioBytes / SoundStruct->BytesPerSample;
	for (DWORD Index = 0; Index < Region2SampleCount; Index++) {
		*Region2Sample++ = *SampleSource++;
		*Region2Sample++ = *SampleSource++;
		++SoundStruct->GlobalSampleIndex;
	}
	GlobalSecondaryBuffer->Unlock(FirstLockedAudioPart, FirstLockedAudioBytes, SecondLockedAudioPart, SecondLockedAudioBytes);
}



void DealButtonState(game_button_state * OldButtonState, game_button_state * NewButtonState, DWORD ButtonsPressedStates, DWORD ButtonBlt) {
	NewButtonState->EndedDown = (bool32)(ButtonsPressedStates & ButtonBlt);
	NewButtonState->HalfTransitionCount += (OldButtonState->EndedDown == NewButtonState->EndedDown) ? 0 : 1;

}

real32 SetAnalogStickShiftValue( int16 ShiftValue) {
	real32 Result = 0;

	if (ShiftValue < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		Result = (real32)((ShiftValue + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
	}
	else if (ShiftValue > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		Result = (real32)((ShiftValue - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
	}

	return Result;
}


void SetAnalogState(game_controller_input * NewController, int16 XShift, int16 YShift) {
	if(NewController->StickAverageX)
	NewController->StickAverageX = SetAnalogStickShiftValue(XShift);
	NewController->StickAverageY = SetAnalogStickShiftValue(YShift);
	if (NewController->StickAverageX != 0 || NewController->StickAverageY) {
		NewController->IsAnalog = true;
	}
}


file_result DebugReadEntireFile( const char* lpFileName) {
	file_result FileResult = {};
	HANDLE FileHandle = CreateFileA(lpFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize) != false) {
			int32 truncatedFileSize=TruncateTo32Bits(FileSize.QuadPart);
			FileResult.FileMemory = VirtualAlloc(NULL, truncatedFileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (FileResult.FileMemory != NULL) {
				DWORD BytesRead;
				bool32 ReadSuccess = ReadFile(FileHandle, FileResult.FileMemory, truncatedFileSize, &BytesRead, 0);
				if (ReadSuccess &&(truncatedFileSize==BytesRead)){
					FileResult.BytesRead = truncatedFileSize;

				}
				else {

				}
			}
			else {

			}
		
		}
		else {

		}
		


	}
	else {

	}
	return FileResult;

}


internal game_controller_input* GetInputController(game_input * Input, int32 Index) {
	Assert(Index < ArrayCount(Input->Controllers));
	game_controller_input * Result = &Input->Controllers[Index];
	return Result;

}


int32 DebugWriteFile(const char * lpFileName,void * FileMemory,int32 BytesToWrite) {

	HANDLE FileHandle = CreateFileA(lpFileName, GENERIC_WRITE, 0, 0,CREATE_ALWAYS,0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		OVERLAPPED FileOverrerLapped = { 0 };
		if (WriteFile(FileHandle, FileMemory, BytesToWrite, NULL, &FileOverrerLapped) != false) {
		}
		else {
			//LOGGING
		}

	}
	else {
		//LOGGING
	}
	return BytesToWrite;

}

void ProcessKeyBoardMessage(game_button_state * KeyBoardKeyState,bool32 WasDown, bool32 IsDown) {
	Assert(KeyBoardKeyState->EndedDown != IsDown);
	KeyBoardKeyState->EndedDown = IsDown;
	char OutputBuffer[256];
	sprintf_s(OutputBuffer, "EndedDown is %d", KeyBoardKeyState->EndedDown);
	OutputDebugString(OutputBuffer);
	++KeyBoardKeyState->HalfTransitionCount;

}
 
 void ProcessPendingMessage( game_controller_input * Input) {
	 MSG msg;
	 
	 while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		 LPARAM lParam = msg.lParam;
		 WPARAM wParam = msg.wParam;
		 bool32 WasDown = ((lParam&(1 << 30)) != 0);
		 bool32 IsDown = ((wParam &(1 << 31)) == 0);
		 uint32 VKCode = wParam;
		 
		 switch (msg.message) {
		 case WM_KEYDOWN:{
				/* check WasDown does not equals to IsDown to make sure that the keyboard has been set to a new state */
				 if (WasDown != IsDown) {
					 if (VKCode == 'W')
					 {
						 OutputDebugString("Key W is pressed");
						 ProcessKeyBoardMessage(&Input->MoveUp, WasDown, IsDown);

					 }
					 else if (VKCode == 'A')
					 {
						 OutputDebugString("Key A is pressed");
						 ProcessKeyBoardMessage(&Input->MoveLeft, WasDown, IsDown);

					 }
					 else if (VKCode == 'S')
					 {
						 OutputDebugString("Key S is pressed");
						 ProcessKeyBoardMessage(&Input->MoveDown, WasDown, IsDown);

					 }
					 else if (VKCode == 'D')
					 {
						 OutputDebugString("Key D is pressed");
						 ProcessKeyBoardMessage(&Input->MoveRight, WasDown, IsDown);

					 }
					 else if (VKCode == 'Q')
					 {
						 OutputDebugString("Key Q is pressed");
						 ProcessKeyBoardMessage(&Input->LeftShoulder, WasDown, IsDown);

					 }
					 else if (VKCode == 'E')
					 {
						 OutputDebugString("Key E is pressed");
						 ProcessKeyBoardMessage(&Input->RightShoulder, WasDown, IsDown);

					 }
					 else if (VKCode == VK_UP)
					 {
						 OutputDebugString("Key VK_UP is pressed");
						 ProcessKeyBoardMessage(&Input->ActionUp, WasDown, IsDown);

					 }
					 else if (VKCode == VK_LEFT)
					 {
						 OutputDebugString("Key VK_LEFT is pressed");
						 ProcessKeyBoardMessage(&Input->ActionLeft, WasDown, IsDown);

					 }
					 else if (VKCode == VK_DOWN)
					 {
						 ProcessKeyBoardMessage(&Input->ActionDown, WasDown, IsDown);

					 }
					 else if (VKCode == VK_RIGHT)
					 {
						 ProcessKeyBoardMessage(&Input->ActionRight, WasDown, IsDown);

					 }
					 else if (VKCode == VK_ESCAPE) {
						 OutputDebugStringA("ESCAPE: ");
						 if (IsDown)
						 {
							 OutputDebugStringA("IsDown ");
						 }
						 if (WasDown)
						 {
							 OutputDebugStringA("WasDown");
						 }
						 OutputDebugStringA("\n");
					 }
					 else if (VKCode == VK_SPACE)
					 {
						 ProcessKeyBoardMessage(&Input->Start, WasDown, IsDown);
					 }
					 else if (VKCode == VK_BACK) {
						 ProcessKeyBoardMessage(&Input->Back, WasDown, IsDown);
					 }

				 }
				 bool32 AtlKeyWasDown = (lParam&(1 << 29));
				 if (AtlKeyWasDown&&VKCode == VK_F4) {
					 GlobalRunning = false;
				 }
		 }break;
			 default:{
				 TranslateMessage(&msg);
				 DispatchMessageA(&msg);

			 }break;
		 }
	 }


}
 void DebugOutput() {
	 OutputDebugStringA("Key is pressed");
 
 }

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
	LARGE_INTEGER FrequencyCounterPerf;
	QueryPerformanceFrequency(&FrequencyCounterPerf);
	int64 Frequency = FrequencyCounterPerf.QuadPart;
    WNDCLASS WindowClass = {};
	Win32LoadXInput();
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
//    WindowClass.hIcon;
	WindowClass.lpszClassName = _T("Handmade Hero Class");

    if(RegisterClassA((const WNDCLASSA*) &WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                (LPCSTR)WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            // NOTE(casey): Since we specified CS_OWNDC, we can just
            // get one device context and use it forever because we
            // are not sharing it with anyone.
			
            HDC DeviceContext = GetDC(Window);	
			GlobalRunning = true;
			Win32SoundStruct SoundStruct = {};
			SoundStruct.BytesPerSample = sizeof(int16) * 2;
			SoundStruct.toneHz = 256;
			SoundStruct.SamplesPerSecond = 48000;
			SoundStruct.SoundBufferSize = SoundStruct.SamplesPerSecond*SoundStruct.BytesPerSample;
			SoundStruct.WavePeriod = SoundStruct.SamplesPerSecond / SoundStruct.toneHz;
			SoundStruct.GlobalSampleIndex = 0;
			SoundStruct.ToneVolume = 3000;
			SoundStruct.LatencySampleCount = SoundStruct.SamplesPerSecond/ 60;
			Win32InitSound(Window, SoundStruct.SamplesPerSecond, SoundStruct.SoundBufferSize);
			Win32ClearBuffer(&SoundStruct);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
			file_result FileReadResult = {};
			const char * File = __FILE__;
		
			int16* Samples = (int16*)VirtualAlloc(0, SoundStruct.SoundBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			LARGE_INTEGER LastCounter;
			uint64 LastCycleCount = __rdtsc();
			QueryPerformanceCounter(&LastCounter);
			game_memroy Memory = {};
			Memory.PermanentMemorySize = MegaBytes(64);
			Memory.TransientMemorySize = GigaBytes(4);
			int64 Value = TeraBytes(2);
			LPVOID BaseAddress = (LPVOID)Value;
			int64 TotalSize = Memory.PermanentMemorySize + Memory.TransientMemorySize;
			Memory.PermanentMemory = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			Memory.TransientMemory= ((uint8*)Memory.PermanentMemory + Memory.PermanentMemorySize);
			if (Memory.PermanentMemory&&Memory.TransientMemory){
				game_input inputs[2];
				game_input * NewInput = &inputs[0];
				game_input * OldInput = &inputs[1];
				DWORD MAXControllers = XUSER_MAX_COUNT;
				DWORD InputControllersCount = ArrayCount(OldInput->Controllers) - 1;
				if (MAXControllers > InputControllersCount) {
					MAXControllers = InputControllersCount;
				}

				while (GlobalRunning)
				{
					game_controller_input * KeyBoardController = &NewInput->Controllers[0];
					game_controller_input ZeroInput = {};
					*KeyBoardController = ZeroInput;
					KeyBoardController->IsConnected = true;
					//Initialization of KeyBoardController, set every
					for (DWORD KeyButtonIndex = 0; KeyButtonIndex < ArrayCount(KeyBoardController->Buttons); KeyButtonIndex++) {
						KeyBoardController->Buttons->EndedDown = OldInput->Controllers[0].Buttons[KeyButtonIndex].EndedDown;
					}
					
					ProcessPendingMessage(KeyBoardController);
					for (DWORD ControllerIndex = 1; ControllerIndex < MAXControllers+1; ControllerIndex++) {
					
						game_controller_input * NewController = GetInputController(NewInput, ControllerIndex);
						game_controller_input * OldController = GetInputController(OldInput, ControllerIndex);
						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
							XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
							DealButtonState(&OldController->MoveUp, &NewController->MoveUp, Pad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
							DealButtonState(&OldController->MoveDown, &NewController->MoveDown, Pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
							DealButtonState(&OldController->MoveLeft, &NewController->MoveLeft, Pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
							DealButtonState(&OldController->MoveRight, &NewController->MoveRight, Pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
							DealButtonState(&OldController->LeftShoulder, &NewController->LeftShoulder, Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
							DealButtonState(&OldController->RightShoulder, &NewController->RightShoulder, Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
							DealButtonState(&OldController->ActionDown, &NewController->ActionDown, Pad->wButtons, XINPUT_GAMEPAD_A);
							DealButtonState(&OldController->Start, &NewController->Start, Pad->wButtons, XINPUT_GAMEPAD_START);
							DealButtonState(&OldController->Back, &NewController->Back, Pad->wButtons, XINPUT_GAMEPAD_BACK);
							SetAnalogState(NewController, Pad->sThumbLX, Pad->sThumbLY);

						}
						else {
							//LOG
						}
					}

					game_off_screen_buffer RenderBuffer = {};
					RenderBuffer.Height = GlobalBackbuffer.Height;
					RenderBuffer.Width = GlobalBackbuffer.Width;
					RenderBuffer.Memory = GlobalBackbuffer.Memory;
					RenderBuffer.Pitch = GlobalBackbuffer.Pitch;
					DWORD CurrentPlayCursor;
					DWORD CurrentWriteCursor;
					GlobalSecondaryBuffer->GetCurrentPosition(&CurrentPlayCursor, &CurrentWriteCursor);
					DWORD WriteBytes = 0;
					DWORD BytesToLock = (SoundStruct.GlobalSampleIndex* SoundStruct.BytesPerSample) % SoundStruct.SoundBufferSize;
					DWORD TargetCursor = (((SoundStruct.LatencySampleCount*SoundStruct.BytesPerSample) + CurrentPlayCursor) % SoundStruct.SoundBufferSize);
					if (BytesToLock > TargetCursor) {
						WriteBytes = SoundStruct.SoundBufferSize - BytesToLock;
						WriteBytes += TargetCursor;
					}
					else {
						WriteBytes = TargetCursor - BytesToLock;
					}
					game_sound_buffer SoundBuffer = {};
					SoundBuffer.Samples = Samples;
					SoundBuffer.SamplesCount = WriteBytes / SoundStruct.BytesPerSample;
					SoundBuffer.SamplesPerSecond = SoundStruct.SamplesPerSecond;
					game_state * State = (game_state*)Memory.PermanentMemory;
					GameUpdateAndRender(State,NewInput, &RenderBuffer, &SoundBuffer);
					Win32FillBuffer(BytesToLock, WriteBytes, &SoundStruct, &SoundBuffer);
					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
						&GlobalBackbuffer);
					LARGE_INTEGER CurrentCounter;
					uint64 CurrentCycleCount = __rdtsc();
					QueryPerformanceCounter(&CurrentCounter);
					int64 CounterElapses = CurrentCounter.QuadPart - LastCounter.QuadPart;
					real64 MSPF = ((real64)CounterElapses*1000.0f) / (real64)Frequency;
					real64 FPS = (real64)Frequency / (real64)CounterElapses;
					LastCounter = CurrentCounter;
					LastCycleCount = CurrentCycleCount;
					char OutputBuffer[256];
					
					//sprintf_s(OutputBuffer, "%.02fms/f,  %.02ff/s\n", MSPF, FPS);
					//OutputDebugStringA(OutputBuffer);
					/*Running Cursor Integer */
					/* Swap OldInput And New Input*/
					game_input * temp = OldInput;
					OldInput = NewInput;
					NewInput = temp;
				
				}
				
			}
        }
        else
        {
           
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    
    return(0);
}
