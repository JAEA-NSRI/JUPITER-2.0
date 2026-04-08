#ifndef JUPITER_RANDOM_BASE_H
#define JUPITER_RANDOM_BASE_H

#ifdef __cplusplus
#define JUPITER_RANDOM_DECL_START extern "C" {
#define JUPITER_RANDOM_DECL_END }
#else
#define JUPITER_RANDOM_DECL_START
#define JUPITER_RANDOM_DECL_END
#endif

JUPITER_RANDOM_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_RANDOM_EXPORT)
#define JUPITER_RANDOM_DECL __declspec(dllexport)
#elif defined(JUPITER_RANDOM_IMPORT)
#define JUPITER_RANDOM_DECL __declspec(dllimport)
#else
#define JUPITER_RANDOM_DECL
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_RANDOM_EXPORT) || defined(JUPITER_RANDOM_IMPORT)
#define JUPITER_RANDOM_DECL __attribute__((visibility("default")))
#else
#define JUPITER_RANDOM_DECL
#endif
#else
#define JUPITER_RANDOM_DECL
#endif

JUPITER_RANDOM_DECL_END

#endif
