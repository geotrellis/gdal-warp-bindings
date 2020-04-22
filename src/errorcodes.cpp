/*
 * Copyright 2019 Azavea
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(__linux__)
#include <sys/types.h>
#include <sys/syscall.h>
#endif

#include <atomic>
#include <map>
#include <chrono>

#include <gdal.h>

#include <pthread.h>

#if defined(__linux__)
typedef pid_t errno_key_t;
#else
typedef pthread_t errno_key_t;
#endif
typedef std::tuple<int, std::chrono::milliseconds> errno_cache_entry_t;
typedef std::map<errno_key_t, errno_cache_entry_t> errno_cache_t;
static errno_cache_t *errno_cache = nullptr;
static pthread_mutex_t errno_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static std::atomic<int> reported_errors{0};

#if defined(__linux__)
#define CALL syscall(SYS_gettid)
#else
#define CALL pthread_self()
#endif

// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_BLACK "\x1b[30;1m"
#define ANSI_COLOR_RED "\x1b[31;1m"
#define ANSI_COLOR_GREEN "\x1b[32;1m"
#define ANSI_COLOR_YELLOW "\x1b[33;1m"
#define ANSI_COLOR_BLUE "\x1b[34;1m"
#define ANSI_COLOR_CYAN "\x1b[36;1m"
#define ANSI_COLOR_BGMAGENTA "\x1b[45;1m"
#define ANSI_COLOR_BGYELLOW "\x1b[103;1m"
#define ANSI_COLOR_RESET "\x1b[0m"

const char *severity_string(CPLErr eErrClass)
{
  switch (eErrClass)
  {
  case CE_None:
    return ANSI_COLOR_GREEN "NON-ERROR(0)";
  case CE_Debug:
    return ANSI_COLOR_CYAN "DEBUG(1)";
  case CE_Warning:
    return ANSI_COLOR_YELLOW "WARNING(2)";
  case CE_Failure:
    return ANSI_COLOR_RED "FAILURE(3)";
  case CE_Fatal:
    return ANSI_COLOR_RED ANSI_COLOR_BGYELLOW "UNRECOVERABLE(4)";
  default:
    return ANSI_COLOR_BLUE "UNCLASSIFIED";
  }
}

const char *severity_string_nonansi(CPLErr eErrClass)
{
  switch (eErrClass)
  {
  case CE_None:
    return "NON-ERROR(0)";
  case CE_Debug:
    return "DEBUG(1)";
  case CE_Warning:
    return "WARNING(2)";
  case CE_Failure:
    return "FAILURE(3)";
  case CE_Fatal:
    return "UNRECOVERABLE(4)";
  default:
    return "UNCLASSIFIED";
  }
}

const char *error_string(int err_no)
{
  switch (err_no)
  {
  case CPLE_None:
    return "CPLE_None(0) \"No error.\"";
  case CPLE_AppDefined:
    return "CPLE_AppDefined(1) \"Application defined error.\"";
  case CPLE_OutOfMemory:
    return "CPLE_OutOfMemory(2) \"Out of memory error.\"";
  case CPLE_FileIO:
    return "CPLE_FileIO(3) \"File I/O error.\"";
  case CPLE_OpenFailed:
    return "CPLE_OpenFailed(4) \"Open failed.\"";
  case CPLE_IllegalArg:
    return "CPLE_IllegalArg(5) \"Illegal argument.\"";
  case CPLE_NotSupported:
    return "CPLE_NotSupported(6) \"Not supported.\"";
  case CPLE_AssertionFailed:
    return "CPLE_AssertionFailed(7) \"Assertion failed.\"";
  case CPLE_NoWriteAccess:
    return "CPLE_NoWriteAccess(8) \"No write access.\"";
  case CPLE_UserInterrupt:
    return "CPLE_UserInterrupt(9) \"User interrupted\"";
  case CPLE_ObjectNull:
    return "CPLE_ObjectNull(10) \"NULL object.\"";
  case CPLE_HttpResponse:
    return "CPLE_HttpResponse(11) \"HTTP response.\"";
  case CPLE_AWSBucketNotFound:
    return "CPLE_AWSBucketNotFound(12) \"AWSBucketNotFound.\"";
  case CPLE_AWSObjectNotFound:
    return "CPLE_AWSObjectNotFound(13) \"AWSObjectNotFound.\"";
  case CPLE_AWSAccessDenied:
    return "CPLE_AWSAccessDenied(14) \"AWSAccessDenied.\"";
  case CPLE_AWSInvalidCredentials:
    return "CPLE_AWSInvalidCredentials(15) \"AWSInvalidCredentials.\"";
  case CPLE_AWSSignatureDoesNotMatch:
    return "CPLE_AWSSignatureDoesNotMatch(16) \"AWSSignatureDoesNotMatch.\"";
#if defined(CPLE_AWSError)
  case CPLE_AWSError:
    return "CPLE_AWSError(17) \"VSIE_AWSError.\"";
#endif
  default:
    return "Unknown error...";
  }
}

/**
 * Put the most recently-encountered err_no into the cache.
 */
