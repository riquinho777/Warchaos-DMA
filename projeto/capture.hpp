// C++17 / capture.hpp / Windows x64 Media Foundation / AVerMedia calibration frame acquisition.
#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <cwctype>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstddef>
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"ole32.lib")

namespace capture {
using Microsoft::WRL::ComPtr;
inline std::string utf8(const std::wstring& value) {
    int size=WideCharToMultiByte(CP_UTF8,0,value.c_str(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    std::string result(size,'\0');
    if(size)WideCharToMultiByte(CP_UTF8,0,value.c_str(),static_cast<int>(value.size()),result.data(),size,nullptr,nullptr);
    return result;
}
struct Frame {UINT32 width=0,height=0;ULONGLONG acquiredMs=0;std::vector<BYTE> pixels;};
class Session {
    bool com=false,mf=false;
    ComPtr<IMFMediaSource> source;
    ComPtr<IMFSourceReader> reader;
public:
    Frame frame;
    ~Session(){reader.Reset();if(source)source->Shutdown();source.Reset();if(mf)MFShutdown();if(com)CoUninitialize();}
    HRESULT open(bool listOnly=false) {
        HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        if(FAILED(hr))return hr;com=true;
        hr=MFStartup(MF_VERSION);if(FAILED(hr))return hr;mf=true;
        ComPtr<IMFAttributes> filters;
        hr=MFCreateAttributes(&filters,1);if(FAILED(hr))return hr;
        hr=filters->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);if(FAILED(hr))return hr;
        IMFActivate** devices=nullptr;UINT32 count=0;
        hr=MFEnumDeviceSources(filters.Get(),&devices,&count);if(FAILED(hr))return hr;
        ComPtr<IMFActivate> selected;
        for(UINT32 i=0;i<count;++i) {
            WCHAR* name=nullptr;UINT32 length=0;
            if(SUCCEEDED(devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,&name,&length))) {
                std::wstring label(name,length);CoTaskMemFree(name);
                std::printf("VIDEO DISPOSITIVO: %s\n",utf8(label).c_str());
                std::transform(label.begin(),label.end(),label.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));});
                if(!selected && (label.find(L"avermedia")!=std::wstring::npos || label.find(L"live gamer")!=std::wstring::npos || label.find(L"gc553")!=std::wstring::npos))selected=devices[i];
            }
            devices[i]->Release();
        }
        CoTaskMemFree(devices);
        if(listOnly)return S_OK;
        if(!selected)return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        std::puts("VIDEO ETAPA: ativar dispositivo selecionado.");
        hr=selected->ActivateObject(IID_PPV_ARGS(&source));if(FAILED(hr))return hr;
        ComPtr<IMFAttributes> options;
        hr=MFCreateAttributes(&options,2);if(FAILED(hr))return hr;
        hr=options->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,TRUE);if(FAILED(hr))return hr;
        hr=options->SetUINT32(MF_LOW_LATENCY,TRUE);if(FAILED(hr))return hr;
        std::puts("VIDEO ETAPA: criar leitor Media Foundation.");
        hr=MFCreateSourceReaderFromMediaSource(source.Get(),options.Get(),&reader);if(FAILED(hr))return hr;
        ComPtr<IMFMediaType> nativeBest;double bestScore=-1e30;
        for(DWORD index=0;;++index) {
            ComPtr<IMFMediaType> type;
            hr=reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,index,&type);
            if(hr==MF_E_NO_MORE_TYPES)break;if(FAILED(hr))return hr;
            UINT32 width=0,height=0,numerator=0,denominator=1;GUID subtype{};
            if(FAILED(MFGetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,&width,&height)))continue;
            MFGetAttributeRatio(type.Get(),MF_MT_FRAME_RATE,&numerator,&denominator);
            type->GetGUID(MF_MT_SUBTYPE,&subtype);
            const double fps=denominator?static_cast<double>(numerator)/denominator:0;
            const double score=(width==1920 && height==1080?1000000:0)+(subtype==MFVideoFormat_NV12?10000:0)-std::abs(fps-60.0)*100-
                std::abs(static_cast<double>(width)*height-1920.0*1080)/1000;
            if(score>bestScore){bestScore=score;nativeBest=type;}
        }
        if(!nativeBest)return MF_E_INVALIDMEDIATYPE;
        std::puts("VIDEO ETAPA: selecionar formato nativo.");
        hr=reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),nullptr,nativeBest.Get());if(FAILED(hr))return hr;
        ComPtr<IMFMediaType> output;
        hr=MFCreateMediaType(&output);if(FAILED(hr))return hr;
        hr=output->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);if(FAILED(hr))return hr;
        hr=output->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);if(FAILED(hr))return hr;
        std::puts("VIDEO ETAPA: configurar conversao RGB32.");
        hr=reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),nullptr,output.Get());if(FAILED(hr))return hr;
        hr=reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE);if(FAILED(hr))return hr;
        return reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM,TRUE);
    }
    HRESULT acquire(unsigned frameCount=3) {
        if(!reader)return MF_E_NOT_INITIALIZED;
        if(frameCount<1 || frameCount>3)return E_INVALIDARG;
        ComPtr<IMFSample> sample;
        // Warm up a few newly acquired frames; there is no persistent preview queue.
        for(unsigned attempt=0,received=0;attempt<90 && received<frameCount;++attempt) {
            DWORD flags=0;LONGLONG timestamp=0;sample.Reset();
            HRESULT hr=reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,nullptr,&flags,&timestamp,&sample);
            if(FAILED(hr))return hr;if(flags&MF_SOURCE_READERF_ERROR)return E_FAIL;
            if(flags&MF_SOURCE_READERF_ENDOFSTREAM)return MF_E_END_OF_STREAM;
            if(sample)++received;
            if(received==frameCount)break;
        }
        if(!sample)return MF_E_END_OF_STREAM;
        frame.acquiredMs=GetTickCount64();
        ComPtr<IMFMediaType> type;HRESULT hr=reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,&type);if(FAILED(hr))return hr;
        hr=MFGetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,&frame.width,&frame.height);if(FAILED(hr))return hr;
        if(frame.width<64||frame.width>7680||frame.height<64||frame.height>4320)return MF_E_INVALIDMEDIATYPE;
        const size_t rowBytes=static_cast<size_t>(frame.width)*4;
        frame.pixels.resize(rowBytes*frame.height);
        ComPtr<IMFMediaBuffer> buffer;hr=sample->ConvertToContiguousBuffer(&buffer);if(FAILED(hr))return hr;
        ComPtr<IMF2DBuffer> twoD;BYTE* top=nullptr;LONG stride=0;
        if(SUCCEEDED(buffer.As(&twoD)) && SUCCEEDED(twoD->Lock2D(&top,&stride))) {
            if(static_cast<size_t>(std::abs(static_cast<long long>(stride)))<rowBytes){twoD->Unlock2D();return E_FAIL;}
            for(UINT32 row=0;row<frame.height;++row)std::memcpy(frame.pixels.data()+row*rowBytes,top+static_cast<ptrdiff_t>(row)*stride,rowBytes);
            twoD->Unlock2D();
        } else {
            DWORD length=0;hr=buffer->Lock(&top,nullptr,&length);if(FAILED(hr))return hr;
            stride=static_cast<LONG>(MFGetAttributeUINT32(type.Get(),MF_MT_DEFAULT_STRIDE,0));
            if(!stride)MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1,frame.width,&stride);
            const size_t step=static_cast<size_t>(std::abs(static_cast<long long>(stride)));
            if(step<rowBytes || length<(frame.height-1)*step+rowBytes){buffer->Unlock();return E_FAIL;}
            if(stride<0)top+=(frame.height-1)*step;
            for(UINT32 row=0;row<frame.height;++row)std::memcpy(frame.pixels.data()+row*rowBytes,top+static_cast<ptrdiff_t>(row)*stride,rowBytes);
            buffer->Unlock();
        }
        return S_OK;
    }
    bool save(const char* path) const {
        if(frame.pixels.empty())return false;
        BITMAPFILEHEADER fileHeader{};fileHeader.bfType=0x4D42;fileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
        fileHeader.bfSize=fileHeader.bfOffBits+static_cast<DWORD>(frame.pixels.size());
        BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=static_cast<LONG>(frame.width);info.biHeight=-static_cast<LONG>(frame.height);
        info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;info.biSizeImage=static_cast<DWORD>(frame.pixels.size());
        std::ofstream file(path,std::ios::binary);if(!file)return false;
        file.write(reinterpret_cast<const char*>(&fileHeader),sizeof(fileHeader));file.write(reinterpret_cast<const char*>(&info),sizeof(info));
        file.write(reinterpret_cast<const char*>(frame.pixels.data()),static_cast<std::streamsize>(frame.pixels.size()));return static_cast<bool>(file);
    }
};
}
