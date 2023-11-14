flow3r badge simulator
===

This is a little simulator that allows quicker development iteration on Python code.

It's a (C)Python application which sets up its environment so that it appears similar enough to the Badge's micropython environment, loading scripts from `python_payload` in the main project directory.

All C-implemented functions are implemented (or maybe just stubbed out) by 'fakes' in the fakes directory. Please try to keep this in sync with the real usermodule implementation.

Of particular interest is how we provide a `ctx`-compatible API: we compile it using emscripten to a WebAssembly bundle, which we then execute using wasmer.

Setting up
---

If not using nix-shell, you'll need Python3 with the following libraries:

 - pygame
 - wasmer
 - wasmer-compiler-cranelift

For full functionality you'll also need:

 - requests
 - pymad

All of these should be available in PyPI.

Installing in a venv
---

```
python -m venv venv
source venv/bin/activate
pip install wasmer wasmer-compiler-cranelift pygame requests pymad
```

Note: The wasmer python module from PyPI [doesn't work with Python version 3.11](https://github.com/wasmerio/wasmer-python/issues/539).
You will get `ImportError: Wasmer is not available on this system` when trying to run the simulator. Instead, install our [rebuilt wasmer wheels](https://flow3r.garden/tmp/wasmer-py311/) using:

```
venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer_compiler_cranelift-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl
venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl
```

Running
---

From the main flow3r-firmware checkout:

```
python3 sim/run.py
```

Known Issues
---

No support for most audio APIs or positional captouch yet.

Hacking
---

A precompiled WASM bundle for ctx is checked into git. If you wish to rebuild it, run `build.sh` in `sim/wasm`.
