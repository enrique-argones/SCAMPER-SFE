use std::env::var;
use std::path::{Path, PathBuf};


fn build(name: &str, src: &Path, defines: &[(&str, &Path)]) -> PathBuf{
    let out = PathBuf::from(var("OUT_DIR").unwrap());

    let mut cfg = cmake::Config::new(src);
    cfg.out_dir(out.join(name)).profile("Release");
    for (key, value) in defines {
        cfg.define(key, value);
    }
    cfg.build()
}

fn main() {
    let root = PathBuf::from(var("CARGO_MANIFEST_DIR").unwrap());

    let threadpool_dir = root.join("libraries/ThreadPool");
    let threadpool_dst = build("threadpool", &threadpool_dir, &[]);
    let threadpool_lib = threadpool_dst.join("lib64/libThreadPool.so");

    let chmm_dir = root.join("libraries/GMMLite");
    let chmm_dst = build("gmmlite", &chmm_dir, &[
        ("THREADPOOL_INCLUDE_DIR", &threadpool_dir),
        ("ThreadPool_LIBRARY", &threadpool_lib),
    ]);
    let chmm_lib = chmm_dst.join("lib64/libGMMLite.so");

    let turbocodes_dir = root.join("libraries/TurboCodes");
    let turbocodes_dst = build("turbocodes", &turbocodes_dir, &[
        ("GMMLITE_INCLUDE_DIR", &chmm_dir),
        ("GMMLITE_LIBRARY", &chmm_lib),
        ("THREADPOOL_INCLUDE_DIR", &threadpool_dir),
        ("ThreadPool_LIBRARY", &threadpool_lib),
    ]);
    let turbocodes_lib = turbocodes_dst.join("lib64/libTurboCodes.so");

    let hstcfe_dir = root.join("libraries/HSTCFE");
    let hstcfe_dst = build("hstcfe", &hstcfe_dir, &[
        ("GMMLITE_INCLUDE_DIR", &chmm_dir),
        ("GMMLITE_LIBRARY", &chmm_lib),
        ("THREADPOOL_INCLUDE_DIR", &threadpool_dir),
        ("THREADPOOL_LIBRARY", &threadpool_lib),
        ("TURBOCODES_INCLUDE_DIR", &turbocodes_dir),
        ("TURBOCODES_LIBRARY", &turbocodes_lib),
    ]);

    cxx_build::bridges([
        "rust-binding/hstcfe/mod.rs",
        "rust-binding/turbocodes/mod.rs",
    ])
        .file("rust-binding/hstcfe/shim.cc")
        .file("rust-binding/turbocodes/shim.cc")
        .include(hstcfe_dst.join("include"))
        .include(chmm_dst.join("include")) 
        .include(threadpool_dst.join("include")) 
        .include(turbocodes_dst.join("include")) 
        .std("c++17")
        .opt_level(3)
        .define("NDEBUG", None)
        .flag_if_supported("-w")
        .compile("sfe");


    for dst_dir  in [&hstcfe_dst, &turbocodes_dst, &chmm_dst, &threadpool_dst]{
        let dst = dst_dir.join("lib64");
        println!("cargo:rustc-link-search=native={}", dst.display());
        println!("cargo:rustc-link-arg=-Wl,-rpath,{}", dst.display());
    }

    println!("cargo:rustc-link-lib=dylib=TurboCodes");
    //println!("cargo:rustc-link-lib=dylib=TurboCodes_Debug");
    println!("cargo:rustc-link-lib=dylib=HSTCFE");
    //println!("cargo:rustc-link-lib=dylib=HSTCFE_Debug");
    //println!("cargo:rustc-link-lib=dylib=CHMM");
    println!("cargo:rustc-link-lib=dylib=GMMLite");
    //println!("cargo:rustc-link-lib=dylib=GMMLite_Debug");
    println!("cargo:rustc-link-lib=dylib=ThreadPool");
    //println!("cargo:rustc-link-lib=dylib=ThreadPool_Debug");
    println!("cargo:rustc-link-lib=dylib=armadillo");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=rust-binding/hstcfe");
    println!("cargo:rerun-if-changed=rust-binding/turbocodes");
}
