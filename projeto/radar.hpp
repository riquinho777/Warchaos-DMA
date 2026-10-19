// C++17 / radar.hpp / Windows x64 / camera-relative horizontal radar.
#pragma once
#include "projection.hpp"
namespace radar {
struct Point{bool valid=false;float right=0,forward=0;bool outside=false;};
inline Point locate(const projection::Matrix& view,float x,float y,float range=50){
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(range)||range<=0)return {};
    for(float value:view)if(!std::isfinite(value))return {};
    const float rx=view[0],ry=view[4],fx=-view[2],fy=-view[6];
    const float rl=std::hypot(rx,ry),fl=std::hypot(fx,fy);
    if(rl<0.001f||fl<0.001f)return {};
    const float eyeX=-(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]);
    const float eyeY=-(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]);
    const float dx=x-eyeX,dy=y-eyeY;
    float right=(dx*rx+dy*ry)/rl/range,forward=(dx*fx+dy*fy)/fl/range;
    const float distance=std::hypot(right,forward);if(!std::isfinite(distance))return {};
    const bool outside=distance>1;if(outside){right/=distance;forward/=distance;}
    return {true,right,forward,outside};
}
}
