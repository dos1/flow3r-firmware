# Based on https://github.com/mirrexagon/nixpkgs-esp-dev/

{ rev ? "v5.1"
, sha256 ? "sha256-IEa9R9VCWvbRjZFRPb2Qq2Qw1RFxsnVALFVgQlBCXMw="
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

  deps = with python3Packages; rec {
    pygdbmi_ = pygdbmi.overrideAttrs (oa: rec {
      version = "0.9.0.2";
      src = fetchFromGitHub {
        owner = "cs01";
        repo = "pygdbmi";
        rev = version;
        hash = "sha256-bZQYcT5lA8xkG2YIK7P7fxkbVJhO6T5YpWo1EdLpOgY=";
      };
    });

    esptool = buildPythonPackage rec {
      pname = "esptool";
      version = "4.6.2";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-VJ75Pu9C7n6UYs5aU8Ft96DHHZGz934Z7BV0mATN8wA=";
      };
      doCheck = false;
      propagatedBuildInputs = [
        cryptography
        pyyaml
        bitstring
        ecdsa
        reedsolo
        pyserial
        python-pkcs11
        construct
      ];
    };

    esp-idf-kconfig = buildPythonPackage rec {
      pname = "esp-idf-kconfig";
      version = "1.1.0";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-s8ZXt6cf5w2pZSxQNIs/SODAUvHNgxyQ+onaCa7UbFA=";
      };
      propagatedBuildInputs = [
        kconfiglib
      ];
    };

    esp-coredump = buildPythonPackage rec {
      pname = "esp-coredump";
      version = "1.2";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-xcBDMy/1/UZo5K1+nXAE220Tb2DaP2VgSkeN5eC3XYg=";
      };
      doCheck = false;
      propagatedBuildInputs = [
        pygdbmi_
        esptool
      ];
    };

    esp-idf-monitor = buildPythonPackage rec {
      pname = "esp-idf-monitor";
      version = "1.1.1";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-c62X3ZHRShhbAFmuPc/d2keqE9T9SXYIlJTyn32LPaE=";
      };
      propagatedBuildInputs = [
        pyserial
        esp-coredump
        pyelftools
        pyparsing
      ];
    };

    esp-idf-size = buildPythonPackage rec {
      pname = "esp-idf-size";
      version = "0.3.1";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-OzthhzKGjyqDJrmJWs4LMkHz0rAwho+3Pyc2BYFK0EU=";
      };
      propagatedBuildInputs = [
        pyyaml
      ];
    };

    pyclang = buildPythonPackage rec {
      pname = "pyclang";
      version = "0.2.3";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-gl3ZaK7/CpMICJlPdPqHfYXb/3MxlKiMpC4AyUtv7MY=";
      };
      propagatedBuildInputs = [
        pyyaml
      ];
    };
  };
in
stdenv.mkDerivation rec {
  pname = "esp-idf";
  version = rev;

  inherit src;

  # This is so that downstream derivations will have IDF_PATH set.
  setupHook = ./setup-hook.sh;

  propagatedBuildInputs = (with python3Packages; [
    setuptools click future pyelftools urllib3
    jinja2 itsdangerous pyyaml

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
  ]) ++ (with deps; [
    esp-idf-monitor esp-idf-kconfig esp-idf-size
    pyclang
  ]);

  patches = [
    ./rack-off-me-nix-mate.patch
  ];

  installPhase = ''
    mkdir -p $out
    cp -r * $out/
  '';
}
