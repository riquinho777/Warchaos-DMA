// C++17 / aim-policy.hpp / screen target selection and Ferrum command policy; no device IO.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace aim {
struct Config {double fov=80,gain=.2,maxStep=12,deadzone=1,triggerRadius=4;std::uint64_t maxAge=35,dwell=40,cooldown=160,visibilityGrace=80;};
struct Target {std::uint32_t id=0;double x=0,y=0,radius=0;bool visible=false;bool bodyMode=false,bodyInside=false,occluded=false;};
struct Frame {std::uint64_t time=0;std::vector<Target> targets;};
inline Frame triggerFrame(const Frame& source,bool requireVisibility){Frame result=source;if(!requireVisibility)for(auto& target:result.targets)target.visible=true;return result;}
struct Action {int dx=0,dy=0;bool fire=false;std::uint32_t target=0;bool visibilityGrace=false;};
class Policy {
    std::uint32_t locked=0,tracking=0;std::uint64_t since=0,lastFire=0,lastFrame=0,lastConfirmedVisible=0;
    double residualX=0,residualY=0;
public:
    void reset(){locked=tracking=0;since=lastFire=lastFrame=lastConfirmedVisible=0;residualX=residualY=0;}
    Action tick(const Config& cfg,const Frame& frame,std::uint64_t now,bool held,bool aimOn,bool triggerOn){
        Action result;
        if(!held||(!aimOn&&!triggerOn)||!frame.time||now<frame.time||now-frame.time>cfg.maxAge){reset();return result;}
        const Target* best=nullptr;const Target* previous=nullptr;double distance=cfg.fov;
        for(const auto& t:frame.targets){if(!t.id||!std::isfinite(t.x)||!std::isfinite(t.y)||(aimOn&&t.occluded))continue;
            const bool retain=aimOn&&t.id==locked&&lastConfirmedVisible&&now>=lastConfirmedVisible&&now-lastConfirmedVisible<=cfg.visibilityGrace;
            if(!t.visible&&!retain)continue;
            const bool bodyTrigger=triggerOn&&!aimOn&&t.bodyMode;
            if(bodyTrigger&&!t.bodyInside)continue;
            const double d=bodyTrigger?0.:std::hypot(t.x-960,t.y-540);if(d>cfg.fov)continue;
            if(t.id==locked)previous=&t;
            if(!best||d<distance){best=&t;distance=d;}
        }
        if(previous&&(!best||(triggerOn&&!aimOn&&previous->bodyMode&&previous->bodyInside)||std::hypot(previous->x-960,previous->y-540)<=distance*1.25))best=previous;
        if(!best){locked=tracking=0;since=lastConfirmedVisible=0;residualX=residualY=0;return result;}
        result.target=best->id;
        if(locked!=best->id){residualX=residualY=0;lastConfirmedVisible=0;locked=best->id;}
        if(best->visible)lastConfirmedVisible=frame.time;
        result.visibilityGrace=!best->visible;
        const double dx=best->x-960,dy=best->y-540;
        if(aimOn&&lastFrame!=frame.time){
            auto step=[&](double error,double& residual){if(std::fabs(error)<=cfg.deadzone){residual=0;return 0;}
                const double value=std::clamp(error*cfg.gain+residual,-cfg.maxStep,cfg.maxStep);
                const int rounded=static_cast<int>(std::round(value));residual=value-rounded;return rounded;};
            result.dx=step(dx,residualX);result.dy=step(dy,residualY);
        }
        lastFrame=frame.time;
        const bool crosshair=best->bodyMode?best->bodyInside:std::hypot(dx,dy)<=(std::min)(cfg.triggerRadius,best->radius);
        if(!triggerOn||!crosshair||!best->visible){tracking=0;since=0;return result;}
        if(tracking!=best->id){tracking=best->id;since=now;}
        if(now>=since&&now-since>=cfg.dwell&&(!lastFire||(now>=lastFire&&now-lastFire>=cfg.cooldown))){result.fire=true;lastFire=now;}
        return result;
    }
};
}
