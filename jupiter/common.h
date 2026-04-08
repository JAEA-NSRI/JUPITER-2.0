#ifndef JUPITER_COMMON_H
#define JUPITER_COMMON_H

#ifdef __cplusplus
#define JUPITER_DECL_START extern "C" {
#define JUPITER_DECL_END }
#else
#define JUPITER_DECL_START
#define JUPITER_DECL_END
#endif

JUPITER_DECL_START

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(JUPITER_EXPORT)
#define JUPITER_DECL __declspec(dllexport)
#define JUPITER_DECL_PRIVATE
#elif defined(JUPITER_IMPORT)
#define JUPITER_DECL __declspec(dllimport)
#define JUPITER_DECL_PRIVATE
#else
#define JUPITER_DECL
#define JUPITER_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(JUPITER_EXPORT) || defined(JUPITER_IMPORT)
#define JUPITER_DECL __attribute__((visibility("default")))
#define JUPITER_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define JUPITER_DECL
#define JUPITER_DECL_PRIVATE
#endif
#else
#define JUPITER_DECL
#define JUPITER_DECL_PRIVATE
#endif

JUPITER_DECL_END

#endif
