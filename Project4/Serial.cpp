#include "Serial.h"

void Serial::executeCommand(const std::string& command, std::stringstream& output)
{
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("Failed to open pipe");
    }

    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        output << buffer.data();
    }

    if (_pclose(pipe) == -1)
    {
        throw std::runtime_error("Failed to close pipe");
    }
}

void Serial::DisplayTitle(const std::string& title)
{
    const std::string sectionLine(Width, LINE_CHAR);
    const std::string centeredTitle = " " + title + " ";
    const int titlePadding = (Width - centeredTitle.length()) / 2;

    std::cout << ANSI_COL2
        << std::setfill(LINE_CHAR) << std::setw(Width) << ""
        << ANSI_RESET << "\n";
    std::cout << ANSI_COL1
        << std::setfill(' ') << std::setw(titlePadding) << ""
        << centeredTitle
        << std::setw(titlePadding) << ""
        << ANSI_RESET << "\n";
    std::cout << ANSI_COL2
        << std::setfill(LINE_CHAR) << std::setw(Width) << ""
        << ANSI_RESET << "\n";
    std::cout << std::endl;
}

void Serial::retrieveMonitorInformation() {
    HRESULT hr = E_FAIL;
    LONG hWmiHandle;
    PWmiMonitorID MonitorID;
    HINSTANCE hDLL = LoadLibrary(L"Advapi32.dll");
    WmiOpenBlock = (WOB)GetProcAddress(hDLL, "WmiOpenBlock");
    WmiQueryAllData = (WQAD)GetProcAddress(hDLL, "WmiQueryAllDataW");
    WmiCloseBlock = (WCB)GetProcAddress(hDLL, "WmiCloseBlock");
    if (WmiOpenBlock != NULL && WmiQueryAllData && WmiCloseBlock)
    {
        WCHAR pszDeviceId[256] = L"";
        hr = WmiOpenBlock((LPGUID)&WmiMonitorID_GUID, GENERIC_READ, &hWmiHandle);
        if (hr == ERROR_SUCCESS)
        {
            ULONG nBufferSize = 0;
            UCHAR* pAllDataBuffer = 0;
            PWNODE_ALL_DATA pWmiAllData;
            hr = WmiQueryAllData(hWmiHandle, &nBufferSize, 0);
            if (hr == ERROR_INSUFFICIENT_BUFFER)
            {
                pAllDataBuffer = (UCHAR*)malloc(nBufferSize);
                hr = WmiQueryAllData(hWmiHandle, &nBufferSize, pAllDataBuffer);
                if (hr == ERROR_SUCCESS)
                {
                    while (1)
                    {
                        pWmiAllData = (PWNODE_ALL_DATA)pAllDataBuffer;
                        if (pWmiAllData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
                            MonitorID = (PWmiMonitorID)&pAllDataBuffer[pWmiAllData->DataBlockOffset];
                        else
                            MonitorID = (PWmiMonitorID)&pAllDataBuffer[pWmiAllData->OffsetInstanceDataAndLength[0].OffsetInstanceData];

                        ULONG nOffset = 0;
                        WCHAR* pwsInstanceName = 0;
                        nOffset = (ULONG)pAllDataBuffer[pWmiAllData->OffsetInstanceNameOffsets];
                        pwsInstanceName = (WCHAR*)OFFSET_TO_PTR(pWmiAllData, nOffset + sizeof(USHORT));
                        WCHAR wsText[255] = L"";
                        swprintf(wsText, L"Instance Name = %s\r\n", pwsInstanceName);
                        std::wcout << L"Instance Name = " << pwsInstanceName << std::endl;

                        WCHAR* pwsUserFriendlyName;
                        pwsUserFriendlyName = (WCHAR*)MonitorID->UserFriendlyName;
                        std::wcout << L"User Friendly Name = " << pwsUserFriendlyName << std::endl;

                        WCHAR* pwsManufacturerName;
                        pwsManufacturerName = (WCHAR*)MonitorID->ManufacturerName;
                        std::wcout << L"Manufacturer Name = " << pwsManufacturerName << std::endl;

                        WCHAR* pwsProductCodeID;
                        pwsProductCodeID = (WCHAR*)MonitorID->ProductCodeID;
                        std::wcout << L"Product Code ID = " << pwsProductCodeID << std::endl;

                        WCHAR* pwsSerialNumberID;
                        pwsSerialNumberID = (WCHAR*)MonitorID->SerialNumberID;
                        std::wcout << L"Serial Number ID = " << pwsSerialNumberID << std::endl;

                        if (!pWmiAllData->WnodeHeader.Linkage)
                            break;
                        pAllDataBuffer += pWmiAllData->WnodeHeader.Linkage;
                    }
                    free(pAllDataBuffer);
                }
            }
            WmiCloseBlock(hWmiHandle);
        }
    }
}

void Serial::executeAndDisplay(const std::string& title, const std::string& command)
{
    std::stringstream output;
    DisplayTitle(title);
    executeCommand(command, output);
    std::cout << ANSI_COL2 << output.str() << ANSI_RESET << "\n\n";
}

void Serial::killWinmgt() {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (snapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to create process snapshot");
    }

    if (!Process32First(snapshot, &entry)) {
        CloseHandle(snapshot);
        throw std::runtime_error("Failed to retrieve process information");
    }

    do {
        if (_wcsicmp(entry.szExeFile, L"WmiPrvSE.exe") == 0) {
            HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            if (process == INVALID_HANDLE_VALUE) {
                CloseHandle(snapshot);
                throw std::runtime_error("Failed to open process");
            }
            printf("Killed Winmgmt\n");
            TerminateProcess(process, 0);
            CloseHandle(process);
            break;
        }
    } while (Process32Next(snapshot, &entry));

    CloseHandle(snapshot);
}

void Serial::showSerials()
{
    killWinmgt();
    system("net stop winmgmt /Y");

    executeAndDisplay("Operating System Information & Operating System Serial Number", "wmic os get Caption,CSDVersion,OSArchitecture,Version & wmic path win32_operatingsystem get SerialNumber");
    executeAndDisplay("Computer System Information & Name", "wmic path win32_computersystem get Name & wmic computersystem get Manufacturer,Model,SystemType");
    executeAndDisplay("Motherboard Information", "wmic baseboard get Product,Manufacturer,Version");
    executeAndDisplay("User Account Information", "wmic useraccount get Name,Status");
    executeAndDisplay("BIOS Serial Number", "wmic bios get serialnumber");
    executeAndDisplay("Physical Media Serial Numbers", "wmic path win32_physicalmedia get SerialNumber");
    executeAndDisplay("CPU Name & CPU Serial Number & Processor ID", "wmic cpu get Name & wmic cpu get serialnumber & wmic path win32_processor get ProcessorId");
    executeAndDisplay("Baseboard Serial Number & UUID", "wmic baseboard get serialnumber & wmic path win32_computersystemproduct get uuid");
    executeAndDisplay("SMBIOS Number", "wmic path win32_bios get SerialNumber");
    executeAndDisplay("System UUID", "wmic path win32_computersystemproduct get IdentifyingNumber");
    executeAndDisplay("Memory Chip Serial Numbers", "wmic memorychip get serialnumber");
    DisplayTitle("Desktop Monitor Information");
    retrieveMonitorInformation();
    executeAndDisplay("Network Adapter MAC Addresses", "wmic networkadapter get MACAddress");
    executeAndDisplay("Printer Information", "wmic printers get Name, PortName, DriverName");
    executeAndDisplay("Sound Device Information", "wmic sounddev get Name, Manufacturer");
    executeAndDisplay("USB Controller Information", "wmic usbcontroller get Name, Manufacturer");
    executeAndDisplay("Graphics Card Description", "wmic path win32_videocontroller get Description & nvidia-smi -L");
    executeAndDisplay("NIC Serials / MAC:", "wmic nic get macaddress, description");
    executeAndDisplay("ARP Cache:", "arp -a");
    executeAndDisplay("Disk Name and Serials", "wmic diskdrive get model,serialnumber");
    executeAndDisplay("Disk Volume Numbers", "vol c: & vol d:");
    executeAndDisplay("Network Adapter IP Addresses", "wmic networkadapterconfiguration get IPAddress, MACAddress");
    executeAndDisplay("Printer Device IDs", "wmic printer get Name, DeviceID");
    executeAndDisplay("IDE Controller Device IDs", "wmic idecontroller get Name, DeviceID");
    std::cin.get();
}


int main() {
    static Serial serial{};
    serial.showSerials();
}