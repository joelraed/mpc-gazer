#include "GazerDSP.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mpcgazer {
namespace {
float clampf(float x, float a, float b) { return std::max(a, std::min(b, x)); }
constexpr float pi = 3.14159265358979323846f;
constexpr int primeTimes[13] = {149, 163, 179, 197, 211, 233, 251, 269, 293, 313, 337, 359, 383};
float tap(const std::vector<float>& r, std::size_t pos, float delay) {
  const int n = (int)r.size();
  float idx = (float)pos - delay;
  while (idx < 0.f) idx += n;
  int i = (int)idx;
  int j = (i + 1) % n;
  float f = idx - i;
  // Four-point Hermite interpolation reduces grain edge aliasing.
  const float y0=r[(std::size_t)((i+n-1)%n)];
  const float y1=r[(std::size_t)i];
  const float y2=r[(std::size_t)j];
  const float y3=r[(std::size_t)((j+1)%n)];
  const float c0=y1;
  const float c1=.5f*(y2-y0);
  const float c2=y0-2.5f*y1+2.f*y2-.5f*y3;
  const float c3=1.5f*(y1-y2)+.5f*(y3-y0);
  return ((c3*f+c2)*f+c1)*f+c0;
}
}
GazerDSP::GazerDSP(float sampleRate) { prepare(sampleRate); }
void GazerDSP::prepare(float sampleRate) {
  if (sampleRate < 8000.f || sampleRate > 192000.f) throw std::invalid_argument("sample rate");
  sr_ = sampleRate;
  const std::size_t maxDelay = (std::size_t)(sr_*2.5f)+64;
  for (int n=0;n<13;++n) for (int ch=0;ch<2;++ch) {
    auto& d=lines_[n][ch]; d.ring.assign(maxDelay,0.f); d.pos=0; d.low=d.hpLow=0;
  }
  for(int stage=0;stage<2;++stage) for(int ch=0;ch<2;++ch) {
    auto& d=diffusers_[stage][ch];
    d.ring.assign((std::size_t)(sr_*(stage==0?0.0053f:0.0117f))+1,0.f);
    d.pos=0;
  }
  for (int ch=0;ch<2;++ch) {
    pitch_[ch].ring.assign((std::size_t)(sr_*.5f)+64,0.f);
    pitch_[ch].write=0; pitch_[ch].phase=0;
    predelay_[ch].assign((std::size_t)(sr_*.3f)+4,0.f);
  }
  reset();
}
void GazerDSP::reset() {
  for (auto& line:lines_) for (auto& d:line) {
    std::fill(d.ring.begin(), d.ring.end(), 0.f);
    d.pos=0; d.low=d.hpLow=0;
  }
  for(auto& p:pitch_) { std::fill(p.ring.begin(),p.ring.end(),0.f); p.write=0; p.phase=0.f; }
  for (auto& p:predelay_) std::fill(p.begin(), p.end(), 0.f);
  for(auto& stage:diffusers_) for(auto& d:stage) {
    std::fill(d.ring.begin(),d.ring.end(),0.f);d.pos=0;
  }
  prePos_=0; shiftFeedback_[0]=shiftFeedback_[1]=0;
  organLow_[0]=organLow_[1]=0; attackEnv_=0; wetSmooth_=p_.wet; lfo_=0;
}
void GazerDSP::setParameters(const Parameters& params) {
  p_=params;
  p_.size=clampf(p_.size,0,1); p_.predelayMs=clampf(p_.predelayMs,0,250);
  p_.feedback=clampf(p_.feedback,0,.94f); p_.damping=clampf(p_.damping,0,1);
  p_.highpass=clampf(p_.highpass,0,1); p_.shimmer=clampf(p_.shimmer,0,1);
  p_.grainMs=clampf(p_.grainMs,30,180); p_.grainFeedback=clampf(p_.grainFeedback,0,.85f);
  p_.pitchSemitones=clampf(p_.pitchSemitones,-12,12); p_.attackMs=clampf(p_.attackMs,10,2000);
  p_.modulation=clampf(p_.modulation,0,1); p_.width=clampf(p_.width,0,1);
  p_.organize=clampf(p_.organize,0,1); p_.wet=clampf(p_.wet,0,1);
  p_.lines=std::max(2,std::min(13,p_.lines));
}
float GazerDSP::pitchProcess(Pitch& p, float input, float semitones, float grainMs) {
  p.ring[p.write]=input;
  float ratio = std::pow(2.f, semitones/12.f);
  float length = std::max(32.f,grainMs*.001f*sr_);
  length = std::min(length,(float)p.ring.size()*.45f);
  float phaseInc = (1.f-ratio)/length;
  p.phase += phaseInc;
  p.phase -= std::floor(p.phase);
  auto grain=[&](float phase) {
    const float window = .5f-.5f*std::cos(2*pi*phase);
    float delay = 16.f+phase*length;
    return tap(p.ring,p.write,delay)*window;
  };
  float a=grain(p.phase);
  float b=grain(std::fmod(p.phase+.5f,1.f));
  p.write=(p.write+1)%p.ring.size();
  return (a+b)*1.41421356f;
}
void GazerDSP::process(const float* inL,const float* inR,float* outL,float* outR,std::size_t frames) {
  if (!inL||!inR||!outL||!outR) throw std::invalid_argument("null channel");
  for(std::size_t f=0;f<frames;++f) {
    const float in[2] = {inL[f],inR[f]};
    float wet[2]={0,0};
    attackEnv_ += (1.f-attackEnv_) * (1.f-std::exp(-1.f/(sr_*p_.attackMs*.001f)));
    lfo_ += .19f/sr_; if(lfo_>=1) lfo_-=1;
    for (int ch=0;ch<2;++ch) {
      auto& pre = predelay_[ch];
      pre[prePos_]=in[ch];
      const float delayed = tap(pre,prePos_,std::max(1.f,p_.predelayMs*.001f*sr_));
      float tonic=delayed;
      organLow_[ch] += .014f * (delayed-organLow_[ch]);
      tonic = (1.f-p_.organize)*tonic+p_.organize*std::tanh(organLow_[ch]*3.f)*.6f;
      // Pitch-shift a filtered ambience, rather than the dry attack. The natural
      // reverberation is the dominant layer; shimmer enters underneath it.
      float shifted=pitchProcess(pitch_[ch], shiftFeedback_[ch]*(.6f+.4f*p_.grainFeedback),
                                  p_.pitchSemitones,p_.grainMs);
      float pitchFeed=clampf(shifted,-1.5f,1.5f)*p_.shimmer*attackEnv_;
      // Two cascaded all-pass diffusers smear attacks before the reverb tank.
      float diffuse=tonic;
      for(int stage=0;stage<2;++stage) {
        auto& ap=diffusers_[stage][ch];
        const float delayed=ap.ring[ap.pos];
        const float g=.67f;
        const float output=delayed-g*diffuse;
        ap.ring[ap.pos]=diffuse+g*output;
        ap.pos=(ap.pos+1)%ap.ring.size();
        diffuse=output;
      }
      float sum=0.f;
      float cross=shiftFeedback_[1-ch]*.06f;
      for(int d=0;d<p_.lines;++d) {
        auto& line=lines_[d][ch];
        float ms=(float)primeTimes[d] * (.5f + p_.size*2.4f);
        float chorus=sr_*.001f*p_.modulation*2.f*std::sin(2*pi*(lfo_+d*.13f+ch*.25f));
        float samples = clampf(ms*.001f*sr_+chorus,1.f,(float)line.ring.size()-3.f);
        float current=tap(line.ring,line.pos,samples);
        float lpCoefficient = .055f + (1.f-p_.damping)*.65f;
        line.low += lpCoefficient*(current-line.low);
        float hpCoefficient=.0002f+p_.highpass*.018f;
        line.hpLow += hpCoefficient*(line.low-line.hpLow);
        float shaped=line.low-line.hpLow;
        float feed = diffuse*.19f + pitchFeed*.075f + cross + shaped*p_.feedback;
        line.ring[line.pos]=std::tanh(feed);
        line.pos=(line.pos+1)%line.ring.size();
        sum += current;
      }
      wet[ch]=std::tanh(sum/(std::sqrt((float)p_.lines)*1.65f));
      shiftFeedback_[ch]=clampf(wet[ch],-1.f,1.f);
    }
    prePos_=(prePos_+1)%predelay_[0].size();
    float mid=(wet[0]+wet[1])*.5f;
    float side=(wet[0]-wet[1])*.5f*p_.width;
    wetSmooth_+=(p_.wet-wetSmooth_)*.003f;
    outL[f]=in[0]*(1.f-wetSmooth_)+(mid+side)*wetSmooth_;
    outR[f]=in[1]*(1.f-wetSmooth_)+(mid-side)*wetSmooth_;
  }
}
}
