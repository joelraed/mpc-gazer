# MPC Gazer 0.1 — standalone DSP prototype

Independent, Stargazer-inspired stereo granular shimmer/delay engine in C++17.

**This is DSP prototype source, not an installable MPC plugin.** It has no VST2 wrapper, Akai skin assets, registration entry, or ARM build. It is not derived from Linda Audio's proprietary DLL.

## Implemented
- Stereo multi-delay (2–13 lines)
- Overlapping-window simple granular pitch shifter (+/-12 semitones)
- Pitch feedback, predelay, size, feedback, damping and high-pass approximation
- LFO-modulated taps, stereo width, organ-like tonal coloration and dry/wet mix
- Basic smoke test

## Build/test on Linux, macOS or WSL
```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Roadmap
1. Integrate a VST2-compatible effect wrapper with a verified MPC host ABI.
2. Cross-compile for original MPC One 32-bit ARM Linux.
3. Generate MPC UI skin, parameters and registration.
4. Test on HAKAI 3.9.0 hardware and benchmark CPU use.
5. Improve pitch quality, diffusion and presets.

The pitch shifter is experimental and has not undergone hardware or listening tests.

Reference projects: https://github.com/sunskiefer/GAIAXXL and https://github.com/sd88me/mpc-vst-plugins
