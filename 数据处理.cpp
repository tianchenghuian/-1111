#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <direct.h> // for _mkdir

#define SHARED_MEMORY_SIZE 1048576 // 1MB

//�ַ�����ĸ�������
std::string extractBetween(const char* start, const char* end) {
    std::string result;
    if (start != nullptr && end != nullptr) {
        while (start != end) {
            result += *start;
            ++start;
        }
    }
    return result;
}

//�ַ�����Ĵ�����
void extractAndPrint(const std::string& s) {
    const char* memory = s.c_str();
    const char* keywords[] = { "Tempeture", "Wet", "Light", "Pressure", nullptr };

    std::string temperature, wet, light, pressure;

    const char** keyword = keywords;
    const char* start = strstr(memory, *keyword);
    if (start != nullptr) {
        start += strlen(*keyword);
    }

    while (*++keyword != nullptr) {
        const char* end = strstr(memory, *keyword);
        if (end != nullptr) {
            std::string data = extractBetween(start, end);
            if (strcmp(*keyword, "Wet") == 0) {
                temperature = data;
            }
            else if (strcmp(*keyword, "Light") == 0) {
                wet = data;
            }
            else if (strcmp(*keyword, "Pressure") == 0) {
                light = data;
            }
            start = end + strlen(*keyword);
        }
    }

    pressure = std::string(start);

    //��ʱ����л�ȡ
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Create folder if not exist
    _mkdir("data");

    // Generate filename with current date
    std::ostringstream filename;
    filename << "data/" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << ".txt";
    std::string filenameStr = filename.str();

    //��д�ļ�
    std::cout << "��ʼ��д�ļ�" << std::endl;
    std::ofstream outFile(filenameStr, std::ios_base::app);
    // Check if file is opened successfully
    if (!outFile.is_open()) {
        std::cerr << "���ļ�ʧ��!" << std::endl;
        return;
    }
    else
    {
        std::cout << "�ļ���������" << std::endl;
    }
    outFile << "Time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << "-" << st.wHour << "-" << st.wMinute << "-" << st.wSecond << "\n";
    outFile << "Tempeture: " << temperature << "\n";
    outFile << "Wet: " << wet << "\n";
    outFile << "Light: " << light << "\n";
    outFile << "Pressure: " << pressure << "\n\n";
    outFile.close();
    std::cout << "��д��ϣ����ڶ���Ļ���д�ӡ" << std::endl;
    // Print on screen
    std::cout << "ʱ��: " << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "��\n";
    std::cout << "�¶�: " << temperature << "\n";
    std::cout << "ʪ��: " << wet << "\n";
    std::cout << "����: " << light << "\n";
    std::cout << "��ѹ: " << pressure << "\n";
    //��д��Ϊupdata���ļ�
    std::ofstream file("update.txt", std::ios::trunc);
    // Check if file is opened successfully
    if (!file.is_open()) {
        std::cerr << "���ļ�ʧ��!" << std::endl;
        return;
    }
    else {
        std::cout << "�ļ���������" << std::endl;
    }
    file << "Time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << "-" << st.wHour << "-" << st.wMinute << "-" << st.wSecond << "\n";
    file << "Tempeture: " << temperature << "\n";
    file << "Wet: " << wet << "\n";
    file << "Light: " << light << "\n";
    file << "Pressure: " << pressure << "\n\n";
    file.close();

}

int main() {
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("SharedMemory"));
    if (hMapFile == NULL) {
        std::cout << "OpenFileMapping ʧ��: " << GetLastError() << std::endl;
        return 1;
    }

    LPVOID lpSharedMemory = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
    if (lpSharedMemory == NULL) {
        std::cout << "MapViewOfFile ʧ��: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("SharedMutex"));
    if (hMutex == NULL) {
        std::cout << "OpenMutex ʧ��: " << GetLastError() << std::endl;
        UnmapViewOfFile(lpSharedMemory);
        CloseHandle(hMapFile);
        return 1;
    }

    long int lastDataChanged = -1;
    int count = 0;
    int choose;
    while (true) {
        DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
        if (dwWaitResult == WAIT_OBJECT_0) {
            long int* pDataChanged = (long int*)lpSharedMemory;
            std::string s;  // ��s�Ķ����Ƴ�if����

            if (*pDataChanged != lastDataChanged) {
                lastDataChanged = *pDataChanged;
                std::cout << "-----------------" << std::endl;
                std::cout << "��ʶ��: " << lastDataChanged << std::endl;
                std::cout << "����: " << std::endl;
                s = (char*)lpSharedMemory + sizeof(long int);
                std::cout << s << std::endl;
                std::cout << std::endl;
                count++;
                choose = 1;

            }

            //�ͷŵ�������
            ReleaseMutex(hMutex);


            if (count > 1 && choose == 1)
            {
                std::cout << "�Ѿ��ͷŵ������壬��ʼ�����ۺϲ���" << std::endl;
                //�����Ƕ��ڽ��յ������ݵľ��������������
                extractAndPrint(s);
                choose = 0;
            }

        }
        else {
            std::cout << "�ȴ�����ʱ����ִ���: " << GetLastError() << std::endl;
        }
    }

    UnmapViewOfFile(lpSharedMemory);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);

    return 0;
}
