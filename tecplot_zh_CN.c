/* Simplified Chinese UI add-on for Tecplot 360 EX 2025 R1 (72401).
 * Local, reversible extension. Uses the host's exported Qt 5.15.2 ABI.
 * Does not patch executable code, touch licensing, or modify dataset values.
 * Build: zig cc -shared -O2 tecplot_zh_CN.c -o tecplot_zh_CN.dll -lversion
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct { void *d; } QString;
typedef struct { uint64_t storage[4]; } QVariant;
typedef struct { volatile LONG ref; int alloc, begin, end; void *array[1]; } ListData;
typedef struct { ListData *d; } QList;
static HMODULE module;
static void *translator;
static FILE *logfile;
static int initialized;
static QString *(*qFromUtf8)(QString*,const char*,int);
static void (*qStringDestroy)(QString*);
static void (*qTranslatorCtor)(void*,void*);
static bool (*qTranslatorLoad)(void*,const QString*,const QString*,const QString*,const QString*);
static bool (*qInstallTranslator)(void*);
static QString *(*qTranslate)(const void*,QString*,const char*,const char*,const char*,int);
static QList *(*qAllWidgets)(QList*);
static QList *(*qActions)(const void*,QList*);
static const QList *(*qChildren)(const void*);
static void (*qListDispose)(ListData*);
static bool (*qInherits)(const void*,const char*);
static QVariant *(*qProperty)(const void*,QVariant*,const char*);
static bool (*qSetProperty)(void*,const char*,const QVariant*);
static QString *(*qToString)(const QVariant*,QString*);
static void (*qVariantCtor)(QVariant*,const QString*);
static void (*qVariantDestroy)(QVariant*);
static void (*lockOn)(void),(*lockOff)(void);
static void *(*registerAddon)(int,const char*,const char*,const char*);
static unsigned char (*addTimer)(uint32_t,intptr_t,unsigned char(*)(intptr_t));
static unsigned char (*lockIsOn)(void);
static unsigned long changed;
static void *(*qQuickRoot)(const void*);
static QList *(*qQuickChildren)(const void*,QList*);
static int (*qComboCount)(const void*);
static QString *(*qComboText)(const void*,QString*,int);
static int (*qTabCount)(const void*);
static QString *(*qTabText)(const void*,QString*,int);
static void (*qTabSetText)(void*,int,const QString*);
static void (*qComboSetText)(void*,int,const QString*);
static bool (*qBlockSignals)(void*,bool);
static int diagnostics;
static unsigned long observed[32768];
static unsigned observedN;
static void recordMissing(const char *kind,const char *text){
    if(!diagnostics||!text||!strpbrk(text,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")||strlen(text)>3000)return;
    unsigned long h=5381;for(const unsigned char*p=(const unsigned char*)text;*p;p++)h=((h<<5)+h)^*p;
    for(unsigned i=0;i<observedN;i++)if(observed[i]==h)return;
    if(observedN<32768)observed[observedN++]=h;
    if(logfile){fprintf(logfile,"MISSING[%s] %s\n",kind,text);fflush(logfile);}
}

static void logmsg(const char *s) { if(logfile){fprintf(logfile,"%s\n",s);fflush(logfile);} }
static FARPROC symbol(HMODULE h,const char *name){
    FARPROC p=h?GetProcAddress(h,name):NULL;
    if(!p){logmsg("Missing host API; add-on remains inactive:");logmsg(name);}
    return p;
}
#define RESOLVE(var,mod,name) do { *(FARPROC*)&var=symbol(mod,name);if(!var)return; } while(0)
static int stringUtf8(const QString *s,char *out,int cap){
    if(!s||!s->d){out[0]=0;return 0;}
    const char *d=s->d;
    int size=*(const int*)(d+4);
    intptr_t offset=*(const intptr_t*)(d+16);
    if(size<=0||size>65536){out[0]=0;return 0;}
    int n=WideCharToMultiByte(CP_UTF8,0,(const WCHAR*)(d+offset),size,out,cap-1,NULL,NULL);
    out[n]=0;return n;
}
static int translated(const char *source,QString *result){
    qTranslate(translator,result,"",source,NULL,-1);
    return result->d && *(int*)((char*)result->d+4)>0;
}
static int translatedTrimmed(const char *source,QString *result){
    if(translated(source,result))return 1;
    if(!strncmp(source,"Version ",8) || !strncmp(source,"License: ",9)){
        if(result->d){qStringDestroy(result);result->d=NULL;}
        char line[4096];snprintf(line,sizeof(line),"%s%s",source[0]=='V'?"版本 ":"许可证：",source+(source[0]=='V'?8:9));
        qFromUtf8(result,line,(int)strlen(line));return 1;
    }
    if(result->d){qStringDestroy(result);result->d=NULL;}
    char normalized[4096];size_t used=0;char accel=0;
    for(const char *p=source;*p && used<sizeof(normalized)-5;){
        if(!strncmp(p,"&nbsp;",6)){normalized[used++]=' ';p+=6;}
        else if(!strncmp(p,"&#160;",6)){normalized[used++]=' ';p+=6;}
        else if(!strncmp(p,"&hellip;",8)){memcpy(normalized+used,"...",3);used+=3;p+=8;}
        else if(!strncmp(p,"\xE2\x80\xA6",3)){memcpy(normalized+used,"...",3);used+=3;p+=3;}
        else if(!strncmp(p,"\xC2\xA0",2)){normalized[used++]=' ';p+=2;}
        else if(*p=='&' && isalpha((unsigned char)p[1])){accel=(char)toupper((unsigned char)p[1]);p++;}
        else {unsigned char c=(unsigned char)*p++;if(isspace(c))c=' ';if(c==' ' && (!used || normalized[used-1]==' '))continue;normalized[used++]=(char)tolower(c);}
    }
    while(used && normalized[used-1]==' ')used--;
    int ellipsis=used>=3 && !memcmp(normalized+used-3,"...",3);
    int colon=used && normalized[used-1]==':';
    while(used && (normalized[used-1]==' '||normalized[used-1]=='.'||normalized[used-1]==':'))used--;
    normalized[used]=0;
    char key[4200];snprintf(key,sizeof(key),"__tec_zh_norm__:%s",normalized);
    if(!translated(key,result))return 0;
    char value[8192];int n=stringUtf8(result,value,sizeof(value));
    if(accel && n<(int)sizeof(value)-16)n+=snprintf(value+n,sizeof(value)-n,"(&%c)",accel);
    if(ellipsis && n<(int)sizeof(value)-8){memcpy(value+n,"...",4);n+=3;}
    else if(colon && n<(int)sizeof(value)-8){memcpy(value+n,"\xEF\xBC\x9A",4);n+=3;}
    qStringDestroy(result);qFromUtf8(result,value,n);return 1;
}
static int translateHtml(const char *source,QString *result){
    if(!strchr(source,'<'))return 0;
    char output[32768];size_t used=0;int changes=0;
    const char *p=source;
    while(*p && used<sizeof(output)-1){
        const char *end;
        if(*p=='<'){
            end=strchr(p,'>');if(!end)return 0;end++;
        }else{
            end=strchr(p,'<');if(!end)end=p+strlen(p);
        }
        size_t n=(size_t)(end-p);
        char part[4096],replacement[4096];const char *copy=p;size_t size=n;
        if(*p!='<' && n>0 && n<sizeof(part)){
            memcpy(part,p,n);part[n]=0;QString word={0};
            if(translatedTrimmed(part,&word)){
                size=(size_t)stringUtf8(&word,replacement,sizeof(replacement));
                copy=replacement;changes++;
            }
            if(word.d)qStringDestroy(&word);
        }
        if(used+size>=sizeof(output))return 0;
        memcpy(output+used,copy,size);used+=size;p=end;
    }
    if(!changes)return 0;output[used]=0;qFromUtf8(result,output,(int)used);return 1;
}
/* Only presentation properties are changed. Never write editable fields,
   combo/list item models, dataset names, plot annotations, or file paths. */
