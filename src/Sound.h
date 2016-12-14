//Sound.h
#include "songs.h"

extern signed int pitch_bend;
extern int playing;
extern int SongActive;
void DAC_Init(void);
void Play(Song *PlaySong);
void DAC_Out(unsigned short data);
void Sound_Init(void);
void MusicStart(void);
void MusicStop(void);

void Rewind(void);
void Pause(void);
void Resume(void);