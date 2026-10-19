// C++17 / frame-policy-test.cpp / Windows x64 / transient frame retention limits.
#include "../frame-policy.hpp"
#include <cassert>
#include <cstdio>
int main(){
    assert(canReuseFrame(true,100,101));
    assert(canReuseFrame(true,100,160));
    assert(!canReuseFrame(true,100,161));
    assert(!canReuseFrame(false,100,101));
    assert(!canReuseFrame(true,0,1));
    assert(!canReuseFrame(true,100,99));
    assert(!canReuseFrame(true,100,1000));
    std::puts("PASS: 7 casos de retencao curta, expiracao e falhas nao transitorias");
}
