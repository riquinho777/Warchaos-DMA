// C++17 / dma-head.hpp / Windows x64 / shared read-only DMA helpers.
#pragma once
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <cstring>
#include <cmath>
#include <array>
#include "vmmdll.h"
#include "projection.hpp"
using DmaRead = decltype(&VMMDLL_MemRead);
static bool plausiblePointer(QWORD value) {
    return value >= 0x10000 && value < 0x0000800000000000ULL && (value & 7) == 0;
}

struct RendererProjection {
    DWORD lane=0;
    projection::Matrix first{},second{};
    std::array<int,4> viewport{};
    unsigned viewportOffset=0;
};

struct HeadSample {
    QWORD character=0,array=0,address=0,leftFoot=0,rightFoot=0;
    DWORD joint=0,bytes=0,entityId=0;
    QWORD definition=0,joints=0;DWORD jointCount=0;
    std::array<float,7> pose{};
    std::array<QWORD,17> skeleton{};
};
static constexpr const char* skeletonNames[]={"Bip01 Head","Bip01 Neck","Bip01 Spine2","Bip01 Spine","Bip01 Pelvis","Bip01 L UpperArm","Bip01 L Forearm","Bip01 L Hand","Bip01 R UpperArm","Bip01 R Forearm","Bip01 R Hand","Bip01 L Thigh","Bip01 L Calf","Bip01 L Foot","Bip01 R Thigh","Bip01 R Calf","Bip01 R Foot"};

static HeadSample discoverHead(DmaRead read,VMM_HANDLE handle,DWORD pid,const BYTE* proxyHeader,DWORD entityId,decltype(&VMMDLL_MemReadEx) fresh=nullptr,const HeadSample* cached=nullptr) {
    HeadSample result{};result.entityId=entityId;
    auto fetch=[&](QWORD address,PBYTE output,DWORD size){if(!fresh)return read(handle,pid,address,output,size)!=FALSE;DWORD received=0;return fresh(handle,pid,address,output,size,&received,VMMDLL_FLAG_NOCACHE)!=FALSE&&received==size;};
    auto qword=[&](QWORD address,QWORD& value){return fetch(address,reinterpret_cast<PBYTE>(&value),8);};
    QWORD begin=0,end=0,slot=0,definition=0,joints=0;DWORD count=0,stored=0;
    std::memcpy(&begin,proxyHeader+0x58,8);std::memcpy(&end,proxyHeader+0x60,8);
    if(!plausiblePointer(begin) || end<=begin || end-begin>4096 || (end-begin)%8 || !qword(begin,slot) || !plausiblePointer(slot) ||
        !qword(slot+0x78,result.character) || !plausiblePointer(result.character) ||
        !qword(result.character+0xB58,definition) || !plausiblePointer(definition) ||
        !fetch(definition+0x28,reinterpret_cast<PBYTE>(&count),4) || count==0 || count>1024 ||
        !qword(definition+0x18,joints) || !plausiblePointer(joints) ||
        !fetch(joints-4,reinterpret_cast<PBYTE>(&stored),4) || (stored&0x3FFFFFFF)!=count ||
        !qword(result.character+0xB78,result.array) || !plausiblePointer(result.array))return result;
    result.definition=definition;result.joints=joints;result.jointCount=count;
    if(cached&&cached->entityId==entityId&&cached->character==result.character&&cached->definition==definition&&cached->joints==joints&&cached->jointCount==count&&cached->address&&cached->leftFoot&&cached->rightFoot){
        auto relocate=[&](QWORD address){return address?result.array+(address-cached->array):0;};
        result.joint=cached->joint;result.address=relocate(cached->address);result.leftFoot=relocate(cached->leftFoot);result.rightFoot=relocate(cached->rightFoot);
        for(unsigned i=0;i<17;++i)result.skeleton[i]=relocate(cached->skeleton[i]);
        return result;
    }
    std::vector<BYTE> names(static_cast<size_t>(count)*0x148);
    if(!fetch(joints,names.data(),static_cast<DWORD>(names.size())))return result;
    for(DWORD joint=0;joint<count;++joint) {
        QWORD pointer=0;std::memcpy(&pointer,names.data()+static_cast<size_t>(joint)*0x148+0x130,8);
        char name[96]={};
        if(pointer>=0x10000 && pointer<0x0000800000000000ULL && fetch(pointer,reinterpret_cast<PBYTE>(name),95)) {
            const QWORD address=result.array+static_cast<QWORD>(joint)*0x1C;
            if(!_stricmp(name,"Bip01 Head")){result.joint=joint;result.address=address;}
            if(!_stricmp(name,"Bip01 L Foot"))result.leftFoot=address;
            if(!_stricmp(name,"Bip01 R Foot"))result.rightFoot=address;
            for(unsigned bone=0;bone<17;++bone)if(!_stricmp(name,skeletonNames[bone]))result.skeleton[bone]=address;
        }
    }
    return result;
}


