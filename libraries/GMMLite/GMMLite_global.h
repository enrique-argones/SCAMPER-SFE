/// \file GMMLite_global.h
/// \brief Symbol visibility macros for the GMMLite shared library.
///
/// \c GMMLITE_EXPORT expands to the export attribute when \c GMMLITE_LIBRARY is defined (i.e. when building the library) and
/// to the import attribute otherwise. The library is compiled with \c -fvisibility=hidden, so only the symbols marked with
/// \c GMMLITE_EXPORT are exported.

#ifndef GMMLITE_GLOBAL_H
#define GMMLITE_GLOBAL_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) \
    || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
/// Attribute that exports a symbol from the shared library being built.
#define GMMLITE_DECL_EXPORT __declspec(dllexport)
/// Attribute that imports a symbol from the shared library.
#define GMMLITE_DECL_IMPORT __declspec(dllimport)
#else
/// Attribute that exports a symbol from the shared library being built.
#define GMMLITE_DECL_EXPORT __attribute__((visibility("default")))
/// Attribute that imports a symbol from the shared library.
#define GMMLITE_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(GMMLITE_LIBRARY)
/// Visibility of the public GMMLite symbols (export while building the library, import otherwise).
#define GMMLITE_EXPORT GMMLITE_DECL_EXPORT
#else
/// Visibility of the public GMMLite symbols (export while building the library, import otherwise).
#define GMMLITE_EXPORT GMMLITE_DECL_IMPORT
#endif

#endif // GMMLITE_GLOBAL_H
