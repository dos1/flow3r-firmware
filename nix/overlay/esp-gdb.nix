{ version ? "12.1_20221002"
, hash ? "sha256-0FbyQ17wXMytrF2Pzvo+/Y+MRWw9hT9euh7bUBrP5Pc="
, stdenv
, lib
, fetchurl
, autoPatchelfHook
, zlib
, python311
}:

assert stdenv.system == "x86_64-linux";

stdenv.mkDerivation rec {
  pname = "esp32s3-toolchain";
  inherit version;

  src = fetchurl {
    url = "https://github.com/espressif/binutils-gdb/releases/download/esp-gdb-v${version}/xtensa-esp-elf-gdb-${version}-x86_64-linux-gnu.tar.gz";
    inherit hash;
  };

  nativeBuildInputs = [
    autoPatchelfHook
  ];

  buildInputs = [
    zlib
    stdenv.cc.cc.lib
    python311
  ];

  installPhase = ''
    rm bin/xtensa-esp-elf-gdb-3.6
    rm bin/xtensa-esp-elf-gdb-3.7
    rm bin/xtensa-esp-elf-gdb-3.8
    rm bin/xtensa-esp-elf-gdb-3.9
    rm bin/xtensa-esp-elf-gdb-3.10
    cp -r . $out
  '';

  meta = with lib; {
    description = "ESP32 gdb";
    homepage = "https://docs.espressif.com/projects/esp-idf/en/stable/get-started/linux-setup.html";
    license = licenses.gpl3;
  };
}

