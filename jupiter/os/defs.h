#ifndef JUPITER_OS_DEFS_H
#define JUPITER_OS_DEFS_H

#ifdef __cplusplus
#define JUPITER_OS_DECL_START extern "C" {
#define JUPITER_OS_DECL_END   }
#else
#define JUPITER_OS_DECL_START
#define JUPITER_OS_DECL_END
#endif

JUPITER_OS_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_OS_EXPORT)
#define JUPITER_OS_DECL __declspec(dllexport)
#define JUPITER_OS_DECL_PRIVATE
#elif defined(JUPITER_OS_IMPORT)
#define JUPITER_OS_DECL __declspec(dllimport)
#define JUPITER_OS_DECL_PRIVATE
#else
#define JUPITER_OS_DECL
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_OS_EXPORT) || defined(JUPITER_OS_IMPORT)
#define JUPITER_OS_DECL __attribute__((visibility("default")))
#define JUPITER_OS_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_OS_DECL
#define JUPITER_OS_DECL_PRIVATE
#endif
#else
#define JUPITER_OS_DECL
#define JUPITER_OS_DECL_PRIVATE
#endif

JUPITER_OS_DECL_END

#endif
