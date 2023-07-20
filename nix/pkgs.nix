let
  sources = import ./sources.nix;
  nixpkgs = import sources.nixpkgs {
    overlays = [
      (import ./overlay)
    ];
  };

in with nixpkgs; rec {
  # nixpkgs passthrough
  inherit (nixpkgs) pkgs lib;
  # All packages require to build/lint the project.
  fwbuild = [
    gcc-xtensa-esp32s3-elf-bin
    esp-idf
    esp-llvm
    esptool
    run-clang-tidy

    git wget gnumake
    flex bison gperf pkgconfig

    cmake ninja

    python3Packages.sphinx
    python3Packages.sphinx_rtd_theme
    mypy
  ];
  fwdev = fwbuild ++ [
    openocd-esp32-bin
    python3Packages.pygame
    python3Packages.wasmer
    python3Packages.wasmer-compiler-cranelift
    emscripten
    ncurses5
  ];
}
