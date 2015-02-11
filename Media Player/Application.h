#ifndef _APPLICATION_H
#define _APPLICATION_H
#include <iostream>
#include <Windows.h>
#include "MediaFactory.h"
#include "CConsoleUI.h"
#include "CGraphicalUI.h"

class Application{
private:
	IMediaPlayer* pMP;
	CConsoleUI* pConsoleUI;
	CGraphicalUI* pGraphicalUI;
public:
	Application() : pMP(NULL), pConsoleUI(NULL), pGraphicalUI(NULL){setlocale(LC_ALL,"chs");}

	void go(const char* arg, HINSTANCE hInst){
		TCHAR path[MAX_PATH];
		wsprintf(path, TEXT("%hs"), arg);

		pGraphicalUI = new CGraphicalUI(&pMP, hInst);
		pGraphicalUI->PlayIt(path);
		while ( 1 ){
			if(pGraphicalUI->WaitForReturn()){
				break;
			}
			delete pGraphicalUI;
			pGraphicalUI = NULL;

			pConsoleUI = new CConsoleUI(&pMP);
			if (pConsoleUI->WaitForReturn()){
				break;
			}
			delete pConsoleUI;
			pConsoleUI = NULL;
			pGraphicalUI = new CGraphicalUI(&pMP, hInst);
		}
	}
	~Application(){
		if (pMP)
			delete pMP;
		if (pConsoleUI)
			delete pConsoleUI;
		if (pGraphicalUI)
			delete pGraphicalUI;
	}
};
#endif
