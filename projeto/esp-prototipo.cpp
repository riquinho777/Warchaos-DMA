// C++17 / esp-prototipo.cpp / Windows x64 / read-only DMA and HDMI fuser output.
#include "dma-head.hpp"
#include "visibility.hpp"
#include "radar.hpp"
#include "frame-policy.hpp"
#include "health.hpp"
#include "registry-policy.hpp"
#include "aim-policy.hpp"
#include "ferrum.hpp"
#include "activation-buttons.hpp"
#include "team-policy.hpp"
#include "collision-head.hpp"
#include "collision-ray.hpp"
#include <memory>
#include <algorithm>
#include <mmsystem.h>
#include <commdlg.h>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"comdlg32.lib")

namespace preview {
struct Marker { DWORD id=0;int health=0;std::string name; projection::Result head{},aimHead{};bool bodyHit=false;float bodyDistance=collisionRay::miss();std::vector<projection::Result> collisionOutline;std::array<projection::Result,17> bones{}; float left=0,top=0,right=0,bottom=0;bool box=false;Visibility visibility=Visibility::Unknown,displayVisibility=Visibility::Unknown; };
struct State { std::vector<Marker> markers;std::vector<radar::Point> radarPoints;ULONGLONG time=0;std::string status="Iniciando DMA";bool transient=false;unsigned partial=0,poseChanged=0,dead=0,unknownLife=0,realHeads=0,validBodyParts=0; };
static State state;
static std::mutex lock;
static std::atomic<bool> stop=false;
static std::atomic<HWND> displayWindow=nullptr;
static std::atomic<bool> paintQueued=false;
static void notifyFrame(){const HWND window=displayWindow.load();if(window&&!paintQueued.exchange(true))if(!PostMessageW(window,WM_APP+1,0,0))paintQueued=false;}
static std::atomic<bool> aimEnabled=false,triggerEnabled=false,ferrumReady=false;
static std::atomic<DWORD> selectedTarget=0;
static std::atomic<unsigned> aimReason=0;
static std::atomic<bool> triggerRequireVisibility=true;
static std::atomic<unsigned> triggerReason=0; // off, button, stale, no collision, visibility, dwell, movement, fire, menu
static bool ferrumRequested=false;
static bool showBox=true;
static bool showStatus=true;
static bool showNames=true;
static bool showSkeleton=true;
static bool showHead=true;
static bool showRadar=true;
static bool showHealth=true;
static std::atomic<bool> ffaMode=false;
#include "settings-panel.hpp"
struct BackBuffer {
    HBRUSH radarEnemy=CreateSolidBrush(RGB(255,70,70));
    HPEN boxPen=CreatePen(PS_SOLID,2,RGB(255,255,255));
    HPEN visiblePen=CreatePen(PS_SOLID,2,RGB(60,255,90)),occludedPen=CreatePen(PS_SOLID,2,RGB(255,70,70)),unknownPen=CreatePen(PS_SOLID,2,RGB(145,150,160));
    HPEN headPen=nullptr,fovPen=nullptr,radarPen=nullptr;
    EspAppearance applied;bool styled=false;
    void syncStyle(){if(styled&&applied.colors==appearance.colors&&applied.width==appearance.width)return;
        auto replace=[&](HPEN& pen,COLORREF color){if(pen)DeleteObject(pen);pen=CreatePen(PS_SOLID,appearance.width,color);};
        replace(boxPen,appearance.colors[0]);replace(headPen,appearance.colors[2]);replace(visiblePen,appearance.colors[3]);replace(occludedPen,appearance.colors[4]);replace(fovPen,appearance.colors[5]);replace(radarPen,appearance.colors[6]);
        if(radarEnemy)DeleteObject(radarEnemy);radarEnemy=CreateSolidBrush(appearance.colors[6]);applied=appearance;styled=true;
    }
    HFONT nameFont=CreateFontW(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ original=nullptr;int width=0,height=0;
    void clear(){if(dc&&original)SelectObject(dc,original);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);dc=nullptr;bitmap=nullptr;original=nullptr;width=height=0;}
    ~BackBuffer(){clear();if(headPen)DeleteObject(headPen);if(fovPen)DeleteObject(fovPen);if(radarPen)DeleteObject(radarPen);if(radarEnemy)DeleteObject(radarEnemy);if(boxPen)DeleteObject(boxPen);if(nameFont)DeleteObject(nameFont);if(visiblePen)DeleteObject(visiblePen);if(occludedPen)DeleteObject(occludedPen);if(unknownPen)DeleteObject(unknownPen);}
    bool resize(HDC screen,int w,int h){if(dc&&width==w&&height==h)return true;clear();dc=CreateCompatibleDC(screen);if(!dc)return false;bitmap=CreateCompatibleBitmap(screen,w,h);if(!bitmap){clear();return false;}original=SelectObject(dc,bitmap);width=w;height=h;return true;}
};
struct Display {RECT area{};std::wstring device;bool primary=false;};
static BOOL CALLBACK listDisplay(HMONITOR monitor,HDC,LPRECT,LPARAM data){
    auto* displays=reinterpret_cast<std::vector<Display>*>(data);MONITORINFOEXW info{};info.cbSize=sizeof(info);
    if(GetMonitorInfoW(monitor,&info))displays->push_back({info.rcMonitor,info.szDevice,(info.dwFlags&MONITORINFOF_PRIMARY)!=0});return TRUE;
}
struct Node {QWORD entity=0,proxy=0;DWORD id=0;HeadSample head;std::string name;QWORD renderTemp=0,actor=0,actorNode=0,actorTable=0;collisionHead::Geometry collision;std::shared_ptr<const std::vector<collisionHead::Geometry>> body;};

class Reader {
public:
    struct VisibilityRecord{DWORD id=0;QWORD character=0,temp=0;VisibilityDisplay display;};
    std::unordered_map<QWORD,VisibilityRecord> visibilityRecords;
    struct CollisionLayout{QWORD character=0;int count=0;std::shared_ptr<const std::vector<collisionHead::Geometry>> body;};
    std::unordered_map<DWORD,CollisionLayout> collisionLayouts; // Discovery thread only.
    std::unordered_map<DWORD,HeadSample> headLayouts; // Discovery thread only; metadata, never live positions.
    HMODULE dll=nullptr;VMM_HANDLE handle=nullptr;DWORD pid=0;QWORD entityBase=0,animationBase=0,rendererBase=0;
    DmaRead read=nullptr;
    std::atomic<DWORD> localId=0;
    QWORD gameBase=0,engineBase=0,actionBase=0,physicsBase=0;
    decltype(&VMMDLL_MemReadEx) readFresh=nullptr;
    decltype(&VMMDLL_Close) close=nullptr;
    decltype(&VMMDLL_Scatter_Initialize) initScatter=nullptr;
    decltype(&VMMDLL_Scatter_PrepareEx) prepare=nullptr;
    decltype(&VMMDLL_Scatter_ExecuteRead) execute=nullptr;
    decltype(&VMMDLL_Scatter_CloseHandle) closeScatter=nullptr;
    ~Reader(){if(handle&&close)close(handle);if(dll)FreeLibrary(dll);}
    bool bytes(QWORD address,void* out,DWORD size){DWORD received=0;return readFresh(handle,pid,address,static_cast<PBYTE>(out),size,&received,VMMDLL_FLAG_NOCACHE)!=FALSE && received==size;}
    bool pointer(QWORD address,QWORD& out){return bytes(address,&out,8)&&plausiblePointer(out);}
    std::string lastError;
    bool open(){
        if(handle&&close)close(handle);handle=nullptr;if(dll)FreeLibrary(dll);dll=nullptr;
        pid=0;entityBase=animationBase=rendererBase=gameBase=engineBase=actionBase=physicsBase=0;
        auto fail=[&](const char* reason){lastError=reason;std::printf("DMA ERRO: %s; Windows=%lu\n",reason,GetLastError());return false;};
        dll=LoadLibraryW(L"vmm.dll");if(!dll)return fail("Nao carregou vmm.dll ou uma dependencia");
#define API(field,name) field=reinterpret_cast<decltype(field)>(GetProcAddress(dll,name))
        auto init=reinterpret_cast<decltype(&VMMDLL_Initialize)>(GetProcAddress(dll,"VMMDLL_Initialize"));
        auto find=reinterpret_cast<decltype(&VMMDLL_PidGetFromName)>(GetProcAddress(dll,"VMMDLL_PidGetFromName"));
        auto modules=reinterpret_cast<decltype(&VMMDLL_Map_GetModuleU)>(GetProcAddress(dll,"VMMDLL_Map_GetModuleU"));
        auto release=reinterpret_cast<decltype(&VMMDLL_MemFree)>(GetProcAddress(dll,"VMMDLL_MemFree"));
        API(read,"VMMDLL_MemRead");API(readFresh,"VMMDLL_MemReadEx");API(close,"VMMDLL_Close");API(initScatter,"VMMDLL_Scatter_Initialize");
        API(prepare,"VMMDLL_Scatter_PrepareEx");API(execute,"VMMDLL_Scatter_ExecuteRead");API(closeScatter,"VMMDLL_Scatter_CloseHandle");
#undef API
        if(!init||!find||!modules||!release||!read||!readFresh||!close||!initScatter||!prepare||!execute||!closeScatter)return fail("API MemProcFS incompleta ou incompativel");
        const char* args[]={"","-device","fpga"};handle=init(3,args);
        if(!handle)return fail("Falha ao inicializar DMA FPGA; confira conexao e outros leitores");
        if(!find(handle,"WarChaos Begins.exe",&pid))return fail("WarChaos Begins.exe nao encontrado no PC do jogo");
        std::printf("DMA: processo encontrado; PID=%lu\n",pid);
        PVMMDLL_MAP_MODULE map=nullptr;if(!modules(handle,pid,&map,VMMDLL_MODULE_FLAG_NORMAL)||!map)return fail("Falha ao enumerar modulos do jogo");
        if(map->dwVersion==VMMDLL_MAP_MODULE_VERSION)for(DWORD i=0;i<map->cMap;++i){const auto& m=map->pMap[i];if(!m.uszText)continue;
            if(!_stricmp(m.uszText,"CryEntitySystem.dll"))entityBase=m.vaBase;
            if(!_stricmp(m.uszText,"CryAnimation.dll"))animationBase=m.vaBase;
            if(!_stricmp(m.uszText,"CryRenderD3D9.dll"))rendererBase=m.vaBase;
            if(!_stricmp(m.uszText,"CryGame.dll"))gameBase=m.vaBase;
            if(!_stricmp(m.uszText,"Cry3DEngine.dll"))engineBase=m.vaBase;
            if(!_stricmp(m.uszText,"CryAction.dll"))actionBase=m.vaBase;
            if(!_stricmp(m.uszText,"CryPhysics.dll"))physicsBase=m.vaBase;
        }release(map);
        if(!entityBase||!animationBase||!rendererBase||!gameBase||!engineBase||!actionBase)return fail("Modulos CryEngine ausentes; aguarde carregar o jogo");
        lastError.clear();std::puts("DMA: conexao, processo e modulos OK");return true;
    }
    bool environment(QWORD& system,QWORD& renderer){QWORD env=0;return pointer(entityBase+0x164F50,env)&&pointer(env+0xD8,system)&&pointer(env+0x48,renderer);}
    struct ActorBinding{QWORD object=0,node=0,table=0;};
    std::unordered_map<DWORD,ActorBinding> actors(){
        std::unordered_map<DWORD,ActorBinding> result;QWORD env=0,game=0,framework=0,system=0,table=0,method=0,header=0,root=0;
        if(!actionBase||!pointer(entityBase+0x164F50,env)||!pointer(env+0xC8,game)||!pointer(game+0x48,framework)||!pointer(framework+0x4E0,system)||
            !pointer(system,table)||table!=actionBase+0x4F0368||!bytes(table+0x78,&method,8)||method!=actionBase+0x59340||!pointer(system+0x30,header)||!pointer(header+8,root))return result;
        std::vector<QWORD> pending{root};std::unordered_set<QWORD> visited;
        for(size_t i=0;i<pending.size()&&i<256;++i){const QWORD node=pending[i];if(node==header||!visited.insert(node).second)continue;
            BYTE record[0x30]={};if(!bytes(node,record,sizeof(record)))return {};if(record[0x19])continue;
            QWORD left=0,right=0,actor=0,vt=0,health=0;DWORD id=0;std::memcpy(&left,record,8);std::memcpy(&right,record+0x10,8);std::memcpy(&id,record+0x20,4);std::memcpy(&actor,record+0x28,8);
            if(plausiblePointer(left))pending.push_back(left);if(plausiblePointer(right))pending.push_back(right);
            if(pointer(actor,vt)&&bytes(vt+0xD8,&health,8)&&health==gameBase+0x219A40)result[id]={actor,node,vt};
        }return result;
    }
    std::shared_ptr<const std::vector<collisionHead::Geometry>> collisionLayout(const HeadSample& head,DWORD id,bool& reused){
        reused=false;const auto found=collisionLayouts.find(id);
        if(found!=collisionLayouts.end()&&found->second.character==head.character&&found->second.body&&!found->second.body->empty()){
            const auto& list=*found->second.body;const auto& first=list.front();QWORD physical=0;std::array<BYTE,0x130> header{};
            if(pointer(head.character+0xB38,physical)&&physical==first.physical&&bytes(physical,header.data(),0x130)&&
                collisionHead::field<QWORD>(header.data(),0)==first.physicalTable&&collisionHead::field<QWORD>(header.data(),0x120)==first.parts&&
                collisionHead::field<int>(header.data(),0x128)==found->second.count&&found->second.count>0&&found->second.count<=128){
                std::vector<BYTE> records(static_cast<size_t>(found->second.count)*0x88);bool same=bytes(first.parts,records.data(),static_cast<DWORD>(records.size()));
                for(const auto& g:list){if(g.index>=static_cast<DWORD>(found->second.count)){same=false;break;}const BYTE* part=records.data()+g.index*0x88;
                    if(collisionHead::field<DWORD>(part,0x48)!=g.joint||collisionHead::field<QWORD>(part,0x38)!=g.descriptor){same=false;break;}}
                if(same){std::vector<QWORD> shapes(list.size()),tables(list.size());std::vector<DWORD> shapeBytes(list.size()),tableBytes(list.size());auto batch=initScatter(handle,pid,VMMDLL_FLAG_NOCACHE);
                    if(batch){for(size_t i=0;i<list.size();++i){prepare(batch,list[i].descriptor,8,reinterpret_cast<PBYTE>(&shapes[i]),&shapeBytes[i]);prepare(batch,list[i].shape,8,reinterpret_cast<PBYTE>(&tables[i]),&tableBytes[i]);}
                        same=execute(batch)!=FALSE;closeScatter(batch);for(size_t i=0;i<list.size();++i)if(shapeBytes[i]!=8||tableBytes[i]!=8||shapes[i]!=list[i].shape||tables[i]!=list[i].shapeTable)same=false;
                        if(same){reused=true;return found->second.body;}
                    }
                }
            }
        }
        auto list=collisionHead::discoverAll([&](QWORD address,void* out,DWORD size){return bytes(address,out,size);},head.character,head.joint,physicsBase);
        auto result=std::make_shared<const std::vector<collisionHead::Geometry>>(std::move(list));int count=0;
        if(!result->empty()&&bytes(result->front().physical+0x128,&count,4)&&count>0&&count<=128)collisionLayouts[id]={head.character,count,result};else collisionLayouts.erase(id);
        return result;
    }
    std::vector<Node> discover(QWORD system){
        const auto actorMap=actors();
        for(auto i=collisionLayouts.begin();i!=collisionLayouts.end();)if(actorMap.find(i->first)==actorMap.end())i=collisionLayouts.erase(i);else ++i;
        unsigned collisionReused=0,collisionNew=0;
        for(auto i=headLayouts.begin();i!=headLayouts.end();)if(actorMap.find(i->first)==actorMap.end())i=headLayouts.erase(i);else ++i;
        unsigned actorEntities=0,proxyRejected=0,bonesRejected=0,nameRejected=0;
        std::vector<Node> result;QWORD table=0;WORD last=0;
        if(!pointer(system+0xC0,table)||!bytes(system+0x40136,&last,2))return result;
        for(auto actor=actorMap.begin();actor!=actorMap.end()&&!stop;++actor){
            const DWORD expectedId=actor->first;const DWORD index=expectedId&0xFFFF;
            QWORD entity=0;std::array<BYTE,256> header{};
            if(index>last||!pointer(table+static_cast<QWORD>(index)*8,entity)||!bytes(entity,header.data(),256))continue;
            const auto* h=header.data();DWORD id=0;QWORD proxy=0,namePointer=0;
            std::memcpy(&id,h+0x18,4);if(id!=expectedId)continue;++actorEntities;
            std::memcpy(&proxy,h+0xB0,8);if(h[0xC]&0x40){int count=0;std::memcpy(&count,h+0xB8,4);if(count<1||!pointer(proxy,proxy))continue;}
            if(!plausiblePointer(proxy))continue;BYTE proxyHeader[0xA8]={};if(!bytes(proxy,proxyHeader,sizeof(proxyHeader)))continue;
            QWORD owner=0,vt=0;std::memcpy(&owner,proxyHeader+0x50,8);std::memcpy(&vt,proxyHeader,8);
            if(owner!=entity||vt!=entityBase+0x110580){++proxyRejected;continue;}
            const auto layout=headLayouts.find(id);
            const auto head=discoverHead(read,handle,pid,proxyHeader,id,readFresh,layout==headLayouts.end()?nullptr:&layout->second);if(!head.address||!head.leftFoot||!head.rightFoot){++bonesRejected;continue;}
            QWORD characterTable=0;if(!bytes(head.character,&characterTable,8)||characterTable!=animationBase+0x27AEF8)continue;
            headLayouts.insert_or_assign(id,head);
            std::memcpy(&namePointer,h+0xE8,8);char name[97]={};if(!namePointer||!bytes(namePointer,name,96)){++nameRejected;continue;}
            for(char& c:name)if(static_cast<unsigned char>(c)<32&&c)c='?';
            // Known local nickname used for team lookup.
            if(!_stricmp(name,"Drevok")){localId=id;continue;}
            bool duplicate=false;for(const auto& n:result)if(n.head.character==head.character)duplicate=true;
            if(!duplicate){QWORD temp=0;pointer(proxy+0x40,temp);result.push_back({entity,proxy,id,head,name,temp,actor->second.object,actor->second.node,actor->second.table});bool reused=false;result.back().body=collisionLayout(head,id,reused);if(reused)++collisionReused;else ++collisionNew;for(const auto& g:*result.back().body)if(g.joint==head.joint)result.back().collision=g;}
        }
        std::printf("DESCOBERTA: atores=%zu; entidades_ator=%u; candidatos=%zu; proxy_rejeitado=%u; ossos_ausentes=%u; nome_falhou=%u\n",actorMap.size(),actorEntities,result.size(),proxyRejected,bonesRejected,nameRejected);
        std::printf("GEOMETRIA CACHE: reutilizada=%u; descoberta=%u\n",collisionReused,collisionNew);
        return result;
    }
    struct CameraBatch {
        struct Lane{RendererProjection value;DWORD firstBytes=0,secondBytes=0;QWORD firstStack=0,secondStack=0,first=0,second=0,checkedFirst=0,checkedSecond=0;DWORD firstPointerBytes=0,secondPointerBytes=0;};
        std::array<Lane,2> lanes{};DWORD order[2]={},sampledOrder[2]={},orderBytes=0,viewportBytes=0;std::array<int,4> viewport{};
    };
    bool prepareCamera(QWORD renderer,VMMDLL_SCATTER_HANDLE batch,CameraBatch& output){
        QWORD vt=0,thread=0;if(!bytes(renderer,&vt,8)||vt!=rendererBase+0x3D6648||!pointer(renderer+0x9360,thread))return false;
        if(!bytes(thread+0x28,output.order,8))return false;
        prepare(batch,thread+0x28,8,reinterpret_cast<PBYTE>(output.sampledOrder),&output.orderBytes);
        prepare(batch,renderer+0x98C0,16,reinterpret_cast<PBYTE>(output.viewport.data()),&output.viewportBytes);
        bool queued=false;
        for(unsigned index=0;index<2;++index){auto& item=output.lanes[index];item.value.lane=index;
            if(!pointer(renderer+index*0x318+0xE98,item.firstStack)||!pointer(item.firstStack,item.first)||!pointer(renderer+index*0x318+0xEA0,item.secondStack)||!pointer(item.secondStack,item.second))continue;
            queued=prepare(batch,item.first,64,reinterpret_cast<PBYTE>(item.value.first.data()),&item.firstBytes)!=FALSE||queued;
            prepare(batch,item.second,64,reinterpret_cast<PBYTE>(item.value.second.data()),&item.secondBytes);
            prepare(batch,item.firstStack,8,reinterpret_cast<PBYTE>(&item.checkedFirst),&item.firstPointerBytes);
            prepare(batch,item.secondStack,8,reinterpret_cast<PBYTE>(&item.checkedSecond),&item.secondPointerBytes);
        }return queued;
    }
    bool selectCamera(const CameraBatch& input,RendererProjection& output){
        if(input.orderBytes!=8||input.viewportBytes!=16||input.viewport[2]<=0||input.viewport[3]<=0)return false;
        // Prefer the render-thread path (+0x2C), then the alternate perspective lane.
        for(DWORD lane:{input.sampledOrder[1],input.sampledOrder[0]}){if(lane>1)continue;const auto& p=input.lanes[lane];
            if(p.firstBytes!=64||p.secondBytes!=64||p.firstPointerBytes!=8||p.secondPointerBytes!=8||p.checkedFirst!=p.first||p.checkedSecond!=p.second)continue;
            bool finite=true;for(float f:p.value.first)finite=finite&&std::isfinite(f);for(float f:p.value.second)finite=finite&&std::isfinite(f);
            if(!finite||std::fabs(p.value.second[15])>0.001f||std::fabs(p.value.second[11])<0.1f)continue;
            output=p.value;output.viewport=input.viewport;return true;
        }return false;
    }
    std::unordered_map<DWORD,DWORD> teams(){
        std::unordered_map<DWORD,DWORD> result;QWORD env=0,game=0,framework=0,rules=0,vt=0,method=0,header=0,root=0;
        if(!gameBase||!pointer(entityBase+0x164F50,env)||!pointer(env+0xC8,game)||!pointer(game+0x48,framework)||!pointer(framework+0x790,rules)||
            !bytes(rules,&vt,8)||vt!=gameBase+0xBCDF98||!bytes(vt+0x368,&method,8)||method!=gameBase+0x595810||!pointer(rules+0x110,header)||!pointer(header+8,root))return result;
        std::vector<QWORD> pending{root};std::unordered_set<QWORD> visited;
        for(unsigned level=0;level<32&&!pending.empty();++level){
            struct TeamNode{std::array<BYTE,0x28> data{};DWORD count=0;};std::vector<TeamNode> records(pending.size());
            auto batch=initScatter(handle,pid,VMMDLL_FLAG_NOCACHE);if(!batch)return {};
            for(size_t i=0;i<pending.size();++i)prepare(batch,pending[i],0x28,records[i].data.data(),&records[i].count);
            const bool ok=execute(batch)!=FALSE;closeScatter(batch);if(!ok)return {};
            std::vector<QWORD> next;
            for(size_t i=0;i<pending.size();++i){if(records[i].count!=0x28)return {};const auto* p=records[i].data.data();if(p[0x19])continue;
                DWORD id=0,team=0;QWORD left=0,right=0;std::memcpy(&id,p+0x1C,4);std::memcpy(&team,p+0x20,4);result[id]=team;
                std::memcpy(&left,p,8);std::memcpy(&right,p+0x10,8);
                for(QWORD child:{left,right})if(plausiblePointer(child)&&child!=header&&visited.insert(child).second){if(visited.size()>256)return {};next.push_back(child);}
            }pending=std::move(next);
        }if(!pending.empty())return {};return result;
    }
    State sample(QWORD renderer,const std::vector<Node>& nodes,bool retryPose=true){
        const auto started=std::chrono::steady_clock::now();
        State out;out.status="Equipe e vida desconhecidas | F8 boxes | Esc sair";
        const bool ffa=ffaMode.load();const DWORD local=localId.load();
        const auto teamMap=ffa?std::unordered_map<DWORD,DWORD>{}:teams();const auto localTeam=teamMap.find(local);
        const DWORD localTeamId=localTeam==teamMap.end()?0:localTeam->second;
        if(!local||(!ffa&&!localTeamId)){out.time=0;out.status=!local?"Jogador local desconhecido; alvos ocultos":"Equipe local desconhecida; confira Vale Tudo no painel";return out;}
        auto isEnemy=[&](DWORD id){const auto team=teamMap.find(id);return enemyInMode(ffa,local,id,localTeamId,team==teamMap.end()?0:team->second);};
        struct Data{collisionHead::Sample collision;std::vector<collisionHead::Sample> body;std::array<BYTE,256> entity{};std::array<BYTE,0x68> actor{};std::array<BYTE,0x30> actorNode{};DWORD actorBytes=0,actorNodeBytes=0;std::array<float,7> head{},left{},right{};std::array<std::array<float,7>,17> bones{};std::array<DWORD,17> boneBytes{};QWORD poseArray=0,currentPose=0,temp=0,checkedTemp=0,renderTable=0;DWORD currentPoseBytes=0,visibilityFrames[2]={},tempBytes=0,checkedTempBytes=0,frameBytes=0,tableBytes=0;DWORD a=0,c=0,d=0,e=0,f=0;};
        std::vector<Data> data(nodes.size());
        auto pointerBatch=initScatter(handle,pid,VMMDLL_FLAG_NOCACHE);
        if(pointerBatch){for(size_t i=0;i<nodes.size();++i){if(isEnemy(nodes[i].id)){prepare(pointerBatch,nodes[i].proxy+0x40,8,reinterpret_cast<PBYTE>(&data[i].temp),&data[i].tempBytes);prepare(pointerBatch,nodes[i].head.character+0xB78,8,reinterpret_cast<PBYTE>(&data[i].currentPose),&data[i].currentPoseBytes);}}
            if(!execute(pointerBatch))for(auto& record:data){record.tempBytes=0;record.currentPoseBytes=0;}closeScatter(pointerBatch);}
        auto batch=initScatter(handle,pid,VMMDLL_FLAG_NOCACHE);if(!batch){out.status="Falha ao preparar lote";out.transient=true;return out;}
        DWORD engineFrame=0,engineFrameBytes=0;if(engineBase)prepare(batch,engineBase+0x3776D8,4,reinterpret_cast<PBYTE>(&engineFrame),&engineFrameBytes);
        for(size_t i=0;i<nodes.size();++i){const auto& n=nodes[i];if(!isEnemy(n.id))continue;
            auto& d=data[i];if(d.currentPoseBytes!=8||!plausiblePointer(d.currentPose))continue;
            auto poseAddress=[&](QWORD reference){return d.currentPose+(reference-n.head.array);};
            prepare(batch,n.entity,256,d.entity.data(),&d.a);prepare(batch,poseAddress(n.head.address),28,reinterpret_cast<PBYTE>(d.head.data()),&d.c);prepare(batch,poseAddress(n.head.leftFoot),28,reinterpret_cast<PBYTE>(d.left.data()),&d.e);prepare(batch,poseAddress(n.head.rightFoot),28,reinterpret_cast<PBYTE>(d.right.data()),&d.f);prepare(batch,n.head.character+0xB78,8,reinterpret_cast<PBYTE>(&d.poseArray),&d.d);
            prepare(batch,n.actor,0x68,d.actor.data(),&d.actorBytes);prepare(batch,n.actorNode,0x30,d.actorNode.data(),&d.actorNodeBytes);
            if(!n.collision.points.empty()){
                prepare(batch,n.collision.physical,0x130,d.collision.physical.data(),&d.collision.physicalBytes);
                prepare(batch,n.collision.parts+n.collision.index*0x88,0x88,d.collision.part.data(),&d.collision.partBytes);
                prepare(batch,n.head.character+0xB38,8,reinterpret_cast<PBYTE>(&d.collision.checkedPhysical),&d.collision.pointerBytes);
                prepare(batch,n.collision.descriptor,8,reinterpret_cast<PBYTE>(&d.collision.checkedShape),&d.collision.shapeBytes);
                if(n.body){d.body.resize(n.body->size());for(size_t j=0;j<n.body->size();++j){const auto& g=(*n.body)[j];auto& sample=d.body[j];
                    prepare(batch,g.parts+g.index*0x88,0x88,sample.part.data(),&sample.partBytes);
                    prepare(batch,g.descriptor,8,reinterpret_cast<PBYTE>(&sample.checkedShape),&sample.shapeBytes);
                }}
            }
            if(d.tempBytes==8&&plausiblePointer(d.temp)){prepare(batch,n.proxy+0x40,8,reinterpret_cast<PBYTE>(&d.checkedTemp),&d.checkedTempBytes);prepare(batch,n.proxy+8,8,reinterpret_cast<PBYTE>(&d.renderTable),&d.tableBytes);prepare(batch,d.temp+0x60,8,reinterpret_cast<PBYTE>(d.visibilityFrames),&d.frameBytes);}
            for(unsigned bone=0;bone<17;++bone)if(n.head.skeleton[bone])prepare(batch,poseAddress(n.head.skeleton[bone]),28,reinterpret_cast<PBYTE>(d.bones[bone].data()),&d.boneBytes[bone]);}
        CameraBatch cameraData{};const bool cameraQueued=prepareCamera(renderer,batch,cameraData);
        const ULONGLONG sampleStarted=GetTickCount64();const bool ok=execute(batch)!=FALSE;closeScatter(batch);out.time=sampleStarted;
        DWORD engineFrameAfter=0;const bool frameAfterOk=engineBase&&bytes(engineBase+0x3776D8,&engineFrameAfter,4);
        RendererProjection camera{};const bool matrixOk=cameraQueued&&selectCamera(cameraData,camera);
        if(!matrixOk||!ok){out.time=0;out.status="Leitura ou matriz invalida; marcadores ocultos";out.transient=true;return out;}
        const auto centerRay=collisionRay::fromCamera(camera);
        for(size_t i=0;i<nodes.size();++i){const auto& n=nodes[i];const auto& d=data[i];if(!isEnemy(n.id))continue;
            if(d.a!=256||d.c!=28||d.d!=8||d.e!=28||d.f!=28){++out.partial;continue;}
            if(d.poseArray!=d.currentPose){++out.poseChanged;continue;}
            DWORD id=0;std::memcpy(&id,d.entity.data()+0x18,4);if(id!=n.id)continue;
            QWORD actorPointer=0,actorTable=0;DWORD actorId=0,encoded=0,key=0;
            std::memcpy(&actorPointer,d.actorNode.data()+0x28,8);std::memcpy(&actorId,d.actorNode.data()+0x20,4);std::memcpy(&actorTable,d.actor.data(),8);
            if(d.actorBytes!=0x68||d.actorNodeBytes!=0x30||d.actorNode[0x19]||actorPointer!=n.actor||actorId!=n.id||actorTable!=n.actorTable){++out.unknownLife;continue;}
            std::memcpy(&encoded,d.actor.data()+0x60,4);std::memcpy(&key,d.actor.data()+0x64,4);const auto life=decodeLife(encoded,key);
            if(life==LifeState::Dead){++out.dead;continue;}if(life!=LifeState::Alive){++out.unknownLife;continue;}
            float m[12]={};std::memcpy(m,d.entity.data()+0x40,sizeof(m));
            const auto radarPoint=radar::locate(camera.first,m[3],m[7]);if(radarPoint.valid)out.radarPoints.push_back(radarPoint);
            auto project=[&](float x,float y,float z){return projection::project(camera.first,camera.second,{m[0]*x+m[1]*y+m[2]*z+m[3],m[4]*x+m[5]*y+m[6]*z+m[7],m[8]*x+m[9]*y+m[10]*z+m[11],1},camera.viewport,1920,1080);};
            DWORD healthBits=encoded^key;float currentHealth=0;std::memcpy(&currentHealth,&healthBits,4);
            Marker marker;marker.health=static_cast<int>(std::ceil(currentHealth));marker.id=n.id;marker.name=n.name;marker.head=project(d.head[4],d.head[5],d.head[6]);if(!marker.head.valid)continue;
            if(collisionHead::valid(n.collision,d.collision)){
                collisionHead::V center{};if(collisionHead::transform(d.collision,n.collision.center,center)){
                    const collisionHead::V boneWorld{m[0]*d.head[4]+m[1]*d.head[5]+m[2]*d.head[6]+m[3],m[4]*d.head[4]+m[5]*d.head[5]+m[6]*d.head[6]+m[7],m[8]*d.head[4]+m[9]*d.head[5]+m[10]*d.head[6]+m[11]};
                    const float distance=std::sqrt((center[0]-boneWorld[0])*(center[0]-boneWorld[0])+(center[1]-boneWorld[1])*(center[1]-boneWorld[1])+(center[2]-boneWorld[2])*(center[2]-boneWorld[2]));
                    if(distance<.6f){marker.aimHead=projection::project(camera.first,camera.second,{center[0],center[1],center[2],1},camera.viewport,1920,1080);std::vector<projection::Result> projected;bool complete=true;
                        for(const auto& shapePoint:n.collision.points){collisionHead::V world{};if(!collisionHead::transform(d.collision,shapePoint,world)){complete=false;break;}const auto point=projection::project(camera.first,camera.second,{world[0],world[1],world[2],1},camera.viewport,1920,1080);if(!point.valid){complete=false;break;}projected.push_back(point);}
                        if(complete)marker.collisionOutline=collisionHead::hull(std::move(projected));
                        if(!marker.collisionOutline.empty())++out.realHeads;
                    }
                }
            }
            // Parts and camera are sampled in the same scatter. Geometry vertices are cached per discovery.
            if(n.body&&n.body->size()==d.body.size()&&collisionHead::valid(n.collision,d.collision)){
                for(size_t j=0;j<n.body->size();++j){const auto& g=(*n.body)[j];if(g.physical!=n.collision.physical||g.parts!=n.collision.parts)continue;
                    auto sample=d.body[j];sample.physical=d.collision.physical;sample.physicalBytes=d.collision.physicalBytes;sample.checkedPhysical=d.collision.checkedPhysical;sample.pointerBytes=d.collision.pointerBytes;
                    if(collisionHead::valid(g,sample))++out.validBodyParts;
                    marker.bodyDistance=(std::min)(marker.bodyDistance,collisionRay::intersect(g,sample,centerRay));
                }
            }
            Visibility rawVisibility=Visibility::Unknown;
            if(engineFrameBytes==4&&frameAfterOk&&d.tempBytes==8&&d.checkedTempBytes==8&&d.tableBytes==8&&d.frameBytes==8&&d.temp==d.checkedTemp&&d.renderTable==entityBase+0x110780)
                rawVisibility=classifySampledOcclusion(engineFrame,engineFrameAfter,d.visibilityFrames[0],d.visibilityFrames[1]);
            if(visibilityRecords.size()>4096)visibilityRecords.clear();
            auto& history=visibilityRecords[n.entity];
            if(history.id!=n.id||history.character!=n.head.character||history.temp!=d.temp){history={};history.id=n.id;history.character=n.head.character;history.temp=d.temp;}
            marker.visibility=rawVisibility;marker.displayVisibility=history.display.update(marker.visibility,out.time);
            for(unsigned bone=0;bone<17;++bone)if(d.boneBytes[bone]==28)marker.bones[bone]=project(d.bones[bone][4],d.bones[bone][5],d.bones[bone][6]);
            const auto leftFoot=project(d.left[4],d.left[5],d.left[6]),rightFoot=project(d.right[4],d.right[5],d.right[6]);
            if(!leftFoot.valid||!rightFoot.valid)continue;
            const float feetY=(std::max)(leftFoot.y,rightFoot.y),height=feetY-marker.head.y;
            if(height<3||height>4000)continue;
            // Conventional 2D box: bone-derived height, estimated width and head margin.
            const float center=(marker.head.x+(leftFoot.x+rightFoot.x)*0.5f)*0.5f;
            const float halfWidth=(std::max)(height*0.23f,std::fabs(leftFoot.x-rightFoot.x)*0.5f+height*0.05f);
            marker.left=center-halfWidth;marker.right=center+halfWidth;marker.top=marker.head.y-height*0.08f;marker.bottom=feetY+height*0.02f;
            marker.box=marker.right>=0&&marker.left<1920&&marker.bottom>=0&&marker.top<1080;
            if(marker.box)out.markers.push_back(marker);
        }
        // Only the nearest intersected enemy can receive a trigger decision.
        Marker* nearest=nullptr;for(auto& marker:out.markers)if(std::isfinite(marker.bodyDistance)&&(!nearest||marker.bodyDistance<nearest->bodyDistance))nearest=&marker;
        if(nearest)nearest->bodyHit=true;
        if(out.poseChanged&&retryPose)return sample(renderer,nodes,false);
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        char status[180]={};std::snprintf(status,sizeof(status),"DMA %.1f ms | %zu inimigos | %u cabecas reais | %s | F6 esqueleto F7 nomes F8 boxes F9 status",ms,out.markers.size(),out.realHeads,ffa?"VALE TUDO":"EQUIPES");out.status=status;return out;
    }
};

struct TimerResolution {bool active=timeBeginPeriod(1)==TIMERR_NOERROR;~TimerResolution(){if(active)timeEndPeriod(1);}};

static void worker(){
    TimerResolution timerResolution;
    Reader reader;
    while(!stop&&!reader.open()){
        {std::lock_guard<std::mutex> guard(lock);state={};state.status=reader.lastError+" | nova tentativa em 2s";}
        notifyFrame();for(unsigned i=0;i<20&&!stop;++i)Sleep(100);
    }
    if(stop)return;
    std::mutex registryLock;std::vector<Node> registry;
    std::thread discovery([&]{CandidateRegistry<Node> candidates;while(!stop){
        QWORD system=0,renderer=0;
        if(reader.environment(system,renderer)){
            const auto next=reader.discover(system);candidates.update(system,next,GetTickCount64());
            auto merged=candidates.snapshot();std::lock_guard<std::mutex> guard(registryLock);registry=std::move(merged);
        }else{candidates.clear();reader.localId=0;std::lock_guard<std::mutex> guard(registryLock);registry.clear();}
        for(unsigned i=0;i<10&&!stop;++i)Sleep(100);
    }});
    ULONGLONG lastLog=GetTickCount64();unsigned good=0,transientDrops=0,held=0,teamDrops=0,partial=0,poseChanged=0,dead=0,unknownLife=0;size_t peakNodes=0;std::vector<double> sampleTimes;sampleTimes.reserve(2048);
    while(!stop){QWORD system=0,renderer=0;if(!reader.environment(system,renderer)){{std::lock_guard<std::mutex> guard(lock);state={};state.status="Jogo indisponivel";}notifyFrame();Sleep(250);continue;}
        std::vector<Node> nodes;{std::lock_guard<std::mutex> guard(registryLock);nodes=registry;}
        if(stop)break;const auto sampleStart=std::chrono::steady_clock::now();auto next=reader.sample(renderer,nodes);const ULONGLONG now=GetTickCount64();
        sampleTimes.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-sampleStart).count());
        peakNodes=(std::max)(peakNodes,nodes.size());if(next.time)++good;else if(next.transient)++transientDrops;else ++teamDrops;
        partial+=next.partial;poseChanged+=next.poseChanged;dead+=next.dead;unknownLife+=next.unknownLife;
        {std::lock_guard<std::mutex> guard(lock);if(!next.time&&canReuseFrame(next.transient,state.time,now)){++held;}else state=std::move(next);}
        if(now-lastLog>=5000){
            if(!sampleTimes.empty()){double total=0;for(double t:sampleTimes)total+=t;std::sort(sampleTimes.begin(),sampleTimes.end());std::printf("TEMPOS: media_ms=%.2f; p95_ms=%.2f; max_ms=%.2f\n",total/sampleTimes.size(),sampleTimes[(sampleTimes.size()-1)*95/100],sampleTimes.back());sampleTimes.clear();}
            std::printf("QUADROS: validos=%u; falha_camera_leitura=%u; preservados_ate_60ms=%u; sem_equipe=%u; candidatos=%zu; parciais=%u; troca_pose=%u; mortos_ocultos=%u; vida_desconhecida=%u\n",good,transientDrops,held,teamDrops,peakNodes,partial,poseChanged,dead,unknownLife);good=transientDrops=held=teamDrops=partial=poseChanged=dead=unknownLife=0;peakNodes=0;lastLog=now;}
        notifyFrame();Sleep(1);
    }
    discovery.join();
}

