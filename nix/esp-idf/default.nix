# Based on https://github.com/mirrexagon/nixpkgs-esp-dev/

{ rev ? "v4.4.5"
, sha256 ? "sha256-Jz9cbTYoourYYNo873kLt4CQXbE704zc9Aeq9kbNdPU="
, stdenv
, python3Packages
, fetchFromGitHub
}:

let
  src = fetchFromGitHub {
    owner = "espressif";
    repo = "esp-idf";
    rev = rev;
    sha256 = sha256;
    fetchSubmodules = true;
  };
in
stdenv.mkDerivation rec {
  pname = "esp-idf";
  version = rev;

  inherit src;

  # This is so that downstream derivations will have IDF_PATH set.
  setupHook = ./setup-hook.sh;

  propagatedBuildInputs = with python3Packages; [
    setuptools click future pyelftools urllib3
    jinja2 itsdangerous

    (pyparsing.overrideAttrs (oa: rec {
      version = "2.3.1";
      src = fetchFromGitHub {
        owner = "pyparsing";
        repo = "pyparsing";
        rev = "pyparsing_${version}";
        hash = "sha256-m4mvPUXjDxz77rofg2Bop4/RnVTBDBNL6lRDd5zkpxM=";
      };
    }))

    (kconfiglib.overrideAttrs (oa: rec {
      version = "13.7.1";
      src = fetchPypi {
        inherit (oa) pname;
        inherit version;
        hash = "sha256-ou6PsGECRCxFllsFlpRPAsKhUX8JL6IIyjB/P9EqCiI=";
      };
    }))

    (construct.overrideAttrs (oa: rec {
      version = "2.10.54";
      src = fetchFromGitHub {
        owner = "construct";
        repo = "construct";
        rev = "v${version}";
        hash = "sha256-iDAxm2Uf1dDA+y+9X/w+PKI36RPK/gDjXnG4Zay+Gtc=";
      };
    }))

    ((python-socketio.overrideAttrs (oa: rec {
      version = "4.6.1";
      src = fetchFromGitHub {
        owner = "miguelgrinberg";
        repo = "python-socketio";
        rev = "v${version}";
        hash = "sha256-hVNH0086mo17nPcCmVMkkCyuCkwo4nQv2ATtE56SsZE=";
      };
      disabledTests = [
        "test_logger"
      ];
    })).override {
      python-engineio = python-engineio.overrideAttrs (oa: rec {
        version = "3.14.2";
        src = fetchFromGitHub {
          owner = "miguelgrinberg";
          repo = "python-engineio";
          rev = "v${version}";
          hash = "sha256-xJDlbxzy6HSQL1+1NQgFa3IjDg0t9n3NAHvZmX/cb+Q";
        };
      });
    })

    (pygdbmi.overrideAttrs (oa: rec {
      version = "0.9.0.2";
      src = fetchFromGitHub {
        owner = "cs01";
        repo = "pygdbmi";
        rev = version;
        hash = "sha256-bZQYcT5lA8xkG2YIK7P7fxkbVJhO6T5YpWo1EdLpOgY=";
      };
    }))
  ];

  patches = [
    # Can't be bothered to package gdbgui and idf-component-manager and we
    # don't need them.
    ./fixup-requirements.patch
  ];

  installPhase = ''
    mkdir -p $out
    cp -r * $out/
  '';
}
