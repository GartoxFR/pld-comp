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
          antlr = pkgs.antlr;
        in
        rec {
          packages = {
              default = pkgs.gcc11Stdenv.mkDerivation {
              name = "ifcc";
              src = self;
              nativeBuildInputs = [ pkgs.cmake pkgs.jdk21 antlr ];
              buildInputs = [ antlr.runtime.cpp ];
            };
          };
          devShells.default = pkgs.mkShell.override {
              stdenv = (pkgs.callPackage mini-compile-commands {}).wrap pkgs.gcc11Stdenv;
          } {
              inputsFrom = [ self.packages.${system}.default ];
              nativeBuildInputs = with pkgs; [python3 graphviz gdb];

              ANTLR_PATH = antlr.outPath;
              ANTLR_RUNTIME_PATH = antlr.runtime.cpp.outPath;
              ANTLR_RUNTIME_DEV_PATH = antlr.runtime.cpp.dev.outPath;
          };
        }
      );
}
