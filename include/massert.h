#ifndef MASSERT_H_
#define MASSERT_H_

#define MACOS

#define INFO "\033[32mINFO\033[0m"
#define ERR "\033[31mERR\033[0m"
#define WAR "WAR"

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
#define ASSERT(Expr, Msg, ...)                    \
  {                                               \
    if (!(Expr)) {                                \
      const size_t BUFSIZE = 200;                 \
      char buf[BUFSIZE];                          \
      snprintf(buf, BUFSIZE, Msg, ##__VA_ARGS__); \
      fprintf(stderr,                             \
              "Assertion failed:\t%s\n"           \
              "Expected:\t%s\n"                   \
              "Source:\t\t%s, line %d\n",         \
              buf, #Expr, __FILE__, __LINE__);    \
      std::abort();                               \
    }                                             \
  }
#else
#define ASSERT(Expr, Msg, ...)
#endif

#define ASSERT_EQ(A, B, Msg, ...) ASSERT((A) == (B), Msg, ##__VA_ARGS__)
#define ASSERT_NE(A, B, Msg, ...) ASSERT((A) != (B), Msg, ##__VA_ARGS__)

#ifdef OPENPOS
#define LOG(Title, Msg, ...)                    \
  {                                             \
    const size_t BUFSIZE = 200;                 \
    char buf[BUFSIZE];                          \
    snprintf(buf, BUFSIZE, Msg, ##__VA_ARGS__); \
    fprintf(stderr,                             \
            "[ %s ] \t%s\n"                     \
            "Source:\t\t%s, line %d\n",         \
            Title, buf, __FILE__, __LINE__);    \
  }
#else
#define LOG(Title, Msg, ...)                      \
  {                                               \
    const size_t BUFSIZE = 200;                   \
    char buf[BUFSIZE];                            \
    snprintf(buf, BUFSIZE, Msg, ##__VA_ARGS__);   \
    fprintf(stderr, "[ %s ] \t%s\n", Title, buf); \
  }
#endif

#endif
