{ python3
, fetchPypi
}:
python3.pkgs.buildPythonApplication rec {
  pname = "mpremote";
  version = "1.20.0";
  format = "pyproject";
  src = fetchPypi {
    inherit pname version;
    hash = "sha256-XDQnYqBHkTCd1JvOY8cKB1qnxUixwAdiYrlvnMw5jKI=";
  };
  doCheck = false;
  nativeBuildInputs = with python3.pkgs; [
    hatchling
    hatch-requirements-txt
    hatch-vcs
  ];
  propagatedBuildInputs = with python3.pkgs; [
    pyserial
    importlib-metadata
  ];
}
