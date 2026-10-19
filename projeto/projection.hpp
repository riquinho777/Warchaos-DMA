// C++17 / projection.hpp / Windows x64 diagnostic / renderer matrix reconstruction.
#pragma once
#include <array>
#include <cmath>

namespace projection {
using Matrix = std::array<float,16>;
using Vector = std::array<float,4>;
inline Vector transform(const Matrix& matrix,const Vector& point) {
    Vector result{};
    for(unsigned row=0;row<4;++row)
        result[row]=matrix[row]*point[0]+matrix[4+row]*point[1]+matrix[8+row]*point[2]+matrix[12+row]*point[3];
    return result;
}
struct Result { bool valid=false; float x=0,y=0,depth=0,w=0; bool inside=false; };
inline Result project(const Matrix& first,const Matrix& second,const Vector& world,
    const std::array<int,4>& viewport,int outputWidth,int outputHeight) {
    if(viewport[2]<=0 || viewport[3]<=0 || outputWidth<=0 || outputHeight<=0) return {};
    const auto clip=transform(second,transform(first,world));
    for(float value:clip) if(!std::isfinite(value)) return {};
    if(clip[3]<=0.00001f) return {};
    const float inverseW=1.0f/clip[3];
    const float nx=clip[0]*inverseW,ny=clip[1]*inverseW,nz=clip[2]*inverseW;
    // Renderer method scales viewport coordinates to percentages; convert to output pixels.
    const float x=(viewport[0]+(nx+1.0f)*viewport[2]*0.5f)/viewport[2]*outputWidth;
    const float y=(viewport[1]+(1.0f-ny)*viewport[3]*0.5f)/viewport[3]*outputHeight;
    if(!std::isfinite(x)||!std::isfinite(y)) return {};
    return {true,x,y,nz,clip[3],x>=0 && x<outputWidth && y>=0 && y<outputHeight && nz>=0 && nz<=1};
}
}
