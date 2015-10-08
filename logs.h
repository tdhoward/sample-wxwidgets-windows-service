/*--------------------------------------------------------

Handles log file interaction



--------------------------------------------------------*/


#define LOG_CRITICAL    2   // A failure in the system's primary application.
#define LOG_ERROR       3   // e.g. An application has exceeded its file storage limit and attempts to write are failing.
#define LOG_WARNING     4   // May indicate that an error will occur if action is not taken.
#define LOG_INFO        6   // Normal operational messages that require no action.
#define LOG_DEBUG       7   // Debugging info only for developers

#define LOG_FILE_LOCATION  "C://app.log"
#define DEBUG_COM_PORT  "\\\\.\\COM7"


bool init_logger(int level);
void set_log_level(int level);
void log(int priority, const char *format, ...);
void close_logger();
