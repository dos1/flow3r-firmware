{
  description = "flow3r badge flake";

  nixConfig = {
    substituters = [
      "https://cache.nixos.org"
      "https://flow3r.cachix.org"
    ];
    trusted-public-keys = [
      "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="
      "flow3r.cachix.org-1:/v8059Hm6UdEVNKE15uxltpYM0z+pulaTpobjIvFM5A="
    ];
  };

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

      legacyPackages = forAllPkgs (pkgs: pkgs);

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

            extraCommands = ''
              mkdir -m 1777 tmp
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

      hydraJobs = let inherit (nixpkgs.lib) hydraJob; in {
        dockerImage = forAllSystems (system: hydraJob self.packages.${system}.dockerImage);
        devShell = forAllSystems (system: hydraJob self.devShells.${system}.default);
      };

      apps = forAllPkgs (pkgs: {
        cache-flake-inputs = {
          type = "app";
          program = toString (pkgs.writers.writeBash "cache-flake-inputs" ''
            set +e +o pipefail
            nix flake archive --json \
              | ${pkgs.jq}/bin/jq -r '.path,(.inputs|to_entries[].value.path)' \
              | ${pkgs.cachix}/bin/cachix push flow3r
          '');
        };
        build-and-cache = {
          type = "app";
          program = toString (pkgs.writers.writeBash "build-and-cache" ''
            set +e +o pipefail
            PACKAGE="$1"

            IS_CACHED=$(
              ${pkgs.nix-eval-jobs}/bin/nix-eval-jobs \
                --gc-roots-dir gcroot \
                --flake ".#hydraJobs.$PACKAGE" \
                --check-cache-status \
                | ${pkgs.jq}/bin/jq '.isCached'
            )

            if [ "$IS_CACHED" == "false" ]; then
              nix build -L --json ".#hydraJobs.$PACKAGE.${pkgs.system}" \
                | ${pkgs.jq}/bin/jq -r '.[].outputs | to_entries[].value' \
                | ${pkgs.cachix}/bin/cachix push flow3r
            fi
          '');
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
