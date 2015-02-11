#include "CConsoleUI.h"
#include <conio.h>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country

int CConsoleUI::WaitForReturn(){
	MainId = GetCurrentThreadId();
	hUIThread = CreateThread(NULL, 0, DrawUI, this, 0, NULL);
	hInputThread = CreateThread(NULL, 0, WaitInput, this, 0, NULL);
	MSG messages;

	if (Filepath[0] == '\0'){
		bool flag = true;
		bFromGUI = false;
		while (GetMessage(&messages, NULL, 0, 0) && flag){
			switch(messages.message){
				case WM_USER + 4:
					if (MusicIter->size())
						break;
					if (MusicList.size() > 0){
						_tcscpy(Filepath, MusicIter->c_str());
						InitMediaPlayer(MusicIter->c_str());
						Refresh = true;
						StopByUser = false;
						flag = false;
					}
					break;
				case WM_USER + 9:
					if (InitMediaPlayer(Filepath))
						flag = false;
					break;
				case WM_USER + 10:
					HANDLE h[2];
					h[0] = hUIThread;
					h[1] = hInputThread;
					WaitForMultipleObjects(2,h,TRUE,-1);
					return 0;
				case WM_USER + 11:
					PlayPrev();
					Refresh = true;
					bFromGUI = true;
					StopByUser = false;
					flag = false;
					break;
				case WM_USER + 12:
					PlayNext();
					Refresh = true;
					bFromGUI = true;
					StopByUser = false;
					flag = false;
					break;
				break;
			}
		}
	}
	if (bFromGUI == true){
		bFromGUI = false;
	}else{
		pMP->Play();
		StopByUser = false;
	}
	volume = pMP->GetVolume();
	length = pMP->GetTotalLength();
	Current = 0;
	bool pause = false;

	PeekMessage(&messages, NULL, 0, 0, PM_NOREMOVE);
	while ((messages.message != WM_QUIT) ){
		GetMessage(&messages, NULL, 0, 0);
		switch(messages.message){
			case WM_USER:
				position = pMP->GetPosition();
				volume = pMP->GetVolume();
				if (pMP->GetStatus() == STOP && StopByUser == false){
						PlayNext();
						Refresh = true;
				}	
				break;
			case WM_USER + 1:
				if (pause == false){
					pause = true;
					pMP->Pause();
				}else{
					pause = false;
					pMP->Resume();
					StopByUser = false;
				}
				break;
			case WM_USER + 2:
				pMP->Resume();
				StopByUser = false;
				break;
			case WM_USER + 3:
				pMP->Stop();
				StopByUser = true;
				break;
			case WM_USER + 4:
				pMP->Replay();
				StopByUser = false;
				break;
			case WM_USER + 5:
				pMP->DecreaseVolume(5);
				break;
			case WM_USER + 6:
				pMP->IncreaseVolume(5);
				break;
			case WM_USER + 7:
				pMP->DecreasePosition(3);
				break;
			case WM_USER + 8:
				pMP->IncreasePosition(3);
				break;
			case WM_USER + 9:
				if (InitMediaPlayer(Filepath)){
					pMP->Play();
					volume = pMP->GetVolume();
					length = pMP->GetTotalLength();
					Current = 0;
				}
				break;
			case WM_USER + 10:
				HANDLE h[2];
				h[0] = hUIThread;
				h[1] = hInputThread;
				WaitForMultipleObjects(2, h, TRUE, -1);
				return 0;
				break;
			case WM_USER + 11:
				PlayPrev();
				Refresh = true;
				break;
			case WM_USER + 12:
				PlayNext();
				Refresh = true;
				break;
		}
	}
	return 1;
}

