
#include "windows.h"
#include "Waveform.h"

#define WUM_PLAYBACK		100

#define WUM_PLAYSTART		(WM_USER + WUM_PLAYBACK + 1)
#define WUM_PLAYING			(WM_USER + WUM_PLAYBACK + 2)
#define WUM_PLAYSTOP		(WM_USER + WUM_PLAYBACK + 3)

bool IsPlaying();
double GetPlayingSecondPosition();

void Playback(HWND hwnd, Waveform& wf, double startSecond = 0.0F);
void PlaybackStop(bool forceStop = false);
void PlaybackClose();
