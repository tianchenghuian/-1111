#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "6598"//11111111
#define DEFAULT_BUFFER_LENGTH 512
#define SHARED_MEMORY_SIZE 1048576 // 1MB

int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL, hints;

    char recvbuf[DEFAULT_BUFFER_LENGTH];
    int recvbuflen = DEFAULT_BUFFER_LENGTH;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup 失败: " << iResult << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    std::cout << "正在获取地址信息..." << std::endl;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo 失败: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "正在创建套接字..." << std::endl;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "socket 失败: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    std::cout << "正在绑定套接字到端口..." << std::endl;
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind 失败: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    std::cout << "正在监听连接请求..." << std::endl;
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen 失败: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "等待客户端连接..." << std::endl;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cout << "accept 失败: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "客户端已连接，开始处理数据..." << std::endl;

    // 创建共享内存
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHARED_MEMORY_SIZE,
        TEXT("SharedMemory")
    );
    if (hMapFile == NULL) {
        std::cout << "CreateFileMapping 失败: " << GetLastError() << std::endl;
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // 映射共享内存
    LPVOID lpSharedMemory = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    );
    if (lpSharedMemory == NULL) {
        std::cout << "MapViewOfFile 失败: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // 创建互斥体
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("SharedMutex"));
    if (hMutex == NULL) {
        std::cout << "CreateMutex 失败: " << GetLastError() << std::endl;
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hMapFile);
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // 获取共享内存中的数据标识符变量的指针
    long int* pDataChanged = (long int*)lpSharedMemory;


    system("start 数据处理.exe");  // 使用start命令打开数据处理.exe
    system("start 小程序通信.exe");  // 使用start命令打开小程序通信.exe  

    // 接收到TCP信号后进行处理的部分
    while (true) {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::cout << "----------------------------------------------------------------------" << std::endl;
            std::cout << "接收到数据：" << std::endl;
            std::cout << recvbuf << std::endl;

            std::cout << "正在获取锁..." << std::endl;
            // 获取互斥体的锁
            DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
            if (dwWaitResult == WAIT_OBJECT_0) {
                std::cout << "已获取锁" << std::endl;

                std::cout << "正在写入数据..." << std::endl;
                // 将接收到的数据存入共享内存中
                strncpy_s((char*)lpSharedMemory + sizeof(long int), DEFAULT_BUFFER_LENGTH - sizeof(long int), recvbuf, iResult);
                ((char*)lpSharedMemory)[iResult + sizeof(long int)] = '\0';
                std::cout << "已写入数据" << std::endl;

                // 自增数据标识符变量
                (*pDataChanged)++;

                std::cout << "正在释放锁..." << std::endl;
                // 释放互斥体的锁
                ReleaseMutex(hMutex);
                std::cout << "已释放锁" << std::endl;
            }
            else {
                std::cout << "等待锁超时或出现错误: " << GetLastError() << std::endl;
            }
        }
        else if (iResult == 0)
            std::cout << "连接关闭..." << std::endl;
        else {
            std::cout << "recv 失败: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            CloseHandle(hMutex);
            UnmapViewOfFile(lpSharedMemory);
            CloseHandle(hMapFile);
            WSACleanup();
            return 1;
        }
    }

    // 关闭与客户端的连接
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cout << "shutdown 失败: " << WSAGetLastError() << std::endl;
        closesocket(ClientSocket);
        CloseHandle(hMutex);
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hMapFile);
        WSACleanup();
        return 1;
    }

    closesocket(ClientSocket);
    CloseHandle(hMutex);
    UnmapViewOfFile(lpSharedMemory);
    CloseHandle(hMapFile);
    WSACleanup();

    return 0;
}
