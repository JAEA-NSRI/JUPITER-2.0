
#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef TEST_LPT_SHARED
#  ifdef TEST_LPT_EXPORT
#   define TEST_LPT_DECL __declspec(dllexport)
#  else
#   define TEST_LPT_DECL __declspec(dllimport)
#  endif
# else
#  define TEST_LPT_DECL
# endif
#else
# define TEST_LPT_DECL
#endif

TEST_LPT_DECL
void xLPTtest_run(const char *name);

TEST_LPT_DECL
int xLPTtest_istat(void);