static void translateProperty(void *obj,const char *name){
    QVariant value={0};QString source={0},result={0};char text[16384];
    qProperty(obj,&value,name);qToString(&value,&source);
    int n=stringUtf8(&source,text,sizeof(text));
    int found=n>0 && n<512 && translatedTrimmed(text,&result);
    if(!found && n>0 && (qInherits(obj,"QLabel")||qInherits(obj,"QQuickText")) && !strcmp(name,"text")){
        if(result.d){qStringDestroy(&result);result.d=NULL;}
        found=translateHtml(text,&result);
    }
    if(!found && n>0)recordMissing(name,text);
    if(found){
        QVariant replacement={0};qVariantCtor(&replacement,&result);
        if(qSetProperty(obj,name,&replacement))changed++;
        qVariantDestroy(&replacement);
    }
    if(result.d)qStringDestroy(&result);
    qStringDestroy(&source);qVariantDestroy(&value);
}
static void translateActionChildren(void *obj,int depth){
    if(depth>24)return;
    const QList *children=qChildren(obj);
    if(!children||!children->d)return;
    ListData *d=children->d;int n=d->end-d->begin;
    if(n<1||n>10000)return;
    void **snapshot=malloc((size_t)n*sizeof(void*));if(!snapshot)return;
    memcpy(snapshot,d->array+d->begin,(size_t)n*sizeof(void*));
    for(int i=0;i<n;i++){
        void *child=snapshot[i];
        if(qInherits(child,"QAction")){
            translateProperty(child,"text");translateProperty(child,"toolTip");
            translateProperty(child,"statusTip");
        }else if(!qInherits(child,"QWidget")){
            if(qInherits(child,"QQuickText"))translateProperty(child,"text");
            translateActionChildren(child,depth+1);
        }
    }
    free(snapshot);
}
static void translateQuickTree(void *obj,int depth){
    if(!obj||depth>32)return;
    translateProperty(obj,"text");translateProperty(obj,"title");
    if(!qQuickChildren)return;
    QList children={0};qQuickChildren(obj,&children);ListData *d=children.d;
    if(d && d->end>=d->begin && d->end-d->begin<10000)
        for(int i=d->begin;i<d->end;i++)translateQuickTree(d->array[i],depth+1);
    if(d && d->ref>=0 && InterlockedDecrement(&d->ref)==0)qListDispose(d);
}
static void translateTabs(void *w){
    for(int i=0,n=qTabCount(w);i<n && i<100;i++){
        QString src={0},dst={0};char text[4096];qTabText(w,&src,i);
        if(stringUtf8(&src,text,sizeof(text))){
            if(translatedTrimmed(text,&dst)){qTabSetText(w,i,&dst);changed++;}
            else recordMissing("TAB",text);
        }
        if(dst.d)qStringDestroy(&dst);qStringDestroy(&src);
    }
}
static void inspectCombo(void *w){
    int n=qComboCount(w);if(n<1||n>100)return;
    char all[4096]={0};size_t used=0;
    for(int i=0;i<n;i++){
        QString src={0};char text[512];qComboText(w,&src,i);stringUtf8(&src,text,sizeof(text));qStringDestroy(&src);
        int bytes=snprintf(all+used,sizeof(all)-used,"%s%s",i?" | ":"",text);
        if(bytes<0||(size_t)bytes>=sizeof(all)-used)break;used+=(size_t)bytes;
    }
    recordMissing("COMBO",all);
}

