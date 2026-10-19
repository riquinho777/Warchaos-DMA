// C++17 / collision-ray.hpp / local mathematical intersections with client geometry; no remote calls.
#pragma once
#include "collision-head.hpp"
#include <limits>
namespace collisionRay {
using V=collisionHead::V;
struct Ray{V origin{},direction{};bool valid=false;};
inline V sub(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline V add(V a,V b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline V mul(V a,float b){return {a[0]*b,a[1]*b,a[2]*b};}
inline float dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline float miss(){return std::numeric_limits<float>::infinity();}
inline bool inverse(const projection::Matrix& input,projection::Matrix& output){double a[4][8]={};for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)a[i][j]=input[j*4+i];a[i][i+4]=1;}
    for(unsigned k=0;k<4;++k){unsigned pivot=k;for(unsigned i=k+1;i<4;++i)if(std::fabs(a[i][k])>std::fabs(a[pivot][k]))pivot=i;if(std::fabs(a[pivot][k])<1e-12)return false;for(unsigned j=0;j<8;++j)std::swap(a[k][j],a[pivot][j]);const double v=a[k][k];for(double& x:a[k])x/=v;for(unsigned i=0;i<4;++i)if(i!=k){const double f=a[i][k];for(unsigned j=0;j<8;++j)a[i][j]-=a[k][j]*f;}}
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){const double v=a[i][j+4];if(!std::isfinite(v))return false;output[j*4+i]=static_cast<float>(v);}return true;
}
inline Ray fromCamera(const RendererProjection& camera){projection::Matrix combined{},inv{};for(unsigned j=0;j<4;++j){projection::Vector column{};for(unsigned i=0;i<4;++i)column[i]=camera.first[j*4+i];const auto transformed=projection::transform(camera.second,column);for(unsigned i=0;i<4;++i)combined[j*4+i]=transformed[i];}
    if(camera.viewport[2]<=0||camera.viewport[3]<=0||!inverse(combined,inv))return {};
    const float x=-2.f*camera.viewport[0]/camera.viewport[2],y=2.f*camera.viewport[1]/camera.viewport[3];
    auto unproject=[&](float z,V& output){const auto p=projection::transform(inv,{x,y,z,1});if(!std::isfinite(p[3])||std::fabs(p[3])<1e-8)return false;output={p[0]/p[3],p[1]/p[3],p[2]/p[3]};return collisionHead::finite(output);};
    // Start at the camera eye. Depth zero can be the far plane (reversed Z),
    // so subtracting unproject(0) from unproject(.5) can reverse the ray.
    projection::Matrix inverseView{};if(!inverse(camera.first,inverseView))return {};
    const auto eye=projection::transform(inverseView,{0,0,0,1});if(!std::isfinite(eye[3])||std::fabs(eye[3])<1e-8)return {};
    Ray ray;ray.origin={eye[0]/eye[3],eye[1]/eye[3],eye[2]/eye[3]};V end{};if(!collisionHead::finite(ray.origin)||!unproject(.5f,end))return {};ray.direction=sub(end,ray.origin);const float length=std::sqrt(dot(ray.direction,ray.direction));if(!std::isfinite(length)||length<1e-7)return {};ray.direction=mul(ray.direction,1/length);
    const auto front=projection::transform(camera.second,projection::transform(camera.first,{end[0],end[1],end[2],1}));if(!std::isfinite(front[3])||front[3]<=0)return {};
    ray.valid=true;return ray;
}
inline bool toLocal(const collisionHead::Sample& s,V world,V& local){float e[7]={},p[8]={};std::memcpy(e,s.physical.data()+0xA8,sizeof(e));std::memcpy(p,s.part.data(),sizeof(p));if(!collisionHead::quaternion(e+3)||!collisionHead::quaternion(p+3)||!std::isfinite(p[7])||p[7]<=0||p[7]>10)return false;
    float eq[4]={-e[3],-e[4],-e[5],e[6]},pq[4]={-p[3],-p[4],-p[5],p[6]};local=collisionHead::rotate(eq,sub(world,{e[0],e[1],e[2]}));local=mul(collisionHead::rotate(pq,sub(local,{p[0],p[1],p[2]})),1/p[7]);return collisionHead::finite(local);
}
inline float sphere(V o,V d,V center,float radius){const V v=sub(o,center);const float a=dot(d,d),b=dot(v,d),c=dot(v,v)-radius*radius,disc=b*b-a*c;if(a<1e-12||disc<0)return miss();const float t=(-b-std::sqrt(disc))/a;return t>=0?t:((-b+std::sqrt(disc))/a>=0?0:miss());}
inline float triangle(V o,V d,V a,V b,V c){const V e1=sub(b,a),e2=sub(c,a),p=cross(d,e2);const float det=dot(e1,p);if(std::fabs(det)<1e-9)return miss();const float inv=1/det;const V t=sub(o,a);const float u=dot(t,p)*inv;if(u<0||u>1)return miss();const V q=cross(t,e1);const float v=dot(d,q)*inv;if(v<0||u+v>1)return miss();const float distance=dot(e2,q)*inv;return distance>=0?distance:miss();}
inline float intersect(const collisionHead::Geometry& g,const collisionHead::Sample& sample,const Ray& ray){if(!ray.valid||!collisionHead::valid(g,sample))return miss();V o{},end{};if(!toLocal(sample,ray.origin,o)||!toLocal(sample,add(ray.origin,ray.direction),end))return miss();const V d=sub(end,o);
    if(g.type==4)return sphere(o,d,g.center,g.radius);
    if(g.type==1){float closest=miss();for(size_t i=0;i+2<g.triangles.size();i+=3){const auto a=g.triangles[i],b=g.triangles[i+1],c=g.triangles[i+2];if(a<0||b<0||c<0||static_cast<size_t>((std::max)({a,b,c}))>=g.points.size())return miss();closest=(std::min)(closest,triangle(o,d,g.points[a],g.points[b],g.points[c]));}return closest;}
    if(g.type==0){V origin=sub(o,g.center),direction=d;if(g.oriented){V rotated{},rd{};for(unsigned i=0;i<3;++i){V row{g.basis[i*3],g.basis[i*3+1],g.basis[i*3+2]};rotated[i]=dot(row,origin);rd[i]=dot(row,direction);}origin=rotated;direction=rd;}float low=0,high=miss();for(unsigned i=0;i<3;++i){if(std::fabs(direction[i])<1e-8){if(std::fabs(origin[i])>g.extents[i])return miss();}else{float a=(-g.extents[i]-origin[i])/direction[i],b=(g.extents[i]-origin[i])/direction[i];if(a>b)std::swap(a,b);low=(std::max)(low,a);high=(std::min)(high,b);if(high<low)return miss();}}return low;}
    if(g.type==6){const V a=sub(g.center,mul(g.axis,g.halfLength)),b=add(g.center,mul(g.axis,g.halfLength)),ba=sub(b,a),oa=sub(o,a);const float baba=dot(ba,ba),bard=dot(ba,d),baoa=dot(ba,oa),rdoa=dot(d,oa),oaoa=dot(oa,oa),aa=baba*dot(d,d)-bard*bard,bb=baba*rdoa-baoa*bard,cc=baba*oaoa-baoa*baoa-g.radius*g.radius*baba;
        const V nearest=add(a,mul(ba,baba>1e-12?std::clamp(baoa/baba,0.f,1.f):0.f));if(dot(sub(o,nearest),sub(o,nearest))<=g.radius*g.radius)return 0;
        float result=(std::min)(sphere(o,d,a,g.radius),sphere(o,d,b,g.radius));const float h=bb*bb-aa*cc;if(aa>1e-10&&h>=0){const float t=(-bb-std::sqrt(h))/aa,y=baoa+t*bard;if(t>=0&&y>=0&&y<=baba)result=(std::min)(result,t);}return result;}
    return miss();
}
}
