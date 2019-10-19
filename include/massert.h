/**
 * @file massert.h
 * @author guanzhichao
 * @brief Library supporting show message
 * @version 0.1
 * @date 2019-10-18
 *
 */

#ifndef MASSERT_H_
#define MASSERT_H_

#define INFO "\033[32mINFO\033[0m"
#define ERR "\033[31mERR \033[0m"
#define WARN "\033[34mWARN\033[0m"

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
#define ASSERT(Expr, Msg, ...)                    \
  {                                               \
    if (!(Expr)) {                                \
      const size_t BUFSIZE = 255;                 \
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

#define LOG(Title, Msg, ...)                      \
  {                                               \
    const size_t BUFSIZE = 255;                   \
    char buf[BUFSIZE];                            \
    snprintf(buf, BUFSIZE, Msg, ##__VA_ARGS__);   \
    fprintf(stderr, "[ %s ] \t%s\n", Title, buf); \
  }

#define LOG_ERR(...) LOG(ERR, ##__VA_ARGS__)
#define LOG_WARN(...) LOG(WARN, ##__VA_ARGS__)
#define LOG_INFO(...) LOG(INFO, ##__VA_ARGS__)

#ifdef DEBUG_INFO
#define LOG_DBG(...) LOG(DBG, ##__VA_ARGS__)
#else
#define LOG_DBG(...)
#endif

#endif
