#ifndef _CGRAPHICALUI_H
#define _CGRAPHICALUI_H
#include <Windows.h>
#include <io.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <sstream>
#include "resource.h"
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


class CGraphicalUI{
private:
	HWND hWnd;
	HANDLE hTime;
	HINSTANCE hInst;
	bool StopByUser;
	int volume;
	long position;
	long length;
	TCHAR Filepath[MAX_PATH];
	ID3v1 info;
	StrVector MusicList;
	StrVector::iterator MusicIter;
	MediaFactory MFac;
	IMediaPlayer** ppMP;
	IMediaPlayer* pMP;
	void InitDlg();
	static BOOL CALLBACK DlgProc (HWND hwnd, UINT message, WPARAM wParam,LPARAM lParam);
	static void CALLBACK UpdateTime(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime);
	bool AddList();
	void BrowseFile();
	void PlayPause();
	void PlayListSelected();
	void Stop();
	void PositionChange(HWND hPosCtrl, WPARAM wParam);
	void VolumeChange(HWND hVolCtrl, WPARAM wParam);
	void AddListItem(const TCHAR* path);
	void DeleteListItem();
	void PlayNext();
public:
	CGraphicalUI(IMediaPlayer** mp,HINSTANCE hInstance) 
		:hWnd(NULL),hInst(hInstance), hTime(NULL), position(0), volume(100), length(0), ppMP(mp) ,StopByUser(true){
		Filepath[0] = '\0';
		memset(&info, 0, sizeof(info));
		hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FORMVIEW), 0, DlgProc) ;
		if (hWnd == NULL){
			MessageBox(0,TEXT("Failed to Create Dialog!"),TEXT("Audio Player"),0);
			ExitProcess(0);
		}
		ShowWindow(hWnd, SW_SHOW); 
		SetTimer(hWnd, 1, 55, UpdateTime);

		MusicList.clear();
		locale &loc=locale::global(locale(locale(),"",LC_CTYPE));
		iFileStream ifile("MusicList.list");
		locale::global(loc);

		int pos;
		ifile>>pos;ifile.get();	
		TCHAR temp[MAX_PATH];
		while (ifile.getline(temp, MAX_PATH)){
			AddListItem(temp);
		}
		ifile.close();
		MusicIter = MusicList.begin() + pos;

		if (*mp){
			pMP = *mp;
			pMP->GetFilePath(Filepath);
			SetDlgItemText(hWnd, IDC_CURRENTMUSIC, Filepath);
			int status = pMP->GetStatus();
			if (status == PLAY){
				SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Pause"));
				volume = pMP->GetVolume();
			}
			else
				SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Play"));

			char temp[100];
			sprintf(temp, "Volume:%d%%",volume);
			SetDlgItemTextA(hWnd, IDC_VOLUME, temp);

			info = pMP->GetID3Info();
			sprintf(temp, "%s\n%s\n%s\n%s\n%s\n%d",info.Title,info.Artist,info.Album,info.Year,info.Comment,info.Genre);
			SetDlgItemTextA(hWnd, IDC_INFO, temp);
		}else{
			*mp = pMP = MFac.CreateMP3Player();
		}
		
			HWND hVolCtrl = GetDlgItem(hWnd, IDC_VOLUMECTRL);
			SendMessage(hVolCtrl, TBM_SETPOS, TRUE, volume);
	}
	~CGraphicalUI(){
		KillTimer(hWnd, 1);

		locale &loc=locale::global(locale(locale(),"",LC_CTYPE)); 
		oFileStream ofile("MusicList.list");
		locale::global(loc);

		ofile<<(int)(MusicIter - MusicList.begin())<<endl;
		for (StrVector::iterator it = MusicList.begin(); it != MusicList.end(); it++){
			ofile.write(it->c_str(), it->length());
			ofile.write(TEXT("\n"), 1);
		}
		ofile.close();
		DestroyWindow(hWnd);
	}
	int WaitForReturn();
	bool CheckFileValid(const TCHAR* path){
		if (_taccess(path,0) != 0)
			return false;
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
		_tcscpy(Filepath, path);
		info = pMP->GetID3Info();
		return true;
	}
	void PlayIt(TCHAR* path){
		if ((path == NULL) || path[0] == '\0'){
			return;
		}
		if (!CheckFileValid(path)){
			MessageBox(hWnd, TEXT("Invalid Path!"), TEXT("Audio Player"), MB_OK);
			return;
		}
		_tcscpy(Filepath, path);
		SetDlgItemText(hWnd, IDC_CURRENTMUSIC, Filepath);
		InitMediaPlayer(path);
		PlayPause();
		pMP->SetVolume(volume);
	}
};
LRESULT CALLBACK PosProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
