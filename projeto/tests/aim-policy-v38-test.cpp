// C++17 / aim-policy-test.cpp / input simulation only, no Ferrum or game process.
#include "../aim-policy.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
    aim::Config cfg;aim::Policy p;aim::Frame f{100,{{1,1000,550,5,true},{2,970,540,5,false}}};
    auto a=p.tick(cfg,f,100,true,true,false);assert(a.target==1&&a.dx==8&&a.dy==2&&!a.fire);
    a=p.tick(cfg,f,101,true,true,false);assert(a.dx==0&&a.dy==0);
    a=p.tick(cfg,f,102,false,true,true);assert(!a.target&&!a.fire);
    a=p.tick(cfg,f,103,true,false,false);assert(!a.target);
    a=p.tick(cfg,f,136,true,true,true);assert(!a.target);
    a=p.tick(cfg,f,99,true,true,true);assert(!a.target);
    f={200,{{1,960,540,5,true}}};assert(!p.tick(cfg,f,200,true,false,true).fire);
    f.time=230;assert(!p.tick(cfg,f,230,true,false,true).fire);
    f.time=240;assert(p.tick(cfg,f,240,true,false,true).fire);
    f.time=250;assert(!p.tick(cfg,f,250,true,false,true).fire);
    f.time=400;assert(p.tick(cfg,f,400,true,false,true).fire);
    f={410,{}};assert(!p.tick(cfg,f,410,true,true,true).target);
    f={420,{{3,960,540,5,true}}};assert(!p.tick(cfg,f,420,true,false,true).fire);
    f={460,{{3,960,540,5,false}}};assert(!p.tick(cfg,f,460,true,false,true).target);
    f={500,{{3,960,540,5,true}}};assert(!p.tick(cfg,f,500,true,false,true).fire);
    f={540,{{3,980,540,5,true}}};assert(!p.tick(cfg,f,540,true,false,true).fire);
    p.reset();f={600,{{4,1200,540,5,true}}};assert(!p.tick(cfg,f,600,true,true,true).target);
    f={610,{{4,std::numeric_limits<double>::quiet_NaN(),540,5,true}}};assert(!p.tick(cfg,f,610,true,true,true).target);
    cfg.gain=2;f={620,{{4,1000,500,5,true}}};a=p.tick(cfg,f,620,true,true,false);assert(a.dx==12&&a.dy==-12);
    p.reset();cfg.dwell=0;f={700,{{8,1100,600,5,false,true,true}}};
    assert(!p.tick(cfg,aim::triggerFrame(f,true),700,true,false,true).fire);
    assert(p.tick(cfg,aim::triggerFrame(f,false),700,true,false,true).fire);
    assert(!f.targets[0].visible);p.reset();
    assert(!p.tick(cfg,f,700,true,true,false).target);
    f.targets[0].bodyInside=false;assert(!p.tick(cfg,aim::triggerFrame(f,false),700,true,false,true).fire);
    std::puts("PASS: hold, toggles, freshness, visibility, range, duplicate frame, step limit, dwell, cooldown, target removal and switch");
}
