// C++17 / collision-cache-v39-test.cpp / mock memory only; no DMA or Ferrum access.
#define main application_main
#include "../esp-prototipo.cpp"
#undef main
#include <cassert>
#include <map>
static std::map<QWORD,std::vector<BYTE>> memory;
template<class T> static void put(QWORD address,T value){for(auto& item:memory)if(address>=item.first&&address+sizeof(T)<=item.first+item.second.size()){std::memcpy(item.second.data()+address-item.first,&value,sizeof(T));return;}assert(false);}
static BOOL readMock(VMM_HANDLE,DWORD,ULONG64 address,PBYTE output,DWORD size,PDWORD received,ULONG64){if(received)*received=0;for(const auto& item:memory)if(address>=item.first&&address+size<=item.first+item.second.size()){std::memcpy(output,item.second.data()+address-item.first,size);if(received)*received=size;return TRUE;}return FALSE;}
struct Request{QWORD address;DWORD size;PBYTE output;PDWORD received;};
static VMMDLL_SCATTER_HANDLE initMock(VMM_HANDLE,DWORD,DWORD){return reinterpret_cast<VMMDLL_SCATTER_HANDLE>(new std::vector<Request>);}
static BOOL prepareMock(VMMDLL_SCATTER_HANDLE batch,QWORD address,DWORD size,PBYTE output,PDWORD received){reinterpret_cast<std::vector<Request>*>(batch)->push_back({address,size,output,received});return TRUE;}
static BOOL executeMock(VMMDLL_SCATTER_HANDLE batch){BOOL ok=TRUE;for(const auto& r:*reinterpret_cast<std::vector<Request>*>(batch))if(!readMock(nullptr,0,r.address,r.output,r.size,r.received,0))ok=FALSE;return ok;}
static VOID closeMock(VMMDLL_SCATTER_HANDLE batch){delete reinterpret_cast<std::vector<Request>*>(batch);}
int main(){preview::Reader reader;reader.readFresh=readMock;reader.initScatter=initMock;reader.prepare=prepareMock;reader.execute=executeMock;reader.closeScatter=closeMock;reader.physicsBase=0x100000000;
HeadSample head;head.character=0x200000;head.joint=13;collisionHead::Geometry g;g.physical=0x300000;g.parts=0x400000;g.descriptor=0x500000;g.shape=0x600000;g.physicalTable=reader.physicsBase+0x1000;g.shapeTable=reader.physicsBase+0x24CCA8;g.joint=13;g.points={{0,0,0}};
memory[head.character+0xB38].resize(8);memory[g.physical].resize(0x130);memory[g.parts].resize(0x88);memory[g.descriptor].resize(8);memory[g.shape].resize(0x90);
put(head.character+0xB38,g.physical);put(g.physical,g.physicalTable);put(g.physical+0x120,g.parts);put<int>(g.physical+0x128,1);put(g.parts+0x38,g.descriptor);put<DWORD>(g.parts+0x48,13);put(g.descriptor,g.shape);put(g.shape,g.shapeTable);put<float>(g.shape+0x3C,.13f);
auto body=std::make_shared<const std::vector<collisionHead::Geometry>>(std::vector<collisionHead::Geometry>{g});const auto baseMemory=memory;auto reset=[&]{memory=baseMemory;reader.collisionLayouts[7]={head.character,1,body};};bool reused=false;
reset();assert(reader.collisionLayout(head,7,reused)==body&&reused);
reset();put<QWORD>(head.character+0xB38,0x301000);reader.collisionLayout(head,7,reused);assert(!reused);
reset();put<int>(g.physical+0x128,2);reader.collisionLayout(head,7,reused);assert(!reused);
reset();put<DWORD>(g.parts+0x48,14);reader.collisionLayout(head,7,reused);assert(!reused);
reset();put<QWORD>(g.parts+0x38,0x501000);reader.collisionLayout(head,7,reused);assert(!reused);
reset();put<QWORD>(g.descriptor,0x601000);reader.collisionLayout(head,7,reused);assert(!reused);
reset();put<QWORD>(g.shape,g.shapeTable+8);reader.collisionLayout(head,7,reused);assert(!reused);
reset();memory.erase(g.descriptor);reader.collisionLayout(head,7,reused);assert(!reused);
reset();reader.collisionLayouts[7].character+=8;reader.collisionLayout(head,7,reused);assert(!reused);
std::puts("PASS: cache hit, physical identity, count, joint, descriptor, shape, vtable, failed reads and character identity");}
