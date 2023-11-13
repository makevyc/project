// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include<list>
#include <stdio.h>
#include <atlimage.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData,size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))
        {
            strOut += "\n";
        }
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() //1->A 2->B  3-> C 4->D  1-26 ->Z  盘 就26个分区 
{
    std::string result;
    for (int i = 1; i <= 26; i++)
    {
        if (_chdrive(i) == 0)
        {
            TRACE("--------\r\n");
            TRACE("当前盘符：%d\r\n", i);
            TRACE("--------\r\n");
            if (result.size() > 0)
            {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());//打包用的
    Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;

}


int MakeDirectoryInfo()
{
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
    {
        OutputDebugString(_T("当前命令，不是获取文件列表，命令解析错误！！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问该目录！！"));
        return -2;
    }
    intptr_t  hfind = 0;
    _finddata_t fdata;
    if ((hfind = _findfirst("*", &fdata)) == -1)
    {
        OutputDebugString(_T("没有找到任何文件"));
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
    } while (!(_findnext(hfind, &fdata)));
    //发送信息到控制端
    FILEINFO finfo; 
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);

    return 0;
}

int RunFile()
{
    std::string strPath; 
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL,strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#pragma warning(disable:4996)
int DownloadFile()
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");

    if (err!=0)
    {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL)
    {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket pack(4, (BYTE*)&data, 8);
        fseek(pFile, 0, SEEK_SET); //设置到文件头部
        char buffer[1024] = "";
        size_t rLen = 0;
        do
        {
            rLen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rLen);
            CServerSocket::getInstance()->Send(pack);

        } while (rLen >= 1024); //读到尾
    }
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    fclose(pFile);
    return 0;
}


int MouseEvent()
{
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse))
    {
        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4: //鼠标移动
            nFlags = 8;
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction)
        {
            // 1 | 10 
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());

            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);//表明已经完成


    }
    else
    {
        OutputDebugString(_T("获取鼠标操作参数失败!"));
        return -1;
    }

    return 0;
}

int SendScreen()
{
    CImage screen;
    HDC hScreen = ::GetDC(NULL); //拿到屏幕的句柄
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL)return -1;
    IStream* pStream = NULL;
    HRESULT ret =  CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG); //流保存的是PNG的数据
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG); //save重载一个是保存到文件上，一个是流上
    //screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}

#include "LockDialog.h"
CLockDialog dlg;
unsigned threadId = 0;
unsigned _stdcall threadLockDlg(void* ary)
{
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    CRect rect;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//得到满屏的像素
    rect.left = 0;
    rect.top = 0;
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    TRACE("rect.bottom : %d", rect.bottom);
    rect.bottom += LONG(89);
    dlg.MoveWindow(rect);
    //窗口置顶
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //隐藏鼠标
    //ShowCursor(false);
    //隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_Traywnd"), NULL), SW_HIDE);
    rect.right = 0;
    rect.left = 0;
    rect.top = 0;
    rect.bottom = 0;
    ClipCursor(rect);    //限制鼠标的移动，将光标限制在屏幕上的矩形区域
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN)
        {
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x41) //按下esc退出
            {
                break;
            }
        }
    }
    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    _endthread();
    return 0;
}
int LockMachine()
{
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) //还没创建，就创建一个线程
    {
        //_beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadId);
        TRACE("threadId = %d\r\n", threadId);

    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine()
{
   // dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
    PostThreadMessage(threadId, WM_KEYDOWN, 0x41, 0);
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect()
{
    CPacket pack(1981, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret = %d\r\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd)
{
    int ret = 0;
    switch (nCmd)
    {
    case 1: //查看磁盘分区
       ret= MakeDriverInfo();
        break;
    case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
        break;
    case 3://打开文件
        ret = RunFile();
        break;
    case 4://下载文件
        ret = DownloadFile();
        break;
    case 5://鼠标操作
        ret = MouseEvent();
        break;
    case 6://发送屏幕内容
        ret = SendScreen();
        break;
    case 7:
        ret = LockMachine(); //锁机
        break;
    case 8:
        ret = UnlockMachine();//解锁
        break;
    case 1981:
        ret = TestConnect();
        break;
    }
    return ret;
}
int main()
{
    int nRetCode = 0;
    
    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            //1.套接字环境初始化
            CServerSocket *pserver =  CServerSocket::getInstance();
            int count = 0;          
            if (pserver->InitSocket() == false)
            {
                MessageBox(NULL, _T("网络初始化失败,请检查网络状态"), _T("初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance()!=NULL)
            {
                if (pserver->AcceptClient() == false)
                {
                    if (count >= 3)
                    {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法接入，正在重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("AceeptClient return true\r\n");
                int ret = pserver->DealCommand();
                TRACE("DealCommand ret %d\r\n", ret);
                if (ret > 0)
                {                    
                    ExcuteCommand(ret);
                    if (ret != 0)
                    {
                        TRACE("执行命令：%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("Command has done\r\n");
                }
            }       
           // Sleep(5000);
           // UnlockMachine();//解锁
           // while ((dlg.m_hWnd != NULL) && (dlg.m_hWnd != INVALID_HANDLE_VALUE));//一直等到为空
           //// Sleep(100);
           // TRACE("m_hWind:%d\r\n", dlg.m_hWnd);
           // while ((dlg.m_hWnd != NULL) && (dlg.m_hWnd != INVALID_HANDLE_VALUE));//一直等到为空
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