static void ferrumWorker(){
    TimerResolution timerResolution;
    try{
        char executable[MAX_PATH]={};if(!GetModuleFileNameA(nullptr,executable,MAX_PATH))throw std::runtime_error("Caminho do executavel indisponivel");
        std::string path=executable;path=path.substr(0,path.find_last_of("\\/")+1)+"Ferrum.ini";
        if(GetFileAttributesA(path.c_str())==INVALID_FILE_ATTRIBUTES)throw std::runtime_error("Ferrum.ini ausente");
        auto number=[&](const char* key,double fallback,double min,double max){char value[64]={};const auto text=std::to_string(fallback);GetPrivateProfileStringA("Ferrum",key,text.c_str(),value,sizeof(value),path.c_str());char* end=nullptr;const double n=std::strtod(value,&end);if(end==value||*end||!std::isfinite(n)||n<min||n>max)throw std::runtime_error(std::string("Valor invalido: ")+key);return n;};
        aim::Config cfg;cfg.fov=number("fov_px",80,5,400);cfg.gain=number("gain",.2,.001,2);cfg.maxStep=number("max_step",12,1,100);cfg.deadzone=number("deadzone_px",1,0,10);
        cfg.triggerRadius=number("trigger_radius_px",4,1,12);cfg.dwell=static_cast<ULONGLONG>(number("trigger_delay_ms",40,0,500));cfg.cooldown=static_cast<ULONGLONG>(number("trigger_interval_ms",160,50,2000));
        char port[32]={};GetPrivateProfileStringA("Ferrum","port","COM4",port,sizeof(port),path.c_str());const DWORD baud=static_cast<DWORD>(number("baud",115200,9600,4000000));
        Ferrum ferrum;ferrum.open(port,baud);aim::Policy policy,triggerPolicy;activation::Bindings previousBindings;bool previousRequireVisibility=true;ferrumReady=true;
        ULONGLONG triggerLog=GetTickCount64();unsigned aimHeldCount=0,aimTargets=0,aimMoves=0,aimGrace=0,aimStale=0;
        unsigned triggerHeldCount=0,triggerTargets=0,triggerFired=0,triggerHits=0,triggerVisible=0,triggerStale=0;
        std::puts("VERSAO: 39; mira conserva somente alvo confirmado por ate 80ms de Unknown; Occluded interrompe imediatamente.");
        std::puts("FERRUM: conectado; F10 mira, F11 trigger; ambos desligados; botoes independentes no painel.");
        while(!stop){
            const bool aimOn=aimEnabled.load(),triggerOn=triggerEnabled.load();
            if(menuOpen||(!aimOn&&!triggerOn)){aimReason=menuOpen?6:0;triggerReason=menuOpen?8:0;selectedTarget=0;policy.reset();triggerPolicy.reset();Sleep(10);continue;}
            cfg=readLiveConfig();
            const auto bindings=readBindings();if(bindings!=previousBindings){policy.reset();triggerPolicy.reset();previousBindings=bindings;}
            auto pressed=[&](unsigned button){const auto result=ferrum.query(activation::query(button));if(result!="0"&&result!="1")throw std::runtime_error("Estado de botao Ferrum invalido");return result=="1";};
            const bool aimHeld=aimOn&&pressed(bindings.aim);
            const bool triggerHeld=triggerOn?(aimOn&&bindings.trigger==bindings.aim?aimHeld:pressed(bindings.trigger)):false;
            aim::Frame frame;
            {std::lock_guard<std::mutex> guard(lock);frame.time=state.time;for(const auto& marker:state.markers){
                const auto& aimPoint=marker.aimHead.valid?marker.aimHead:marker.head;
                if(aimPoint.valid)frame.targets.push_back({marker.id,aimPoint.x,aimPoint.y,std::clamp((marker.bottom-marker.top)*.045,1.,12.),marker.visibility==Visibility::Visible,true,marker.bodyHit,marker.visibility==Visibility::Occluded});
            }}
            const bool requireVisibility=triggerRequireVisibility.load();if(requireVisibility!=previousRequireVisibility){triggerPolicy.reset();previousRequireVisibility=requireVisibility;}
            const auto triggerFrame=aim::triggerFrame(frame,requireVisibility);
            if(triggerHeld){const auto now=GetTickCount64();if(!frame.time||now<frame.time||now-frame.time>cfg.maxAge)++triggerStale;
                for(const auto& target:frame.targets)if(target.bodyInside){++triggerHits;if(target.visible)++triggerVisible;}
            }
            unsigned reason=triggerOn?1:0;
            if(triggerOn&&triggerHeld){const auto now=GetTickCount64();reason=3;
                if(!frame.time||now<frame.time||now-frame.time>cfg.maxAge)reason=2;
                else for(const auto& target:frame.targets)if(target.bodyInside){reason=(target.visible||!requireVisibility)?5:4;break;}
            }
            triggerReason=reason;
            auto action=policy.tick(cfg,frame,GetTickCount64(),aimHeld,aimOn,false);
            if(aimHeld){++aimHeldCount;const auto now=GetTickCount64();const bool stale=!frame.time||now<frame.time||now-frame.time>cfg.maxAge;
                if(stale){++aimStale;aimReason=1;}else if(action.target){++aimTargets;if(action.visibilityGrace){++aimGrace;aimReason=4;}else aimReason=(action.dx||action.dy)?3:5;}else aimReason=2;
            }else aimReason=0;
            const auto fireAction=triggerPolicy.tick(cfg,triggerFrame,GetTickCount64(),triggerHeld,false,triggerOn);if(triggerHeld)++triggerHeldCount;if(fireAction.target)++triggerTargets;selectedTarget=action.target?action.target:fireAction.target;
            if(stop||menuOpen||bindings!=readBindings()||aimOn!=aimEnabled.load()||triggerOn!=triggerEnabled.load()||requireVisibility!=triggerRequireVisibility.load())continue;
            // Never move then fire against a projection collected before that movement.
            if(aimEnabled&&(action.dx||action.dy)){if(triggerOn&&triggerHeld)triggerReason=6;ferrum.move(action.dx,action.dy);++aimMoves;}
            else if(triggerEnabled&&fireAction.fire){triggerReason=7;ferrum.press();Sleep(12);ferrum.release();++triggerFired;}
            if(GetTickCount64()-triggerLog>=5000){std::printf("AIM: botao_ativo=%u; alvo=%u; movimentos=%u; lacuna_breve=%u; leitura_atrasada=%u\n",aimHeldCount,aimTargets,aimMoves,aimGrace,aimStale);aimHeldCount=aimTargets=aimMoves=aimGrace=aimStale=0;std::printf("TRIGGER: botao_ativo=%u; colisao=%u; colisao_visivel=%u; leitura_atrasada=%u; alvo_corpo=%u; cliques=%u\n",triggerHeldCount,triggerHits,triggerVisible,triggerStale,triggerTargets,triggerFired);triggerHeldCount=triggerTargets=triggerFired=triggerHits=triggerVisible=triggerStale=0;triggerLog=GetTickCount64();}
            Sleep(2);
        }
    }catch(const std::exception& e){std::printf("FERRUM ERRO: %s; mira e trigger desligados.\n",e.what());}
    ferrumReady=false;aimEnabled=false;triggerEnabled=false;selectedTarget=0;
}

static LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    static BackBuffer back;
    if(message==WM_HOTKEY&&w==19){togglePanel();return 0;}
    if(message==WM_APP+1){paintQueued=false;InvalidateRect(window,nullptr,FALSE);UpdateWindow(window);return 0;}
    if(message==WM_TIMER){ULONGLONG timestamp=0;{std::lock_guard<std::mutex> guard(lock);timestamp=state.time;}if(timestamp&&GetTickCount64()-timestamp>=90)InvalidateRect(window,nullptr,FALSE);return 0;}
    if(message==WM_KEYDOWN){if(w==VK_F10||w==VK_F11){if(!(l&(1LL<<30))){if(ferrumReady){auto& option=w==VK_F10?aimEnabled:triggerEnabled;option=!option.load();std::printf("FERRUM: %s %s\n",w==VK_F10?"mira":"trigger",option.load()?"ON":"OFF");}else std::puts("FERRUM: nao conectado; use Iniciar-ESP-Ferrum.cmd e consulte o log");}return 0;}if(w==VK_ESCAPE)DestroyWindow(window);if(w==VK_F8)showBox=!showBox;if(w==VK_F9)showStatus=!showStatus;if(w==VK_F7)showNames=!showNames;if(w==VK_F6)showSkeleton=!showSkeleton;if(w==VK_F5)showHead=!showHead;if(w==VK_F4)showRadar=!showRadar;return 0;}
    if(message==WM_DESTROY){UnregisterHotKey(window,19);if(settingsWindow)DestroyWindow(settingsWindow);KillTimer(window,1);displayWindow=nullptr;back.clear();stop=true;PostQuitMessage(0);return 0;}
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){PAINTSTRUCT paint;HDC screen=BeginPaint(window,&paint);RECT rect{};GetClientRect(window,&rect);const int width=rect.right,height=rect.bottom;
        if(width>0&&height>0&&back.resize(screen,width,height)){HDC dc=back.dc;back.syncStyle();FillRect(dc,&rect,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            const float scale=(std::min)(width/1920.f,height/1080.f);const int ox=static_cast<int>((width-1920*scale)/2),oy=static_cast<int>((height-1080*scale)/2);
            State snapshot;{std::lock_guard<std::mutex> guard(lock);snapshot=state;}SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,220,70));
            auto pen=CreatePen(PS_SOLID,2,RGB(255,220,70));auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
            HGDIOBJ oldFont=back.nameFont?SelectObject(dc,back.nameFont):nullptr;
            if(showFov){const int radius=static_cast<int>(readLiveConfig().fov*scale);const int cx=ox+static_cast<int>(960*scale),cy=oy+static_cast<int>(540*scale);HGDIOBJ prev=SelectObject(dc,back.fovPen?back.fovPen:pen);Ellipse(dc,cx-radius,cy-radius,cx+radius+1,cy+radius+1);SelectObject(dc,prev);}
            if(snapshot.time&&GetTickCount64()-snapshot.time<90)for(const auto& m:snapshot.markers){auto sx=[&](float x){return ox+static_cast<int>((std::max)(-100.f,(std::min)(2020.f,x))*scale);};auto sy=[&](float y){return oy+static_cast<int>((std::max)(-100.f,(std::min)(1180.f,y))*scale);};
                const auto& aimPoint=m.aimHead.valid?m.aimHead:m.head;
                if(m.id==selectedTarget.load()&&aimPoint.inside){const int x=sx(aimPoint.x),y=sy(aimPoint.y);Ellipse(dc,x-7,y-7,x+8,y+8);}
                if(showBox&&m.box){HGDIOBJ previous=back.boxPen?SelectObject(dc,back.boxPen):nullptr;Rectangle(dc,sx(m.left),sy(m.top),sx(m.right),sy(m.bottom));if(previous)SelectObject(dc,previous);}
                if(showHealth){char hp[32]={};const int length=std::snprintf(hp,sizeof(hp),"%d HP",m.health);const COLORREF oldColor=SetTextColor(dc,appearance.colors[1]);
                    TextOutA(dc,(std::max)(0,sx(m.left)),(std::max)(0,(std::min)(height-16,sy(m.bottom)+3)),hp,length);SetTextColor(dc,oldColor);}
                if(showHead&&m.head.inside){
                    const int radius=(std::max)(3,static_cast<int>((std::min)(18.f,(m.bottom-m.top)*0.045f)*scale));
                    const int x=sx(m.head.x),y=sy(m.head.y);
                    HPEN headColor=m.displayVisibility==Visibility::Occluded?back.occludedPen:m.displayVisibility==Visibility::Visible?back.headPen:back.unknownPen;HGDIOBJ headOld=SelectObject(dc,headColor?headColor:pen);
                    if(!m.collisionOutline.empty()){std::vector<POINT> outline;for(const auto& point:m.collisionOutline)outline.push_back({sx(point.x),sy(point.y)});outline.push_back(outline.front());Polyline(dc,outline.data(),static_cast<int>(outline.size()));}
                    else Ellipse(dc,x-radius,y-radius,x+radius+1,y+radius+1);SelectObject(dc,headOld);
                }
                if(showSkeleton){static constexpr unsigned links[][2]={{0,1},{1,2},{2,3},{3,4},{1,5},{5,6},{6,7},{1,8},{8,9},{9,10},{4,11},{11,12},{12,13},{4,14},{14,15},{15,16}};
                    HPEN skeletonPen=m.displayVisibility==Visibility::Visible?back.visiblePen:m.displayVisibility==Visibility::Occluded?back.occludedPen:back.unknownPen;
                    HGDIOBJ previousPen=skeletonPen?SelectObject(dc,skeletonPen):nullptr;
                    for(const auto& link:links){const auto& a=m.bones[link[0]];const auto& b=m.bones[link[1]];if(a.valid&&b.valid){MoveToEx(dc,sx(a.x),sy(a.y),nullptr);LineTo(dc,sx(b.x),sy(b.y));}}
                    if(previousPen)SelectObject(dc,previousPen);}
                if(showNames){
                    const int length=static_cast<int>(m.name.size());SIZE textSize{};
                    if(GetTextExtentPoint32A(dc,m.name.c_str(),length,&textSize)){
                        const int x=(std::max)(0,(std::min)(width-static_cast<int>(textSize.cx),sx((m.left+m.right)*0.5f)-static_cast<int>(textSize.cx)/2));
                        const int y=(std::max)(0,sy(m.top)-static_cast<int>(textSize.cy)-4);
                        const COLORREF previous=SetTextColor(dc,appearance.colors[1]);
                        TextOutA(dc,x,y,m.name.c_str(),length);
                        SetTextColor(dc,previous);
                    }
                }}
            if(showRadar&&snapshot.time&&GetTickCount64()-snapshot.time<90){
                auto px=[&](float x){return ox+static_cast<int>(x*scale);};auto py=[&](float y){return oy+static_cast<int>(y*scale);};
                HGDIOBJ radarOldPen=SelectObject(dc,back.boxPen?back.boxPen:pen);HGDIOBJ radarOldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
                Rectangle(dc,px(20),py(42),px(240),py(262));Ellipse(dc,px(34),py(56),px(226),py(248));
                MoveToEx(dc,px(130),py(56),nullptr);LineTo(dc,px(130),py(248));MoveToEx(dc,px(34),py(152),nullptr);LineTo(dc,px(226),py(152));
                const COLORREF textColor=SetTextColor(dc,RGB(255,255,255));TextOutA(dc,px(28),py(46),"RADAR 50u",9);SetTextColor(dc,textColor);
                SelectObject(dc,GetStockObject(WHITE_BRUSH));POINT local[]={{px(130),py(146)},{px(125),py(158)},{px(135),py(158)}};Polygon(dc,local,3);
                SelectObject(dc,back.radarPen?back.radarPen:pen);SelectObject(dc,back.radarEnemy?back.radarEnemy:GetStockObject(NULL_BRUSH));
                const int radius=(std::max)(2,static_cast<int>(3*scale));
                for(const auto& point:snapshot.radarPoints){const int x=px(130+point.right*92),y=py(152-point.forward*92);
                    SelectObject(dc,point.outside?GetStockObject(NULL_BRUSH):(back.radarEnemy?back.radarEnemy:GetStockObject(NULL_BRUSH)));Ellipse(dc,x-radius,y-radius,x+radius+1,y+radius+1);}
                SelectObject(dc,radarOldPen);SelectObject(dc,radarOldBrush);
            }
            if(oldFont)SelectObject(dc,oldFont);
            SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);if(showStatus)TextOutA(dc,12,12,snapshot.status.c_str(),static_cast<int>(snapshot.status.size()));
            if(ferrumRequested){char label[160]={};std::snprintf(label,sizeof(label),"v39 | Ferrum %s | F10 mira %s | F11 trigger %s | botoes no painel",ferrumReady.load()?"OK":"OFF",aimEnabled.load()?"ON":"OFF",triggerEnabled.load()?"ON":"OFF");SetTextColor(dc,RGB(255,255,255));TextOutA(dc,12,height-24,label,static_cast<int>(std::strlen(label)));}
            if(ferrumRequested&&triggerEnabled){static const char* reasons[]={"desligado","aguardando botao","leitura atrasada","mira fora da colisao","visibilidade nao confirmada","aguardando atraso/intervalo","mira em movimento","disparo enviado","menu aberto"};
                const unsigned reason=triggerReason.load();char text[140]={};std::snprintf(text,sizeof(text),"Trigger: %s | visible check %s",reasons[reason<9?reason:0],triggerRequireVisibility.load()?"ON":"OFF");SetTextColor(dc,RGB(255,255,255));TextOutA(dc,12,height-44,text,static_cast<int>(std::strlen(text)));}
            if(ferrumRequested&&aimEnabled){static const char* reasons[]={"aguardando botao","leitura atrasada","sem alvo confirmado na FOV","acompanhando","lacuna breve do visible check","centralizado / mesmo quadro","menu aberto"};const unsigned reason=aimReason.load();char text[150]={};std::snprintf(text,sizeof(text),"Mira: %s",reasons[reason<7?reason:0]);SetTextColor(dc,RGB(255,255,255));TextOutA(dc,12,height-64,text,static_cast<int>(std::strlen(text)));}
            BitBlt(screen,0,0,width,height,dc,0,0,SRCCOPY);
        }EndPaint(window,&paint);return 0;}return DefWindowProcW(window,message,w,l);
}
}

