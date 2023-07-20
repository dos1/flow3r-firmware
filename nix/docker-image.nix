with import ./pkgs.nix;

pkgs.dockerTools.buildImage {
  name = "registry.k0.hswaw.net/q3k/flow3r-build";
  copyToRoot = pkgs.buildEnv {
    name = "image-root";
    paths = with pkgs; [
      # interactive shell
      bashInteractive
      coreutils-full

      # esp crud
      esp-idf
      esptool
      run-clang-tidy
      gcc-xtensa-esp32s3-elf-bin
      # esp-llvm goes into PATH because it has conflicting binary names.

      mypy

      # random build tools
      python3 gcc gnused findutils gnugrep
      git wget gnumake
      cmake ninja pkgconfig
    ];
    pathsToLink = [ "/bin" ];
  };

  runAsRoot = ''
    #!${pkgs.runtimeShell}
    mkdir -p /tmp
  '';

  config = {
    Env = [
      "PATH=/bin:${pkgs.esp-idf}/tools:${pkgs.esp-llvm}/bin"
      "PYTHONPATH=${pkgs.python3.pkgs.makePythonPath pkgs.esp-idf.propagatedBuildInputs}"
      "IDF_PATH=${pkgs.esp-idf}"
      "IDF_COMPONENT_MANAGER=0"
      "TMPDIR=/tmp"
    ];
  };
}
