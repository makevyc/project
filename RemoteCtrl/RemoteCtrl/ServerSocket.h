#pragma once
#include "pch.h"
#include "framework.h"


class CServerSocket
{

public:
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket;
		}
		return m_instance;
	}
	bool InitSocket()
	{
		if (m_socket == -1)return false;
		sockaddr_in serv_adr, client_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.S_un.S_addr = INADDR_ANY; //服务器有多个Ip,对外加对内。
		serv_adr.sin_port = htons(9527);
		//绑定
		if (bind(m_socket, (sockaddr*)&m_socket, sizeof(m_socket)) == -1)
		{
			return false;
		}
		if (listen(m_socket, 1) == -1)
		{
			return false;

		}
		return true;

	}


	bool AcceptClient()
	{
		sockaddr_in  client_adr;

		char buffer[1024];
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_socket, (sockaddr*)&client_adr, &cli_sz);
		if(m_client==-1)
		{
			return false;
		}
		return true;
		// recv(client, buffer, sizeof(buffer), 0);
		//send(client, buffer, sizeof(buffer), 0);
	}
	 
	int DealCommand()
	{
		if (m_client == -1) return false;
		char buffer[1024] = "";
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len < 0)
			{
				return -1;
			}
			//TODO:
		}
	}

	bool Send(const char* pData, size_t nSize)
	{

		return send(m_client, pData, nSize, 0) > 0;
	}


private:
	SOCKET m_socket;
	SOCKET m_client;
	CServerSocket& operator=(const CServerSocket& ss)
	{

	}
	CServerSocket(const CServerSocket& ss)
	{
		m_socket == ss.m_socket;
		m_client = ss.m_client;
	}


	CServerSocket()
	{
		m_client = INVALID_SOCKET;// =-1 无效的套接字
		if (InitSockEnv()==FALSE)//i c on error
		{
			MessageBox(NULL, _T("无法初始化套接字环境，检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		 m_socket = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket()
	{
		closesocket(m_socket);
		WSACleanup();
	}

	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	static CServerSocket*  m_instance;
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};
//extern CServerSocket server;
