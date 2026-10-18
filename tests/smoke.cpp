#include "GazerDSP.h"
#include <vector>
#include <iostream>
#include <cmath>
int main(){
  mpcgazer::GazerDSP fx(48000);
  mpcgazer::Parameters p; p.predelayMs=0.f; p.wet=.7f; fx.setParameters(p);
  constexpr int count=96000;
  std::vector<float> left(count,0),right(count,0),outL(count),outR(count);
  left[0]=right[0]=.7f;
  for(int i=0;i<count;i+=128) fx.process(left.data()+i,right.data()+i,outL.data()+i,outR.data()+i,128);
  float sum=0;
  for(int i=0;i<count;++i) { if(!std::isfinite(outL[i])||!std::isfinite(outR[i])) return 2; sum+=std::abs(outL[i])+std::abs(outR[i]); }
  std::cout<<"MPC Gazer DSP smoke test passed. Output magnitude: "<<sum<<"\n";
  return sum>0 ? 0:1;
}
