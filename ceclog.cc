/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2014, 2015 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements logging functions
 *
 */

#include <vdr/plugin.h>
#include "ceclog.h"

namespace cecplugin {

int cecplugin_loglevel = SysLogLevel;

/**
 * @brief Logs a message to syslog with severity filtering.
 *
 * Formats the message with [cecremote] prefix and routes to
 * appropriate syslog facility based on severity level.
 *
 * @param severity Log severity (0=error, 1=warning, 2=debug)
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void ceclogmsg (int severity, const char *format, ...)
{
    if (cecplugin_loglevel > severity) {
        char fmt[MAXSYSLOGBUF];

        int facility_priority = LOG_ERR;
        if (severity == 1) {
            facility_priority = LOG_WARNING;
        }
        else if (severity == 2) {
            facility_priority = LOG_DEBUG;
        }

        snprintf(fmt, sizeof(fmt)-1, "[cecremote] %s", format);
        va_list ap;
        va_start(ap, format);
        vsyslog(facility_priority ,fmt, ap);
        va_end(ap);
    }
}

}
