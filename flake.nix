{
  description = "SDL3 small projects flake";

  inputs = { };

  outputs =
    {
      nixpkgs,
      ...
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
      lib = pkgs.lib;
    in
    {
      devShells.${system}.default = pkgs.mkShell.override { stdenv = pkgs.llvmPackages_20.stdenv; } {
        # LD_LIBRARY_PATH = "${lib.getLib pkgs.libGL}/lib/libGL.so.1:${lib.getLib pkgs.libGL}/lib/libEGL.so.1";

        packages = with pkgs; [
          # IDE/Dev Tools:
          cmake
          gnumake
          ninja
          pkg-config
          llvmPackages_20.clang-tools # LSP & Formatter
          lldb_20 # Debugger
          pre-commit
          gdbgui
          valgrind

          # Shader stuff:
          glslang
          shaderc

          # Vendor SDL & glm:
          SDL
          sdl3
          sdl3-image
          glm

          # runtime:
          libGL
          libglvnd
        ];
      };
    };
}
