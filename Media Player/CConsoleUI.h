#ifndef _CCONSOLEUI_H
#define _CCONSOLEUI_H
#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <io.h>
#include <vector>
#include <fstream>
#include "MediaFactory.h"
using namespace std;

#ifdef UNICODE
	typedef vector<wstring>  StrVector;
	typedef wifstream        iFileStream;
	typedef wofstream        oFileStream;
#else
	typedef vector<string>   StrVector;
	typedef ifstream         iFileStream;
	typedef ofstream         oFileStream;
#endif

class CConsoleUI{
private:
	HANDLE hUIThread;
	HANDLE hInputThread;
	DWORD MainId;
	int Current;
	int volume;
	long position;
	long length;
	bool bFromGUI;
	TCHAR Filepath[MAX_PATH];
	bool StopByUser;
	bool Refresh;
	StrVector MusicList;
	StrVector::iterator MusicIter;
	ID3v1 info;
	MediaFactory MFac;
	IMediaPlayer** ppMP;
	IMediaPlayer* pMP;
	void PlayPrev();
	void PlayNext();
	static DWORD WINAPI DrawUI(LPVOID lpParam);
	static DWORD WINAPI WaitInput(LPVOID lpParam);
public:
	CConsoleUI(IMediaPlayer** mp) :position(0), volume(100), length(0), MainId(0),StopByUser(true),Refresh(false),
						hUIThread(NULL), hInputThread(NULL), Current(0), bFromGUI(false), ppMP(mp){
		AllocConsole();
		freopen("CONIN$", "r+t", stdin);
		freopen("CONOUT$", "w+t", stdout);
		SetConsoleTitle(TEXT("Audio Player"));
		Filepath[0] = '\0';

		MusicList.clear();
		locale &loc=locale::global(locale(locale(),"",LC_CTYPE));
		iFileStream ifile("MusicList.list");
		locale::global(loc);

		int pos;
		ifile>>pos;ifile.get();

		TCHAR temp[MAX_PATH];
		while (ifile.getline(temp, MAX_PATH)){
			MusicList.push_back(temp);
		}
		ifile.close();

		MusicIter = MusicList.begin() + pos;
		memset(&info, 0, sizeof(info));
		if (*mp){
			pMP = *mp;
			pMP->GetFilePath(Filepath);
			info = pMP->GetID3Info();
			bFromGUI = true;
		}else{
			*mp = pMP = MFac.CreateMP3Player();
		}
	}
	~CConsoleUI(){
		if (hUIThread){
			TerminateThread(hUIThread,0);
			CloseHandle(hUIThread);
		}
		if (hInputThread){
			TerminateThread(hInputThread,0);
			CloseHandle(hInputThread);
		}

		locale &loc=locale::global(locale(locale(),"",LC_CTYPE)); 
		oFileStream ofile("MusicList.list");
		locale::global(loc);

		ofile<<(int)(MusicIter - MusicList.begin())<<endl;
		for (StrVector::iterator it = MusicList.begin(); it != MusicList.end(); it++){
			ofile.write(it->c_str(), it->length());
			ofile.write(TEXT("\n"), 1);
		}
		ofile.close();
		system("cls");
		FreeConsole();
	}
	int WaitForReturn();
	bool CheckFileValid(const TCHAR* path){
		if (_taccess(path,0) != 0)
			return false;
		return true;
	}
	bool CheckFileFormat(const TCHAR* path){
		TCHAR* pos = new TCHAR[_tcslen(path) + 1];
		_tcscpy(pos,path);
		pos = _tcsrchr(pos,'.');
		if (!pos)
			return false;

		pos++;
		TCHAR* temp = pos;
		while(*temp != '\0'){
			if (*temp >= 'A' && *temp <= 'Z')
                *temp += 32;
            temp++;
		}


		return true;
	}
	bool InitMediaPlayer(const TCHAR* path){
		if (path[0] == '\0')
			return true;
		if (!CheckFileValid(path))
			return false;
		if (pMP){
			pMP->Stop();
			pMP->Close();
			delete pMP;
			pMP = NULL;
		}
		*ppMP = pMP = MFac.CreateMP3Player();
		pMP->SetFilePath(path);
		info = pMP->GetID3Info();
		return true;
	}
};
#endif
