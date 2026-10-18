# MPC Gazer standalone for original MPC One (HAKAI 3.9.0)

This branch is **experimental**. Its ARMv7 `.so` is **not a complete MPC install package**.

## Build

On a machine with Docker and ARMv7 emulation enabled:

```sh
bash scripts/build-mpc-arm.sh
```

The build uses `arm32v7/gcc:11-bullseye` to avoid dependence on too-new glibc versions. It produces `dist/MPCGazer.so` and checks the ARM ELF header and VSTPluginMain symbol.

On GitHub, use Actions -> **Build MPC One standalone (experimental)** and download the ARMv7 artifact when the job succeeds.

## Remaining for a safe install

1. Confirm the effect's VST2 dispatcher, category, parameter descriptions and program state are accepted by MPC OS 3.9.0 on a *test project*.
2. Generate a matching MPC skin, including `version.xml`, `Plugin Skins/TUI.json` and `Q-Links.json`, using the [mpc-vst-plugins](https://github.com/sd88me/mpc-vst-plugins) tools.
3. Use their packager and installer to back up and register the plugin in `MPC.settings` (do not manually replace another synth's files).
4. Test for audio dropouts at 44.1 kHz / 128-frame blocks and measure CPU utilization on the original MPC One.
5. Confirm parameter recall across save/reopen, plus bypass and correct stereo behavior.

**Do not copy the Mac .dylib to the MPC.** MPC One needs an ARM Linux .so.

**Do not install raw binaries or edit MPC.settings yet.** Existing release workflow does not produce a verified skin + installer ZIP. Keep project and device backups.

## Security and license

Only install to a modded MPC with root access, on a trusted network. This is independent DSP, not a binary conversion of Linda Audio Stargazer.
