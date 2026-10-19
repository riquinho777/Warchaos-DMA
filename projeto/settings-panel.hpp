// C++17 / settings-panel.hpp / native Windows configuration panel and live settings.
static std::mutex settingsLock;
static aim::Config liveConfig;
static activation::Bindings liveBindings;
static activation::Bindings readBindings(){std::lock_guard<std::mutex> guard(settingsLock);return liveBindings;}
static std::atomic<bool> menuOpen=false;
static bool showFov=true;
static HWND settingsWindow=nullptr;
static std::array<bool,12> panelChecks{};
static std::array<double,7> panelValues{80,10,12,1,4,40,160};
struct EspAppearance {std::array<COLORREF,7> colors{RGB(255,255,255),RGB(255,255,255),RGB(255,220,70),RGB(60,255,90),RGB(255,70,70),RGB(255,255,255),RGB(255,70,70)};int width=2;};
static EspAppearance appearance;
static double widthSliderValue=2;
static const char* colorKeys[]={"box_color","name_color","head_color","visible_color","occluded_color","fov_color","radar_color"};
static unsigned panelPage=0;
static std::string panelMessage="Ajuste as opcoes e clique em Salvar.";
struct PanelTheme {
    HBRUSH background=CreateSolidBrush(RGB(15,18,25)),field=CreateSolidBrush(RGB(32,38,51)),card=CreateSolidBrush(RGB(23,28,38));
    HFONT normal=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HFONT title=CreateFontW(-25,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    ~PanelTheme(){DeleteObject(background);DeleteObject(field);DeleteObject(card);DeleteObject(normal);DeleteObject(title);}
};
static PanelTheme panelTheme;
static std::string settingsPath(){char exe[MAX_PATH]={};GetModuleFileNameA(nullptr,exe,MAX_PATH);std::string path=exe;return path.substr(0,path.find_last_of("\\/")+1)+"Ferrum.ini";}
struct SettingField {const char* label;const char* key;double minimum,maximum;};
static const SettingField settingFields[]={
    {"FOV: raio em pixels","fov_px",5,400},{"Forca: ganho por pixel","gain",.001,2},
    {"Movimento maximo","max_step",1,100},{"Zona morta: pixels","deadzone_px",0,10},
    {"Trigger: raio pixels","trigger_radius_px",1,12},{"Trigger: atraso ms","trigger_delay_ms",0,500},
    {"Trigger: intervalo ms","trigger_interval_ms",50,2000}};
static std::array<double,7> configValues(const aim::Config& c){return {c.fov,c.gain,c.maxStep,c.deadzone,c.triggerRadius,static_cast<double>(c.dwell),static_cast<double>(c.cooldown)};}
static aim::Config fromValues(const std::array<double,7>& v){aim::Config c;c.fov=v[0];c.gain=v[1];c.maxStep=v[2];c.deadzone=v[3];c.triggerRadius=v[4];c.dwell=static_cast<std::uint64_t>(v[5]);c.cooldown=static_cast<std::uint64_t>(v[6]);return c;}
static double sliderMinimum(unsigned i){return i==7?1.:i==1?1.:settingFields[i].minimum;}
static double sliderMaximum(unsigned i){return i==7?6.:i==1?100.:settingFields[i].maximum;}
static double& sliderValue(unsigned i){return i==7?widthSliderValue:panelValues[i];}
static std::array<double,7> panelConfigValues(){auto values=panelValues;values[1]=2./std::clamp(values[1],1.,100.);return values;}
static aim::Config readLiveConfig(){std::lock_guard<std::mutex> guard(settingsLock);return liveConfig;}
static void loadPanelSettings(){auto values=configValues(liveConfig);const auto path=settingsPath();
    for(size_t i=0;i<values.size();++i){char buffer[64]={};const auto fallback=std::to_string(values[i]);GetPrivateProfileStringA("Ferrum",settingFields[i].key,fallback.c_str(),buffer,sizeof(buffer),path.c_str());char* end=nullptr;const double n=std::strtod(buffer,&end);if(end!=buffer&&!*end&&std::isfinite(n)&&n>=settingFields[i].minimum&&n<=settingFields[i].maximum)values[i]=n;}
    liveConfig=fromValues(values);
    triggerRequireVisibility=GetPrivateProfileIntA("Ferrum","trigger_require_visibility",1,path.c_str())!=0;
    showHealth=GetPrivateProfileIntA("ESP","health",1,path.c_str())!=0;
    appearance.width=std::clamp(static_cast<int>(GetPrivateProfileIntA("ESP","line_width",2,path.c_str())),1,6);
    for(unsigned i=0;i<7;++i)appearance.colors[i]=GetPrivateProfileIntA("ESP",colorKeys[i],appearance.colors[i],path.c_str())&0xFFFFFF;
    ffaMode=GetPrivateProfileIntA("ESP","ffa_mode",0,path.c_str())!=0;
    liveBindings.aim=activation::valid(GetPrivateProfileIntA("Ferrum","aim_button",1,path.c_str()));
    liveBindings.trigger=activation::valid(GetPrivateProfileIntA("Ferrum","trigger_button",1,path.c_str()));
    bool* flags[]={&showBox,&showNames,&showSkeleton,&showHead,&showRadar,&showStatus,&showFov};const char* keys[]={"box","names","skeleton","head","radar","status","fov"};
    for(unsigned i=0;i<7;++i)*flags[i]=GetPrivateProfileIntA("ESP",keys[i],1,path.c_str())!=0;
}
static void refreshPanel(){if(!settingsWindow)return;
    widthSliderValue=appearance.width;InvalidateRect(GetDlgItem(settingsWindow,207),nullptr,FALSE);for(unsigned i=0;i<7;++i)InvalidateRect(GetDlgItem(settingsWindow,800+i),nullptr,FALSE);const auto values=configValues(readLiveConfig());
    for(unsigned i=0;i<7;++i){panelValues[i]=i==1?std::clamp(2./values[i],1.,100.):values[i];InvalidateRect(GetDlgItem(settingsWindow,200+i),nullptr,FALSE);}
    const auto bindings=readBindings();SetDlgItemTextA(settingsWindow,500,(std::string(activation::label(bindings.aim))+"  v").c_str());SetDlgItemTextA(settingsWindow,501,(std::string(activation::label(bindings.trigger))+"  v").c_str());
    const bool flags[]={showBox,showNames,showSkeleton,showHead,showRadar,showStatus,showFov,aimEnabled.load(),triggerEnabled.load(),ffaMode.load(),showHealth,triggerRequireVisibility.load()};
    for(unsigned i=0;i<12;++i){panelChecks[i]=flags[i];InvalidateRect(GetDlgItem(settingsWindow,100+i),nullptr,FALSE);}
}
static bool applyPanel(bool save){std::array<double,7> values{};
    values=panelConfigValues();
    for(unsigned i=0;i<7;++i)if(!std::isfinite(values[i])||values[i]<settingFields[i].minimum||values[i]>settingFields[i].maximum)return false;
    {std::lock_guard<std::mutex> guard(settingsLock);liveConfig=fromValues(values);}
    bool* flags[]={&showBox,&showNames,&showSkeleton,&showHead,&showRadar,&showStatus,&showFov};const char* keys[]={"box","names","skeleton","head","radar","status","fov"};
    const auto path=settingsPath();bool saved=true;
    for(unsigned i=0;i<7;++i){*flags[i]=panelChecks[i];if(save)saved=WritePrivateProfileStringA("ESP",keys[i],*flags[i]?"1":"0",path.c_str())&&saved;}
    triggerRequireVisibility=panelChecks[11];if(save)saved=WritePrivateProfileStringA("Ferrum","trigger_require_visibility",panelChecks[11]?"1":"0",path.c_str())&&saved;
    showHealth=panelChecks[10];if(save)saved=WritePrivateProfileStringA("ESP","health",showHealth?"1":"0",path.c_str())&&saved;
    ffaMode=panelChecks[9];if(save)saved=WritePrivateProfileStringA("ESP","ffa_mode",panelChecks[9]?"1":"0",path.c_str())&&saved;
    aimEnabled=ferrumReady&&panelChecks[7];
    triggerEnabled=ferrumReady&&panelChecks[8];
    if(save)for(unsigned i=0;i<7;++i){const auto text=std::to_string(values[i]);saved=WritePrivateProfileStringA("Ferrum",settingFields[i].key,text.c_str(),path.c_str())&&saved;}
    if(save){const auto bindings=readBindings();saved=WritePrivateProfileStringA("Ferrum","aim_button",std::to_string(bindings.aim).c_str(),path.c_str())&&saved;saved=WritePrivateProfileStringA("Ferrum","trigger_button",std::to_string(bindings.trigger).c_str(),path.c_str())&&saved;}
    if(save){saved=WritePrivateProfileStringA("ESP","line_width",std::to_string(appearance.width).c_str(),path.c_str())&&saved;for(unsigned i=0;i<7;++i)saved=WritePrivateProfileStringA("ESP",colorKeys[i],std::to_string(appearance.colors[i]).c_str(),path.c_str())&&saved;}
    if(!saved)MessageBoxW(settingsWindow,L"Ajustes aplicados, mas falhou ao salvar Ferrum.ini.",L"Configuracao",MB_OK|MB_ICONERROR);
    panelMessage=save?(saved?"Configuracoes salvas.":"Aplicado; falha ao salvar."):"Configuracoes aplicadas.";
    refreshPanel();InvalidateRect(settingsWindow,nullptr,FALSE);return true;
}
static void hidePanel(){ShowWindow(settingsWindow,SW_HIDE);menuOpen=false;}
static void panelText(HDC dc,const char* text,RECT r,COLORREF color,HFONT font=nullptr,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
    const auto old=SelectObject(dc,font?font:panelTheme.normal);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextA(dc,text,-1,&r,flags);SelectObject(dc,old);
}
static void panelRound(HDC dc,RECT r,COLORREF color,int radius=12){HBRUSH brush=CreateSolidBrush(color);auto b=SelectObject(dc,brush);auto pen=SelectObject(dc,GetStockObject(NULL_PEN));RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,b);SelectObject(dc,pen);DeleteObject(brush);}
static void changeSlider(HWND w,double value){const unsigned i=GetDlgCtrlID(w)-200;if(i>=8)return;
    const double step=i==3?.1:1.;
    sliderValue(i)=std::clamp(std::round(value/step)*step,sliderMinimum(i),sliderMaximum(i));
    if(i==7)appearance.width=static_cast<int>(widthSliderValue);else{std::lock_guard<std::mutex> guard(settingsLock);liveConfig=fromValues(panelConfigValues());}
    panelMessage="Ajuste em tempo real. Salvar grava os valores.";InvalidateRect(w,nullptr,FALSE);InvalidateRect(settingsWindow,nullptr,FALSE);notifyFrame();
}
static LRESULT CALLBACK sliderProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    const unsigned i=GetDlgCtrlID(w)-200;
    if(i>=8)return DefWindowProcA(w,m,wp,lp);
    auto fromMouse=[&]{RECT r{};GetClientRect(w,&r);const int x=static_cast<short>(LOWORD(lp));const double f=std::clamp((x-8.)/(r.right-16.),0.,1.);changeSlider(w,sliderMinimum(i)+f*(sliderMaximum(i)-sliderMinimum(i)));};
    if(m==WM_GETDLGCODE)return DLGC_WANTARROWS;
    if(m==WM_LBUTTONDOWN){SetFocus(w);SetCapture(w);fromMouse();return 0;}
    if(m==WM_MOUSEMOVE&&GetCapture()==w){fromMouse();return 0;}
    if(m==WM_LBUTTONUP){if(GetCapture()==w){fromMouse();ReleaseCapture();}return 0;}
    if(m==WM_KEYDOWN){const double step=(i==3?.1:1.)*((GetKeyState(VK_SHIFT)&0x8000)?10:1);
        if(wp==VK_LEFT||wp==VK_DOWN)changeSlider(w,sliderValue(i)-step);
        else if(wp==VK_RIGHT||wp==VK_UP)changeSlider(w,sliderValue(i)+step);
        else if(wp==VK_HOME)changeSlider(w,sliderMinimum(i));else if(wp==VK_END)changeSlider(w,sliderMaximum(i));return 0;}
    if(m==WM_SETFOCUS||m==WM_KILLFOCUS){InvalidateRect(w,nullptr,FALSE);return 0;}
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){PAINTSTRUCT paint;HDC dc=BeginPaint(w,&paint);RECT r{};GetClientRect(w,&r);FillRect(dc,&r,panelTheme.card);panelRound(dc,r,RGB(32,43,64),7);
        const double fraction=(sliderValue(i)-sliderMinimum(i))/(sliderMaximum(i)-sliderMinimum(i));
        RECT fill=r;fill.right=static_cast<LONG>(fraction*r.right);if(fill.right>0)panelRound(dc,fill,RGB(49,78,138),7);
        const int x=8+static_cast<int>(fraction*(r.right-16));RECT thumb{x-2,5,x+2,r.bottom-5};panelRound(dc,thumb,RGB(108,148,255),4);
        char value[32]={};std::snprintf(value,sizeof(value),i==3?"%.1f":"%.0f",sliderValue(i));panelText(dc,value,r,RGB(242,246,255),nullptr,DT_CENTER|DT_SINGLELINE|DT_VCENTER);
        if(GetFocus()==w){RECT f=r;InflateRect(&f,-2,-2);DrawFocusRect(dc,&f);}EndPaint(w,&paint);return 0;}
    return DefWindowProcA(w,m,wp,lp);
}
static void layoutPanel(){
    unsigned row=0;
    for(unsigned i=0;i<12;++i){const bool visible=panelPage==0?(i<6||i==9||i==10):panelPage==1?(i==6||i==7):panelPage==2&&(i==8||i==11);
        if(visible){const unsigned order=panelPage==1?(i==7?0:1):row++;MoveWindow(GetDlgItem(settingsWindow,100+i),216,112+order*(panelPage==0?34:40),478,32,TRUE);}
        ShowWindow(GetDlgItem(settingsWindow,100+i),visible?SW_SHOW:SW_HIDE);
    }
    for(unsigned i=0;i<7;++i){const bool visible=panelPage==1?i<4:panelPage==2&&i>=5;
        const int y=panelPage==1?238+static_cast<int>(i)*36:248+(static_cast<int>(i)-5)*52;
        if(visible){MoveWindow(GetDlgItem(settingsWindow,600+i),224,y,200,27,TRUE);MoveWindow(GetDlgItem(settingsWindow,200+i),434,y,244,28,TRUE);}
        ShowWindow(GetDlgItem(settingsWindow,600+i),visible?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(settingsWindow,200+i),visible?SW_SHOW:SW_HIDE);
    }
    for(unsigned i=0;i<2;++i){const bool visible=panelPage==i+1;ShowWindow(GetDlgItem(settingsWindow,500+i),visible?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(settingsWindow,700+i),visible?SW_SHOW:SW_HIDE);}
    for(unsigned i=0;i<7;++i)ShowWindow(GetDlgItem(settingsWindow,800+i),panelPage==3?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(settingsWindow,207),panelPage==3?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(settingsWindow,607),panelPage==3?SW_SHOW:SW_HIDE);
    InvalidateRect(settingsWindow,nullptr,TRUE);
}
static LRESULT CALLBACK settingsProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_CREATE){
        auto control=[&](const char* cls,const char* label,DWORD style,int x,int y,int width,int height,int id){HWND child=CreateWindowExA(0,cls,label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|style,x,y,width,height,w,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);if(!child)std::printf("CONTROLE FALHOU: %s id=%d erro=%lu\n",cls,id,GetLastError());SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(panelTheme.normal),FALSE);return child;};
        const char* tabs[]={"01   ESP","02   Mira","03   Trigger","04   Cores"};for(unsigned i=0;i<4;++i)control("BUTTON",tabs[i],BS_OWNERDRAW,18,112+static_cast<int>(i)*48,148,39,400+i);
        const char* labels[]={"Box","Nomes","Esqueleto","Cabeca","Radar","Informacoes","Mostrar circulo da FOV","Ativar aimbot","Ativar trigger","Vale Tudo (todos contra todos)","Vida dos inimigos (HP)","Exigir visible check (cache)"};
        for(unsigned i=0;i<12;++i)control("BUTTON",labels[i],BS_OWNERDRAW,216,112,478,34,100+i);
        const char* fields[]={"Raio da FOV (px)","Forca (menor = forte)","Limite do movimento","Zona morta (px)","Margem do corpo (px)","Atraso para disparar (ms)","Intervalo entre disparos (ms)"};
        for(unsigned i=0;i<7;++i){control("STATIC",fields[i],0,224,200,300,27,600+i);control("ZyonSlider","",0,434,200,244,28,200+i);}
        control("STATIC","Botao de ativacao",0,224,194,200,27,700);control("BUTTON","",BS_OWNERDRAW,434,192,244,28,500);
        control("STATIC","Botao de ativacao",0,224,194,200,27,701);control("BUTTON","",BS_OWNERDRAW,434,192,244,28,501);
        const char* colorLabels[]={"Box","Nomes","Cabeca","Esqueleto visivel","Esqueleto oculto","Circulo da FOV","Pontos do radar"};
        for(unsigned i=0;i<7;++i)control("BUTTON",colorLabels[i],BS_OWNERDRAW,216,110+i*31,478,28,800+i);
        control("STATIC","Espessura das linhas",0,224,352,200,27,607);control("ZyonSlider","",0,434,352,244,28,207);
        control("BUTTON","Salvar",BS_OWNERDRAW,474,422,108,36,301);control("BUTTON","Voltar ao jogo",BS_OWNERDRAW,594,422,122,36,302);
        SetTimer(w,2,1000,nullptr);return 0;
    }
    if(m==WM_NCHITTEST){const LRESULT hit=DefWindowProcW(w,m,wp,lp);POINT p{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};ScreenToClient(w,&p);return hit==HTCLIENT&&p.y<80?HTCAPTION:hit;}
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_CTLCOLOREDIT){HDC dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(235,239,247));SetBkColor(dc,RGB(32,38,51));return reinterpret_cast<LRESULT>(panelTheme.field);}
    if(m==WM_CTLCOLORSTATIC){HDC dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(185,194,211));SetBkColor(dc,RGB(23,28,38));return reinterpret_cast<LRESULT>(panelTheme.card);}
    if(m==WM_DRAWITEM){const auto* item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);RECT r=item->rcItem;const unsigned id=item->CtlID;const bool pressed=(item->itemState&ODS_SELECTED)!=0;char label[100]={};GetWindowTextA(item->hwndItem,label,sizeof(label));
        FillRect(item->hDC,&r,id>=100&&id<=111?panelTheme.card:panelTheme.background);
        if(id>=800&&id<807){FillRect(item->hDC,&r,panelTheme.card);RECT text=r;text.left+=8;panelText(item->hDC,label,text,RGB(220,228,242));RECT swatch{r.right-62,4,r.right-10,r.bottom-4};panelRound(item->hDC,swatch,appearance.colors[id-800],6);}
        else if(id>=100&&id<=111){const bool on=panelChecks[id-100];panelRound(item->hDC,r,pressed?RGB(37,46,63):RGB(23,28,38));RECT text=r;text.left+=8;text.right-=68;panelText(item->hDC,label,text,RGB(225,231,241));RECT track{r.right-53,8,r.right-9,26};panelRound(item->hDC,track,on?RGB(78,115,238):RGB(58,66,83),18);RECT dot{on?track.right-17:track.left+3,11,on?track.right-3:track.left+17,23};panelRound(item->hDC,dot,RGB(245,247,255),14);}
        else{const bool selected=id>=400&&id<=403&&panelPage==id-400;const bool primary=id==301;panelRound(item->hDC,r,pressed?RGB(53,74,141):(primary?RGB(76,109,226):selected?RGB(35,46,74):RGB(24,30,42)));if(selected){RECT accent{0,8,3,r.bottom-8};panelRound(item->hDC,accent,RGB(111,142,255),3);}RECT text=r;text.left+=id>=400?14:0;panelText(item->hDC,label,text,selected||primary?RGB(245,247,255):RGB(185,195,215),nullptr,id>=400?DT_LEFT|DT_VCENTER|DT_SINGLELINE:DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
        if(item->itemState&ODS_FOCUS){RECT focus=r;InflateRect(&focus,-3,-3);DrawFocusRect(item->hDC,&focus);}return TRUE;
    }
    if(m==WM_PAINT){PAINTSTRUCT paint;HDC dc=BeginPaint(w,&paint);RECT r{};GetClientRect(w,&r);HDC mem=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);const auto old=SelectObject(mem,bitmap);FillRect(mem,&r,panelTheme.background);
        panelText(mem,"ZYON",{22,22,170,55},RGB(237,242,255),panelTheme.title);panelText(mem,"CONTROL PANEL",{24,55,175,76},RGB(109,123,151));
        const char* titles[]={"Visualizacao","Mira e FOV","Disparo automatico","Cores e espessura"};const char* captions[]={"Escolha o que aparece no ESP.","Forca menor = mais forte. Maior = mais suave.","Filtro desligado pode disparar contra alvos atras de paredes.","Clique na cor para escolher. Espessura de 1 a 6 px."};
        panelText(mem,titles[panelPage],{212,19,710,52},RGB(240,244,253),panelTheme.title);panelText(mem,captions[panelPage],{214,55,710,77},RGB(139,153,178));
        panelRound(mem,{196,96,716,392},RGB(23,28,38),16);
        panelText(mem,ferrumReady?"Ferrum conectado":"Ferrum desconectado",{22,307,180,331},ferrumReady?RGB(88,208,153):RGB(225,156,106));
        panelText(mem,"INSERT  abrir / fechar",{22,337,187,362},RGB(128,144,172));
        panelText(mem,"Menu aberto:\ncomandos pausados",{22,369,184,413},RGB(128,144,172),nullptr,DT_LEFT|DT_WORDBREAK);
        panelText(mem,panelMessage.c_str(),{22,428,462,453},RGB(137,155,185));
        BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bitmap);DeleteDC(mem);EndPaint(w,&paint);return 0;
    }
    if(m==WM_TIMER){InvalidateRect(w,nullptr,FALSE);return 0;}
    if(m==WM_COMMAND){const unsigned id=LOWORD(wp);if(HIWORD(wp)!=BN_CLICKED)return 0;
        if(id>=800&&id<807){static COLORREF custom[16]={};CHOOSECOLORW picker{};picker.lStructSize=sizeof(picker);picker.hwndOwner=w;picker.rgbResult=appearance.colors[id-800];picker.lpCustColors=custom;picker.Flags=CC_FULLOPEN|CC_RGBINIT;
            if(ChooseColorW(&picker)){appearance.colors[id-800]=picker.rgbResult;panelMessage="Cor atualizada. Salvar grava as alteracoes.";InvalidateRect(w,nullptr,FALSE);InvalidateRect(GetDlgItem(w,id),nullptr,FALSE);notifyFrame();}}
        else if(id==500||id==501){
            const auto before=readBindings();const unsigned current=id==500?before.aim:before.trigger;
            HMENU options=CreatePopupMenu();if(!options)return 0;
            for(unsigned i=0;i<5;++i)AppendMenuA(options,MF_STRING|(i==current?MF_CHECKED:MF_UNCHECKED),1000+i,activation::label(i));
            RECT anchor{};GetWindowRect(GetDlgItem(w,id),&anchor);
            const UINT choice=TrackPopupMenu(options,TPM_RETURNCMD|TPM_NONOTIFY|TPM_LEFTALIGN|TPM_TOPALIGN,anchor.left,anchor.bottom,0,w,nullptr);
            DestroyMenu(options);
            if(choice>=1000&&choice<1005){
                {std::lock_guard<std::mutex> guard(settingsLock);auto& binding=id==500?liveBindings.aim:liveBindings.trigger;binding=choice-1000;}
                SetDlgItemTextA(w,id,(std::string(activation::label(choice-1000))+"  v").c_str());
                panelMessage="Botao de ativacao atualizado.";InvalidateRect(w,nullptr,FALSE);
            }
        }
        else if(id>=400&&id<=403){panelPage=id-400;layoutPanel();}
        else if(id>=100&&id<=111){if((id==107||id==108)&&!ferrumReady){panelMessage="Conecte o Ferrum para ativar.";InvalidateRect(w,nullptr,FALSE);return 0;}panelChecks[id-100]=!panelChecks[id-100];InvalidateRect(GetDlgItem(w,id),nullptr,FALSE);}
        else if(id==301)applyPanel(true);else if(id==302){if(applyPanel(true))hidePanel();}return 0;
    }
    if(m==WM_CLOSE){if(applyPanel(true))hidePanel();return 0;}
    if(m==WM_DESTROY){KillTimer(w,2);return 0;}return DefWindowProcW(w,m,wp,lp);
}
static void togglePanel(){if(menuOpen){if(applyPanel(true))hidePanel();return;}if(!settingsWindow){WNDCLASSA sliderClass{};sliderClass.lpfnWndProc=sliderProc;sliderClass.hInstance=GetModuleHandleW(nullptr);sliderClass.lpszClassName="ZyonSlider";sliderClass.hCursor=LoadCursor(nullptr,IDC_SIZEWE);RegisterClassA(&sliderClass);WNDCLASSW cls{};cls.lpfnWndProc=settingsProc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"WarChaosSettings";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);cls.hbrBackground=panelTheme.background;RegisterClassW(&cls);
    settingsWindow=CreateWindowExW(WS_EX_TOPMOST,L"WarChaosSettings",L"Zyon | Control Panel",WS_POPUP|WS_SYSMENU|WS_CLIPCHILDREN,80,80,740,480,nullptr,nullptr,cls.hInstance,nullptr);}
    if(settingsWindow){menuOpen=true;refreshPanel();layoutPanel();ShowWindow(settingsWindow,SW_SHOW);SetForegroundWindow(settingsWindow);}
}




