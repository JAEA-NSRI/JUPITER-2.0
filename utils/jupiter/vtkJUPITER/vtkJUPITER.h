#ifndef VTKJUPITER_H
#define VTKJUPITER_H

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(VTKJUPITER_EXPORT)
#define VTKJUPITER_DECL __declspec(dllexport)
#define VTKJUPITER_DECL_PRIVATE
#elif defined(VTKJUPITER_IMPORT)
#define VTKJUPITER_DECL __declspec(dllimport)
#define VTKJUPITER_DECL_PRIVATE
#else
#define VTKJUPITER_DECL
#define VTKJUPITER_DECL_PRIVATE
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#if defined(VTKJUPITER_EXPORT) || defined(VTKJUPITER_IMPORT)
#define VTKJUPITER_DECL __attribute__((visibility("default")))
#define VTKJUPITER_DECL_PRIVATE __attribute__((visibility("hidden")))
#else
#define VTKJUPITER_DECL
#define VTKJUPITER_DECL_PRIVATE
#endif
#else
#define VTKJUPITER_DECL
#define VTKJUPITER_DECL_PRIVATE
#endif

#endif
