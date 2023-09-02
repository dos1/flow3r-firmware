# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Added
- Added the _Audio Passthrough_ app for toggling audio passthrough through
  line-in/mic to speaker or lineout.
- Added the _w1f1 app_ for managing wifi access, incorporates an in-progress
  cap-touch multi-tap keyboard.
- Added the _Scalar__ app in __Music__ category for playing scales.
- Added configuration of wifi credentials + hostname via _settings.json_
- Added a settings.settings\_loaded flag
- Added wifi preference for apps
- Added task profiler
- Added an error screen to the _Nick_ app when `nick.json` is invalid.
- Added `urequests` support in the simulator.
- Added audio/video media framework, and _Wurzelitzer_ app as a small jukebox
  frontend - currently supporting mp3, mpeg1 and protracker modules.
- Added exporting of built firmwares as part of CI
- graphics: sprite sheet support for ctx.image()
- graphics: ctx.parse() for parsing SVG path data/ctx protocol.
- graphics: raw frame-buffer access in 4,8,16,24 and 32bit graphics modes.
- graphics: clipped overlay buffer
- graphics: allow a graphics state depth of up to 10 (ctx.save() ctx.restore())

### Changed
- Switched the REPL/fatal/disk restart button to right shoulder button.
- Improved performance of the `gr33nhouse` app list by not rendering hidden
  entries and scrolling ones too long to fit on screen.
- Moved the _Clouds_ app to the _Badge_ menu and updated it to use IMU data and
  render in 32bpp graphics mode.
- Added a more sane commandline interface to the simulator.
- More stub functions for the simulator.
- Improved performance of system menus by not rendering hidden entries.
- Added visualization of state, instead of an audio scope in UI of __harmonic
  demo__ and __melodic demo__.
- The system provided scope is now rendered stroked rather than filled.
- Improved BPM tap accuracy in __gay drums__
- Some shell code rewritten to avoid the expensive calls ctx.save\_group and
  ctx.restore\_group().
- overlay graphics is rendered in a separate pass, rate limited
- The entry section in __flow3r.toml__ can now be omitted if the Application
  class is called __App__.
- Improved handedness in buttons

### Fixed
- Fixed _tiny_sampler_ keeping the microphone active after app exit.
- Fixed missing `include/` dir on builds on Darwin.
- Fixed the `time` module in the simulator being broken for apps trying to use it.
- Fixed wrong petal ordering in the simulator.
- Fixed simulator not exiting when closed.
- Fixed _Comic Mono_ missing in the simulator.
- Fixed create nick.json on launch if absent
- Fixed initialization orientation of display (which also fixed incorrect transform of gradients.)
- Fixed cleanup at exit for firmware apps
- Fixed sequencer bug in bl00mbox
=======


## [1.2.0] - 2023-08-18
### Added
- Added a WiFi status indicator icon.
- Added a battery status indicator icon.
- Added support for loading apps from `/sd/apps` and `/flash/apps`.
- Added an error screen when apps fail to start.
- Added Python/st3m API to access battery charge status.
- Added the ability to always hide icons (*System* ➜ *Settings* ➜ *Show Icons*).

### Fixed
- File descriptor leak on app load.  This would lead to the OS crashing when
  too many apps are installed.


## [1.1.1] - 2023-08-17
### Fixed
- Crash on WiFi startup


## [1.1.0] - 2023-08-17

### Added
- Added _Comic Mono_ font.
- Added a setting for automatically connecting to Camp WiFi.
- Added a *System* ➜ *Get Apps* App for downloading apps directly from <https://flow3r.garden/apps/>.
- Added `ApplicationContext.bundle_path` so apps can find out what directory
  they live in.  This should be used for loading app assets from the correct
  path.
- Added TLS support for MicroPython.
- Added modules for `.tar.gz` extraction.
- Added `aioble` module (Bluetooth Low Energy).
- Added line-in support to bl00mbox.
- Added pronoun support in the Nick app.
- Added color options in the Nick app.
- Added IMU-based rotation in the Nick app.

### Changed
- `ctx.get_font_name()` now raises an exception for unknown fonts.
- Raised umber of concurrent textures to 32.

### Fixed
- Fixed PNG without alpha and JPEG support by enabling `CTX_FORMAT_RGB8`.
- Fixed image cache eviction by introducing a ctx frameclock.
- Fixed incorrect merging of settings dicts in `st3m.settings`.
- Fixed some USB problems.
- Fixed sound filenames not working when they start with `/flash/` or `/sd/`.
- Fixed syntax errors in `flow3r.toml` crashing the menu.


## [1.0.0] - 2023-08-13

Initial Release


[unreleased]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/compare/v1.2.0...main
[1.2.0]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/compare/v1.1.1...v1.2.0
[1.1.1]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/compare/v1.1.0...v1.1.1
[1.1.0]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/compare/v1.0.0...v1.1.0
[1.0.0]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/tags/v1.0.0

