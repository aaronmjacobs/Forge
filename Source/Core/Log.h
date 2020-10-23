#pragma once

#include <logging.h>

#include <cstring>
#include <ctime>
#include <string>

namespace Log
{
   std::string center(const std::string& input, int width);
   std::string formatTime(const tm& time);
   std::string hex(uint8_t value);
   std::string hex(uint16_t value);
   tm getCurrentTime();

   // Text formatting policy
   class text_formating_policy : public templog::formating_policy_base<text_formating_policy>
   {
   public:
      template<class WritePolicy_, int Sev_, int Aud_, class WriteToken_, class ParamList_>
      static void write(WriteToken_& token, TEMPLOG_SOURCE_SIGN, const ParamList_& parameters)
      {
         // Width of the severity tag in log output (e.g. [ warning ])
         static const int kSevNameWidth = 9;

         write_obj<WritePolicy_>(token, TEMPLOG_SOURCE_FILE);
         write_obj<WritePolicy_>(token, "(");
         write_obj<WritePolicy_>(token, TEMPLOG_SOURCE_LINE);
         write_obj<WritePolicy_>(token, "): ");
         write_obj<WritePolicy_>(token, "[");
         write_obj<WritePolicy_>(token, center(get_name(static_cast<templog::severity>(Sev_)), kSevNameWidth));
         write_obj<WritePolicy_>(token, "] <");
         write_obj<WritePolicy_>(token, formatTime(getCurrentTime()));
         write_obj<WritePolicy_>(token, "> ");

         write_params<WritePolicy_>(token, parameters);
      }
   };

#if FORGE_DEBUG
#  define LOG_CERR_SEV_THRESHOLD templog::sev_debug
#else
// Prevent text logging in release builds
#  define LOG_CERR_SEV_THRESHOLD templog::sev_fatal + 1
#endif

   using cerr_logger = templog::logger<templog::non_filtering_logger<text_formating_policy, templog::std_write_policy>
                                       , LOG_CERR_SEV_THRESHOLD
                                       , templog::audience_list<templog::aud_developer>>;

#undef LOG_CERR_SEV_THRESHOLD
}

// Simplify logging calls

// Debug   : Used to check values, locations, etc. (sort of a replacement for printf)
// Info    : For logging interesting, but expected, information
// Message : For logging more detailed informational messages
// Warning : For logging information of concern, which may cause issues
// Error   : For logging errors that do not prevent the program from continuing
// Fatal   : For logging fatal errors that prevent the program from continuing

#if FORGE_DEBUG
#  define LOG(log_message, log_severity) do { TEMPLOG_LOG(Log::cerr_logger, log_severity, templog::aud_developer) << log_message; } while (0)
#else
#  define LOG(_log_message_, log_severity) do {} while (0)
#endif

#define LOG_DEBUG(log_message) LOG(log_message, templog::sev_debug)
#define LOG_INFO(log_message) LOG(log_message, templog::sev_info)
#define LOG_MESSAGE(log_message) LOG(log_message, templog::sev_message)
#define LOG_WARNING(log_message) LOG(log_message, templog::sev_warning)
#define LOG_ERROR(log_message) LOG(log_message, templog::sev_error)

// Fatal errors (kill the program, even in release)

#define LOG_FATAL(log_message) \
do \
{ \
   LOG(log_message, templog::sev_fatal); \
   abort(); \
} while(0)
