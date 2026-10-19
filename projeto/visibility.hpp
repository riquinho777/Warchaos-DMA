// C++17 / visibility.hpp / Windows x64 / engine occlusion cache classification.
#pragma once
#include <cstdint>
enum class Visibility { Unknown,Occluded,Visible };
inline Visibility classifyOcclusion(std::uint32_t frame,std::uint32_t visible,std::uint32_t occluded){
    if(!frame||(!visible&&!occluded))return Visibility::Unknown;
    const auto visibleAge=static_cast<std::int32_t>(frame-visible);
    const auto occludedAge=static_cast<std::int32_t>(frame-occluded);
    if(occluded && static_cast<std::int32_t>(occluded-visible)>=0 && occludedAge>=-2 && occludedAge<=32)return Visibility::Occluded;
    if(visible && (!occluded||static_cast<std::int32_t>(visible-occluded)>0) && visibleAge>=0 && visibleAge<=2)return Visibility::Visible;
    return Visibility::Unknown;
}
inline Visibility classifySampledOcclusion(std::uint32_t before,std::uint32_t after,std::uint32_t visible,std::uint32_t occluded){
    // A bounded end-of-read counter avoids calling an asynchronously sampled object "future".
    if(!before||!after||static_cast<std::int32_t>(after-before)<0||after-before>16)return Visibility::Unknown;
    return classifyOcclusion(after,visible,occluded);
}
// Presentation only. Never feed the held color back into targeting decisions.
struct VisibilityDisplay {
    Visibility last=Visibility::Unknown;std::uint64_t time=0;
    Visibility update(Visibility current,std::uint64_t now){
        if(current!=Visibility::Unknown){last=current;time=now;return current;}
        if(time&&now>=time&&now-time<=120)return last;
        last=Visibility::Unknown;time=0;return last;
    }
};
