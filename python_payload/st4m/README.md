st4m
====

Experimental, heavily-typed flow3r UI/UX framework.

Proof of concept of:
 1. Strongly typed Python (using MyPy) which is also used as docs.
 2. A deep class hierarchy that is still performant enough.
 3. Pretty UX.

Does not implement application lifecycle, but that could be built upon
View/ViewWithInput.

Should be trickle-merged into st3m.

Typechecking
------------

    MYPYPATH=$(pwd)/python_payload/mypystubs mypy python_payload/main_st4m.py --strict

Running
-------

Move `main_st4m.py` to `main.py` then either run on badge or in simulator.
