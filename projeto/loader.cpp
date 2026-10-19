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
static HWND startButton,stopButton,mode,monitorEdit,statusLabel;
static HFONT font=nullptr;
static bool exists(const std::wstring& p){const DWORD a=GetFileAttributesW(p.c_str());return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
static std::wstring missing(){for(const auto* name:{L"esp-fuser-v37.exe",L"Ferrum.ini",L"vmm.dll",L"leechcore.dll",L"FTD3XX.dll",L"vcruntime140.dll",L"dbghelp.dll",L"symsrv.dll"})if(!exists(app+L"\\"+name))return name;return {};}
static void error(HWND w,const std::wstring& text){MessageBoxW(w,text.c_str(),L"Zyon - Loader",MB_OK|MB_ICONERROR);}
static void update(){const bool active=child&&WaitForSingleObject(child,0)==WAIT_TIMEOUT;EnableWindow(startButton,!active);EnableWindow(stopButton,active);EnableWindow(mode,!active);EnableWindow(monitorEdit,!active);if(active)SetWindowTextW(statusLabel,L"Em execucao. INSERT abre o painel no segundo PC.");else if(child){DWORD code=0;GetExitCodeProcess(child,&code);CloseHandle(child);child=nullptr;childPid=0;const auto text=code?L"Encerrado com erro. Consulte o log.":L"Encerrado. Pronto para iniciar novamente.";SetWindowTextW(statusLabel,text);}}
static void findRunning(){HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return;PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);if(Process32FirstW(snapshot,&entry))do{if(_wcsicmp(entry.szExeFile,L"esp-fuser-v37.exe"))continue;HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,entry.th32ProcessID);if(!process)continue;wchar_t path[32768];DWORD length=32768;if(QueryFullProcessImageNameW(process,0,path,&length)&&!_wcsicmp(path,exe.c_str())){child=process;childPid=entry.th32ProcessID;break;}CloseHandle(process);}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);}
static void launch(HWND w){update();if(child)return;const auto absent=missing();if(!absent.empty()){error(w,L"Arquivo ausente em app: "+absent);return;}BOOL translated=FALSE;const UINT selected=GetDlgItemInt(w,104,&translated,FALSE);if(!translated||selected<1||selected>32){error(w,L"Informe um monitor de 1 a 32. O monitor principal e o numero 1.");return;}
    if(!CreateDirectoryW(logs.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS){error(w,L"Nao foi possivel criar a pasta logs.");return;}
    SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};HANDLE output=CreateFileW((logs+(startupTest?L"\\validacao.log":L"\\execucao.log")).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,OPEN_EXISTING,0,nullptr);
    if(output==INVALID_HANDLE_VALUE||input==INVALID_HANDLE_VALUE){if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);error(w,L"Nao foi possivel abrir o log.");return;}
    const bool ferrum=SendMessageW(mode,BM_GETCHECK,0,0)==BST_CHECKED;std::wstring command=L"\""+exe+L"\""+(startupTest?L" --listar-monitores":L" --monitor="+std::to_wstring(selected)+(ferrum?L" --ferrum":L""));std::vector<wchar_t> buffer(command.begin(),command.end());buffer.push_back(0);
    STARTUPINFOEXW info{};info.StartupInfo.cb=sizeof(info);info.StartupInfo.dwFlags=STARTF_USESTDHANDLES;info.StartupInfo.hStdOutput=output;info.StartupInfo.hStdError=output;info.StartupInfo.hStdInput=input;
    SIZE_T attributeSize=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeSize);std::vector<BYTE> attributeMemory(attributeSize);info.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeMemory.data());
    HANDLE inherited[]={output,input};BOOL initialized=InitializeProcThreadAttributeList(info.lpAttributeList,1,0,&attributeSize);BOOL prepared=initialized&&UpdateProcThreadAttribute(info.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr);PROCESS_INFORMATION process{};
    const BOOL success=prepared&&CreateProcessW(exe.c_str(),buffer.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,app.c_str(),&info.StartupInfo,&process);const DWORD failure=GetLastError();if(initialized)DeleteProcThreadAttributeList(info.lpAttributeList);CloseHandle(output);CloseHandle(input);
    if(!success){error(w,L"Falha ao iniciar. Codigo Windows: "+std::to_wstring(failure));return;}CloseHandle(process.hThread);child=process.hProcess;childPid=process.dwProcessId;WritePrivateProfileStringW(L"Loader",L"monitor",std::to_wstring(selected).c_str(),(app+L"\\Loader.ini").c_str());WritePrivateProfileStringW(L"Loader",L"ferrum",ferrum?L"1":L"0",(app+L"\\Loader.ini").c_str());update();}
