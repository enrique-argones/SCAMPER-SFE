{
    description = "Benchmarks of BBS-based Anonymous Credentials";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
	};

    outputs = { self, nixpkgs, ... } @ inputs: 
    let
        overlays = [  ];
        pkgs = import inputs.nixpkgs { inherit system overlays; };
        system = "x86_64-linux";
 
    in {
        devShells."${system}" = {
            default = pkgs.mkShell {
                name = "Dev";

                buildInputs = with pkgs; [
                    cargo rustc cmake armadillo argagg
                        gdb
                ];

                    #LD_LIBRARY_PATH="../chmm/build:./TurboCodes/build:./HSTCFE/build:$LD_LIBRARY_PATH";

                shellHook = ''
                    build_HTCSFE(){
                        cmake -S . -B build \
                        -DCHMM_LIBRARY=../../chmm/build/libCHMM.so \
                        -DTurboCodes_LIBRARY=../Turbocodes/build/libTurboCodes.so \
                        -DThreadPool_LIBRARY=../../thread-pool/libThreadPool.so \
                        -DHSTCFE_LIBRARY=../HSTCFE/build/libHSTCFE.so \
                        -DCHMM_INCLUDE_DIR=../.. \
                        -DTURBOCODES_INCLUDE_DIR=../ \
                        -DTHREADPOOL_INCLUDE_DIR=../.. \
                        -DHSTCFE_INCLUDE_DIR=../
                    }
                '';
            };
        };

    };
}