DWORD WINAPI CConsoleUI::DrawUI(LPVOID lpParam){
	CConsoleUI* pMe = (CConsoleUI*)lpParam;
	HANDLE   hCon;
	COORD   setps;
	int vol;
	long now;
	long total ;
	bool first0 = true, first1 = false ,first2 = false;
	hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	while (pMe->Current != -1){
		PostThreadMessage(pMe->MainId,WM_USER,0,0);
		if (pMe->Current == 0){
			now = pMe->position;
			vol = pMe->volume;
			total = pMe->length;
			if (first0 == true || pMe->Refresh){
				pMe->Refresh = false;
				first0 = false;
				first1 = true;
				first2 = true;
				system("cls");
				TCHAR path[100];
				if (_tcslen(pMe->Filepath) > 43){
					_tcsncpy(path, pMe->Filepath, 10);
					path[10] = '\0';
					_tcscat(path, TEXT("..."));
					_tcscat(path, &pMe->Filepath[_tcslen(pMe->Filepath) - 30]);
				}else
					_tcscpy(path,pMe->Filepath);
				cout<<"©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥Audio Player©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥©¥"<<endl;
				cout<<"      Current Music:  ";
				setlocale(LC_CTYPE, "");
				(path[0] == '\0') ? _tprintf(TEXT("  Please Press F to Select a File\n")) : _tprintf(TEXT("%s\n"),path);
				setlocale(LC_CTYPE, "chs");
				cout<<"      ©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤"<<endl;
				cout<<"           Detail Info:"<<endl;
				cout<<"               Title: ";printf(pMe->info.Title);cout<<endl;
				cout<<"              Artist: ";printf(pMe->info.Artist);cout<<endl;
				cout<<"               Album: ";printf(pMe->info.Album);cout<<endl;
				cout<<"                Year: ";printf(pMe->info.Year);cout<<endl;
				cout<<"             Comment: ";printf(pMe->info.Comment);cout<<endl;
				cout<<"               Genre: "<<(int)pMe->info.Genre<<endl;
				cout<<"©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤"<<endl;
				cout<<now / 60<<":"<<((now % 60 < 10) ? "0" : "")<<now % 60<<"/";
				cout<<total / 60<<":"<<((total % 60 < 10) ? "0" : "")<<total % 60;
				cout<<"  Volume:"<<setw(3)<<vol<<"%  [>                                                ]"<<endl;
				cout<<"¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T¨T"<<endl;
				cout<<endl;
				cout<<" P=Play/Replay | SPACE=Pause/Resume         PLAY LIST"<<endl;
				cout<<" R=Resume      | S=Stop                     ----------------------------"<<endl;
				cout<<" C=Change UI   | F=Select File"<<endl;
				cout<<" Q=Quit        | L=Edit List"<<endl;
				cout<<"¡ü=Play Prev   |¡ý=Play Next  "<<endl<<endl;
				cout<<" - = Decrease Volume by 5%  "<<endl;
				cout<<" = = Increase Volume by 5%  "<<endl;
				cout<<" , = Decrease Position by 3s "<<endl;
				cout<<" . = Increase Position by 3s "<<endl;
				setps.X = 42;
				setps.Y = 16;
				SetConsoleCursorPosition(hCon,setps);
				StrVector::iterator it = pMe->MusicList.begin();
				int num = 7;
				bool _1 = false, _2 = false;

				if (pMe->MusicList.size() > 7){
					if (pMe->MusicIter - pMe->MusicList.begin() > 3){
						_1 = true;
						it = pMe->MusicIter  - 2;
						num--;
					}
					if (pMe->MusicList.end() - pMe->MusicIter > 4){
						_2 = true;
						num--;
					}else{
						it = pMe->MusicList.end() - 6;
					}
				}

				if (_1){
					SetConsoleCursorPosition(hCon,setps);
					setps.Y++;
					cout<<"         ......";
				}
				for (int i = 0; i < num && it != pMe->MusicList.end(); i++){
					SetConsoleCursorPosition(hCon,setps);
					setps.Y++;
					if (it == pMe->MusicIter)
						_tprintf(TEXT("=> %s"),PathFindFileName(it->c_str()));
					else
						_tprintf(TEXT("   %s"),PathFindFileName(it->c_str()));
					it++;
				}
				if (_2){
					SetConsoleCursorPosition(hCon,setps);
					cout<<"         ......";
				}
			}else {
				setps.X = 0;
				setps.Y = 11;
				SetConsoleCursorPosition(hCon,setps);
				cout<<now / 60<<":"<<((now % 60 < 10) ? "0" : "")<<now % 60<<"/";
				cout<<total / 60<<":"<<((total % 60 < 10) ? "0" : "")<<total % 60;
				cout<<"  Volume:"<<setw(3)<<vol<<"%  [";
				int i,n;
				if (total == 0)
					n = 0;
				else
					n = int((double)now / (double)total * 48);
				for (i = 0; i < n; i++)
					cout<<"/";
				cout<<">";
				for (i = 0; i < 48 - n; i++)
					cout<<" ";
				cout<<"]";
			}
		}else if (pMe->Current == 1){
			if (first1 == true){
				first1 = false;
				first0 = true;
				first2 = true;
				system("cls");
				cout<<"Please Input the Filename of Music File (Enter to Return): "<<endl;
			}
		}else if (pMe->Current == 2){
			if (first2 == true || pMe->Refresh){
				system("cls");
				first0 = true;
				first1 = true;
				first2 = false;
				StrVector::iterator it;
				int i;
				for (i = 1, it = pMe->MusicList.begin(); it != pMe->MusicList.end(); it++, i++){
						if(it == pMe->MusicIter)
							_tprintf(TEXT("        -> %d. %s\n"), i, PathFindFileName(it->c_str()));
						else
							_tprintf(TEXT("           %d. %s\n"), i, PathFindFileName(it->c_str()));
				}
				cout<<endl<<"      1.Add a music to play list";
				cout<<endl<<"      2.Delete a music from play list";
				cout<<endl<<"      3.Return";
				cout<<endl<<"Please input yout choice >";
			}
		}
		Sleep(250);
	}
	return 0;
}

