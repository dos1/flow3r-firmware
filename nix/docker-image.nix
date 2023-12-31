with import ./pkgs.nix;

pkgs.dockerTools.buildImage {
  name = "registry.gitlab.com/flow3r-badge/flow3r-build";
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

      (python3.withPackages (ps: with ps; [
        sphinx sphinx_rtd_theme
        black
        
        # simulator deps
        pygame wasmer
        wasmer-compiler-cranelift
        pymad requests
      ]))

      # random build tools
      gcc gnused findutils gnugrep
      git wget gnumake
      cmake ninja pkgconfig
      gnutar curl bzip2
      cacert
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
      "NIX_SSL_CERT_FILE=${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"
    ];
  };
}
