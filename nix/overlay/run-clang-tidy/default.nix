{ stdenv
, lib
, fetchurl
, makeWrapper
, buildFHSUserEnv
}:

stdenv.mkDerivation rec {
  pname = "run-clang-tidy";
  version = "20230720";

  src = ./run-clang-tidy.py;

  phases = [ "installPhase" ];

  installPhase = ''
    mkdir -p $out/bin
    cp $src $out/bin/run-clang-tidy.py
    chmod +x $out/bin/run-clang-tidy.py
  '';
}

