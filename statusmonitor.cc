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

#include "statusmonitor.h"
#include "ceclog.h"
#include "ceccontrol.h"

namespace cecplugin {

/**
 * @brief Handles VDR channel switch events.
 *
 * Monitors channel switches on the primary device to detect
 * transitions between TV and Radio modes, executing configured
 * command queues when the mode changes.
 *
 * @param Device The device that switched channels
 * @param ChannelNumber The new channel number
 * @param LiveView Whether this is live viewing
 */
void cStatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber,
                                   bool LiveView)
{
    char l = 'f';
    if (LiveView) {
        l = 't';
    }
    if (Device->IsPrimaryDevice()) {
        Dsyslog("Primary device, Channel Switch %d %c", ChannelNumber,l);
#if (APIVERSNUM >= 20301)
        LOCK_CHANNELS_READ;
        const cChannel* channel = Channels->GetByNumber(ChannelNumber);
#else
        const cChannel* channel = Channels.GetByNumber(ChannelNumber);
#endif
        if (channel != NULL) {
            if (channel->Vpid() == 0) {
                Dsyslog("  Radio : %s", channel->Name());
                if (mMonitorStatus != RADIO) {
                    // Ignore first switch, this is covered by <onstart>
                    if (mMonitorStatus != UNKNOWN) {
                        mPlugin->PushCmdQueue(mPlugin->mConfigFileParser.
                                              mGlobalOptions.mOnSwitchToRadio);
                    }
                    mMonitorStatus = RADIO;
                }
            }
            else {
                Dsyslog("  TV    : %s", channel->Name());
                if (mMonitorStatus != TV) {
                    // Ignore first switch, this is covered by <onstart>
                    if (mMonitorStatus != UNKNOWN) {
                        mPlugin->PushCmdQueue(mPlugin->mConfigFileParser.
                                              mGlobalOptions.mOnSwitchToTV);
                    }
                    mMonitorStatus = TV;
                }
            }
        }
        Csyslog("Channel switch OK");
    }
    else {
        Dsyslog("Not primary device, Channel Switch %d %c", ChannelNumber, l);
    }
}

/**
 * @brief Handles VDR replay start/stop events.
 *
 * Executes the configured onSwitchToReplay command queue
 * when replay mode begins.
 *
 * @param Control The control object for the replay
 * @param Name Name of the recording being played
 * @param FileName File path of the recording
 * @param On true if replay started, false if stopped
 */
void cStatusMonitor::Replaying(const cControl *Control, const char *Name,
                               const char *FileName, bool On)
{
    Dsyslog("Replaying");
    if (On) {
        if (mMonitorStatus != REPLAYING) {
            mMonitorStatus = REPLAYING;
            mPlugin->PushCmdQueue(mPlugin->mConfigFileParser.mGlobalOptions.mOnSwitchToReplay);
        }
    }
}

/**
 * @brief Handles VDR volume change events.
 *
 * Forwards volume changes to the configured audio device via CEC,
 * and executes any menu-specific volume handlers if a still
 * picture player is running.
 *
 * @param Volume Volume value or delta
 * @param Absolute true if Volume is absolute, false if relative
 */
void cStatusMonitor::SetVolume(int Volume, bool Absolute)
{
    cCECMenu menuitem;
    int newvol = 128;
    Dsyslog("SetVolume %d %d", Volume, Absolute);
    if (mVolume > 0) {
        newvol = mVolume;
    }
    if (!Absolute) {
        newvol += Volume;
    }
    else {
        newvol = Volume;
    }
    // Always ignore first call.
    if (mVolume == -1) {
        mVolume = newvol;
        return;
    }

    // No volume change, return
    if (newvol == mVolume)
        return;

    // Handle global volume keypresses
    if (newvol > mVolume) {
        cCmd cmd(CEC_VDRKEYPRESS, (int)kVolUp, &mPlugin->mConfigFileParser.mGlobalOptions.mAudioDevice);
        mPlugin->PushCmd(cmd);
    }
    else {
        cCmd cmd(CEC_VDRKEYPRESS, (int)kVolDn, &mPlugin->mConfigFileParser.mGlobalOptions.mAudioDevice);
        mPlugin->PushCmd(cmd);
    }

    cMutexLock lock;
    cControl *c = cControl::Control(lock);
    if (c == NULL) {
        mVolume = newvol;
        return;
    }
    cCECControl *cont = dynamic_cast<cCECControl*>(c);
    if (cont == NULL) {
        mVolume = newvol;
        return;
    }
    Dsyslog("Stillpic Player running %s", cont->getMenuTitle().c_str());
    menuitem = cont->getConfig();
    if (newvol > mVolume) {
        mPlugin->PushCmdQueue(menuitem.mOnVolumeUp);
    }
    else {
        mPlugin->PushCmdQueue(menuitem.mOnVolumeDown);
    }
    mVolume = newvol;
}

} // namespace cecplugin
