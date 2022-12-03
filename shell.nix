let
  pkgs = import <nixpkgs> {};
  ignoringVulns = x: x // { meta = (x.meta // { knownVulnerabilities = []; }); };
  libavIgnoringVulns = pkgs.libav_12.overrideAttrs ignoringVulns;
in
  pkgs.mkShell {
    buildInputs = with pkgs; [
      v8
      gcc
      libcxx
      gtkmm4
      curl
    ];
    nativeBuildInputs = with pkgs; [
      pkg-config
    ];
  }
