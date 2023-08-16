(final: prev: {
  gcc-xtensa-esp32s3-elf-bin = final.callPackage ./esp32s3-toolchain-bin.nix {};
  openocd-esp32-bin = final.callPackage ./openocd-esp32-bin.nix {};
  esp-idf = final.callPackage ./esp-idf {};
  esp-llvm = final.callPackage ./esp-llvm.nix {};
  esp-gdb = final.callPackage ./esp-gdb.nix {};
  run-clang-tidy = final.callPackage ./run-clang-tidy {};
  mpremote = final.python310Packages.callPackage ./mpremote {};
})
