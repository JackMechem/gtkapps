{
  description = "GTK4 Layer Shell App with GLib, GIO, and cJSON";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: 
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        gtkapps = pkgs.stdenv.mkDerivation {
          pname = "gtkapps";
          version = "0.0.1";
          src = ./.;

          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.meson
            pkgs.ninja
            pkgs.makeWrapper
            pkgs.wrapGAppsHook4
          ];

          buildInputs = [
            pkgs.gtk4
            pkgs.gtk4-layer-shell
            pkgs.glib
            pkgs.glib.dev  # for gio-unix-2.0 and headers
            pkgs.cjson
          ];


        };
      in {
        packages.default = gtkapps;

        devShell = pkgs.mkShell {
          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.meson
            pkgs.ninja
            pkgs.wrapGAppsHook4
          ];
          buildInputs = [
            pkgs.gtk4
            pkgs.gtk4-layer-shell
            pkgs.glib
            pkgs.glib.dev
            pkgs.cjson
          ];
        };
      });
}

