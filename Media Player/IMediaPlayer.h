#ifndef _IMEDIA_PLAYER_H
#define _IMEDIA_PLAYER_H
#include <Windows.h>
#include <tchar.h>
#include <fstream>
using namespace std;
enum FORMAT{MP_FORMAT_MP3};
enum _status{PLAY, PAUSE, STOP, NOT_READY};
struct ID3v1{
	char Title[30];
	char Artist[30];
	char Album[30];
	char Year[4];
	char Comment[30];
	char Genre;
};

class IMediaPlayer{
protected:
	TCHAR path[MAX_PATH];
	TCHAR longpath[MAX_PATH];
	int status;
public:
	IMediaPlayer() : status(STOP){
		path[0] = '\0';
		longpath[0] = '\0';
	}
	IMediaPlayer(const TCHAR* FilePath) : status(STOP){
		SetFilePath(FilePath);
	}
	virtual ~IMediaPlayer(){}
	void SetFilePath(const TCHAR* FilePath){
		_tcsncpy(longpath, FilePath, MAX_PATH);
		GetShortPathName(longpath, path, MAX_PATH);
	}
	void GetFilePath(TCHAR* out){
		_tcscpy(out, longpath);
	}
	ID3v1 GetID3Info(){
		ID3v1 info;
		memset(&info, 0, sizeof(info));
#ifdef UNICODE
        char path[MAX_PATH];
        wsprintfA(path, "%ls", this->path);
#endif
		ifstream fin(path);
		fin.seekg(-128, ios::end);
		if ((fin.get() == 'T') && (fin.get() == 'A') && (fin.get() == 'G')){
			fin.read((char*)&info,sizeof(info));
		}
		fin.close();
		return info;
	}
	virtual int GetStatus() = 0;
	virtual DWORD Open() = 0;
	virtual DWORD Close() = 0;
	virtual DWORD Play() = 0;
	virtual DWORD Pause() = 0;
	virtual DWORD Resume() = 0;
	virtual DWORD Stop() = 0;
	virtual DWORD Replay() = 0;
	virtual int GetVolume() = 0;
	virtual void SetVolume(int num) = 0;
	virtual void IncreaseVolume(int vol) = 0;
	virtual void DecreaseVolume(int vol) = 0;
	virtual void IncreasePosition(int pos) = 0;
	virtual void DecreasePosition(int pos) = 0;
	virtual long GetTotalLength() = 0;
	virtual long GetPosition() = 0;
	virtual void SetPosition(int) = 0;
};

class CAudioPlayer : public IMediaPlayer{
public:
	CAudioPlayer(){}
	CAudioPlayer(TCHAR* Filename) : IMediaPlayer(Filename){}
	~CAudioPlayer(){}
};

class CCDPlayer : public IMediaPlayer{
public:
	CCDPlayer(){}
	~CCDPlayer(){}
};

class CVedioPlayer : public IMediaPlayer{
public:
	CVedioPlayer(){}
	~CVedioPlayer(){}
};
#endif
