fn main() {
    let root = std::path::PathBuf::from(
        std::env::var("CARGO_MANIFEST_DIR").unwrap()
    );

    let mut lib_dirs = vec![];

    let threadpool_dir = root.join("libraries/ThreadPool");
    let threadpool_dst = cmake::Config::new(&threadpool_dir)
        .build();
    lib_dirs.push(threadpool_dst.join("lib64"));
    let threadpool_lib = threadpool_dst.join("lib64/libThreadPool_Debug.so");


    let chmm_dir = root.join("libraries/GMMLite");
    let chmm_dst = cmake::Config::new(&chmm_dir)
        .define("THREADPOOL_INCLUDE_DIR", &threadpool_dir)
        .define("ThreadPool_LIBRARY", &threadpool_lib)
        .build();
    let chmm_lib = chmm_dst.join("lib64/libGMMLite_Debug.so");
    lib_dirs.push(chmm_dst.join("lib64"));


    let turbocodes_dir = root.join("libraries/TurboCodes");
    let turbocodes_dst = cmake::Config::new(&turbocodes_dir)
        .define("CHMM_INCLUDE_DIR", &chmm_dir)
        .define("CHMM_LIBRARY", &chmm_lib)
        .define("THREADPOOL_INCLUDE_DIR", &threadpool_dir)
        .define("ThreadPool_LIBRARY", &threadpool_dir)
        .build();
    let turbocodes_lib = turbocodes_dst.join("lib64/libTurboCodes_Debug.so");
    lib_dirs.push(turbocodes_dir.join("lib64"));

    let hstcfe_dir = root.join("libraries/HSTCFE");
    let hstcfe_dst = cmake::Config::new(&hstcfe_dir)
        .define("CHMM_INCLUDE_DIR", &chmm_dir)
        .define("CHMM_LIBRARY", &chmm_lib)
        .define("THREADPOOL_INCLUDE_DIR", &threadpool_dir)
        .define("ThreadPool_LIBRARY", &threadpool_dir)
        .define("TURBOCODES_INCLUDE_DIR", &turbocodes_dir)
        .define("TurboCodes_LIBRARY", &turbocodes_lib)
        .build();
    lib_dirs.push(hstcfe_dir.join("lib64"));

    cxx_build::bridges([
        "rust-binding/lib.rs",
        "rust-binding/turbocodes/mod.rs",
    ])
        /*.file("libraries/HSTCFE/hybridstrengthenedturbocodefuzzyextractor.cpp")
        .file("libraries/TurboCodes/turbocode.cpp")
        .file("TurboCodes/vectorfiles.cpp")
        .file("TurboCodes/constellation.cpp")
        .file("TurboCodes/psk.cpp")
        .file("TurboCodes/pam2.cpp")*/
        .file("rust-binding/hstcfe/shim.cc")
        .file("rust-binding/turbocodes/shim.cc")
        .include(hstcfe_dst.join("include"))
        .include(chmm_dst.join("include")) 
        .include(threadpool_dst.join("include")) 
        .include(turbocodes_dst.join("include")) 
        .std("c++17")
        .flag_if_supported("-w")
        .compile("sfe");

    for dir in lib_dirs{
        println!("cargo:rustc-link-search=native={}", dir.display());
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", dir.display());
    }

    //println!("cargo:rustc-link-lib=dylib=TurboCodes");
    println!("cargo:rustc-link-lib=dylib=TurboCodes_Debug");
    //println!("cargo:rustc-link-lib=dylib=HSTCFE");
    println!("cargo:rustc-link-lib=dylib=HSTCFE_Debug");
    //println!("cargo:rustc-link-lib=dylib=CHMM");
    println!("cargo:rustc-link-lib=dylib=GMMLite_Debug");
    //println!("cargo:rustc-link-lib=dylib=ThreadPool");
    println!("cargo:rustc-link-lib=dylib=ThreadPool_Debug");
    println!("cargo:rustc-link-lib=dylib=armadillo");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=rust-binding/lib.rs");
    println!("cargo:rerun-if-changed=rust-binding/hstcfe");
    println!("cargo:rerun-if-changed=rust-binding/turbocodes");
}
