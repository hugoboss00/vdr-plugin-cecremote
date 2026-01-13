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

#ifndef CECLOG_H_
#define CECLOG_H_

namespace cecplugin {

#define MAXSYSLOGBUF 1024
extern int cecplugin_loglevel;  ///< Current logging level (0=error, 1=info, 2=debug)

/**
 * @brief Logs a message to syslog if severity is at or below current level.
 * @param severity Message severity (0=error, 1=info, 2=debug).
 * @param format Printf-style format string.
 * @param ... Format arguments.
 */
void ceclogmsg (int severity, const char *format, ...);

/** @brief Log an error message (always logged). */
#define Esyslog(a...) ceclogmsg(0, a)

/** @brief Log an info message (logged if level >= 1). */
#define Isyslog(a...) ceclogmsg(1, a)

/** @brief Log a debug message (logged if level >= 2). */
#define Dsyslog(a...) ceclogmsg(2, a)

#ifdef VERBOSEDEBUG
/** @brief Log a verbose debug message (only when VERBOSEDEBUG is defined). */
#define Csyslog(a...) ceclogmsg(0, a)
#else
/** @brief Verbose debug logging disabled - expands to nothing. */
#define Csyslog(a...)
#endif

} // namespace cecplugin

#endif /* CECLOG_H_ */
