#pragma once
#include<string>
#include<WinSock2.h>

using namespace std;

class  Config
{
public:
	static const int MAXCONNECTION;		//最大连接数
	static const int BUFFERLENGTH;		//缓冲区大小
	static const string SERVERADDRESS;  //服务器地址
	static const int PORT;				//服务器端口
	static const u_long BLOCKMODE;			//SOCKET阻塞模式
private:
	Config();
	~Config();
};
