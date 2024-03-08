{
  description = "PLD Comp";

  inputs = {
      nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
      systems.url = "github:nix-systems/default-linux";
      flake-utils.url = "github:numtide/flake-utils";

      mini-compile-commands = { url = github:danielbarter/mini_compile_commands; flake = false;};
  };

  outputs = { self, nixpkgs, mini-compile-commands, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let 
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        rec {
          packages = {
              default = pkgs.gcc13Stdenv.mkDerivation {
              name = "ifcc";
              src = self;
              nativeBuildInputs = [ pkgs.cmake pkgs.jdk21 pkgs.antlr ];
              buildInputs = [ pkgs.antlr.runtime.cpp ];
            };
          };
          devShells.default = pkgs.mkShell.override {
              stdenv = (pkgs.callPackage mini-compile-commands {}).wrap pkgs.gcc13Stdenv;
          } {
              inputsFrom = [ self.packages.${system}.default ];
              nativeBuildInputs = [pkgs.python3];

              ANTLR_PATH = pkgs.antlr.outPath;
              ANTLR_RUNTIME_PATH = pkgs.antlr.runtime.cpp.outPath;
              ANTLR_RUNTIME_DEV_PATH = pkgs.antlr.runtime.cpp.dev.outPath;
          };
        }
      );
}
