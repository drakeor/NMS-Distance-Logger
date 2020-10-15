#include <iostream>
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <comdef.h>  
#include <chrono>

int main()
{
    // Parameters to set for logging
    const char* procName = "NMS.exe";
    const __int64 distAddr = 0x1D444BE4FCC;
    
    const char* logFileName = "log.csv";
    const int maxEntries = 1000;
    const int sampleIntervalMS = 50;

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Scan for NMS.exe
    HANDLE pHandle = NULL;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &processEntry) == 1)
    {
        while (Process32Next(snapshot, &processEntry) == 1)
        {
            _bstr_t asChar(processEntry.szExeFile);
            if (_stricmp(asChar, procName) == 0)
            {
                pHandle = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_QUERY_INFORMATION, FALSE, processEntry.th32ProcessID);
                std::cout << "Found the process!" << std::endl;
                break;
            }
        }
    }

    // Double check we have a process
    if (pHandle == NULL) {
        std::cerr << "Failed to find " << procName << ". Exiting..." << std::endl; 
        return -1;
    }

    // Get the log file ready
    std::ofstream logFile;
    logFile.open(logFileName, std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open " << logFileName << std::endl;
    }
    logFile << "time,distance" << std::endl;

    // Sample every second
    int cEntries = 0;
    float timeElapsed = 0;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    while (cEntries < maxEntries) {
        // Grab distance
        float cDist = 0;
        SIZE_T bytesRead;
        BOOL result = ReadProcessMemory(pHandle, (void*)distAddr, &cDist, sizeof(cDist), &bytesRead);
        if (!result) {
            std::cerr << "Failed to access memory! Bytes Read: " << bytesRead << "/" << sizeof(cDist) << std::endl;
            std::cerr << "Detailed Error: " << GetLastError() << std::endl;
            return -1;

        }

        // Log result
        std::chrono::steady_clock::time_point cTime = std::chrono::steady_clock::now();
        float countTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(cTime - startTime).count()) / 1000.f;

        std::stringstream ss;
        ss << countTime << "," << cDist << std::endl;
        std::cout << ss.str();
        logFile << ss.str();
        logFile.flush();

        Sleep(sampleIntervalMS);
    }

    // Cleanup
    if (pHandle != NULL) {
        CloseHandle(pHandle);
    }
    CloseHandle(snapshot);
    logFile.close();

    return 0;

}

