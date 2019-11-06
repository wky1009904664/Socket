#include <iostream>
#include <string>
#include "Server.h"
#include "WinsockEnv.h"
#include "Config.h"
#include <winsock2.h>
#include <algorithm>
#include<fstream>
#include<thread>
#pragma comment(lib, "Ws2_32.lib")

Server::Server()
{
	this->recvBuf = new char[Config::BUFFERLENGTH];
	memset(this->recvBuf, '\0', Config::BUFFERLENGTH);
	this->rcvedMessages = new list<string>();
	this->sessions = new list<SOCKET>();
	this->closedSessions = new list<SOCKET>();
	this->clientAddrMaps = new map<SOCKET, string>();
}

Server::~Server()
{

}

int Server::WinsockStartup()
{
	/*if (WinsockEnv::Startup() == -1)
		return -1;*/
	return WinsockEnv::Startup();
}

//��ʼ��Server����������sockect���󶨵�IP��PORT
int Server::ServerStartup()
{
	this->srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->srvSocket == INVALID_SOCKET) {
		cout << "Server socket creare error !\n";
		WSACleanup();
		return -1;
	}
	cout << "Server socket create ok!\n";

	this->srvAddr.sin_family = AF_INET;
	this->srvAddr.sin_port = htons(Config::PORT);
	//this->srvAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	this->srvAddr.sin_addr.S_un.S_addr = inet_addr(Config::SERVERADDRESS.c_str());

	int rtn = ::bind(this->srvSocket, (LPSOCKADDR)&(this->srvAddr), sizeof(this->srvAddr));

	if (rtn == SOCKET_ERROR) {
		cout << "Server socket bind error!\n";
		closesocket(this->srvSocket);
		WSACleanup();
		return -1;
	}

	cout << "Server socket bind ok!\n";
	return 0;
}

//��ʼ����,�ȴ��ͻ�����������
int Server::ListenStartup()
{
	int rtn = listen(this->srvSocket, Config::MAXCONNECTION);
	if (rtn == SOCKET_ERROR) {
		cout << "Server socket listen error!\n";
		closesocket(this->srvSocket);
		WSACleanup();
		return -1;
	}

	cout << "Server socket listen ok!\n";
	return 0;
}

//���յ��Ŀͻ�����Ϣ���浽��Ϣ����
void Server::AddRecvMessage(string str)
{
	if (!str.empty())
		this->rcvedMessages->push_back(str);
}

//���µĻỰSOCKET�������
void Server::AddSession(SOCKET s)
{
	if (s != INVALID_SOCKET)
		this->sessions->push_back(s);
}

//��ʧЧ�ĻỰSOCKET�������
void Server::AddClosedSession(SOCKET s)
{
	if (s != INVALID_SOCKET)
		this->closedSessions->push_back(s);
}

//��ʧЧ��SOCKET�ӻỰSOCKET����ɾ��
void Server::RemoveClosedSession(SOCKET closedSession)
{
	if (closedSession != INVALID_SOCKET) {
		auto it = find(this->sessions->begin(), this->sessions->end(), closedSession);
		if (it != this->sessions->end())
			this->sessions->erase(it);
	}
}

void Server::RemoveClosedSession()
{
	for (auto it = this->closedSessions->begin(); it != this->closedSessions->end(); it++) {
		this->RemoveClosedSession(*it);
	}
}
// �õ��ͻ���IP��ַ
string Server::GetClientAddress(SOCKET s)
{
	string clientAddress;
	sockaddr_in clientAddr;
	int nameLen, rtn;
	nameLen = sizeof(clientAddr);
	rtn = getsockname(s, (LPSOCKADDR)& clientAddr, &nameLen);
	if (rtn != SOCKET_ERROR) {
		clientAddress += inet_ntoa(clientAddr.sin_addr);
	}
	return clientAddress;
}

string  Server::GetClientAddress(map<SOCKET, string> *maps, SOCKET s) {
	auto itor = maps->find(s);
	if (itor != maps->end())
		return (*itor).second;
	else {
		return string("");
	}
}

//��SOCKET������Ϣ
void Server::recvMessage(SOCKET socket)
{
	int receivedBytes = recv(socket, this->recvBuf, Config::BUFFERLENGTH, 0);
	if (receivedBytes == SOCKET_ERROR || receivedBytes == 0) {
		//this->AddClosedSession(socket);
		string s(this->GetClientAddress(this->clientAddrMaps, socket) + " Left\n");
		//this->AddRecvMessage(s);
		cout << s;
	}
	else {
		recvBuf[receivedBytes] = '\0';
		string s( this->GetClientAddress(this->clientAddrMaps, socket) + "���ο�˵:" + recvBuf + "\n");
		//this->AddRecvMessage(s); //���յ�����Ϣ���뵽��Ϣ����
		cout << s;
		//Head
		string respond = "HTTP/1.1 200 OK\r\n";
		respond += "Content-Type: text/html\r\n";
		
		//Data
		auto spos = s.find_first_of("/");
		auto epos = spos;
		while (s[epos] != ' ')
			epos++;
		string path = s.substr(spos + 1, epos - spos - 1);
		ifstream fin;
		cout << path << endl;
		fin.open(path.c_str(), ios::binary);
		if (!fin) {
			cout << "Open File Error" << endl;
		}
		const int DataBufferLength = 1024;
		char Databuffer[DataBufferLength];
		memset(Databuffer, '\0', DataBufferLength);
		fin.read(Databuffer, DataBufferLength * sizeof(char));
		string tmp(Databuffer);
		cout << tmp << endl;
		respond += "Content-Length: " + to_string(tmp.length()) + "\r\n";
		respond += "\r\n";
		respond += tmp;
		//this->AddRecvMessage(respond);
		//send(socket, respond.c_str(), respond.length(), 0);
		sendMessage(socket, respond);
		cout << respond << endl;
		memset(this->recvBuf, '\0', Config::BUFFERLENGTH);//������ܻ�����
	}
}

