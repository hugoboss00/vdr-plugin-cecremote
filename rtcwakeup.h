/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2016 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * rtcwakeup.h: Static helper class to detect if the VDR was started via
 *              the RTC.
 */

#ifndef PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_RTCWAKEUP_H_
#define PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_RTCWAKEUP_H_

namespace cecplugin {

/**
 * @class rtcwakeup
 * @brief Detects whether VDR was started by RTC alarm or manually.
 *
 * This class checks the Linux RTC (Real Time Clock) alarm status to determine
 * if VDR was started due to a scheduled timer (rtcwake) or by manual user action.
 * This information is used to execute different command lists on startup
 * (onstart vs onmanualstart).
 */
class rtcwakeup {
protected:

    static const char *RESET_RTCALARM;  ///< Path to reset RTC alarm
    static const char *RTC_DEVICE;      ///< Path to RTC device
    static const char *ALARM_KEY;       ///< Key to look for in RTC status

    /**
     * @brief Trims whitespace from a string in place.
     * @param str String to trim.
     */
    static void trim(char *str);

    /**
     * @brief Resets the RTC alarm after detection.
     */
    static void reset_alarm(void);

public:
    /** @brief RTC wakeup detection result. */
    typedef enum {
        RTC_WAKEUP,    ///< VDR was started by RTC alarm (timer)
        OTHER_WAKEUP,  ///< VDR was started manually
        RTC_ERROR      ///< Could not determine wakeup reason
    } RTC_WAKEUP_TYPE;

    /**
     * @brief Checks if VDR was started via the RTC.
     * @return RTC_WAKEUP if wakeup from RTC was detected,
     *         OTHER_WAKEUP if no wakeup from the RTC was detected,
     *         RTC_ERROR if it was not possible to detect the startup
     *         reason (e.g., problems accessing /proc or /sys filesystems).
     */
    static RTC_WAKEUP_TYPE check(void);
};

} /* namespace cecplugin */

#endif /* PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_RTCWAKEUP_H_ */
