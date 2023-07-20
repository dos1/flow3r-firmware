{ version ? "15.0.0-20221201"
, hash ? "sha256-g55a36f0SYLootgoaA9uSqQ13NPR33ZeAvAVsEKGBW8="
, stdenv
, lib
, fetchurl
, buildFHSUserEnv
, autoPatchelfHook
, zlib
, libxml2
}:

assert stdenv.system == "x86_64-linux";

stdenv.mkDerivation rec {
  pname = "esp-llvm";
  inherit version;

  src = fetchurl {
    url = "https://github.com/espressif/llvm-project/releases/download/esp-${version}/llvm-esp-${version}-linux-amd64.tar.xz";
    inherit hash;
  };

  nativeBuildInputs = [
    autoPatchelfHook
  ];

  buildInputs = [
    zlib
    libxml2
    stdenv.cc.cc.lib
  ];

  installPhase = ''
    cp -r . $out
    # remove unused architectures
    rm -r $out/riscv32-esp-elf
    rm -r $out/xtensa-esp32-elf
    rm -r $out/xtensa-esp32s2-elf
    rm -r $out/bin/riscv32-*
    rm -r $out/bin/xtensa-esp32-*
    rm -r $out/bin/xtensa-esp32s2-*
  '';

  meta = with lib; {
    description = "Espressif LLVM/Clang fork binary build";
    homepage = "https://docs.espressif.com/projects/esp-idf/en/stable/get-started/linux-setup.html";
    license = licenses.gpl3;
  };
}

