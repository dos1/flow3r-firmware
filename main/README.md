micropython shim module
===

This is the 'main' module from ESP-IDF, and its job is to just start
micropython and continue executing `app_main` from
`components/micropython/vendor/ports/esp32/main.c`.
