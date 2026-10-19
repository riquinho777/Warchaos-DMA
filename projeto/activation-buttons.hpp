// C++17 / activation-buttons.hpp / Ferrum physical mouse button mappings.
#pragma once
#include <string>
namespace activation {
struct Bindings{unsigned aim=1,trigger=1;bool operator==(const Bindings& b)const{return aim==b.aim&&trigger==b.trigger;}bool operator!=(const Bindings& b)const{return !(*this==b);}};
inline unsigned valid(unsigned i){return i<5?i:1;}
inline const char* label(unsigned i){static const char* names[]={"Mouse esquerdo","Mouse direito","Mouse central","Lateral 1 (Mouse 4)","Lateral 2 (Mouse 5)"};return names[valid(i)];}
inline std::string query(unsigned i){static const char* methods[]={"left","right","middle","side1","side2"};return std::string("km.")+methods[valid(i)]+"()";}
}
