/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the status monitor for channel switch information.
 */


#ifndef _CECSTATUSMONITOR_H_
#define _CECSTATUSMONITOR_H_

#include <vdr/plugin.h>
#include <vdr/status.h>

#include "cecremoteplugin.h"

namespace cecplugin {

/**
 * @class cStatusMonitor
 * @brief Monitors VDR status changes and triggers CEC commands.
 *
 * This class hooks into VDR's status notification system to execute
 * CEC commands when certain events occur:
 * - Channel switches (TV vs Radio detection)
 * - Replay start/stop
 * - Volume changes
 *
 * The monitor tracks the current playback state and triggers the
 * appropriate command lists from the global configuration.
 */
class cStatusMonitor : public cStatus {
protected:
    /** @brief Playback state enumeration. */
    typedef enum {
        UNKNOWN,    ///< Initial state
        RADIO,      ///< Playing radio channel
        TV,         ///< Playing TV channel
        REPLAYING   ///< Playing a recording
    } MonitorStatus;

    /** @brief Timer change notification (not used). */
    virtual void TimerChange(const cTimer *Timer, eTimerChange Change) {};

    /**
     * @brief Called when a channel switch occurs.
     * @param Device The device that switched channels.
     * @param ChannelNumber The new channel number (0 = no channel).
     * @param LiveView true if this is the live view device.
     *
     * Triggers onswitchtotv or onswitchtoradio commands based on channel type.
     */
    virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView);

    /** @brief Recording notification (not used). */
    virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On) {};

    /**
     * @brief Called when replay starts or stops.
     * @param Control The control object (nullptr when stopping).
     * @param Name Name of the recording.
     * @param FileName File path of the recording.
     * @param On true when starting, false when stopping.
     *
     * Triggers onswitchtoreplay commands when replay starts.
     */
    virtual void Replaying(const cControl *Control, const char *Name,
                           const char *FileName, bool On);

    /**
     * @brief Called when volume changes.
     * @param Volume New volume level.
     * @param Absolute true if absolute volume, false if relative.
     *
     * Triggers onvolumeup or onvolumedown commands based on volume direction.
     */
    virtual void SetVolume(int Volume, bool Absolute);

    // Unused VDR status callbacks
    virtual void SetAudioTrack(int Index, const char * const *Tracks) {};
    virtual void SetAudioChannel(int AudioChannel) {};
    virtual void SetSubtitleTrack(int Index, const char * const *Tracks) {};
    virtual void OsdClear(void) {};
    virtual void OsdTitle(const char *Title) {};
    virtual void OsdStatusMessage(const char *Message) {};
    virtual void OsdHelpKeys(const char *Red, const char *Green,
                             const char *Yellow, const char *Blue) {};
    virtual void OsdItem(const char *Text, int Index) {};
    virtual void OsdCurrentItem(const char *Text) {};
    virtual void OsdTextItem(const char *Text, bool Scroll) {};
    virtual void OsdChannel(const char *Text) {};
    virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle,
                              const char *PresentSubtitle, time_t FollowingTime,
                              const char *FollowingTitle, const char *FollowingSubtitle) {};

    MonitorStatus mMonitorStatus = UNKNOWN;  ///< Current playback state
    cPluginCecremote *mPlugin = nullptr;     ///< Parent plugin instance
    int mVolume = 0;                         ///< Last known volume level
public:
    /** @brief Deleted default constructor - plugin pointer is required. */
    cStatusMonitor() = delete;

    /**
     * @brief Constructs a status monitor.
     * @param plugin Pointer to the parent plugin instance.
     */
    explicit cStatusMonitor(cPluginCecremote *plugin) : mPlugin(plugin) {};

    /** @brief Destructor. */
    virtual ~cStatusMonitor() {};
};

} // namespace cecplugin

#endif /*_CECSTATUSMONITOR_H_ */