/* Translate only the text handed to Qt's widget painter. The QAction, model,
   combo-box item and dataset strings remain unchanged. IAT entries are scoped
   to Qt Widgets and Tecplot dialog modules; no on-disk vendor binary is edited. */
typedef void (*DrawRect)(void*,const void*,int,const QString*,void*);
typedef void (*DrawOption)(void*,const void*,const QString*,const void*);
static DrawRect drawRectOriginal,drawRectFOriginal;
static DrawOption drawOptionOriginal;
static void *(*painterDevice)(const void*);
static void *(*parentWidget)(const void*);
static int paintingTranslation;
static void stripPaintMnemonic(QString *s){
    char text[8192];int n=stringUtf8(s,text,sizeof(text));int changedText=0;
    for(char*p=text;*p;){
        if(p[0]=='(' && p[1]=='&' && isalpha((unsigned char)p[2]) && p[3]==')'){
            memmove(p,p+4,strlen(p+4)+1);changedText=1;
        }else p++;
    }
    if(changedText){qStringDestroy(s);qFromUtf8(s,text,(int)strlen(text));}
}
static void *(*viewItemDelegate)(const void*);
static QString *(*displayTextOriginal)(const void*,QString*,const QVariant*,const void*);
static QString *displayTextLocalized(const void*self,QString*out,const QVariant*value,const void*locale){
    displayTextOriginal(self,out,value,locale);
    char text[4096];QString translatedText={0};
    if(translator && stringUtf8(out,text,sizeof(text)) && translatedTrimmed(text,&translatedText)){
        stripPaintMnemonic(&translatedText);qStringDestroy(out);*out=translatedText;translatedText.d=NULL;
    }
    if(translatedText.d)qStringDestroy(&translatedText);return out;
}
static void (*drawDisplayOriginal)(const void*,void*,const void*,const void*,const QString*);
static void drawDisplayLocalized(const void*self,void*painter,const void*option,const void*rect,const QString*text){
    QString t={0};char source[4096];int yes=0;
    if(translator && stringUtf8(text,source,sizeof(source)))yes=translatedTrimmed(source,&t);
    if(yes)stripPaintMnemonic(&t);else recordMissing("LEGACY_ITEM",source);
    drawDisplayOriginal(self,painter,option,rect,yes?&t:text);if(t.d)qStringDestroy(&t);
}
static void localizeItemDelegate(void *view){
    void *delegate=viewItemDelegate(view);
    if(!delegate||(!qInherits(delegate,"QStyledItemDelegate")&&!qInherits(delegate,"QItemDelegate")))return;
    void **table=*(void***)delegate;
    for(int i=0;i<40;i++){
        if(table[i]==(void*)displayTextLocalized||table[i]==(void*)drawDisplayLocalized)return;
        void *replacement=table[i]==(void*)displayTextOriginal?(void*)displayTextLocalized:table[i]==(void*)drawDisplayOriginal?(void*)drawDisplayLocalized:NULL;
        if(replacement){
            DWORD previous,ignored;
            if(VirtualProtect(&table[i],sizeof(void*),PAGE_READWRITE,&previous)){
                InterlockedExchangePointer((PVOID volatile*)&table[i],replacement);
                VirtualProtect(&table[i],sizeof(void*),previous,&ignored);
            }
            return;
        }
    }
}
static int isPresentationPainter(void *painter){
    void *device=painterDevice(painter),*widget=NULL;
    QList widgets={0};qAllWidgets(&widgets);ListData *d=widgets.d;
    /* Qt 5.15.2 x64: QObject base is 16 bytes, followed by QPaintDevice. */
    if(d && d->end>=d->begin && d->end-d->begin<50000)
        for(int i=d->begin;i<d->end;i++)if((char*)d->array[i]+16==(char*)device){widget=d->array[i];break;}
    if(d && d->ref>=0 && InterlockedDecrement(&d->ref)==0)qListDispose(d);
    if(!widget)return 0;
    if(!paintingTranslation){
        void *top=widget,*parent=NULL;int level=0;
        while((parent=parentWidget(top)) && level++<12)top=parent;
        if(qInherits(top,"QDialog")){paintingTranslation=1;translateProperty(top,"windowTitle");paintingTranslation=0;}
    }
    if(qInherits(widget,"QLineEdit")||qInherits(widget,"QTextEdit")||qInherits(widget,"QPlainTextEdit"))return 0;
    if(qInherits(widget,"QMenu")||qInherits(widget,"QMenuBar")||qInherits(widget,"QAbstractButton")||
       qInherits(widget,"QLabel")||qInherits(widget,"QGroupBox")||qInherits(widget,"QTabBar")||
       qInherits(widget,"QHeaderView")||qInherits(widget,"QComboBox"))return 1;
    void *parent=parentWidget(widget);
    if(parent && qInherits(parent,"QHeaderView"))return 1;
    /* Static choice lists and zone-style tables render through a viewport.
       Translation is display-only, even if a user-defined item matches a term. */
    if(parent && (qInherits(parent,"QAbstractItemView"))){localizeItemDelegate(parent);return 1;}
    return 0;
}
static int paintTranslation(void*p,const QString*s,QString*out){
    if(paintingTranslation||!translator||!isPresentationPainter(p))return 0;
    char text[4096];if(!stringUtf8(s,text,sizeof(text)))return 0;
    paintingTranslation=1;int ok=translatedTrimmed(text,out);paintingTranslation=0;
    if(ok)stripPaintMnemonic(out);else recordMissing("PAINT",text);
    return ok;
}
static void drawRectLocalized(void*p,const void*r,int flags,const QString*s,void*b){
    QString t={0};int yes=paintTranslation(p,s,&t);
    drawRectOriginal(p,r,flags,yes?&t:s,b);if(t.d)qStringDestroy(&t);
}
static void drawRectFLocalized(void*p,const void*r,int flags,const QString*s,void*b){
    QString t={0};int yes=paintTranslation(p,s,&t);
    drawRectFOriginal(p,r,flags,yes?&t:s,b);if(t.d)qStringDestroy(&t);
}
static void drawOptionLocalized(void*p,const void*r,const QString*s,const void*o){
    QString t={0};int yes=paintTranslation(p,s,&t);
    drawOptionOriginal(p,r,yes?&t:s,o);if(t.d)qStringDestroy(&t);
}
static void replaceUiPaintImports(HMODULE h){
    if(!h)return;unsigned char *base=(unsigned char*)h;
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return;
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);
    DWORD offset=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if(!offset)return;
    IMAGE_IMPORT_DESCRIPTOR *desc=(IMAGE_IMPORT_DESCRIPTOR*)(base+offset);
    for(;desc->Name;desc++){
        if(_stricmp((char*)base+desc->Name,"Qt5Gui.dll") && _stricmp((char*)base+desc->Name,"Qt5Widgets.dll"))continue;
        IMAGE_THUNK_DATA64 *slot=(IMAGE_THUNK_DATA64*)(base+desc->FirstThunk);
        for(;slot->u1.Function;slot++){
            void *old=(void*)(uintptr_t)slot->u1.Function,*replacement=NULL;
            if(old==(void*)drawRectOriginal)replacement=(void*)drawRectLocalized;
            else if(old==(void*)drawRectFOriginal)replacement=(void*)drawRectFLocalized;
            else if(old==(void*)drawOptionOriginal)replacement=(void*)drawOptionLocalized;
            else if(old==(void*)drawDisplayOriginal)replacement=(void*)drawDisplayLocalized;
            if(replacement){DWORD previous,ignored;
                if(VirtualProtect(&slot->u1.Function,sizeof(void*),PAGE_READWRITE,&previous)){
                    InterlockedExchangePointer((PVOID volatile*)&slot->u1.Function,replacement);
                    VirtualProtect(&slot->u1.Function,sizeof(void*),previous,&ignored);
                }
            }
        }
    }
}
static void installPaintTranslations(void){
    static const wchar_t *modules[]={L"Qt5Widgets.dll",L"qwindowsvistastyle.dll",L"tpsdkdialogs_common_qt.dll",L"tpsdkdialogs_data_qt.dll",
      L"tpsdkdialogs_fieldplot_qt.dll",L"tpsdkdialogs_io_qt.dll",L"tpsdkdialogs_lineplot_qt.dll",
      L"tpsdkdialogs_misc_qt.dll",L"tpsdkdialogs_prompt_qt.dll",L"tpsdkdialogs_scripting_qt.dll",
      L"tpsdkdialogs_view_qt.dll",L"tpsdkdialogs_zonestyle_qt.dll",L"tpsdkdialogsx_qt.dll",
      L"tpsdkolddialogs_qt.dll",L"tpsdkuicommon_qt.dll",L"tpsdkpropertybindings_qt.dll"};
    for(unsigned i=0;i<sizeof(modules)/sizeof(modules[0]);i++)replaceUiPaintImports(GetModuleHandleW(modules[i]));
}
static unsigned char timerCallback(intptr_t unused){
    (void)unused;
    if(!translator)return 1;
    installPaintTranslations();
    if(!qQuickRoot){
        HMODULE quick=GetModuleHandleW(L"Qt5QuickWidgets.dll");
        if(quick)*(FARPROC*)&qQuickRoot=GetProcAddress(quick,"?rootObject@QQuickWidget@@QEBAPEAVQQuickItem@@XZ");
        HMODULE scene=GetModuleHandleW(L"Qt5Quick.dll");
        if(scene)*(FARPROC*)&qQuickChildren=GetProcAddress(scene,"?childItems@QQuickItem@@QEBA?AV?$QList@PEAVQQuickItem@@@@XZ");
    }
    QList widgets={0};qAllWidgets(&widgets);
    ListData *d=widgets.d;
    if(d && d->end>=d->begin && d->end-d->begin<50000){
        for(int i=d->begin;i<d->end;i++){
            void *w=d->array[i];
            if(qInherits(w,"QAbstractItemView"))localizeItemDelegate(w);
            if(diagnostics && qInherits(w,"QComboBox"))inspectCombo(w);

            if(qInherits(w,"QLabel"))translateProperty(w,"text");
            translateProperty(w,"windowTitle");
            translateProperty(w,"toolTip");
            translateProperty(w,"statusTip");
            if(qQuickRoot && qInherits(w,"QQuickWidget")){
                void *root=qQuickRoot(w);
                if(root){translateActionChildren(root,0);translateQuickTree(root,0);}
            }
        }
    }
    if(d && d->ref>=0 && InterlockedDecrement(&d->ref)==0)qListDispose(d);
    static int logged=0;
    if(changed && !logged){char s[160];snprintf(s,sizeof(s),"Qt translation active; first UI pass updated %lu presentation properties.",changed);logmsg(s);logged=1;}
    return 1;
}
static bool supportedVersion(HMODULE file,DWORD ms,DWORD ls){
    WCHAR exe[MAX_PATH];GetModuleFileNameW(file,exe,MAX_PATH);
    DWORD size=GetFileVersionInfoSizeW(exe,NULL);
    void *buffer=malloc(size);if(!buffer)return false;
    VS_FIXEDFILEINFO *info=NULL;UINT len=0;bool ok=false;
    if(GetFileVersionInfoW(exe,0,size,buffer)&&VerQueryValueW(buffer,L"\\",(LPVOID*)&info,&len))
        ok=info->dwFileVersionMS==ms&&info->dwFileVersionLS==ls;
    free(buffer);return ok;
}
__declspec(dllexport) void InitTecAddOn113(void){
    if(initialized++)return;
    char diag[8]={0};diagnostics=GetEnvironmentVariableA("TECPLOT_ZH_CN_DIAGNOSTICS",diag,sizeof(diag)) && !strcmp(diag,"1");
    WCHAR own[MAX_PATH];GetModuleFileNameW(module,own,MAX_PATH);
    WCHAR *slash=wcsrchr(own,L'\\');if(!slash)return;slash[1]=0;
    WCHAR logpath[MAX_PATH];swprintf(logpath,MAX_PATH,L"%lstecplot_zh_CN.log",own);
    logfile=_wfopen(logpath,L"w");
    logmsg("Tecplot 2025 R1 Simplified Chinese UI add-on 0.3.0");
    if(!supportedVersion(NULL,MAKELONG(1,2025),MAKELONG(6865,0))){logmsg("Unsupported host version; no UI changes made.");return;}
    HMODULE core=GetModuleHandleW(L"Qt5Core.dll"),widgets=GetModuleHandleW(L"Qt5Widgets.dll");
    HMODULE sdk=GetModuleHandleW(L"tpsdkbase.dll"),events=GetModuleHandleW(L"tp360event.dll");
    if(!core||!supportedVersion(core,MAKELONG(15,5),MAKELONG(0,2))){logmsg("Unsupported Qt runtime; no UI changes made.");return;}
    RESOLVE(qFromUtf8,core,"?fromUtf8_helper@QString@@CA?AV1@PEBDH@Z");
    RESOLVE(qStringDestroy,core,"??1QString@@QEAA@XZ");
    RESOLVE(qTranslatorCtor,core,"??0QTranslator@@QEAA@PEAVQObject@@@Z");
    RESOLVE(qTranslatorLoad,core,"?load@QTranslator@@QEAA_NAEBVQString@@000@Z");
    RESOLVE(qInstallTranslator,core,"?installTranslator@QCoreApplication@@SA_NPEAVQTranslator@@@Z");
    RESOLVE(qTranslate,core,"?translate@QTranslator@@UEBA?AVQString@@PEBD00H@Z");
    HMODULE gui=GetModuleHandleW(L"Qt5Gui.dll");
    RESOLVE(painterDevice,gui,"?device@QPainter@@QEBAPEAVQPaintDevice@@XZ");
    RESOLVE(parentWidget,widgets,"?parentWidget@QWidget@@QEBAPEAV1@XZ");
    RESOLVE(drawRectOriginal,gui,"?drawText@QPainter@@QEAAXAEBVQRect@@HAEBVQString@@PEAV2@@Z");
    RESOLVE(drawRectFOriginal,gui,"?drawText@QPainter@@QEAAXAEBVQRectF@@HAEBVQString@@PEAV2@@Z");
    RESOLVE(drawOptionOriginal,gui,"?drawText@QPainter@@QEAAXAEBVQRectF@@AEBVQString@@AEBVQTextOption@@@Z");
    RESOLVE(drawDisplayOriginal,widgets,"?drawDisplay@QItemDelegate@@MEBAXPEAVQPainter@@AEBVQStyleOptionViewItem@@AEBVQRect@@AEBVQString@@@Z");
    RESOLVE(viewItemDelegate,widgets,"?itemDelegate@QAbstractItemView@@QEBAPEAVQAbstractItemDelegate@@XZ");
    RESOLVE(displayTextOriginal,widgets,"?displayText@QStyledItemDelegate@@UEBA?AVQString@@AEBVQVariant@@AEBVQLocale@@@Z");
    RESOLVE(qComboCount,widgets,"?count@QComboBox@@QEBAHXZ");
    RESOLVE(qComboText,widgets,"?itemText@QComboBox@@QEBA?AVQString@@H@Z");
    RESOLVE(qComboSetText,widgets,"?setItemText@QComboBox@@QEAAXHAEBVQString@@@Z");
    RESOLVE(qTabCount,widgets,"?count@QTabBar@@QEBAHXZ");
    RESOLVE(qTabText,widgets,"?tabText@QTabBar@@QEBA?AVQString@@H@Z");
    RESOLVE(qTabSetText,widgets,"?setTabText@QTabBar@@QEAAXHAEBVQString@@@Z");
    RESOLVE(qBlockSignals,core,"?blockSignals@QObject@@QEAA_N_N@Z");
    RESOLVE(qAllWidgets,widgets,"?allWidgets@QApplication@@SA?AV?$QList@PEAVQWidget@@@@XZ");
    RESOLVE(qActions,widgets,"?actions@QWidget@@QEBA?AV?$QList@PEAVQAction@@@@XZ");
    RESOLVE(qChildren,core,"?children@QObject@@QEBAAEBV?$QList@PEAVQObject@@@@XZ");
    RESOLVE(qListDispose,core,"?dispose@QListData@@SAXPEAUData@1@@Z");
    RESOLVE(qInherits,core,"?inherits@QObject@@QEBA_NPEBD@Z");
    RESOLVE(qProperty,core,"?property@QObject@@QEBA?AVQVariant@@PEBD@Z");
    RESOLVE(qSetProperty,core,"?setProperty@QObject@@QEAA_NPEBDAEBVQVariant@@@Z");
    RESOLVE(qToString,core,"?toString@QVariant@@QEBA?AVQString@@XZ");
    RESOLVE(qVariantCtor,core,"??0QVariant@@QEAA@AEBVQString@@@Z");
    RESOLVE(qVariantDestroy,core,"??1QVariant@@QEAA@XZ");
    RESOLVE(lockOn,sdk,"TecUtilLockOn");RESOLVE(lockOff,sdk,"TecUtilLockOff");
    RESOLVE(registerAddon,sdk,"TecUtilAddOnRegister");
    RESOLVE(lockIsOn,sdk,"TecUtilLockIsOn");
    RESOLVE(addTimer,events,"TecUtilTimerAddCallback");
    logmsg("Host version and exported API checks passed.");
    char path[MAX_PATH*3];WCHAR qmpath[MAX_PATH];swprintf(qmpath,MAX_PATH,L"%lstecplot_zh_CN.qm",own);
    WideCharToMultiByte(CP_UTF8,0,qmpath,-1,path,sizeof(path),NULL,NULL);
    QString filename={0},empty={0};qFromUtf8(&filename,path,(int)strlen(path));qFromUtf8(&empty,"",0);
    logmsg("Catalog filename prepared.");
    translator=calloc(1,512);if(!translator){logmsg("Allocation failed.");return;}
    qTranslatorCtor(translator,NULL);
    logmsg("QTranslator constructed.");
    bool loaded=qTranslatorLoad(translator,&filename,&empty,&empty,&empty);
    qStringDestroy(&filename);qStringDestroy(&empty);
    if(!loaded){logmsg("Catalog load failed; no UI changes made.");translator=NULL;return;}
    logmsg("Catalog loaded.");
    qInstallTranslator(translator);
    installPaintTranslations();
    logmsg("Translator installed.");
    lockOn();registerAddon(100,"Simplified Chinese UI (2025 R1)","0.3.0 - 2026-10-22","Local user customization");
    unsigned char timer=addTimer(750,0,timerCallback);lockOff();
    logmsg(timer?"Catalog installed and UI timer registered.":"Catalog installed; timer unavailable.");
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID reserved){
    (void)reserved;if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);}return TRUE;
}
