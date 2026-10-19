// C++17 / aim-grace-v38-test.cpp / fresh-position lock continuity and immediate occlusion release.
#include "../aim-policy.hpp"
#include <cassert>
#include <cstdio>
int main(){aim::Config cfg;cfg.dwell=0;aim::Policy p;
auto frame=[](unsigned long long time,double x,bool visible,bool occluded=false){return aim::Frame{time,{{1,x,540,5,visible,true,true,occluded}}};};
auto a=p.tick(cfg,frame(1000,1000,true),1000,true,true,false);assert(a.target==1&&a.dx==8&&!a.visibilityGrace);
a=p.tick(cfg,frame(1010,980,false),1010,true,true,false);assert(a.target==1&&a.dx==4&&a.visibilityGrace&&!a.fire);
a=p.tick(cfg,frame(1010,980,false),1011,true,true,false);assert(a.target==1&&a.dx==0&&a.visibilityGrace);
a=p.tick(cfg,frame(1080,980,false),1080,true,true,false);assert(a.target==1&&a.visibilityGrace);
a=p.tick(cfg,frame(1081,980,false),1081,true,true,false);assert(!a.target);
p.reset();assert(!p.tick(cfg,frame(1090,980,false),1090,true,true,false).target);
assert(p.tick(cfg,frame(1100,980,true),1100,true,true,false).target);
assert(!p.tick(cfg,frame(1120,980,false,true),1120,true,true,false).target);
assert(!p.tick(cfg,frame(1121,980,false),1121,true,true,false).target);
assert(p.tick(cfg,frame(1200,980,true),1200,true,true,false).target);
assert(!p.tick(cfg,aim::Frame{1220,{}},1220,true,true,false).target);
assert(p.tick(cfg,frame(1300,980,true),1300,true,true,false).target);
assert(!p.tick(cfg,frame(1320,1800,false),1320,true,true,false).target);
assert(p.tick(cfg,frame(1400,980,true),1400,true,true,false).target);
assert(!p.tick(cfg,frame(1410,980,false),1446,true,true,false).target);
assert(p.tick(cfg,frame(1500,960,true),1500,true,true,true).fire);
a=p.tick(cfg,frame(1510,980,false),1510,true,true,true);assert(a.target&&a.visibilityGrace&&!a.fire);
assert(!p.tick(cfg,frame(1520,980,false),1520,false,true,false).target);
assert(!p.tick(cfg,frame(1521,980,false),1521,true,true,false).target);
p.reset();assert(p.tick(cfg,frame(1600,960,true),1600,true,false,true).fire);
assert(!p.tick(cfg,frame(1610,960,false),1610,true,false,true).target);
std::puts("PASS: same ID/new position, 80ms expiry, duplicate frame, no unknown acquisition, immediate occlusion, removal, FOV, stale data, button release and strict trigger");}
