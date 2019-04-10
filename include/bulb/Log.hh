#ifndef _LOG_HH
#define _LOG_HH

#include <string>
#include <utility>

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include <stdio.h>
#endif

#define FMT_HEADER_ONLY
#include "fmt/format.h"

namespace bulb
{
   struct Log
      {
         std::string tag, annotation;

         explicit Log(const char* cannotation =nullptr, const char* ctag =nullptr)
         {
            static std::string TAG = "libMAR";
            if (cannotation != nullptr)
               annotation = cannotation;
            else
               annotation = "";
            if (ctag == nullptr)
               tag = TAG;
            else
               TAG = tag = ctag;
         }

         Log(Log const&) = delete;
         Log(Log&&) = delete;
         Log& operator=(Log const&) = delete;
         Log& operator=(Log &&) = delete;
         virtual ~Log() { }

         template <typename S, typename ...Args>
         void error(const S templ, const Args... args)
         //--------------------------------------------
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, templ, std::forward<const Args>(args)...);
            // fmt::format(templ, std::forward<Args>(args)...);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", buf.data());
#else
            fprintf(stderr, "ERROR %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         void error(const char* message)
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, "%s", message);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", buf.data());
#else
            fprintf(stderr, "ERROR %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         template <typename S, typename ...Args>
         void warn(const S templ, const Args... args)
         //--------------------------------------------
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, templ, std::forward<const Args>(args)...);
            // fmt::format(templ, std::forward<Args>(args)...);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, tag.c_str(), "%s", buf.data());
#else
            fprintf(stderr, "WARN %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif

         }

         void warn(const char* message)
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, "%s", message);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, tag.c_str(), "%s", buf.data());
#else
            fprintf(stderr, "WARN %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         template <typename S, typename ...Args>
         void info(const S templ, const Args... args)
         //--------------------------------------------
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, templ, std::forward<const Args>(args)...);
            // fmt::format(templ, std::forward<Args>(args)...);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", buf.data());
#else
            fprintf(stdout, "INFO %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         void info(const char* message)
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, "%s", message);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", buf.data());
#else
            fprintf(stdout, "INFO %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         template <typename S, typename ...Args>
         void debug(const S templ, const Args... args)
         //--------------------------------------------
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, templ, std::forward<const Args>(args)...);
            // fmt::format(templ, std::forward<Args>(args)...);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), "%s", buf.data());
#else
            fprintf(stdout, "DEBUG %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }

         void debug(const char* message)
         {
            fmt::memory_buffer buf;
            if (! annotation.empty())
               fmt::format_to(buf, "{} ", annotation);
            fmt::format_to(buf, "%s", message);
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), "%s", buf.data());
#else
            fprintf(stdout, "DEBUG %s %s\n", tag.c_str(), fmt::to_string(buf).c_str());
#endif
         }
      };
}
 #endif
