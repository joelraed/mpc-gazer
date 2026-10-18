#include "GazerDSP.h"
#include <vector>
#include <cmath>
#include <iostream>
int main() {
 constexpr int n=192000,block=128;
 std::vector<float> l(n),r(n),a(n),b(n);
 l[0]=r[0]=0.8f;
 mpcgazer::GazerDSP fx(48000.f);
 mpcgazer::Parameters p;
 p.size=.75f;p.feedback=.67f;p.shimmer=.25f;p.grainFeedback=.20f;
 p.damping=.45f;p.modulation=.15f;p.wet=1.f;
 fx.setParameters(p);
 for(int i=0;i<n;i+=block) fx.process(l.data()+i,r.data()+i,a.data()+i,b.data()+i,block);
 double tail=0;
 for(int i=24000;i<n;++i) {
   if(!std::isfinite(a[i])||!std::isfinite(b[i])||std::abs(a[i])>20||std::abs(b[i])>20) return 2;
   tail+=std::abs(a[i])+std::abs(b[i]);
 }
 std::cout<<"Celestial Glow tail energy: "<<tail<<"\n";
 return tail>1e-5 ? 0:1;
}
