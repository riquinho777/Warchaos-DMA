// C++17 / ferrum.hpp / Windows x64 / Ferrum Software KM serial API, single IO thread.
#pragma once
#include <Windows.h>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <cctype>
inline std::string parseFerrumReply(const std::string& command,const std::string& input,bool allowEmpty=false){
        std::string lower=input;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        if(lower.find("error")!=std::string::npos||lower.find("unknown")!=std::string::npos)throw std::runtime_error("Ferrum retornou erro: "+input);
        if(input.find(">>> ")==std::string::npos)throw std::runtime_error("Tempo de resposta Ferrum excedido");
        std::string result;size_t start=0;
        while(start<input.size()){auto end=input.find('\n',start);if(end==std::string::npos)end=input.size();auto line=input.substr(start,end-start);start=end+1;
            for(size_t p;(p=line.find(">>> "))!=std::string::npos;)line.erase(p,4);
            const auto first=line.find_first_not_of(" \r\t"),last=line.find_last_not_of(" \r\t");if(first==std::string::npos)continue;line=line.substr(first,last-first+1);
            if(line!=command&&!line.empty()){if(!result.empty())result+='\n';result+=line;}
        }
        if(result.empty()&&!allowEmpty)throw std::runtime_error("Resposta Ferrum vazia a "+command);return result;
}

class Ferrum {
    HANDLE port=INVALID_HANDLE_VALUE;bool down=false;
    void write(const std::string& command){DWORD count=0;const auto wire=command+"\r\n";
        if(!WriteFile(port,wire.data(),static_cast<DWORD>(wire.size()),&count,nullptr)||count!=wire.size())throw std::runtime_error("Falha na escrita serial Ferrum");
    }
public:
    ~Ferrum(){close();}
    Ferrum()=default;Ferrum(const Ferrum&)=delete;Ferrum& operator=(const Ferrum&)=delete;
    void close(){if(port!=INVALID_HANDLE_VALUE){if(down){DWORD count=0;const char release[]="km.left(0)\r\n";WriteFile(port,release,sizeof(release)-1,&count,nullptr);}CloseHandle(port);port=INVALID_HANDLE_VALUE;down=false;}}
    void open(const std::string& name,DWORD baud){
        close();if(name.size()<4||name.substr(0,3)!="COM"||!std::all_of(name.begin()+3,name.end(),[](unsigned char c){return std::isdigit(c)!=0;}))throw std::runtime_error("Porta Ferrum invalida; use COM5, por exemplo");
        const auto device="\\\\.\\"+name;port=CreateFileA(device.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(port==INVALID_HANDLE_VALUE)throw std::runtime_error("Nao foi possivel abrir a porta Ferrum");
        DCB dcb{};dcb.DCBlength=sizeof(dcb);if(!GetCommState(port,&dcb))throw std::runtime_error("GetCommState falhou");
        dcb.BaudRate=baud;dcb.ByteSize=8;dcb.Parity=NOPARITY;dcb.StopBits=ONESTOPBIT;dcb.fBinary=TRUE;dcb.fParity=FALSE;
        dcb.fOutxCtsFlow=FALSE;dcb.fOutxDsrFlow=FALSE;dcb.fDtrControl=DTR_CONTROL_DISABLE;dcb.fRtsControl=RTS_CONTROL_DISABLE;dcb.fOutX=FALSE;dcb.fInX=FALSE;dcb.fDsrSensitivity=FALSE;
        COMMTIMEOUTS timeouts{};timeouts.ReadIntervalTimeout=MAXDWORD;timeouts.WriteTotalTimeoutConstant=50;
        if(!SetCommState(port,&dcb)||!SetCommTimeouts(port,&timeouts)||!PurgeComm(port,PURGE_RXCLEAR|PURGE_TXCLEAR))throw std::runtime_error("Configuracao serial falhou");
        const auto version=query("km.version()",1000);std::string lower=version;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        if(lower.find("ferrum")==std::string::npos)throw std::runtime_error("A porta nao identificou Ferrum Software API");
        if(query("km.has_mouse()",1000)!="1")throw std::runtime_error("Ferrum nao confirmou mouse presente");
        std::printf("FERRUM: %s; %lu baud; %s\n",name.c_str(),baud,version.c_str());
    }
    std::string query(const std::string& command,DWORD timeout=100,bool allowEmpty=false){
        write(command);std::string input;const auto deadline=GetTickCount64()+timeout;
        while(GetTickCount64()<deadline){char buffer[512];DWORD read=0;if(!ReadFile(port,buffer,sizeof(buffer),&read,nullptr))throw std::runtime_error("Falha na leitura serial");
            input.append(buffer,read);if(input.size()>8192)throw std::runtime_error("Resposta Ferrum excedeu limite");
            if(input.find(">>> ")!=std::string::npos)break;Sleep(1);
        }
        return parseFerrumReply(command,input,allowEmpty);
    }
    void move(int x,int y){if(x||y)query("km.move("+std::to_string(x)+","+std::to_string(y)+")",100,true);}
    void press(){down=true;query("km.left(1)",100,true);}
    void release(){if(down){query("km.left(0)",100,true);down=false;}}
};
