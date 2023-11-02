# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.3.0] - 2023-11-?
### Added
- Added the `Audio Passthrough` app for toggling audio passthrough through
  line-in/mic to speaker or lineout.
- Added the `w1f1` app for managing wifi access to Settings, incorporates
  an in-progress cap-touch multi-tap keyboard (`k3yboard`).
- Added the `Scalar` app in `Music` category for playing scales.
- Added the 'Mandelbrot' app, which illustrates how one can do direct
  framebuffer access and control 8bpp palettized modes from python.
- Added configuration of audio adjustments and startup dB level to `settings.json`.
- Added configuration of wifi credentials + hostname via `settings.json`
- Added support for apps to set wifi state automatically (configurable in
  settings: `Let apps change WiFi`)
- Added task profiler which prints over serial (configurable in
  settings: `Debug: ftop`)
- Added an error screen to the `Nick` app when `nick.json` is invalid.
- Added `urequests` and `uos` support in the simulator.
- Added audio/video media framework, and `Wurzelitzer` app as a small jukebox
  frontend - currently supporting mp3 audio, mpeg1 video and protracker
  .mod files.
- Added exporting of built firmwares as part of CI.
- Added basic implementation of `os.statvfs()` to fetch full/available
  disk space on flash and SD.
- Added `set_position` and `scroll_to` methods to `ScrollController`
- graphics: sprite sheet support for `ctx.image()`
- graphics: `ctx.parse()` for parsing SVG-path-data/ctx-protocol.
- graphics: 1,2,4,8,16,24 and 32 bits-per-pixel graphics modes.
- graphics: direct framebuffer access.
- graphics: palette setting in 1,2,4 and 8bpp graphics modes.
- graphics: flags for 2x 3x and 4x pixel doubling, low-latency and direct-ctx modes.
- graphics: clipped and composited overlay buffer
- graphics: allow a graphics state depth of up to 10 (`ctx.save()` `ctx.restore()`)
- battery: Main menu shows a charge percentage estimate based on real battery characterization

### Changed
- Changed the st3m\_tar logic to only update files on flash after an update
  if they've been changed, greatly improving start times after an update.
- Flashing flow3r through idf.py now automatically restarts it.
- Switched the REPL/fatal/disk restart button to the OS shoulder button (right
  shoulder button, unless swapped in settings).
- Improved performance of the `gr33nhouse` app list by not rendering hidden
  entries and scrolling ones too long to fit on screen.
- Changed the audio adjustment logic, and added support for holding down the
  shoulder to keep increasing/decreasing level.
- Improved download reliability of the `gr33nhouse` app by adding chunked
  downloads, some `gc.collect()` calls and an error screen.
- Settings are now automatically loaded and saved when entering and leaving
  the settings page.
- Moved the `Clouds` app to the `Badge` menu and updated it to use IMU data.
- Added a more sane commandline interface to the simulator.
- More stub functions for the simulator.
- Improved performance of system menus by not rendering hidden entries.
- Added visualization of state, instead of an audio scope in UI of `harmonic
  demo` and `melodic demo`.
- The system provided scope is now always stroked.
- Improved BPM tap accuracy in `gay drums`.
- Some shell code rewritten to avoid the expensive calls `ctx.save_group()` and
  `ctx.restore_group()`.
- Overlay graphics gets rendered to a separate framebuffer, of which a clipped rectangle
  is composited during scan-out. The python overlay code has been adapted to keep track
  of which parts of overlay need refresh.
- Slightly lower AA quality; for a 3x performance boost in the
  worst-case scanline rasterization code path.
- The entry section in `flow3r.toml` can now be omitted if the Application
  class is called `App`.
- disabled support for compositing group API in ctx, where used global\_alpha on
  its own did was responsible for it seeming to work.
- Split the `settings.py` file into two, creating `settings_menu.py` to hold
  UI-related code and allow `settings` to be used import loops without
  in many cases.
- When running apps through REPL mode (with `mpremote` etc), multi-view apps
  are now properly handled and don't restart whenever OS shoulder is pressed.
- Added a delete button to `fil3s`/Files app to allow for deleting files,
  folders and apps on-device.
- A multitude of built-in apps now scroll names to fit the screen.
- Apps are now sorted and deduplicated by display name, not by folder name.
- Improved handedness in buttons.

### Fixed
- Fixed `tiny sampler` keeping the microphone active after app exit.
- Fixed missing `include/` dir on builds on Darwin.
- Fixed the `time` module in the simulator being broken for apps trying to use it.
- Fixed wrong petal ordering in the simulator.
- Fixed simulator not exiting when closed.
- Fixed `Comic Mono` missing in the simulator.
- Fixed initialization orientation of display and transform initialization for
  ctx contexts, (this enables arbitrary transformations to images and gradients.)
- Fixed cleanup at exit for firmware apps
- Fixed sequencer bug in bl00mbox
- Fixed reset of graphics subsystem upon entering REPL / using mpremote.


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
- Raised max concurrent texture limit to 32.

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

