#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

namespace mpcgazer {
struct Parameters {
  float size = 0.55f;
  float predelayMs = 35.f;
  float feedback = 0.72f;
  float damping = 0.35f;
  float highpass = 0.15f;
  float shimmer = 0.45f;
  float grainMs = 95.f;
  float grainFeedback = 0.30f;
  float pitchSemitones = 12.f;
  float attackMs = 180.f;
  float modulation = 0.2f;
  float width = 0.9f;
  float organize = 0.f;
  float wet = 0.45f;
  int lines = 8;
};
class GazerDSP {
public:
  explicit GazerDSP(float sampleRate = 48000.f);
  void prepare(float sampleRate);
  void reset();
  void setParameters(const Parameters& params);
  const Parameters& parameters() const { return p_; }
  void process(const float* inL, const float* inR, float* outL, float* outR, std::size_t frames);
private:
  struct Delay {
    std::vector<float> ring;
    std::size_t pos = 0;
    float low = 0.f, hpLow = 0.f;
  };
  struct Pitch {
    std::vector<float> ring;
    std::size_t write = 0;
    float phase = 0.f;
  };
  float sr_ = 48000.f;
  Parameters p_{};
  Delay lines_[13][2];
  Pitch pitch_[2];
  struct Diffuser { std::vector<float> ring; std::size_t pos=0; };
  Diffuser diffusers_[2][2];
  std::vector<float> predelay_[2];
  std::size_t prePos_ = 0;
  float shiftFeedback_[2] = {0.f, 0.f};
  float wetSmooth_ = 0.45f;
  float attackEnv_ = 0.f;
  float lfo_ = 0.f;
  float organLow_[2] = {0.f, 0.f};
  float pitchProcess(Pitch& p, float input, float semitones, float grainMs);
};
}
