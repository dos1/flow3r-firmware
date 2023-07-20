# Based on https://github.com/mirrexagon/nixpkgs-esp-dev/

{ version ? "12.2.0_20230208"
, hash ? "sha256-KbXqazDZgjHwwX8jJ0BBCeCr9ZtI0PKJDZ2YmWeKiaM="
, stdenv
, lib
, fetchurl
, autoPatchelfHook
, zlib
}:

assert stdenv.system == "x86_64-linux";

stdenv.mkDerivation rec {
  pname = "esp32s3-toolchain";
  inherit version;

  src = fetchurl {
    url = "https://github.com/espressif/crosstool-NG/releases/download/esp-${version}/xtensa-esp32s3-elf-${version}-x86_64-linux-gnu.tar.xz";
    inherit hash;
  };

  nativeBuildInputs = [
    autoPatchelfHook
  ];

  buildInputs = [
    zlib
    stdenv.cc.cc.lib
  ];

  installPhase = ''
    cp -r . $out
  '';

  meta = with lib; {
    description = "ESP32-S3 compiler toolchain";
    homepage = "https://docs.espressif.com/projects/esp-idf/en/stable/get-started/linux-setup.html";
    license = licenses.gpl3;
  };
}

