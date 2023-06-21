#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <direct.h> // for _mkdir

#define SHARED_MEMORY_SIZE 1048576 // 1MB

//字符处理的辅助函数
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

//字符具体的处理函数
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

    //对时间进行获取
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Create folder if not exist
    _mkdir("data");

    // Generate filename with current date
    std::ostringstream filename;
    filename << "data/" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << ".txt";
    std::string filenameStr = filename.str();

    //书写文件
    std::cout << "开始书写文件" << std::endl;
    std::ofstream outFile(filenameStr, std::ios_base::app);
    // Check if file is opened successfully
    if (!outFile.is_open()) {
        std::cerr << "打开文件失败!" << std::endl;
        return;
    }
    else
    {
        std::cout << "文件被正常打开" << std::endl;
    }
    outFile << "Time: " << st.wYear << "-" << st.wMonth << "-" << st.wDay << "-" << st.wHour << "-" << st.wMinute << "-" << st.wSecond << "\n";
    outFile << "Tempeture: " << temperature << "\n";
    outFile << "Wet: " << wet << "\n";
    outFile << "Light: " << light << "\n";
    outFile << "Pressure: " << pressure << "\n\n";
    outFile.close();
    std::cout << "书写完毕，现在对屏幕进行打印" << std::endl;
    // Print on screen
    std::cout << "时间: " << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒\n";
    std::cout << "温度: " << temperature << "\n";
    std::cout << "湿度: " << wet << "\n";
    std::cout << "光照: " << light << "\n";
    std::cout << "气压: " << pressure << "\n";
    //书写名为updata的文件
    std::ofstream file("update.txt", std::ios::trunc);
    // Check if file is opened successfully
    if (!file.is_open()) {
        std::cerr << "打开文件失败!" << std::endl;
        return;
    }
    else {
        std::cout << "文件被正常打开" << std::endl;
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
        std::cout << "OpenFileMapping 失败: " << GetLastError() << std::endl;
        return 1;
    }

    LPVOID lpSharedMemory = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
    if (lpSharedMemory == NULL) {
        std::cout << "MapViewOfFile 失败: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("SharedMutex"));
    if (hMutex == NULL) {
        std::cout << "OpenMutex 失败: " << GetLastError() << std::endl;
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
            std::string s;  // 将s的定义移出if语句块

            if (*pDataChanged != lastDataChanged) {
                lastDataChanged = *pDataChanged;
                std::cout << "-----------------" << std::endl;
                std::cout << "标识符: " << lastDataChanged << std::endl;
                std::cout << "内容: " << std::endl;
                s = (char*)lpSharedMemory + sizeof(long int);
                std::cout << s << std::endl;
                std::cout << std::endl;
                count++;
                choose = 1;

            }

            //释放掉互斥体
            ReleaseMutex(hMutex);


            if (count > 1 && choose == 1)
            {
                std::cout << "已经释放掉互斥体，开始进行综合操作" << std::endl;
                //下面是对于接收到的数据的具体操作操作部分
                extractAndPrint(s);
                choose = 0;
            }

        }
        else {
            std::cout << "等待锁超时或出现错误: " << GetLastError() << std::endl;
        }
    }

    UnmapViewOfFile(lpSharedMemory);
    CloseHandle(hMutex);
    CloseHandle(hMapFile);

    return 0;
}
