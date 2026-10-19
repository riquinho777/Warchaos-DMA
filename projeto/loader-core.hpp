// C++17 / loader-core.hpp / local process lifecycle, paths and logging.
#pragma once
// C++17 / loader.cpp / Windows x64 / launcher for the saved v37 build.
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"shell32.lib")
static std::wstring root,app,exe,logs;static bool startupTest=false;
static HANDLE child=nullptr;static DWORD childPid=0;
static std::wstring appStatus=L"Pronto para iniciar.";

static bool exists(const std::wstring& p){const DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
static std::wstring missing(){for(const auto* name:{L"esp-fuser-v39.exe",L"Ferrum.ini",L"vmm.dll",L"leechcore.dll",L"FTD3XX.dll",L"vcruntime140.dll",L"dbghelp.dll",L"symsrv.dll"})if(!exists(app+L"\\"+name))return name;return {};}
static void error(HWND w,const std::wstring& text){MessageBoxW(w,text.c_str(),L"Zyon - Loader",MB_OK|MB_ICONERROR);}
static bool running(){return child&&WaitForSingleObject(child,0)==WAIT_TIMEOUT;}
static void update(){if(running())appStatus=L"Em execucao";else if(child){DWORD code=0;GetExitCodeProcess(child,&code);CloseHandle(child);child=nullptr;childPid=0;appStatus=code?L"Encerrado com erro. Consulte o log.":L"Pronto para iniciar.";}}
static void findRunning(){HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return;PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);if(Process32FirstW(snapshot,&entry))do{if(_wcsicmp(entry.szExeFile,L"esp-fuser-v39.exe"))continue;HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!process)continue;wchar_t path[32768];DWORD length=32768;if(QueryFullProcessImageNameW(process,0,path,&length)&&!_wcsicmp(path,exe.c_str())){child=process;childPid=entry.th32ProcessID;break;}CloseHandle(process);}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);}
static void launch(HWND w,UINT selected,bool ferrum){update();if(child)return;const auto absent=missing();if(!absent.empty()){error(w,L"Arquivo ausente em app: "+absent);return;}if(selected<1||selected>32){error(w,L"Selecione um monitor valido.");return;}
    if(!CreateDirectoryW(logs.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS){error(w,L"Nao foi possivel criar a pasta logs.");return;}
    SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};HANDLE output=CreateFileW((logs+(startupTest?L"\\validacao.log":L"\\execucao.log")).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,OPEN_EXISTING,0,nullptr);
    if(output==INVALID_HANDLE_VALUE||input==INVALID_HANDLE_VALUE){if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);error(w,L"Nao foi possivel abrir o log.");return;}
    std::wstring command=L"\""+exe+L"\""+(startupTest?L" --listar-monitores":L" --monitor="+std::to_wstring(selected)+(ferrum?L" --ferrum":L""));std::vector<wchar_t> buffer(command.begin(),command.end());buffer.push_back(0);
    STARTUPINFOEXW info{};info.StartupInfo.cb=sizeof(info);info.StartupInfo.dwFlags=STARTF_USESTDHANDLES;info.StartupInfo.hStdOutput=output;info.StartupInfo.hStdError=output;info.StartupInfo.hStdInput=input;
    SIZE_T attributeSize=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeSize);std::vector<BYTE> attributeMemory(attributeSize);info.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeMemory.data());
    HANDLE inherited[]={output,input};BOOL initialized=InitializeProcThreadAttributeList(info.lpAttributeList,1,0,&attributeSize);BOOL prepared=initialized&&UpdateProcThreadAttribute(info.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr);PROCESS_INFORMATION process{};
    const BOOL success=prepared&&CreateProcessW(exe.c_str(),buffer.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,app.c_str(),&info.StartupInfo,&process);const DWORD failure=GetLastError();if(initialized)DeleteProcThreadAttributeList(info.lpAttributeList);CloseHandle(output);CloseHandle(input);
    if(!success){error(w,L"Falha ao iniciar. Codigo Windows: "+std::to_wstring(failure));return;}CloseHandle(process.hThread);child=process.hProcess;childPid=process.dwProcessId;if(!startupTest){WritePrivateProfileStringW(L"Loader",L"monitor",std::to_wstring(selected).c_str(),(app+L"\\Loader.ini").c_str());WritePrivateProfileStringW(L"Loader",L"ferrum",ferrum?L"1":L"0",(app+L"\\Loader.ini").c_str());}update();}
static BOOL CALLBACK closeChild(HWND w,LPARAM pid){DWORD owner=0;GetWindowThreadProcessId(w,&owner);if(owner==static_cast<DWORD>(pid)){wchar_t cls[80]={};GetClassNameW(w,cls,80);if(!wcscmp(cls,L"WarChaosDmaPreview"))PostMessageW(w,WM_CLOSE,0,0);}return TRUE;}

