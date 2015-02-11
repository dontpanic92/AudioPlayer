#include <windows.h>
#include <sstream>
#include "Application.h"
#include <iostream>
#include <fstream>
using namespace std;
void JustForLab9(char* str){
	AllocConsole();
	freopen("CONIN$", "r+t", stdin);
	freopen("CONOUT$", "w+t", stdout);
	char* pos = strrchr(str,'.');
	if (pos){
        pos++;
        char* temp = pos;
        while(*temp != '\0'){
            if (*temp >= 'A' && *temp <= 'Z')
                *temp += 32;
            temp++;
        }

        if (strcmp(pos,"mp3") == 0)
			cout<<"It's an mp3 file."<<endl;
        else
            cout<<"It's not an mp3 file."<<endl;
    }else
        cout<<"It's not an mp3 file."<<endl;

    ID3v1 info;
	memset(&info, 0, sizeof(info));
	ifstream fin(str);
	fin.seekg(-128, ios::end);
	if ((fin.get() == 'T') && (fin.get() == 'A') && (fin.get() == 'G')){
		fin.read((char*)&info,sizeof(info));
        cout<<"ID3v1 Info:"<<endl;
		cout<<"Title: ";printf(info.Title);cout<<endl;
		cout<<"Artist: ";printf(info.Artist);cout<<endl;
		cout<<"Album: ";printf(info.Album);cout<<endl;
		cout<<"Year: ";printf(info.Year);cout<<endl;
		cout<<"Comment: ";printf(info.Comment);cout<<endl;
	}else
        cout<<"It has no ID3v1 Infomation!"<<endl;

    fin.close();
    system("pause");
    system("cls");
	FreeConsole();
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){
	Application app;
	if (lpCmdLine[0] != '\0'){
		JustForLab9(lpCmdLine);
		app.go(lpCmdLine,hInstance);
	}else
		app.go("",hInstance);
    return 0;
}
