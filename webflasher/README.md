# flow3r Web Flasher Tool

```
!!! Chrome only !!!
```

Flashes the flow3r badge from the browser. Available releases are loaded from a remote manifest. See configuration.

## Build

The flasher uses `npm` to handle build and dependencies and `esbuild` to bundle.

To build, run

```
npm run build
```

In most cases, running a clean first is a good idea:
```
npm run clean && npm run build
```

## Run locally

The flasher can run locally against the current firmware build in the parent firmware directory. For this, it provides a dummy manifest in `static/api/releases.json`. The firmware images are copied over during this step, so make sure to run a firmware build before.

To use the built-in HTTP server, run
```
npm run clean && npm run build && npm run serve
```

## Configuration

The URL to the `releases.json` manifest is specified in `src/main.ts` (`manifestUrl`).
