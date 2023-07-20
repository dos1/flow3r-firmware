let
  sources = import ./sources.nix;
  nixpkgs = import sources.nixpkgs {
    overlays = [
      (self: super: {
        gcc-xtensa-esp32s3-elf-bin = super.callPackage ./esp32s3-toolchain-bin.nix {};
        openocd-esp32-bin = super.callPackage ./openocd-esp32-bin.nix {};
        esp-idf = super.callPackage ./esp-idf {};
        esp-llvm = super.callPackage ./esp-llvm.nix {};
        run-clang-tidy = super.callPackage ./run-clang-tidy {};
      })
    ];
  };

in with nixpkgs; pkgs.mkShell {
  name = "flow3r-shell";
  buildInputs = with pkgs; [
    gcc-xtensa-esp32s3-elf-bin
    openocd-esp32-bin
    esp-idf
    esp-llvm
    esptool
    run-clang-tidy

    git wget gnumake
    flex bison gperf pkgconfig

    cmake ninja

    ncurses5

    (python3.withPackages (ps: with ps; [
      pygame wasmer wasmer-compiler-cranelift
      sphinx sphinx_rtd_theme
    ]))
    emscripten
    mypy
  ];
  shellHook = ''
    # For esp.py openocd integration.
    export OPENOCD_SCRIPTS="${pkgs.openocd-esp32-bin}/share/openocd/scripts"

    # Some nice-to-have defaults.
    export ESPPORT=/dev/ttyACM0
    export OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg"
  '';
}
