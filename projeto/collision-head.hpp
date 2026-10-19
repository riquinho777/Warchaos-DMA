// C++17 / collision-head.hpp / observed CryPhysics client geometry; DMA reads only.
#pragma once
#include "dma-head.hpp"
#include <algorithm>
namespace collisionHead {
using V=std::array<float,3>;
struct Geometry {QWORD physical=0,physicalTable=0,parts=0,descriptor=0,shape=0,shapeTable=0;DWORD index=0,joint=0,type=0;std::vector<V> points;std::vector<int> triangles;V center{},axis{},extents{};std::array<float,9> basis{};float radius=0,halfLength=0;bool oriented=false;};
struct Sample {std::array<BYTE,0x130> physical{};std::array<BYTE,0x88> part{};QWORD checkedPhysical=0,checkedShape=0;DWORD physicalBytes=0,partBytes=0,pointerBytes=0,shapeBytes=0;};
template<class T> T field(const BYTE* bytes,unsigned offset){T value{};std::memcpy(&value,bytes+offset,sizeof(value));return value;}
inline bool finite(V p){for(float f:p)if(!std::isfinite(f)||std::fabs(f)>100000)return false;return true;}
inline bool quaternion(const float* q){float n=0;for(unsigned i=0;i<4;++i){if(!std::isfinite(q[i]))return false;n+=q[i]*q[i];}return n>.9f&&n<1.1f;}
inline V rotate(const float* q,V v){V t{2*(q[1]*v[2]-q[2]*v[1]),2*(q[2]*v[0]-q[0]*v[2]),2*(q[0]*v[1]-q[1]*v[0])};return {v[0]+q[3]*t[0]+q[1]*t[2]-q[2]*t[1],v[1]+q[3]*t[1]+q[2]*t[0]-q[0]*t[2],v[2]+q[3]*t[2]+q[0]*t[1]-q[1]*t[0]};}
template<class Read> Geometry discover(Read read,QWORD character,DWORD joint,QWORD physicsBase){
    Geometry result;if(!physicsBase)return result;QWORD physical=0;if(!read(character+0xB38,&physical,8)||!plausiblePointer(physical))return result;
    BYTE header[0x130]={};if(!read(physical,header,sizeof(header)))return result;
    const QWORD parts=field<QWORD>(header,0x120),vt=field<QWORD>(header,0);const int count=field<int>(header,0x128);
    if(!plausiblePointer(parts)||vt<physicsBase||vt>=physicsBase+0x3D3080||count<1||count>128)return result;
    std::vector<BYTE> records(static_cast<size_t>(count)*0x88);if(!read(parts,records.data(),static_cast<DWORD>(records.size())))return result;
    for(int i=0;i<count;++i){const BYTE* part=records.data()+i*0x88;if(field<DWORD>(part,0x48)!=joint)continue;
        const QWORD descriptor=field<QWORD>(part,0x38);QWORD shape=0;if(!plausiblePointer(descriptor)||!read(descriptor,&shape,8)||!plausiblePointer(shape))return {};
        BYTE geometry[0x90]={};if(!read(shape,geometry,sizeof(geometry)))return {};const QWORD table=field<QWORD>(geometry,0);
        result.physical=physical;result.physicalTable=vt;result.parts=parts;result.index=i;result.joint=joint;result.descriptor=descriptor;result.shape=shape;result.shapeTable=table;
        if(table==physicsBase+0x24CCA8){result.type=4;std::memcpy(result.center.data(),geometry+0x30,12);const float radius=field<float>(geometry,0x3C);result.radius=radius;
            if(!finite(result.center)||!std::isfinite(radius)||radius<.01f||radius>1.f)return {};
            // Surface samples from the real sphere radius; 2D contour is sampled, not an engine raycast.
            for(int latitude=-5;latitude<=5;++latitude){const double a=latitude*3.141592653589793/12.;for(int longitude=0;longitude<24;++longitude){const double b=longitude*3.141592653589793/12.;result.points.push_back({result.center[0]+radius*static_cast<float>(std::cos(a)*std::cos(b)),result.center[1]+radius*static_cast<float>(std::cos(a)*std::sin(b)),result.center[2]+radius*static_cast<float>(std::sin(a))});}}
            result.points.push_back({result.center[0],result.center[1],result.center[2]+radius});result.points.push_back({result.center[0],result.center[1],result.center[2]-radius});
        }else if(table==physicsBase+0x24E770){result.type=1;const int stride=field<int>(geometry,0x60),triangles=field<int>(geometry,0x80),vertices=field<int>(geometry,0x84);const QWORD positions=field<QWORD>(geometry,0x58),indices=field<QWORD>(geometry,0x40);
            if(stride<12||stride>64||vertices<4||vertices>512||triangles<1||triangles>1024||!plausiblePointer(positions)||!plausiblePointer(indices))return {};
            std::vector<BYTE> data(static_cast<size_t>(vertices)*stride);std::vector<int> faces(static_cast<size_t>(triangles)*3);
            if(!read(positions,data.data(),static_cast<DWORD>(data.size()))||!read(indices,faces.data(),static_cast<DWORD>(faces.size()*4)))return {};
            for(int index:faces)if(index<0||index>=vertices)return {};
            result.triangles=faces;
            V low{10000,10000,10000},high{-10000,-10000,-10000};
            for(int j=0;j<vertices;++j){V point{};std::memcpy(point.data(),data.data()+static_cast<size_t>(j)*stride,12);if(!finite(point))return {};result.points.push_back(point);for(unsigned k=0;k<3;++k){low[k]=(std::min)(low[k],point[k]);high[k]=(std::max)(high[k],point[k]);}}
            for(unsigned k=0;k<3;++k){if(high[k]-low[k]>2)return {};result.center[k]=(low[k]+high[k])*.5f;}
        }else if(table==physicsBase+0x24DEA0){result.type=6;std::memcpy(result.center.data(),geometry+0x30,12);std::memcpy(result.axis.data(),geometry+0x3C,12);result.radius=field<float>(geometry,0x48);result.halfLength=field<float>(geometry,0x4C);
            const float norm=result.axis[0]*result.axis[0]+result.axis[1]*result.axis[1]+result.axis[2]*result.axis[2];if(!finite(result.center)||!finite(result.axis)||norm<.9f||norm>1.1f||!std::isfinite(result.radius)||result.radius<=0||result.radius>2||!std::isfinite(result.halfLength)||result.halfLength<0||result.halfLength>3)return {};result.points.push_back(result.center);
        }else if(table==physicsBase+0x24D6C0){result.type=0;std::memcpy(result.basis.data(),geometry+0x30,36);result.oriented=field<int>(geometry,0x54)!=0;std::memcpy(result.center.data(),geometry+0x58,12);std::memcpy(result.extents.data(),geometry+0x64,12);
            if(!finite(result.center)||!finite(result.extents))return {};for(float f:result.extents)if(f<=0||f>3)return {};for(float f:result.basis)if(!std::isfinite(f))return {};result.points.push_back(result.center);
        }else return {};
        return result;
    }return {};
}
template<class Read> std::vector<Geometry> discoverAll(Read read,QWORD character,DWORD headJoint,QWORD physicsBase){
    QWORD physical=0;if(!read(character+0xB38,&physical,8)||!plausiblePointer(physical))return {};
    std::array<BYTE,0x130> header{};if(!read(physical,header.data(),static_cast<DWORD>(header.size())))return {};
    const QWORD parts=field<QWORD>(header.data(),0x120);const int count=field<int>(header.data(),0x128);if(!plausiblePointer(parts)||count<1||count>128)return {};
    std::vector<BYTE> records(static_cast<size_t>(count)*0x88);if(!read(parts,records.data(),static_cast<DWORD>(records.size())))return {};
    auto cachedRead=[&](QWORD address,void* output,DWORD size){if(address==character+0xB38&&size==8){std::memcpy(output,&physical,8);return true;}if(address==physical&&size==header.size()){std::memcpy(output,header.data(),size);return true;}if(address==parts&&size==records.size()){std::memcpy(output,records.data(),size);return true;}return read(address,output,size);};
    std::vector<Geometry> result;
    for(int i=0;i<count;++i){const DWORD joint=field<DWORD>(records.data()+i*0x88,0x48);auto g=discover(cachedRead,character,joint,physicsBase);if(g.points.empty())continue;
        if(g.type==4&&joint!=headJoint)g.points={g.center};result.push_back(std::move(g));}
    return result;
}
inline bool valid(const Geometry& g,const Sample& sample){
    return !g.points.empty()&&sample.physicalBytes==0x130&&sample.partBytes==0x88&&sample.pointerBytes==8&&sample.shapeBytes==8&&sample.checkedPhysical==g.physical&&sample.checkedShape==g.shape&&
        field<QWORD>(sample.physical.data(),0)==g.physicalTable&&field<QWORD>(sample.physical.data(),0x120)==g.parts&&field<int>(sample.physical.data(),0x128)>static_cast<int>(g.index)&&
        field<DWORD>(sample.part.data(),0x48)==g.joint&&field<QWORD>(sample.part.data(),0x38)==g.descriptor;
}
inline bool transform(const Sample& s,V local,V& world){float entity[7]={},part[8]={};std::memcpy(entity,s.physical.data()+0xA8,sizeof(entity));std::memcpy(part,s.part.data(),sizeof(part));
    if(!quaternion(entity+3)||!quaternion(part+3)||!std::isfinite(part[7])||part[7]<=0||part[7]>10||!finite(local))return false;
    for(float& v:local)v*=part[7];local=rotate(part+3,local);for(unsigned i=0;i<3;++i)local[i]+=part[i];world=rotate(entity+3,local);for(unsigned i=0;i<3;++i)world[i]+=entity[i];return finite(world);
}
inline std::vector<projection::Result> hull(std::vector<projection::Result> points){
    points.erase(std::remove_if(points.begin(),points.end(),[](const auto& p){return !p.valid||!std::isfinite(p.x)||!std::isfinite(p.y);}),points.end());if(points.size()<3)return {};
    std::sort(points.begin(),points.end(),[](const auto& a,const auto& b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});
    auto cross=[](const auto& a,const auto& b,const auto& c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);};
    std::vector<projection::Result> output;for(const auto& p:points){while(output.size()>1&&cross(output[output.size()-2],output.back(),p)<=0)output.pop_back();output.push_back(p);}const size_t lower=output.size();
    for(size_t i=points.size()-1;i-->0;){const auto& p=points[i];while(output.size()>lower&&cross(output[output.size()-2],output.back(),p)<=0)output.pop_back();output.push_back(p);}if(output.size()>1)output.pop_back();return output.size()>=3?output:std::vector<projection::Result>{};
}
}
