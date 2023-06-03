badg23 simulator
===

This is a little simulator that allows quicker development iteration on Python code.

It's a (C)Python application which sets up its environment so that it appears similar enough to the Badge's micropython environment, loading scripts from `python_payload` and `python_modules` in the main project directory.

All C-implemented functions are implemented (or maybe just stubbed out) by 'fakes' in the fakes directory. Please try to keep this in sync with the real usermodule implementation.

Of particular interest is how we provide a `ctx`-compatible API: we compile it using emscripten to a WebAssembly bundle, which we then execute using wasmer.

Setting up
---

If not using nix-shell, you'll need Python3 with the following libraries:

 - pygame
 - wasmer
 - wasmer-compiler-cranelift

All of these should be available in PyPI.

Running
---

From the main badge23-firmware checkout:

```
python3 sim/run.py
```

Known Issues
---

No support for input of any kind yet (captouch, three-way buttons).

No support for audio yet.

Hacking
---

A precompiled WASM bundle for ctx is checked into git. If you wish to rebuild it, run `build.sh` in `sim/wasm`.