void put_last_errno(CPLErr eErrClass, int err_no, const char *msg)
{
  int max_reported_errors = 1000; // Following GDAL
  char const *cpl_max_error_reports = nullptr;

  // Re-read environment variable upon every error
  if ((cpl_max_error_reports = getenv("CPL_MAX_ERROR_REPORTS")))
  {
    sscanf(cpl_max_error_reports, "%d", &max_reported_errors);
  }

  if (reported_errors < max_reported_errors)
  {
    reported_errors++;
    if (getenv("GDALWARP_NONANSI_MESSAGES") == nullptr)
    {
      fprintf(stderr, ANSI_COLOR_BLACK ANSI_COLOR_BGMAGENTA "[%d of %d]" ANSI_COLOR_RESET " %s %s %s " ANSI_COLOR_RESET "\n",
              reported_errors.load(), max_reported_errors, severity_string(eErrClass), error_string(err_no), msg);
    }
    else
    {
      fprintf(stderr, "[%d of %d] %s %s %s \n",
              reported_errors.load(), max_reported_errors, severity_string_nonansi(eErrClass), error_string(err_no), msg);
    }
  }

  if (eErrClass == CE_Fatal)
  {
    exit(-1);
  }
  else
  {
    errno_key_t tid = CALL;
    pthread_mutex_lock(&errno_cache_lock);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    errno_cache->operator[](tid) = std::make_tuple(err_no, timestamp);
    pthread_mutex_unlock(&errno_cache_lock);
  }
}

/**
 * Get the last err_no generated by this thread.
 */
int get_last_errno()
{
  errno_key_t tid = CALL;

  pthread_mutex_lock(&errno_cache_lock);
  int retval;
  if (errno_cache->count(tid) > 0)
  {
    std::chrono::milliseconds timestamp;
    std::tie(retval, timestamp) = errno_cache->at(tid);
    errno_cache->erase(tid);
  }
  else
  {
    retval = CPLE_None;
  }
  if (errno_cache->size() > (1 << 20))
  {
    // XXX Can contain errors from 2**20 unique threads before
    // possibly losing information
    errno_cache->clear();
  }
  pthread_mutex_unlock(&errno_cache_lock);
  return retval;
}

/**
 * Get the last err_no_timestamp generated by this thread.
 */
std::chrono::milliseconds get_last_errno_timestamp()
{
  errno_key_t tid = CALL;

  pthread_mutex_lock(&errno_cache_lock);
  auto timestamp = std::chrono::milliseconds::zero();
  if (errno_cache->count(tid) > 0)
  {
    int retval;
    std::tie(retval, timestamp) = errno_cache->at(tid);
  }
  pthread_mutex_unlock(&errno_cache_lock);
  return timestamp;
}

/**
 * Initialize error-handling code.
 */
void errno_init()
{
  errno_cache = new errno_cache_t();
  if (errno_cache == nullptr)
  {
    throw std::bad_alloc();
  }
  CPLSetErrorHandler(put_last_errno);
}

/**
 * Deinitialize error-handling code.
 */
void errno_deinit()
{
  pthread_mutex_lock(&errno_cache_lock);
  if (errno_cache != nullptr)
  {
    delete errno_cache;
    errno_cache = nullptr;
  }
  pthread_mutex_unlock(&errno_cache_lock);
}
