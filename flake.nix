{
  description = "flow3r badge flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  outputs =
    { self
    , nixpkgs
    , ...
    }:
    let
      supportedSystems = [
        /* FIXME "aarch64-linux" */
        "x86_64-linux"
      ];
      pkgsFor = system:
        nixpkgs.legacyPackages.${system}.extend self.overlays.default;
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      forAllPkgs = f: forAllSystems (system: f (pkgsFor system));

      # All packages require to build/lint the project.
      fwbuild = pkgs: with pkgs; [
        gcc-xtensa-esp32s3-elf-bin
        esp-idf
        esp-llvm
        esptool
        run-clang-tidy

        git
        wget
        gnumake
        flex
        bison
        gperf
        pkgconfig
        gnutar
        curl
        bzip2
        gcc
        gnused
        findutils
        gnugrep

        cmake
        ninja

        python3Packages.sphinx
        python3Packages.sphinx_rtd_theme
        python3Packages.black
        mypy
      ];
      fwdev = pkgs: (fwbuild pkgs) ++ (with pkgs; [
        openocd-esp32-bin
        python3Packages.pygame
        python3Packages.wasmer
        python3Packages.wasmer-compiler-cranelift
        emscripten
        ncurses5
        esp-gdb
        mpremote
      ]);
    in
    {
      overlays.default = import ./nix/overlay;

      packages = forAllPkgs (pkgs:
        {
          dockerImage = pkgs.dockerTools.buildImage {
            name = "flow3r-build";
            copyToRoot = pkgs.buildEnv {
              name = "flow3r-build-root";
              paths = with pkgs; [
                # interactive shell
                bashInteractive
                coreutils-full
                cacert
              ] ++ fwbuild pkgs;
              pathsToLink = [ "/bin" ];
            };

            runAsRoot = ''
              #!${pkgs.runtimeShell}
              mkdir -p /tmp
            '';

            config = {
              Env = [
                "PATH=/bin:${pkgs.esp-idf}/tools"
                "PYTHONPATH=${pkgs.python3.pkgs.makePythonPath pkgs.esp-idf.propagatedBuildInputs}"
                "IDF_PATH=${pkgs.esp-idf}"
                "IDF_COMPONENT_MANAGER=0"
                "TMPDIR=/tmp"
                "NIX_SSL_CERT_FILE=${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"
              ];
            };
          };
        });

      devShells = forAllPkgs (pkgs:
        {
          default = pkgs.mkShell {
            name = "flow3r-shell";
            buildInputs = (fwdev pkgs) ++ (with pkgs; [
              micropython
            ]);
            shellHook = ''
              # For esp.py openocd integration.
              export OPENOCD_SCRIPTS="${pkgs.openocd-esp32-bin}/share/openocd/scripts"

              # Some nice-to-have defaults.
              export ESPPORT=/dev/ttyACM0
              export OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg"

              # openocd wants udev
              export LD_LIBRARY_PATH="${pkgs.systemd}/lib:$LD_LIBRARY_PATH"
            '';
          };
        });
    };
}
