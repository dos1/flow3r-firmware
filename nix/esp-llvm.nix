{ version ? "15.0.0-20221201"
, hash ? "sha256-g55a36f0SYLootgoaA9uSqQ13NPR33ZeAvAVsEKGBW8="
, stdenv
, lib
, fetchurl
, makeWrapper
, buildFHSUserEnv
}:

let
  fhsEnv = buildFHSUserEnv {
    name = "esp-llvm-env";
    targetPkgs = pkgs: with pkgs; [ zlib libxml2 ];
    runScript = "";
  };
in

assert stdenv.system == "x86_64-linux";

stdenv.mkDerivation rec {
  pname = "esp-llvm";
  inherit version;

  src = fetchurl {
    url = "https://github.com/espressif/llvm-project/releases/download/esp-${version}/llvm-esp-${version}-linux-amd64.tar.xz";
    inherit hash;
  };

  buildInputs = [ makeWrapper ];

  phases = [ "unpackPhase" "installPhase" ];

  installPhase = ''
    cp -r . $out
    for FILE in $(ls $out/bin); do
      FILE_PATH="$out/bin/$FILE"
      FILE_PATH_UNWRAPPED="$out/.bin-unwrapped/$FILE"
      mkdir -p $out/.bin-unwrapped
      if [[ -x $FILE_PATH ]]; then
        mv $FILE_PATH $FILE_PATH_UNWRAPPED
        makeWrapper ${fhsEnv}/bin/esp-llvm-env $FILE_PATH --add-flags "$FILE_PATH_UNWRAPPED"
      fi
    done
  '';

  meta = with lib; {
    description = "Espressif LLVM/Clang fork binary build";
    homepage = "https://docs.espressif.com/projects/esp-idf/en/stable/get-started/linux-setup.html";
    license = licenses.gpl3;
  };
}

