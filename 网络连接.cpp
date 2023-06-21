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
        std::cout << "WSAStartup ʧ��: " << iResult << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    std::cout << "���ڻ�ȡ��ַ��Ϣ..." << std::endl;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo ʧ��: " << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "���ڴ����׽���..." << std::endl;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "socket ʧ��: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    std::cout << "���ڰ��׽��ֵ��˿�..." << std::endl;
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind ʧ��: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    std::cout << "���ڼ�����������..." << std::endl;
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen ʧ��: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "�ȴ��ͻ�������..." << std::endl;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cout << "accept ʧ��: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "�ͻ��������ӣ���ʼ��������..." << std::endl;

    // ���������ڴ�
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHARED_MEMORY_SIZE,
        TEXT("SharedMemory")
    );
    if (hMapFile == NULL) {
        std::cout << "CreateFileMapping ʧ��: " << GetLastError() << std::endl;
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // ӳ�乲���ڴ�
    LPVOID lpSharedMemory = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        SHARED_MEMORY_SIZE
    );
    if (lpSharedMemory == NULL) {
        std::cout << "MapViewOfFile ʧ��: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // ����������
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("SharedMutex"));
    if (hMutex == NULL) {
        std::cout << "CreateMutex ʧ��: " << GetLastError() << std::endl;
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hMapFile);
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // ��ȡ�����ڴ��е����ݱ�ʶ��������ָ��
    long int* pDataChanged = (long int*)lpSharedMemory;


    system("start ���ݴ���.exe");  // ʹ��start��������ݴ���.exe
    system("start С����ͨ��.exe");  // ʹ��start�����С����ͨ��.exe  

    // ���յ�TCP�źź���д���Ĳ���
    while (true) {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::cout << "----------------------------------------------------------------------" << std::endl;
            std::cout << "���յ����ݣ�" << std::endl;
            std::cout << recvbuf << std::endl;

            std::cout << "���ڻ�ȡ��..." << std::endl;
            // ��ȡ���������
            DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
            if (dwWaitResult == WAIT_OBJECT_0) {
                std::cout << "�ѻ�ȡ��" << std::endl;

                std::cout << "����д������..." << std::endl;
                // �����յ������ݴ��빲���ڴ���
                strncpy_s((char*)lpSharedMemory + sizeof(long int), DEFAULT_BUFFER_LENGTH - sizeof(long int), recvbuf, iResult);
                ((char*)lpSharedMemory)[iResult + sizeof(long int)] = '\0';
                std::cout << "��д������" << std::endl;

                // �������ݱ�ʶ������
                (*pDataChanged)++;

                std::cout << "�����ͷ���..." << std::endl;
                // �ͷŻ��������
                ReleaseMutex(hMutex);
                std::cout << "���ͷ���" << std::endl;
            }
            else {
                std::cout << "�ȴ�����ʱ����ִ���: " << GetLastError() << std::endl;
            }
        }
        else if (iResult == 0)
            std::cout << "���ӹر�..." << std::endl;
        else {
            std::cout << "recv ʧ��: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            CloseHandle(hMutex);
            UnmapViewOfFile(lpSharedMemory);
            CloseHandle(hMapFile);
            WSACleanup();
            return 1;
        }
    }

    // �ر���ͻ��˵�����
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::cout << "shutdown ʧ��: " << WSAGetLastError() << std::endl;
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