DWORD WINAPI CConsoleUI::WaitInput(LPVOID lpParam){
	CConsoleUI* pMe = (CConsoleUI*)lpParam;
	bool flag = true;
	INPUT_RECORD InRec;
    DWORD NumRead;
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);        
	while (flag){
		if (pMe->Current == 0){
			ReadConsoleInput(hIn,&InRec,1,&NumRead);
			if(InRec.EventType!=KEY_EVENT)
				continue;
			if(InRec.Event.KeyEvent.bKeyDown == 0)
				continue;
			switch (InRec.Event.KeyEvent.wVirtualKeyCode){
			case 'q':
			case 'Q':
				pMe->Current = -1;
				flag = false;
				break;
			case ' ':
				PostThreadMessage(pMe->MainId,WM_USER + 1,0,0);
				break;
			case 'r':
			case 'R':
				PostThreadMessage(pMe->MainId,WM_USER + 2,0,0);
				break;
			case 's':
			case 'S':
				PostThreadMessage(pMe->MainId,WM_USER + 3,0,0);
				break;
			//case 'p':
			case 'P':
				PostThreadMessage(pMe->MainId,WM_USER + 4,0,0);
				break;
			case VK_OEM_MINUS:
				PostThreadMessage(pMe->MainId,WM_USER + 5,0,0);
				break;
			case VK_OEM_PLUS:
				PostThreadMessage(pMe->MainId,WM_USER + 6,0,0);
				break;
			case VK_OEM_COMMA:
				PostThreadMessage(pMe->MainId,WM_USER + 7,0,0);
				break;
			case VK_OEM_PERIOD:
				PostThreadMessage(pMe->MainId,WM_USER + 8,0,0);
				break;
			case VK_UP:
				PostThreadMessage(pMe->MainId,WM_USER + 11,0,0);
				break;
			case VK_DOWN:
				PostThreadMessage(pMe->MainId,WM_USER + 12,0,0);
				break;
			case 'f':
			case 'F':
				pMe->Current = 1;
				break;
			case 'c':
			case 'C':
				pMe->Current = -1;
				flag = false;
				PostThreadMessage(pMe->MainId,WM_USER + 10,0,0);
				return 0;
				break;
			case 'L':
				pMe->Current = 2;
				break;
			}
		}else if(pMe->Current == 1){
#ifdef UNICODE
				wchar_t path[MAX_PATH];
				wcin.getline(path, MAX_PATH);
#else
				char path[MAX_PATH];
				cin.getline(path, MAX_PATH);
#endif
				if (pMe->CheckFileValid(path) && pMe->CheckFileFormat(path)){
					_tcscpy(pMe->Filepath, path);
					PostThreadMessage(pMe->MainId, WM_USER + 9, 0, 0);
				}
				else if (path[0] != '\0'){
					cout<<"Invalid File Name Or UnSupported Format! Press Any Key to Return..."<<endl;
					getch();
				}
				pMe->Current = 0;
		}else if (pMe->Current == 2){
				ReadConsoleInput(hIn,&InRec,1,&NumRead);
			if(InRec.EventType!=KEY_EVENT)
				continue;
			if(InRec.Event.KeyEvent.bKeyDown == 0)
				continue;
			switch (InRec.Event.KeyEvent.wVirtualKeyCode){			
			case '3':
				cout<<"3"<<endl;
				pMe->Current = 0;
				break;
			}
		}
	}
	PostThreadMessage(pMe->MainId, WM_QUIT, 0, 0);
	return 0;
}

void CConsoleUI::PlayNext(){
	if (MusicList.empty()){
		MusicIter = MusicList.begin();
		return;
	}
	MusicIter++;
	if (MusicIter == MusicList.end()){
		MusicIter = MusicList.begin();
	}

	_tcscpy(Filepath, MusicIter->c_str());
	InitMediaPlayer(MusicIter->c_str());
	pMP->Play();
	pMP->SetVolume(volume);
	length = pMP->GetTotalLength();
}
void CConsoleUI::PlayPrev(){
	if (MusicList.empty()){
		MusicIter = MusicList.begin();
		return;
	}
	if (MusicIter == MusicList.begin()){
		MusicIter = MusicList.end();
	}
	MusicIter--;

	_tcscpy(Filepath, MusicIter->c_str());
	InitMediaPlayer(MusicIter->c_str());
	pMP->Play();
	pMP->SetVolume(volume);
	length = pMP->GetTotalLength();
}
