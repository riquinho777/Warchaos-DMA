// C++17 / trigger-body.hpp / projected skeleton capsules; approximation, not engine hitboxes.
#pragma once
#include "projection.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace triggerBody {
inline bool contains(const std::array<projection::Result,17>& bones,double height,double margin,double x=960,double y=540){
    if(!std::isfinite(height)||height<3||height>4000||!std::isfinite(margin))return false;
    const double padding=std::clamp(margin,0.,12.)*.25;
    auto valid=[](const projection::Result& p){return p.valid&&std::isfinite(p.x)&&std::isfinite(p.y);};
    auto capsule=[&](unsigned a,unsigned b,double radius){const auto& p=bones[a];const auto& q=bones[b];if(!valid(p)||!valid(q))return false;
        const double dx=q.x-p.x,dy=q.y-p.y,length=dx*dx+dy*dy;
        const double t=length>1e-8?std::clamp(((x-p.x)*dx+(y-p.y)*dy)/length,0.,1.):0;
        return std::hypot(x-p.x-t*dx,y-p.y-t*dy)<=radius+padding;};
    const double limb=(std::max)(1.,height*.025),torso=(std::max)(2.,height*.07),head=(std::max)(1.,height*.045);
    if(capsule(0,0,head)||capsule(0,1,limb)||capsule(1,4,torso)||capsule(5,8,limb))return true;
    static constexpr unsigned links[][2]={{1,5},{5,6},{6,7},{1,8},{8,9},{9,10},{4,11},{11,12},{12,13},{4,14},{14,15},{15,16}};
    for(const auto& link:links)if(capsule(link[0],link[1],limb))return true;return false;
}
}
