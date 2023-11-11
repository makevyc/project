#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)

class CPacket
{
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0)
	{

	}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0;j<strData.size();j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}

	}
	CPacket(const CPacket& pack) //复制构造
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		strData = pack.strData;
		sCmd = pack.sCmd;
		sSum = pack.sSum;

	}
	CPacket(const BYTE* pData,size_t &nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 +2 +2  > nSize) //len cmd dum 可以没有 data 
		{
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) //包没有完全接收到
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;//i到达
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j] & 0xFF);
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;

	}

	CPacket& operator=(const CPacket &pack) //重载=
	{
		if (this != &pack)
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			strData = pack.strData;
			sCmd = pack.sCmd;
			sSum = pack.sSum;
		}
		return *this;
	}

	~CPacket()
	{

	}
	int Size()
	{
		return nLength + 6;
	}

	const char * Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(WORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead; //包头 FEFF
	DWORD nLength; //包长度 （从控制命令开始，到和校验结束）
	WORD sCmd; //控制命令
	std::string strData; // 包数据
	WORD sSum; //和校验
	std::string strOut;


};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击，移动，攻击
	WORD nButton;//左键，右键，中键
	POINT ptXY;//坐标

}MOUSEEV,*PMOUSEEV;

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
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.S_un.S_addr = INADDR_ANY; //服务器有多个Ip, 对外加对内。
		serv_adr.sin_port = htons(9527);
		//绑定
		if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
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
#define BUFFER_SIZE 4096
	int DealCommand() //处理消息/命令
	{
		if (m_client == -1) return false;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE -index, 0);
			if (len < 0)
			{
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, index);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize)
	{
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send( CPacket& pack)
	{
		if (m_client == -1)return false;
		return send(m_client,pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))  //命令2才执行
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent( MOUSEEV &mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

private:
	SOCKET m_socket;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss)
	{

	}
	CServerSocket(const CServerSocket& ss)
	{
		m_socket = ss.m_socket;
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
