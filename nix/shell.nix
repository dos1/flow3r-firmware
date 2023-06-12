let
  sources = import ./niv/sources.nix;
  nixpkgs = import sources.nixpkgs {
    overlays = [
      (self: super: {
        gcc-xtensa-esp32s3-elf-bin = super.callPackage ./esp32s3-toolchain-bin.nix {};
        openocd-esp32-bin = super.callPackage ./openocd-esp32-bin.nix {};
        esp-idf = super.callPackage ./esp-idf {};
      })
    ];
  };

in with nixpkgs; pkgs.mkShell {
  name = "flow3r-shell";
  buildInputs = with pkgs; [
    gcc-xtensa-esp32s3-elf-bin
    openocd-esp32-bin
    esp-idf
    esptool

    git wget gnumake
    flex bison gperf pkgconfig

    cmake ninja

    ncurses5

    (python3.withPackages (ps: with ps; [ pygame wasmer wasmer-compiler-cranelift ]))
    emscripten
  ];
  shellHook = ''
    # For esp.py openocd integration.
    export OPENOCD_SCRIPTS="${pkgs.openocd-esp32-bin}/share/openocd/scripts"
    # For GDB to be able to find libpython2.7 (????).
    export LD_LIBRARY_PATH="${pkgs.python2}/lib:$LD_LIBRARY_PATH"

    # Some nice-to-have defaults.
    export ESPPORT=/dev/ttyACM0
    export OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg"
  '';
}
