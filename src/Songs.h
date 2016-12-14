//songs.h
//contains songs to be played by 8-bit DJ
//default properties of each channel

#ifndef SONGS_H
#define SONGS_H
struct Channel_Options {
	int Active;				//1 if active channel, 0 if inactive
	int Tone;										//Sound Wave to use from Tone.h
  int Attack;            				// name of song
	int Decay;
	int Sustain;
	int Release;
};
typedef const struct Channel_Options Channel;

struct Note {
	int channel;			//channel to be played on
	int midi_note;		//midi note number
	int start_time;		//start_time of note
	int end_time;			//end time of note
};
typedef const struct Note Music_Sheet;

struct Song_Struct {
  char name[30];            				// name of song, maximum of 16 characters
	Channel Channel[16];					//16 different channels
	Music_Sheet *Song;
};
typedef const struct Song_Struct Song;

//extern Song Megaman;
//extern Music_Sheet	Megaman_Sheet[];

//extern Song GuilesTheme;
//extern Music_Sheet GuilesTheme_Sheet[];

//extern Song FlandresTheme;
//extern Music_Sheet FlandresTheme_Sheet[];

//extern Song Whatislove;
//extern Music_Sheet	Whatislove_Sheet[];

extern Song overworld;
extern Music_Sheet overworld_sheet[];

#endif