## Approach
- Read existing files before writing. Don't re-read unless changed.
- Thorough in reasoning, concise in output.
- Skip files over 100KB unless required.
- No sycophantic openers or closing fluff.
- No emojis or em-dashes.
- Do not guess APIs, versions, flags, commit SHAs, or package names. Verify by reading code or docs before asserting.

## Project: Santos Leveler
VST3 voice auto level rider for Windows x64, C++17 with JUCE 8.0.12, AGPL-3.0-only. Current version: 1.0.3. The owner is a video editor, not a programmer: explain things in plain Spanish, step by step, without jargon.

### Layout
- `Source/`: engine (`LevelerEngine.h`), compressor, True Peak Limiter, loudness meter and the plugin processor. All DSP is header-only.
- `Source/UI/`: editor look. `Theme.h` holds the colours (single violet accent) and the design size 1310 x 640. `LookAndFeel.h`, `Controls.h`, `Displays.h`, `HeaderButtons.h` hold the widgets.
- `Tests/LevelerDSPTests.cpp`: DSP tests with `assert`, built so they stay active in Release.
- Signal path: input, Rider, Leveler Out trim, compressor, True Peak Limiter, latency-aligned bypass, meters.

### Build and test
- Quick DSP test, no JUCE needed: `g++ -std=c++17 -O2 -ISource Tests/LevelerDSPTests.cpp -o dsptests && ./dsptests`.
- Full build on Windows: `build-windows.ps1` (targets `SantosLeveler_VST3` and `SantosLevelerDSPTests`). CI (`.github/workflows/build-windows-vst3.yml`) builds and tests on every push to `main` and on pull requests.
- The editor can be rendered on Linux to check the look: build a small console target that links `SantosLeveler`, create the editor and call `createComponentSnapshot` under `xvfb-run`.

### Things that bite
- A parameter is listed in three places: the layout and `abParameterIds` in `PluginProcessor.cpp`, and the preset arrays in `HeaderButtons.h`. Change all three together.
- The audio thread must not allocate. The integrated LUFS uses a fixed histogram for that reason.
- The limiter lookahead is 2 ms (96 samples at 48 kHz) and a test asserts it. If it changes, update the test, the README and the changelog.
- Any change to compressor, limiter or Peak 2 needs a measurement and a regression test, not only a listen.
- The window keeps a fixed aspect ratio (min 655 x 320, max 2620 x 1280).

### Releasing a version
1. Raise `VERSION` in `CMakeLists.txt`. The plugin footer, the CI artifact name and the build script read it from there.
2. Add the entry to `CHANGELOG.md` and update both READMEs (`README.md`, `README_EN.md`).
3. Open a pull request, wait for the CI, merge.
4. Download the CI artifact `Santos-Leveler-vX.Y.Z-Windows-x64-VST3`. It must be uploaded to the release as `Santos-Leveler-vX.Y.Z-Windows-x64-VST3.zip`, the name the README links to.
5. In the GitHub web: Releases, Draft a new release, new tag `vX.Y.Z` on `main`, paste the notes as raw markdown, attach the zip, publish. Claude cannot push tags or create releases from its sessions, so this step is manual.

### Working rules
- Never edit `README.md` in the GitHub web editor: it was overwritten once by mistake. Release notes go in the release description, not in the README.
- Work on a new branch from `main` and open a pull request. Do not delete the old `feature/*` history without asking: those branches were removed by the owner on 2026-10-05.
- Do not guess about GitHub settings or the owner's local machine. Say what cannot be done from here and give the manual steps.

### Pending, optional
- Re-tune the factory presets after the compressor change in 1.0.3 (values were not changed).
- Redo the PDF manuals: the v1.0.0 manuals describe the previous interface (they are release assets, not repository files).
- The v1.0.3 release notes on GitHub have no formatting; the formatted text is in `CHANGELOG.md`.
