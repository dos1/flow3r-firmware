# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Added
- Added the `Audio Passthrough` app for toggling audio passthrough through
  line-in/mic to speaker or lineout.
- Added the `w1f1` app for managing wifi access to Settings, incorporates
  an in-progress cap-touch multi-tap keyboard (`k3yboard`).
- Added the `Scalar` app in `Music` category for playing scales.
- Added configuration of wifi credentials + hostname via `settings.json`
- Added support for apps to set wifi state automatically (configurable in
  settings: `Let apps change WiFi`)
- Added task profiler which prints over serial (configurable in
  settings: `Debug: ftop`)
- Added an error screen to the `Nick` app when `nick.json` is invalid.
- Added `urequests` support in the simulator.
- Added audio/video media framework, and `Wurzelitzer` app as a small jukebox
  frontend - currently supporting mp3, mpeg1 and protracker modules.
- Added exporting of built firmwares as part of CI
- graphics: sprite sheet support for `ctx.image()`
- graphics: `ctx.parse()` for parsing SVG path data/ctx protocol.
- graphics: raw frame-buffer access in 4,8,16,24 and 32bit graphics modes.
- graphics: clipped overlay buffer
- graphics: allow a graphics state depth of up to 10 (`ctx.save()` `ctx.restore()`)

### Changed
- Switched the REPL/fatal/disk restart button to the OS shoulder button (right
  shoulder button, unless swapped in settings).
- Improved performance of the `gr33nhouse` app list by not rendering hidden
  entries and scrolling ones too long to fit on screen.
- Improved download reliability of the `gr33nhouse` app by adding chunked
  downloads, some `gc.collect()` calls and an error screen.
- Settings are now automatically loaded and saved when entering and leaving
  the settings page.
- Moved the `Clouds` app to the `Badge` menu and updated it to use IMU data and
  render in 32bpp graphics mode.
- Added a more sane commandline interface to the simulator.
- More stub functions for the simulator.
- Improved performance of system menus by not rendering hidden entries.
- Added visualization of state, instead of an audio scope in UI of `harmonic
  demo` and `melodic demo`.
- The system provided scope is now rendered stroked rather than filled.
- Improved BPM tap accuracy in `gay drums`.
- Some shell code rewritten to avoid the expensive calls `ctx.save_group()` and
  `ctx.restore_group()`.
- overlay graphics is rendered in a separate pass, rate limited.
- The entry section in `flow3r.toml` can now be omitted if the Application
  class is called `App`.
- `ctx.image()` now supports clipping and drawing a part of the given image.
- Split the `settings.py` file into two, creating `settings_menu.py` to hold
  UI-related code and allow `settings` to be used import loops without
  in many cases.
- Improved handedness in buttons.

### Fixed
- Fixed `tiny sampler` keeping the microphone active after app exit.
- Fixed missing `include/` dir on builds on Darwin.
- Fixed the `time` module in the simulator being broken for apps trying to use it.
- Fixed wrong petal ordering in the simulator.
- Fixed simulator not exiting when closed.
- Fixed `Comic Mono` missing in the simulator.
- Fixed initialization orientation of display (which also fixed incorrect
  transform of gradients)
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

