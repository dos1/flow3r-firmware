(self: super: {
  gcc-xtensa-esp32s3-elf-bin = super.callPackage ./esp32s3-toolchain-bin.nix {};
  openocd-esp32-bin = super.callPackage ./openocd-esp32-bin.nix {};
  esp-idf = super.callPackage ./esp-idf {};
  esp-llvm = super.callPackage ./esp-llvm.nix {};
  esp-gdb = super.callPackage ./esp-gdb.nix {};
  run-clang-tidy = super.callPackage ./run-clang-tidy {};
})
