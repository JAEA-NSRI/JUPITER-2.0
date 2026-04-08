#ifndef VTKIOJUPITER_H
#define VTKIOJUPITER_H

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(VTKIOJUPITER_EXPORT)
#define VTKIOJUPITER_DECL __declspec(dllexport)
#define VTKIOJUPITER_DECL_PRIVATE
#elif defined(VTKIOJUPITER_IMPORT)
#define VTKIOJUPITER_DECL __declspec(dllimport)
#define VTKIOJUPITER_DECL_PRIVATE
#else
#define VTKIOJUPITER_DECL
#define VTKIOJUPITER_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(VTKIOJUPITER_EXPORT) || defined(VTKIOJUPITER_IMPORT)
#define VTKIOJUPITER_DECL __attribute__((visibility("default")))
#define VTKIOJUPITER_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define VTKIOJUPITER_DECL
#define VTKIOJUPITER_DECL_PRIVATE
#endif
#else
#define VTKIOJUPITER_DECL
#define VTKIOJUPITER_DECL_PRIVATE
#endif

#endif
