#ifndef _CMP3PLAYER_H
#define _CMP3PLAYER_H

#include "IMediaPlayer.h"
#include <MMSystem.h>
#include <fstream>
#pragma comment(lib,"winmm.lib")

class CMP3Player : public CAudioPlayer{
public:
	TCHAR command[MAX_PATH + 100];
public:
	CMP3Player() {}
	CMP3Player(TCHAR* Filename) : CAudioPlayer(Filename){}
	~CMP3Player(){}
	int GetStatus(){
		TCHAR t[256];
		wsprintf(command,TEXT("status %s mode"),path);
		mciSendString(command, t, 256, NULL);
		if (_tcscmp(t, TEXT("playing")) == 0)
			return PLAY;
		else if (_tcscmp(t, TEXT("paused")) == 0)
			return PAUSE;
		else if (_tcscmp(t, TEXT("stopped")) == 0)
			return STOP;
		else if (_tcscmp(t, TEXT("not ready")) == 0)
			return NOT_READY;
		return -1;
	}
	DWORD Open(){
		wsprintf(command,TEXT("open %s"),path);
		return mciSendString(command,0,0,0);
	}
	DWORD Close(){
		wsprintf(command,TEXT("close %s"),path);
		return mciSendString(command,0,0,0);	
	}
	DWORD Play(){
		wsprintf(command,TEXT("play %s"),path);
		return mciSendString(command,0,0,0);
	}
	DWORD Pause(){
		wsprintf(command, TEXT("pause %s"), path);
		return mciSendString(command,0,0,0);
	}
	DWORD Resume(){
		wsprintf(command, TEXT("resume %s"), path);
		return mciSendString(command,0,0,0);
	}
	DWORD Stop(){
		wsprintf(command, TEXT("stop %s"), path);
		return mciSendString(command,0,0,0);
	}
	DWORD Replay(){
		Stop();
		Close();
		return Play();
	}
	int GetVolume(){
		wsprintf(command, TEXT("status %s volume"), path);
		TCHAR volume[256];
		mciSendString(command, volume, sizeof(volume), 0 );
		return _ttoi(volume) / 10;
	}
	void SetVolume(int num){
		wsprintf(command, TEXT("setaudio %s volume to %d"), path, num * 10);
		mciSendString(command,0,0,0);
	}
	void IncreaseVolume(int vol){
		int volume = GetVolume();
		wsprintf(command, TEXT("setaudio %s volume to %d"), path, (volume + vol) * 10);
		mciSendString(command,0,0,0);
	}
	void DecreaseVolume(int vol){
		int volume = GetVolume();
		wsprintf(command, TEXT("setaudio %s volume to %d"), path, (volume - vol) * 10);
		mciSendString(command,0,0,0);
	}
	long GetTotalLength(){
		wsprintf(command, TEXT("status %s length"), path);
		TCHAR length[256];
		mciSendString(command, length, sizeof(length), 0 );
		return _ttol(length);		
	}
	long GetPosition(){
		wsprintf(command, TEXT("status %s position"), path);
		TCHAR position[256];
		mciSendString(command, position, sizeof(position), 0 );
		return _ttol(position);			
	}
	void SetPosition(int pos){
		int vol = GetVolume();
		Stop();
		if (pos < 0)
			pos = 0;
		wsprintf(command,TEXT("play %s from %d"), path, pos);
		mciSendString(command,0,0,0);
		SetVolume(vol);
	}
	void IncreasePosition(int pos){
		int position = GetPosition();
		SetPosition(position + pos);
	}
	void DecreasePosition(int pos){
		int position = GetPosition();
		SetPosition(position - pos);
	}	
};


#endif