static BOOL CALLBACK closeChild(HWND w,LPARAM pid){DWORD owner=0;GetWindowThreadProcessId(w,&owner);if(owner==static_cast<DWORD>(pid)){wchar_t cls[80]={};GetClassNameW(w,cls,80);if(!wcscmp(cls,L"WarChaosDmaPreview"))PostMessageW(w,WM_CLOSE,0,0);}return TRUE;}
static LRESULT CALLBACK procedure(HWND w,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_CREATE){auto control=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id){HWND c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return c;};
        control(L"STATIC",L"ZYON  |  Warchaos",0,24,20,410,28,100);control(L"STATIC",L"Versao salva: 37  -  DMA / HDMI fuser",0,24,52,410,22,101);
        mode=control(L"BUTTON",L"Usar Ferrum",BS_AUTOCHECKBOX|WS_TABSTOP,24,92,310,28,102);SendMessageW(mode,BM_SETCHECK,GetPrivateProfileIntW(L"Loader",L"ferrum",1,(app+L"\\Loader.ini").c_str())?BST_CHECKED:BST_UNCHECKED,0);
        control(L"STATIC",L"Monitor de saida:",0,24,134,180,24,103);monitorEdit=control(L"EDIT",L"1",WS_BORDER|ES_NUMBER|WS_TABSTOP,220,130,80,28,104);SetDlgItemInt(w,104,GetPrivateProfileIntW(L"Loader",L"monitor",1,(app+L"\\Loader.ini").c_str()),FALSE);
        startButton=control(L"BUTTON",L"Iniciar",BS_DEFPUSHBUTTON|WS_TABSTOP,24,180,196,40,105);stopButton=control(L"BUTTON",L"Encerrar",WS_TABSTOP,236,180,196,40,106);
        control(L"BUTTON",L"Ver log",WS_TABSTOP,24,236,128,34,107);control(L"BUTTON",L"Abrir pasta",WS_TABSTOP,164,236,128,34,108);control(L"BUTTON",L"Instrucoes",WS_TABSTOP,304,236,128,34,109);
        statusLabel=control(L"STATIC",L"Pronto. Clique em Iniciar.",0,24,290,410,46,110);findRunning();update();SetTimer(w,1,300,nullptr);return 0;}
    if(m==WM_COMMAND){switch(LOWORD(wp)){case 105:launch(w);break;case 106:if(child){EnumWindows(closeChild,childPid);SetWindowTextW(statusLabel,L"Encerrando...");}break;case 107:if(exists(logs+L"\\execucao.log"))ShellExecuteW(w,L"open",(logs+L"\\execucao.log").c_str(),nullptr,nullptr,SW_SHOWNORMAL);else MessageBoxW(w,L"O log sera criado ao iniciar.",L"Zyon",MB_OK);break;case 108:ShellExecuteW(w,L"open",root.c_str(),nullptr,nullptr,SW_SHOWNORMAL);break;case 109:ShellExecuteW(w,L"open",(root+L"\\LEIA-ME.txt").c_str(),nullptr,nullptr,SW_SHOWNORMAL);break;}return 0;}
    if(m==WM_TIMER){update();return 0;}if(m==WM_DESTROY){KillTimer(w,1);if(child)CloseHandle(child);PostQuitMessage(0);return 0;}return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR arguments,int show){wchar_t path[32768];const DWORD length=GetModuleFileNameW(nullptr,path,32768);if(!length||length>=32768)return 1;root=std::wstring(path,length);root.resize(root.find_last_of(L"\\/"));app=root+L"\\app";exe=app+L"\\esp-fuser-v37.exe";logs=root+L"\\logs";
    if(!wcscmp(arguments,L"--verificar"))return missing().empty()?0:2;
    HANDLE singleton=CreateMutexW(nullptr,FALSE,L"Local\\ZyonWarchaosLoaderV37");if(!singleton)return 3;if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(singleton);return 0;}
    SetProcessDPIAware();font=CreateFontW(-17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=procedure;cls.lpszClassName=L"ZyonLoaderV37";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);if(!RegisterClassW(&cls)){CloseHandle(singleton);return 4;}
    HWND w=CreateWindowW(cls.lpszClassName,L"Zyon - Warchaos",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,474,390,nullptr,nullptr,instance,nullptr);if(!w){DeleteObject(font);CloseHandle(singleton);return 5;}
    if(!wcscmp(arguments,L"--teste-inicializacao")){startupTest=true;if(child){DestroyWindow(w);DeleteObject(font);CloseHandle(singleton);return 7;}launch(w);DWORD code=8;if(child&&WaitForSingleObject(child,10000)==WAIT_OBJECT_0)GetExitCodeProcess(child,&code);DestroyWindow(w);DeleteObject(font);CloseHandle(singleton);return static_cast<int>(code);}
    if(!wcscmp(arguments,L"--teste-interface")){const bool valid=GetDlgItem(w,105)&&GetDlgItem(w,110);DestroyWindow(w);DeleteObject(font);CloseHandle(singleton);return valid?0:6;}
    ShowWindow(w,show);UpdateWindow(w);MSG message{};while(GetMessageW(&message,nullptr,0,0)>0)if(!IsDialogMessageW(w,&message)){TranslateMessage(&message);DispatchMessageW(&message);}DeleteObject(font);CloseHandle(singleton);return 0;
}
