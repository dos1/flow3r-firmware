# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Added
- Added _Comic Mono_ font.
- Added a setting for automatically connecting to Camp WiFi.
- Added `ApplicationContext.bundle_path` so apps can find out what directory
  they live in.  This should be used for loading app assets from the correct
  path.
- Added TLS support for MicroPython.
- Added modules for `.tar.gz` extraction.
- Added `aioble` module (Bluetooth Low Energy).
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


## [1.0.0] - 2023-08-13

Initial Release


[unreleased]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/compare/v1.0.0...main
[1.0.0]: https://git.flow3r.garden/flow3r/flow3r-firmware/-/tags/v1.0.0