int main(int argc,char** argv){
    using namespace preview;std::setvbuf(stdout,nullptr,_IONBF,0);
    if(argc>1&&(!std::strcmp(argv[1],"--teste-cache")||!std::strcmp(argv[1],"--teste-dma")||!std::strcmp(argv[1],"--benchmark")||!std::strcmp(argv[1],"--teste-oclusao")||!std::strcmp(argv[1],"--teste-camera")||!std::strcmp(argv[1],"--teste-vida"))){
        Reader reader;if(!reader.open()){std::puts("TESTE: inicializacao falhou");return 2;}
        QWORD system=0,renderer=0;if(!reader.environment(system,renderer))return 3;
        if(!std::strcmp(argv[1],"--teste-vida")){QWORD env=0,game=0,framework=0,actors=0,table=0,method=0;
            if(reader.pointer(reader.entityBase+0x164F50,env)&&reader.pointer(env+0xC8,game)&&reader.pointer(game+0x48,framework)&&reader.pointer(framework+0x4E0,actors)&&reader.pointer(actors,table)&&reader.bytes(table+0x78,&method,8)){
                std::printf("ATORES: sistema=0x%llX; tabela_rva=0x%llX; getter_rva=0x%llX\n",actors,table-reader.actionBase,method-reader.actionBase);
                QWORD header=0,root=0;if(!reader.pointer(actors+0x30,header)||!reader.pointer(header+8,root))return 8;
                std::vector<QWORD> pending{root};std::unordered_set<QWORD> visited;
                for(size_t i=0;i<pending.size()&&i<128;++i){const QWORD address=pending[i];if(address==header||!visited.insert(address).second)continue;BYTE record[0x30]={};if(!reader.bytes(address,record,sizeof(record))||record[0x19])continue;
                    QWORD left=0,right=0,actor=0,vt=0,health=0;DWORD id=0;std::memcpy(&left,record,8);std::memcpy(&right,record+0x10,8);std::memcpy(&id,record+0x20,4);std::memcpy(&actor,record+0x28,8);
                    if(plausiblePointer(left))pending.push_back(left);if(plausiblePointer(right))pending.push_back(right);
                    if(reader.pointer(actor,vt)&&reader.bytes(vt+0xD8,&health,8)){
                        std::printf("ATOR: id=0x%08lX; objeto=0x%llX; tabela_rva=0x%llX; vida_getter_rva=0x%llX\n",id,actor,vt-reader.gameBase,health-reader.gameBase);
                        DWORD healthData[2]={};if(health==reader.gameBase+0x219A40&&reader.bytes(actor+0x60,healthData,8)){
                            const auto life=decodeLife(healthData[0],healthData[1]);DWORD raw=healthData[1]?healthData[0]^healthData[1]:0;float value=0;std::memcpy(&value,&raw,4);
                            std::printf("VIDA LIDA: id=0x%08lX; valor=%.6g; estado=%s\n",id,value,life==LifeState::Alive?"vivo":life==LifeState::Dead?"morto":"desconhecido");}
                    }
                }return 0;}return 8;}
        if(!std::strcmp(argv[1],"--teste-camera")){unsigned valid=0;const auto started=std::chrono::steady_clock::now();
            for(unsigned i=0;i<120;++i){auto batch=reader.initScatter(reader.handle,reader.pid,VMMDLL_FLAG_NOCACHE);if(!batch)continue;Reader::CameraBatch data{};const bool queued=reader.prepareCamera(renderer,batch,data);const bool readOk=reader.execute(batch)!=FALSE;reader.closeScatter(batch);RendererProjection result{};if(queued&&readOk&&reader.selectCamera(data,result)){++valid;if(i==0){const auto ray=collisionRay::fromCamera(result);const auto test=collisionRay::add(ray.origin,collisionRay::mul(ray.direction,10));const auto screen=projection::project(result.first,result.second,{test[0],test[1],test[2],1},result.viewport,1920,1080);
                    std::printf("RAIO: valido=%d; projecao_valida=%d; x=%.3f; y=%.3f; w=%.3f; proj_z=%.6f,%.6f\n",ray.valid,screen.valid,screen.x,screen.y,screen.w,result.second[10],result.second[14]);}}}
            const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()/120;
            std::printf("CAMERA: validas=%u/120; media_ms=%.2f; matrizes_em_lote=1\n",valid,ms);return valid>=108?0:4;}
        const auto discoveryStart=std::chrono::steady_clock::now();const auto nodes=reader.discover(system);const auto firstDiscovery=std::chrono::steady_clock::now();
        if(!std::strcmp(argv[1],"--teste-cache")){const auto again=reader.discover(system);const auto end=std::chrono::steady_clock::now();std::printf("CACHE TESTE: primeira_ms=%.2f; repetida_ms=%.2f; candidatos=%zu/%zu; sem_mouse=1\n",std::chrono::duration<double,std::milli>(firstDiscovery-discoveryStart).count(),std::chrono::duration<double,std::milli>(end-firstDiscovery).count(),nodes.size(),again.size());return nodes.empty()?4:0;}
        const bool benchmark=!std::strcmp(argv[1],"--benchmark");
        if(!std::strcmp(argv[1],"--teste-oclusao")){
            DWORD currentFrame=0;if(!reader.engineBase||!reader.bytes(reader.engineBase+0x3776D8,&currentFrame,4))return 8;
            std::printf("OCLUSAO FRAME: %lu\n",currentFrame);
            for(const auto& n:nodes){QWORD temp=0,renderTable=0;DWORD ids[2]={};
                if(reader.bytes(n.proxy+8,&renderTable,8)&&reader.pointer(n.proxy+0x40,temp)&&reader.bytes(temp+0x60,ids,8))
                    std::printf("OCLUSAO CANDIDATA: nome=%s; render_vt_rva=0x%llX; temp=0x%llX; visivel=%lu; oculto=%lu; idade_visivel=%lu; idade_oculto=%lu\n",n.name.c_str(),renderTable-reader.entityBase,temp,ids[0],ids[1],currentFrame-ids[0],currentFrame-ids[1]);
            }return 0;
        }
        size_t types[7]={};for(const auto& node:nodes)if(node.body)for(const auto& g:*node.body)if(g.type<7)++types[g.type];
        std::printf("GEOMETRIAS: caixas=%zu; malhas=%zu; esferas=%zu; capsulas=%zu\n",types[0],types[1],types[4],types[6]);
        unsigned maxValidParts=0,bodyHitFrames=0;
        const unsigned iterations=benchmark?120:1;unsigned valid=0;size_t markers=0,radarCount=0,green=0,red=0,yellow=0;std::vector<double> times;std::string lastStatus;
        for(unsigned i=0;i<iterations;++i){const auto start=std::chrono::steady_clock::now();const auto result=reader.sample(renderer,nodes);lastStatus=result.status;maxValidParts=(std::max)(maxValidParts,result.validBodyParts);for(const auto& m:result.markers)if(m.bodyHit)++bodyHitFrames;times.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());if(result.time)++valid;markers=(std::max)(markers,result.markers.size());radarCount=(std::max)(radarCount,result.radarPoints.size());for(const auto& marker:result.markers){if(marker.visibility==Visibility::Visible)++green;else if(marker.visibility==Visibility::Occluded)++red;else ++yellow;}}
        double total=0;for(double value:times)total+=value;std::sort(times.begin(),times.end());
        std::printf("TESTE: candidatos=%zu; max_boxes=%zu; amostras_validas=%u/%u; media_ms=%.2f; p95_ms=%.2f; max_ms=%.2f\n",nodes.size(),markers,valid,iterations,total/iterations,times[(times.size()-1)*95/100],times.back());
        std::printf("STATUS: %s\n",lastStatus.c_str());
        std::printf("OCLUSAO: observacoes_visiveis=%zu; ocultas=%zu; desconhecidas=%zu\n",green,red,yellow);
        std::printf("RADAR: max_inimigos=%zu\n",radarCount);
        std::printf("COLISAO: max_partes_validas=%u; quadros_com_intersecao=%u; sem_comandos_de_mouse=1\n",maxValidParts,bodyHitFrames);
        return valid*100>=iterations*90?0:4;
    }
    loadPanelSettings();
    SetProcessDPIAware();std::vector<Display> displays;EnumDisplayMonitors(nullptr,nullptr,listDisplay,reinterpret_cast<LPARAM>(&displays));
    std::stable_sort(displays.begin(),displays.end(),[](const Display& a,const Display& b){return a.primary&&!b.primary;});
    unsigned selected=0;bool listOnly=false;
    for(int i=1;i<argc;++i){if(!std::strcmp(argv[i],"--ferrum"))ferrumRequested=true;
        else if(!std::strcmp(argv[i],"--listar-monitores"))listOnly=true;
        else if(!std::strncmp(argv[i],"--monitor=",10)){char* end=nullptr;const unsigned long value=std::strtoul(argv[i]+10,&end,10);if(!end||*end||value==0||value>displays.size()){std::puts("Monitor invalido; use --listar-monitores");return 5;}selected=static_cast<unsigned>(value-1);}
        else {std::puts("Argumento desconhecido. Use --listar-monitores, --monitor=N ou --teste-dma");return 5;}}
    for(size_t i=0;i<displays.size();++i)std::printf("MONITOR %zu: %ls; %ldx%ld; principal=%d\n",i+1,displays[i].device.c_str(),displays[i].area.right-displays[i].area.left,displays[i].area.bottom-displays[i].area.top,displays[i].primary);
    if(listOnly)return displays.empty()?5:0;if(displays.empty())return 5;
    const RECT area=displays[selected].area;
    WNDCLASSW cls{};cls.lpfnWndProc=windowProc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"WarChaosDmaPreview";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);if(!RegisterClassW(&cls))return 6;
    HWND window=CreateWindowExW(WS_EX_TOPMOST,cls.lpszClassName,L"ESP DMA - saida HDMI para fuser",WS_POPUP|WS_VISIBLE,area.left,area.top,area.right-area.left,area.bottom-area.top,nullptr,nullptr,cls.hInstance,nullptr);if(!window)return 7;
    SetForegroundWindow(window);
    displayWindow=window;
    if(!RegisterHotKey(window,19,MOD_NOREPEAT,VK_INSERT))std::puts("PAINEL: Insert indisponivel; use a janela inicial.");
    togglePanel();
    SetTimer(window,1,30,nullptr);
    std::thread dma(worker);std::thread ferrumThread;if(ferrumRequested)ferrumThread=std::thread(ferrumWorker);MSG message{};int exitCode=0;
    while(!stop){const BOOL result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(result<0)exitCode=8;break;}if(settingsWindow&&IsDialogMessageW(settingsWindow,&message))continue;TranslateMessage(&message);DispatchMessageW(&message);}
    stop=true;aimEnabled=false;triggerEnabled=false;if(ferrumThread.joinable())ferrumThread.join();dma.join();if(IsWindow(window))DestroyWindow(window);return exitCode;
}


