#include "GazerDSP.h"
#include "engine.h"
#include <array>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
namespace {
constexpr const char* keys[]={"size","predelay","feedback","damping","highpass","shimmer","grain","grain_fb","pitch","attack","modulate","width","organize","wet","lines"};
constexpr float lo[]={0,0,0,0,0,0,30,0,-12,10,0,0,0,0,2};
constexpr float hi[]={1,250,.94f,1,1,1,180,.85f,12,2000,1,1,1,1,13};
struct Instance {
 mpcgazer::GazerDSP dsp{44100.f};
 std::array<float,15> v{.75f,35.f,.67f,.45f,.15f,.25f,95.f,.20f,12.f,180.f,.15f,.9f,0.f,.35f,8.f};
 float inL[128]{},inR[128]{},outL[128]{},outR[128]{};
 void sync(){
  mpcgazer::Parameters p;
  p.size=v[0];p.predelayMs=v[1];p.feedback=v[2];p.damping=v[3];p.highpass=v[4];
  p.shimmer=v[5];p.grainMs=v[6];p.grainFeedback=v[7];p.pitchSemitones=v[8];
  p.attackMs=v[9];p.modulation=v[10];p.width=v[11];p.organize=v[12];
  p.wet=v[13];p.lines=(int)std::round(v[14]);dsp.setParameters(p);
 }
 Instance(){sync();}
};
int idx(const char* key){for(int i=0;i<15;++i)if(!std::strcmp(keys[i],key))return i;return -1;}
void* create(const char*){return new Instance();}
void destroy(void* v){delete static_cast<Instance*>(v);}
void midi(void*,const uint8_t*,int){}
void set(void* o,const char* key,const char* val) {
 auto* p=static_cast<Instance*>(o);int i=idx(key);if(i<0||!val)return;
 char* end=nullptr;float x=std::strtof(val,&end);
 if(end==val||!std::isfinite(x))return;
 p->v[i]=std::max(lo[i],std::min(hi[i],x));p->sync();
}
int get(void* o,const char* key,char* b,int n) {
 if(!b||n<=0)return 0;
 auto* p=static_cast<Instance*>(o);int i=idx(key);if(i<0)return 0;
 int z=std::snprintf(b,(size_t)n,"%.4f",p->v[i]);return z>0?1:0;
}
void render(void*,int16_t* out,int frames){if(out&&frames>0)std::memset(out,0,(size_t)frames*2*sizeof(int16_t));}
void process(void* o,const int16_t* in,int16_t* out,int frames) {
 if(!in||!out||frames<=0)return;
 auto* p=static_cast<Instance*>(o);
 int done=0;
 while(done<frames){
  int n=std::min(128,frames-done);
  for(int i=0;i<n;++i) {
   p->inL[i]=in[(done+i)*2]/32768.f;p->inR[i]=in[(done+i)*2+1]/32768.f;
  }
  p->dsp.process(p->inL,p->inR,p->outL,p->outR,(size_t)n);
  for(int i=0;i<n;++i){
   auto convert=[](float x)->int16_t {
    if(!std::isfinite(x))return 0;
    return (int16_t)std::lrint(std::max(-1.f,std::min(.999969f,x))*32768.f);
   };
   out[(done+i)*2]=convert(p->outL[i]);out[(done+i)*2+1]=convert(p->outR[i]);
  }
  done+=n;
 }
}
const mpc_engine_t engine={create,destroy,midi,set,get,render,process,nullptr};
}
extern "C" const mpc_engine_t* mpc_engine(void){return &engine;}
