#pragma once
#include <signal.h>

#include <exception>
#include <string>

#include "Units.hpp"
// -------------------------------------------------------------------------------------
#define imply(lhs, rhs) (!(lhs) || (rhs))
// -------------------------------------------------------------------------------------
#define posix_check(expr) \
  if (!(expr)) {          \
    perror(#expr);        \
    assert(false);        \
  }
//--------------------------------------------------------------------------------------
#define Generic_Exception(name)                                                                                      \
  struct name : public std::exception {                                                                              \
    const std::string msg;                                                                                           \
    explicit name() : msg(#name) { printf("Throwing exception: %s\n", #name); }                                      \
    explicit name(const std::string& msg) : msg(msg) { printf("Throwing exception: %s(%s)\n", #name, msg.c_str()); } \
    ~name() = default;                                                                                               \
    virtual const char* what() const noexcept { return msg.c_str(); }                                                \
  };                                                                                                                 \
//--------------------------------------------------------------------------------------
namespace leanstore
{
namespace ex
{
Generic_Exception(GenericException);
Generic_Exception(EnsureFailed);
Generic_Exception(UnReachable);
Generic_Exception(TODO);
void OnEnsureFailedPrint(const std::string& func, const std::string& file, int line, const std::string& expression);
}  // namespace ex
}  // namespace leanstore
// -------------------------------------------------------------------------------------
#define UNREACHABLE() throw ex::UnReachable(std::string(__FILE__) + ":" + std::string(std::to_string(__LINE__)));
// -------------------------------------------------------------------------------------
#ifdef DEBUG
#define ensure(e) assert(e);
#else
#define ensure(e)                                                                                                                               \
  (__builtin_expect(!(e), 0) ? throw leanstore::ex::EnsureFailed(std::string(__func__) + " in " + std::string(__FILE__) + "@" + std::to_string(__LINE__) + \
                                                      " msg: " + std::string(#e))                                                               \
                             : (void)0)
#endif
// -------------------------------------------------------------------------------------
#define TODO() throw leanstore::ex::TODO(std::string(__FILE__) + ":" + std::string(std::to_string(__LINE__)));
// -------------------------------------------------------------------------------------
#define explain(e)  \
  if (!(e)) {       \
    raise(SIGTRAP); \
  };

// -------------------------------------------------------------------------------------
#ifdef DEBUG
#define DEBUG_BLOCK() if(true)
#else
#define DEBUG_BLOCK() if(false)
#endif
// -------------------------------------------------------------------------------------
#ifdef MACRO_FLAG_COUNTERS
#define COUNTERS_BLOCK() if(true)
#else
#define COUNTERS_BLOCK() if(false)
#endif
// -------------------------------------------------------------------------------------
template <typename T>
inline void DO_NOT_OPTIMIZE(T const& value)
{
#if defined(__clang__)
  asm volatile("" : : "g"(value) : "memory");
#else
  asm volatile("" : : "i,r,m"(value) : "memory");
#endif
}
