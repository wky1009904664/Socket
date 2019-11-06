#include"WinsockEnv.h"
#include<iostream>
#include<WinSock2.h>
#pragma comment(lib,"Ws2_32.lib")

using namespace std;

#define MAJORVERSION 2  //Winsock主版本号
#define MINORVERSION 2	//Winsock次版本号

WinsockEnv::WinsockEnv()
{
}

WinsockEnv::~WinsockEnv()
{
}

int WinsockEnv::Startup()
{
	WSADATA wsaData;
	int rtn;
	WORD wVersionRequested = MAKEWORD(MAJORVERSION, MINORVERSION);

	//Initialize
	rtn = WSAStartup(wVersionRequested, &wsaData);
	if (rtn) {
		cout << "Winsock startup error" << endl;
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != MAJORVERSION || HIBYTE(wsaData.wVersion) != MINORVERSION) {
		WSACleanup();
		cout << "Winsock version error!\n";
		return -1;
	}

	cout << "Winsock startup ok!\n";
	return 0;
}