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
}
