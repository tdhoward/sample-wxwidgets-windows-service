/*--------------------------------------------------------

Handles all logging interactions, including:
    log files
    logging to the database
    logging to the serial port  (for debugging only)

--------------------------------------------------------*/

#include <stdarg.h>

#include <wx/intl.h>
#include <wx/string.h>
#include <wx/utils.h>
#include <wx/file.h>

#include "logs.h"
#include "simple_serial.h"


int log_max_level = -1;  // -1 means the logger is not active.
wxFile LogFile;


// sets the maximum level of logging
void set_log_level(int level)
{
    log_max_level = level;
}

// initializes the logger using the specified log level
bool init_logger(int level)
{
    log_max_level = level;

    // open/create a log file
    if(!LogFile.Open(LOG_FILE_LOCATION,wxFile::write_append))
    {
        if(log_max_level >= LOG_DEBUG)
            WriteDebug("close_logger: LogFile was NULL!");
    }

    LogFile.Write("Begin logging...\r\n");

    // if debugging, open the debug com port
    if(log_max_level >= LOG_DEBUG)
        OpenDebug();

    return true;
}


void log(int priority, const char *format, ...)
{
    if(log_max_level == -1)  // logger is not active
        return;

	if(priority > log_max_level)
        return;

	// convert the fmt string and varargs to a wxString  (use wxFormat)
	va_list argptr;  // get the variable number of arguments into a list
    va_start(argptr, format);
	wxString message = wxString::FormatV(format, argptr);

	// translate multiple carriage returns to a single <br>
	message.Replace(_("\r"),_("\n"));
	message.Replace(_("\n\n"),_("\n"));
	message.Replace(_("\n"),_("<br>"));

	// get a string of the current date/time
    wxString cur_dt = wxNow();

	// translate priority to a string
	wxString pri;
	switch(priority)
    {
        case LOG_CRITICAL:
            pri = _("CRITICAL");
            break;

        case LOG_ERROR:
            pri = _("ERROR");
            break;

        case LOG_WARNING:
            pri = _("WARNING");
            break;

        case LOG_INFO:
            pri = _("INFO");
            break;

        case LOG_DEBUG:
            pri = _("DEBUG");
            break;

        default:
            pri = _("(UNKNOWN)");
            break;
    }

	// append everything... so now we have the final string we are trying to log
	wxString output;
	output = cur_dt + _(", ") + pri + _(": ") + message + _("\r\n");

	// log everything to the log file
	LogFile.Write(output);

	// if log_max_level is debug, route to debug serial port
	if(log_max_level >= LOG_DEBUG)
        WriteDebug(output);

    va_end(argptr);  // clean up
}

void close_logger()
{
    if(log_max_level == -1)
        return;

    log(LOG_DEBUG,"Closing logger.");
    // close the log file
    if (!LogFile.Close())
    {
        if(log_max_level >= LOG_DEBUG)
            WriteDebug("close_logger: Error closing log file!");
    }

    if(log_max_level >= LOG_DEBUG)
        CloseDebug();

    log_max_level = -1;  // the logger is inactive
}


