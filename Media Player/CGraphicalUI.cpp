#include "CGraphicalUI.h"
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

FARPROC OldProc = NULL;
#define POSRANGE 1000.0
BOOL CALLBACK CGraphicalUI::DlgProc (HWND hwnd, UINT message, WPARAM wParam,LPARAM lParam){
	DWORD tid = GetWindowThreadProcessId(hwnd, NULL);
	HWND hPosCtrl = GetDlgItem(hwnd, IDC_POSITIONCTRL);
	switch (message){
	case WM_INITDIALOG:
		PostThreadMessage(tid, WM_USER, 0, 0);
		SetFocus(hwnd);
		return true;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)){
		case IDC_BUTTONBROWSE:
			PostThreadMessage(tid, WM_USER + 1, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_BUTTONPLAY:
			PostThreadMessage(tid, WM_USER + 2, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_BUTTONSTOP:
			PostThreadMessage(tid, WM_USER + 3, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_CHANGEUI:
			PostThreadMessage(tid, WM_USER + 7, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_BUTTONADD:
			PostThreadMessage(tid, WM_USER + 8, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_BUTTONDEL:
			PostThreadMessage(tid, WM_USER + 9, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_BUTTONCA:
			PostThreadMessage(tid, WM_USER + 10, 0, 0);
			SetFocus(hwnd);
			break;
		case IDC_LISTMUSIC:
			if (HIWORD(wParam) == LBN_DBLCLK)
				PostThreadMessage(tid, WM_USER + 11, 0, 0);
			break;
		}
		//SetFocus(hwnd);
		break;
	case WM_HSCROLL:
		if (hPosCtrl == (HWND)lParam)
			PostThreadMessage(tid, WM_USER + 4, wParam, lParam);
		else 
			PostThreadMessage(tid, WM_USER + 5, wParam, lParam);
		SetFocus(hwnd);
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		return false;
		break;
	}
	return false;
}
int CGraphicalUI::WaitForReturn(){
	MSG msg;
	int ret = 1;
	HWND hList;
	while (ret && GetMessage (&msg, NULL, 0, 0)){
		switch (msg.message){
		case WM_USER:
			InitDlg();
			break;
		case WM_USER + 1:
			BrowseFile();
			break;
		case WM_USER + 2:
			PlayPause();
			break;
		case WM_USER + 3:
			Stop();
			break;
		case WM_USER + 4:
			PositionChange((HWND)msg.lParam, msg.wParam);
			break;
		case WM_USER + 5:
			VolumeChange((HWND)msg.lParam, msg.wParam);
			break;
		case WM_USER + 6:
			if (pMP){
				position = pMP->GetPosition() ;/// 1000;
				length = pMP->GetTotalLength() ;/// 1000;
			}
			if (pMP->GetStatus() == STOP){
				SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Play"));
				if(StopByUser == false){
					PlayNext();
				}
			}	
			TCHAR time[50];
			wsprintf(time,(position % 60 < 10) ? TEXT("%d:0%d") : TEXT("%d:%d"), position / 60, position % 60);
			wsprintf(time,(length % 60 < 10) ? TEXT("%s/%d:0%d") : TEXT("%s/%d:%d"), time, length / 60, length % 60);
			if (!SendMessage(GetDlgItem(hWnd, IDC_POSITIONCTRL),TBM_SETPOS, true, LPARAM(position * POSRANGE / length)))
				SetDlgItemText(hWnd, IDC_TIME, time);
			break;
		case WM_USER + 7:
			ret = 0;
			break;
		case WM_USER + 8:
			AddList();
			break;
		case WM_USER + 9:
			DeleteListItem();
			break;
		case WM_USER + 10:
			hList = GetDlgItem(hWnd,IDC_LISTMUSIC);
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			MusicList.clear();
			MusicIter = MusicList.begin();
			break;
		case WM_USER + 11:
			PlayListSelected();
			break;
		}
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	}
	return ret;
}
void CGraphicalUI::InitDlg(){
		HWND hPosCtrl = GetDlgItem(hWnd, IDC_POSITIONCTRL);
		HWND hVolCtrl = GetDlgItem(hWnd, IDC_VOLUMECTRL);
		HWND hComboMode = GetDlgItem(hWnd, IDC_COMBOMODE);

		OldProc = (FARPROC)SetWindowLong(hPosCtrl, GWL_WNDPROC, (long)PosProc); 
		SendMessage(hPosCtrl, TBM_SETPAGESIZE, 0, 0);
		SendMessage(hPosCtrl, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, POSRANGE));
		SendMessage(hVolCtrl, TBM_SETPAGESIZE, 0, 5);
		TCHAR* mode[] = {TEXT("Once"), TEXT("Once Repeat"), TEXT("Order"), TEXT("Shuffle"), TEXT("All Repeat")};
		for (int i = 0; i < 5; i++)
			SendMessage(hComboMode, CB_INSERTSTRING, 0, (LPARAM)mode[i]);

		SendMessage(hComboMode, CB_SETCURSEL, 0, 0);
}
void CGraphicalUI::BrowseFile(){
	if (!AddList())
		return;
	SetDlgItemText(hWnd, IDC_CURRENTMUSIC, Filepath);
	InitMediaPlayer(Filepath);
	PlayPause();
	pMP->SetVolume(volume);
}
bool CGraphicalUI::AddList(){
	OPENFILENAME ofn;
	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Filepath;
	ofn.lpstrFile[0] = TEXT('\0');
	ofn.nMaxFile = sizeof(Filepath);
	ofn.lpstrFilter = TEXT("Supported Music File\0*.mp3;*.wma;*.ogg;*.ape\0All File\0*.*\0\0"); 
	ofn.nFilterIndex = 1; 
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_EXPLORER |OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	TCHAR AppPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, AppPath);
	GetOpenFileName(&ofn);
	SetCurrentDirectory(AppPath);

	if (Filepath[0] == '\0'){
		return false;
	}else if (!CheckFileValid(Filepath)){
		MessageBox(hWnd, TEXT("Invalid Path!"), TEXT("Audio Player"), MB_OK);
		return false;
	}
	AddListItem(Filepath);
	return true;
}
void CGraphicalUI::PlayPause(){
	if(!pMP)
		return;
	int status = pMP->GetStatus();
	if (status == PLAY){
		pMP->Pause();
		SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Play"));
	}else if(status == PAUSE){
		StopByUser = false;
		pMP->Resume();
		SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Pause"));
	}else{
		StopByUser = false;
		info = pMP->GetID3Info();
		char temp[100];
		sprintf(temp, "%s\n%s\n%s\n%s\n%s\n%d",info.Title,info.Artist,info.Album,info.Year,info.Comment,info.Genre);
		SetDlgItemTextA(hWnd, IDC_INFO, temp);
		pMP->Play();
		SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Pause"));
	}
}

void CGraphicalUI::Stop(){
		if(!pMP)
			return;
		pMP->Stop();
		StopByUser = true;
		SetDlgItemText(hWnd, IDC_BUTTONPLAY, TEXT("Play"));
}

void CGraphicalUI::PositionChange(HWND hPosCtrl, WPARAM wParam){
	int cur;
	switch(LOWORD(wParam)){
	case SB_THUMBTRACK:
		if (pMP)
			length = pMP->GetTotalLength();
		cur =int( (double)HIWORD(wParam) * (double)length / POSRANGE);
		TCHAR time[50];
		wsprintf(time,(cur % 60 < 10) ? TEXT("%d:0%d") : TEXT("%d:%d"), cur / 60, cur % 60);
		wsprintf(time,(length % 60 < 10) ? TEXT("%s/%d:0%d") : TEXT("%s/%d:%d"), time, length / 60, length % 60);
		SetDlgItemText(hWnd, IDC_TIME, time);
		break;
	case SB_THUMBPOSITION:
		if (pMP)
			length = pMP->GetTotalLength();
		cur =int( HIWORD(wParam) * length / POSRANGE);
		//cur = HIWORD(wParam);
		if (pMP)
			pMP->SetPosition(cur);
		break;
	}

}
void CGraphicalUI::VolumeChange(HWND hVolCtrl, WPARAM wParam){
	TCHAR vol[50];
	switch(LOWORD(wParam)){
	case SB_THUMBTRACK:
		volume = HIWORD(wParam);
		wsprintf(vol, TEXT("Volume:%d%%"),volume);
		if (pMP)
			pMP->SetVolume(volume);
		SetDlgItemText(hWnd, IDC_VOLUME, vol);
		break;
	case SB_PAGELEFT:
		if (pMP){
			volume -= 5;
			pMP->SetVolume(volume);	
			if (volume < 0)
				volume = 0;
		}
		wsprintf(vol, TEXT("Volume:%d%%"),volume);
		SetDlgItemText(hWnd, IDC_VOLUME, vol);
		break;
	case SB_PAGERIGHT:
		if (pMP){
			volume += 5;
			pMP->SetVolume(volume);	
			if (volume > 100)
				volume = 100;
		}
		wsprintf(vol, TEXT("Volume:%d%%"),volume);
		SetDlgItemText(hWnd, IDC_VOLUME, vol);
		break;
	}
}
void CGraphicalUI::AddListItem(const TCHAR* path){
	if (path[0] == '\0')
		return;
	TCHAR* Filename = PathFindFileName(path);
	MusicList.push_back(path);
	HWND hList = GetDlgItem(hWnd, IDC_LISTMUSIC);
	SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)Filename);
}
void CGraphicalUI::DeleteListItem(){
	HWND hList = GetDlgItem(hWnd, IDC_LISTMUSIC);
	int index = SendMessage(hList, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR)
		return;
	SendMessage(hList, LB_DELETESTRING, index, 0);

	StrVector::iterator it = MusicList.begin();
	it += index;
	MusicList.erase(it);
}
void CGraphicalUI::PlayListSelected(){
	HWND hList = GetDlgItem(hWnd, IDC_LISTMUSIC);
	int index = SendMessage(hList, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR)
		return;
	MusicIter = MusicList.begin();
	MusicIter += index;
	InitMediaPlayer(MusicIter->c_str());
	PlayPause();
	SetDlgItemText(hWnd, IDC_CURRENTMUSIC, Filepath);
	pMP->SetVolume(volume);
}
void CGraphicalUI::PlayNext(){
	if (MusicList.empty()){
		MusicIter = MusicList.begin();
		return;
	}
	MusicIter++;
	if (MusicIter == MusicList.end()){
		MusicIter = MusicList.begin();
	}
	InitMediaPlayer(MusicIter->c_str());
	PlayPause();
	SetDlgItemText(hWnd, IDC_CURRENTMUSIC, Filepath);
	pMP->SetVolume(volume);
}
void CALLBACK CGraphicalUI::UpdateTime(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime){
	DWORD tid = GetWindowThreadProcessId(hWnd, NULL);
	PostThreadMessage(tid, WM_USER + 6, 0, 0);
}
LRESULT CALLBACK PosProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	HWND hDlg = GetParent(hWnd);
	RECT   Rect;
	static bool OnDraging = false;
	int Position;
	switch (uMsg){
	case WM_LBUTTONDOWN:
		SendMessage(hWnd,TBM_GETCHANNELRECT, 0, (long)&Rect); 
		Position = int(1.0 * POSRANGE *(LOWORD(lParam)-Rect.left)/(Rect.right - Rect.left) + 0.5); 
		SendMessage(hWnd,TBM_SETPOS, true, Position);
		OnDraging = true;
		break;
	case WM_LBUTTONUP:
		SendMessage(hWnd,TBM_GETCHANNELRECT, 0, (long)&Rect); 
		Position = int(1.0 * POSRANGE *(LOWORD(lParam)-Rect.left)/(Rect.right - Rect.left) + 0.5); 
		OnDraging = false;
		SendMessage(hWnd,TBM_SETPOS, true, Position);
		PostMessage(hDlg, WM_HSCROLL, MAKELONG(Position, SB_THUMBPOSITION), (LPARAM)hWnd);
		break;
	case TBM_SETPOS:
		if (OnDraging)
			return 1;
		break;
	}

	return CallWindowProc((WNDPROC)OldProc,   hWnd,   uMsg,   wParam,   lParam);
}
