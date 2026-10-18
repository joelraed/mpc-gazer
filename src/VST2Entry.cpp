#include "GazerDSP.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <new>
extern "C" {
struct AEffect;
using VstIntPtr=intptr_t;
using AudioMaster=VstIntPtr(*)(AEffect*,int32_t,int32_t,VstIntPtr,void*,float);
using Dispatcher=VstIntPtr(*)(AEffect*,int32_t,int32_t,VstIntPtr,void*,float);
using Process=void(*)(AEffect*,float**,float**,int32_t);
using SetParam=void(*)(AEffect*,int32_t,float);
using GetParam=float(*)(AEffect*,int32_t);
struct AEffect {
 int32_t magic; Dispatcher dispatcher; Process process; SetParam setParameter; GetParam getParameter;
 int32_t numPrograms,numParams,numInputs,numOutputs,flags; void* resvd1;void* resvd2;
 int32_t initialDelay,realQualities,offQualities;float ioRatio;void* object;void* user;
 int32_t uniqueID,version;Process processReplacing;Process processDoubleReplacing;char future[56];
};
}
namespace {
constexpr int N=15;
const char* labels[N]={"Size","PreDelay","Feedback","Damping","Highpass","Shimmer","Grain","Grain FB","Pitch","Attack","Modulate","Width","Organize","Wet","Lines"};
struct Plugin {
 AEffect effect{};
 mpcgazer::GazerDSP dsp;
 float v[N]={.75f,.14f,.713f,.45f,.15f,.25f,.433f,.235f,1.f,.085f,.15f,.9f,0.f,.35f,.545f};
 Plugin():dsp(48000.f){update();}
 void update(){
   mpcgazer::Parameters p;
   p.size=v[0];p.predelayMs=v[1]*250.f;p.feedback=v[2]*.94f;
   p.damping=v[3];p.highpass=v[4];p.shimmer=v[5];
   p.grainMs=30.f+150.f*v[6];p.grainFeedback=.85f*v[7];
   p.pitchSemitones=-12.f+24.f*v[8];p.attackMs=10.f+1990.f*v[9];
   p.modulation=v[10];p.width=v[11];p.organize=v[12];p.wet=v[13];
   p.lines=2+int(v[14]*11.f+.5f);dsp.setParameters(p);
 }
};
Plugin* get(AEffect* e){return e?static_cast<Plugin*>(e->object):nullptr;}
void set(AEffect* e,int32_t i,float value) {
 auto* p=get(e);if(!p||i<0||i>=N||!std::isfinite(value))return;
 p->v[i]=std::max(0.f,std::min(1.f,value));p->update();
}
float getValue(AEffect* e,int32_t i){auto* p=get(e);return p&&i>=0&&i<N?p->v[i]:0.f;}
void process(AEffect* e,float** in,float** out,int32_t count){
 auto* p=get(e);if(!p||count<=0||!in||!out||!in[0]||!in[1]||!out[0]||!out[1])return;
 p->dsp.process(in[0],in[1],out[0],out[1],(size_t)count);
}
VstIntPtr dispatch(AEffect* e,int32_t op,int32_t i,VstIntPtr value,void* ptr,float f) {
 auto* p=get(e);if(!p)return 0;
 switch(op) {
 case 0:return 1; // effOpen
 case 1:delete p;return 1; // effClose
 case 6:if(ptr){((char*)ptr)[0]=0;return 1;}return 0; // param unit
 case 7:if(ptr&&i>=0&&i<N){std::snprintf((char*)ptr,8,"%.2f",p->v[i]);return 1;}return 0;
 case 8:if(ptr&&i>=0&&i<N){std::strncpy((char*)ptr,labels[i],7);((char*)ptr)[7]=0;return 1;}return 0;
 case 10:if(f>=8000.f&&f<=192000.f)p->dsp.prepare(f);return 1;
 case 11:return 1; // block size
 case 12:if(value==0)p->dsp.reset();return 1;
 case 35:return 1; // effect
 case 45:case 47:case 48:if(ptr){std::strncpy((char*)ptr,"MPC Gazer",63);((char*)ptr)[63]=0;return 1;}return 0;
 case 49:return 100;
 default:return 0;
 }
}
}
extern "C" __attribute__((visibility("default"))) AEffect* VSTPluginMain(AudioMaster host){
 if(!host)return nullptr;
 auto* p=new(std::nothrow) Plugin();if(!p)return nullptr;
 AEffect& e=p->effect;
 e.magic=0x56737450;e.dispatcher=dispatch;e.process=process;e.processReplacing=process;
 e.setParameter=set;e.getParameter=getValue;e.numPrograms=1;e.numParams=N;
 e.numInputs=2;e.numOutputs=2;e.flags=1<<4;e.object=p;e.uniqueID=0x4d477a72;e.version=100;
 return &e;
}
