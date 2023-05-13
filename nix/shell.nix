let
  sources = import ./sources.nix;
  nixpkgs = import sources.nixpkgs {
    overlays = [
      (import "${sources.nixpkgs-esp-dev}/overlay.nix")
    ];
  };

in with nixpkgs; pkgs.mkShell {
  name = "badg23-shell";
  buildInputs = with pkgs; [
    gcc-xtensa-esp32s3-elf-bin
    openocd-esp32-bin
    esp-idf
    esptool

    git wget gnumake
    flex bison gperf pkgconfig

    cmake ninja

    ncurses5
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
