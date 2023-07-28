with import ./pkgs.nix;
pkgs.mkShell {
  name = "flow3r-shell";
  buildInputs = fwdev;
  shellHook = ''
    # For esp.py openocd integration.
    export OPENOCD_SCRIPTS="${pkgs.openocd-esp32-bin}/share/openocd/scripts"

    # Some nice-to-have defaults.
    export ESPPORT=/dev/ttyACM0
    export OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg"

    # openocd wants udev
    export LD_LIBRARY_PATH="${pkgs.systemd}/lib:$LD_LIBRARY_PATH"
  '';
}