//��SOCKET s������Ϣ
void Server::sendMessage(SOCKET socket, string msg) 
{
	int rtn = send(socket, msg.c_str(), msg.length(), 0);
	if (rtn == SOCKET_ERROR) {
		string s(this->GetClientAddress(this->clientAddrMaps, socket) + "Left\n");
		//this->AddRecvMessage(s);
		this->AddClosedSession(socket);
		cout << s;
	}
}

const int bufferLength = 1024;
char recvBuf[bufferLength];
void RespondHTTP(SOCKET newSession)
{
	if (newSession == INVALID_SOCKET) {
		cout << "Server accept connection request error!" << endl;
		//return -1;
	}
	cout << "New client connection request arrived and new session created" << endl;
	auto blockMode = Config::BLOCKMODE;//
	if (ioctlsocket(newSession, FIONBIO, &blockMode) == SOCKET_ERROR) {
		cout << "ioctlsocket() for new session failed with error!\n";
		//return -1;
	}
	
	int receivedBytes = recv(newSession,recvBuf, bufferLength, 0);
	if (receivedBytes == SOCKET_ERROR || receivedBytes == 0) {
		cout << "Recv Error!" << endl;
	}
	else {
		recvBuf[receivedBytes] = '\0';
		//Head
		string respond = "HTTP/1.1 200 OK\r\n";
		respond += "Content-Type: text/html\r\n";
		string s(recvBuf);
		//Data
		auto spos = s.find_first_of("/");
		auto epos = spos;
		while (s[epos] != ' ')
			epos++;
		string path = s.substr(spos + 1, epos - spos - 1);
		ifstream fin;
		cout << path << endl;
		fin.open(path.c_str(), ios::binary);
		if (!fin) {
			cout << "Open File Error" << endl;
		}
		const int DataBufferLength = 1024;
		char Databuffer[DataBufferLength];
		memset(Databuffer, '\0', DataBufferLength);
		fin.read(Databuffer, DataBufferLength * sizeof(char));
		string tmp(Databuffer);
		cout << tmp << endl;
		respond += "Content-Length: " + to_string(tmp.length()) + "\r\n";
		respond += "\r\n";
		respond += tmp;
		//this->AddRecvMessage(respond);
		//send(socket, respond.c_str(), respond.length(), 0);
		//sendMessage(newSession, respond);
		int rtn = send(newSession, respond.c_str(), respond.length(), 0);
		if (rtn == SOCKET_ERROR) {
			cout << "Send Error" << endl;
		}
		cout << respond << endl;
		//memset(this->recvBuf, '\0', Config::BUFFERLENGTH);//������ܻ�����
	}
	//this->AddSession(newSession);
	//this->clientAddrMaps->insert(map<SOCKET, string>::value_type(newSession, this->GetClientAddress(newSession)));//�����ַ
	//string s(this->GetClientAddress(this->clientAddrMaps, newSession) + "Come\n");
	//this->AddRecvMessage(s);
	//cout << s;
}


//����Ƿ��пͻ��������ӵ���
int Server::AcceptRequestionFromClient()
{
	sockaddr_in clientAddr;
	int addrlen = sizeof(clientAddr);
	auto blockMode = Config::BLOCKMODE;//

	//���srvSocket�Ƿ��յ��û���������
	if (this->numOfSocketSignaled > 0)
	{
		if (FD_ISSET(this->srvSocket, &rfds)) {
			this->numOfSocketSignaled--;

			SOCKET newSession = accept(this->srvSocket, (LPSOCKADDR)&clientAddr, &addrlen);//Wrong
			int timeOut = 100;
			setsockopt(newSession, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeOut, sizeof(timeOut));
			
			RespondHTTP(newSession);
			//thread th(RespondHTTP);

		}
	}
	return 0;
}


//���ܿͻ��˷���������
void Server::ReceieveMessageFromClients()
{
	if (this->numOfSocketSignaled > 0) {
		//�����Ự�б��е�����socket������Ƿ������ݵ���
		for (auto itor = this->sessions->begin(); itor != this->sessions->end(); itor++) {
			if (FD_ISSET(*itor, &rfds)) {  //ĳ�Ựsocket�����ݵ���
				//��������
				this->recvMessage(*itor);
			}
		}//end for
	}
}

//���ܿͻ��˷�������������ݲ�ת��
int Server::Loop()
{
	auto blockMode = Config::BLOCKMODE;
	int rtn;
	
	if ((rtn = ioctlsocket(this->srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return -1;
	}
	cout << "ioctlsocket() for server socket ok!Waiting for client connection and data\n";

	while (true)
	{
		this->RemoveClosedSession();

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		FD_SET(this->srvSocket, &rfds);
		for (auto it = this->sessions->begin(); it != this->sessions->end(); ++it) {
			FD_SET(*it, &rfds);
			FD_SET(*it, &wfds);
		}

		if ((this->numOfSocketSignaled = select(0, &this->rfds, &this->wfds, NULL, NULL)) == SOCKET_ERROR) {
			cout << "select() failed with error!\n";
			return -1;
		}
		//���������е������ζ�����û������������������û����ݵ��������лỰsocket���Է�������

		//���ȼ���Ƿ��пͻ��������ӵ���
		if (this->AcceptRequestionFromClient() != 0) return -1;

		//�����ܿͻ��˷���������
		//this->ReceieveMessageFromClients();
	}
	return 0;
}