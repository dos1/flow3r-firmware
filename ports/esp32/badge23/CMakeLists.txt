idf_component_register(SRCS "espan.c" "apa102LEDStrip.c" "audio.c" "captouch.c" "decode_image.c" "leds.c" "display.c" "synth.c" "scope.c"
    EMBED_FILES
        ${project_dir}/resources/boot.snd
        ${project_dir}/resources/pan.s16
        ${project_dir}/resources/image.jpg
                    INCLUDE_DIRS ".")
