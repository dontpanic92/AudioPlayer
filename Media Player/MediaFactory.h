#ifndef _MEDIAFACTORY_H
#define _MEDIAFACTORY_H
#include "CMP3Player.h"
#include "CAudioPlayer.h"
class MediaFactory{
public:
	MediaFactory(){}
	~MediaFactory(){}
	IMediaPlayer* CreateMediaPlayer(FORMAT Format){
		switch (Format){
		case MP_FORMAT_MP3:
			return CreateMP3Player();
			break;
		default:
			return NULL;
			break;
		}
	}
	IMediaPlayer* CreateMediaPlayer(FORMAT Format, TCHAR* Filename){
		switch (Format){
		case MP_FORMAT_MP3:
			return CreateMP3Player(Filename);
			break;
		default:
			return NULL;
			break;
		}
	}
	IMediaPlayer* CreateMP3Player(){
		return (IMediaPlayer*)new CAudioPlayer2();
	}
	IMediaPlayer* CreateMP3Player(TCHAR* Filename){
		return (IMediaPlayer*)new CAudioPlayer2(Filename);
	}
};
#endif
