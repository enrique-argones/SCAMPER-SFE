/// \file MHSTCFE_global.h
/// \brief Symbol visibility macros for the MHSTCFE shared library.
///
/// \c MHSTCE_EXPORT expands to the export attribute when \c MHSTCE_LIBRARY is defined (i.e. when building the library) and to the import attribute otherwise.

#ifndef MHSTCFE_GLOBAL_H
#define MHSTCFE_GLOBAL_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) \
    || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define Q_DECL_EXPORT __declspec(dllexport)
#define Q_DECL_IMPORT __declspec(dllimport)
#else
#define Q_DECL_EXPORT __attribute__((visibility("default")))
#define Q_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(MHSTCE_LIBRARY)
#define MHSTCE_EXPORT Q_DECL_EXPORT
#else
#define MHSTCE_EXPORT Q_DECL_IMPORT
#endif

#endif // MHSTCFE_GLOBAL_H
