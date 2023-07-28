flow3rbadge st3m
===

These are the sources for the Python part of st3m, which implements the core functionality of the flow3rbadge.

You're either seeing this in our git repository (in `python_payload`) or on a badge itself (in `/flash/sys`).

On the badge, these files are required for the badge to function, and are extracted on first startup. You can edit these to your heart's conent, but this generally shouldn't be necessary.

If you break something, the badge will probably not boot. In this case, start it in recovery mode and remove the `sys` directory fully. This will cause the badge to re-extract the files on next startup.

For more info, see [docs.flow3r.garden](https://docs.flow3r.garden).
