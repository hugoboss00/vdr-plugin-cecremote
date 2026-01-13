/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2014-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the VDR cControl for the cec player
 *
 */

#include "ceccontrol.h"
#include "stillpicplayer.h"
#include "ceclog.h"

namespace cecplugin {

/**
 * @brief Constructs a CEC control for still picture playback.
 *
 * Creates a player for the specified menu item, activates the
 * appropriate keymaps, and executes the onStart command queue.
 *
 * @param menuitem Reference to the menu configuration
 * @param plugin Pointer to the parent plugin instance
 */
cCECControl::cCECControl(const cCECMenu &menuitem, cPluginCecremote *plugin) :
    cControl(new cStillPicPlayer(menuitem))
{
    mPlugin = plugin;
    mMenuItem = menuitem;
    mConfig = static_cast<cStillPicPlayer *>(player)->getConfig();
    mPlugin->mKeyMaps.SetActiveKeymaps(menuitem.mVDRKeymap, menuitem.mCECKeymap, cKeyMaps::DEFAULTKEYMAP);
    mPlugin->PushCmdQueue(mMenuItem.mOnStart);
}

/**
 * @brief Destructor that executes onStop commands and restores keymaps.
 */
cCECControl::~cCECControl() {
    mPlugin->PushCmdQueue(mMenuItem.mOnStop);
    mPlugin->SetDefaultKeymaps();
}

/**
 * @brief Called when the OSD should be hidden.
 */
void cCECControl::Hide(void)
{
    Dsyslog("Hide cCECControl");
}

/**
 * @brief Processes VDR key events during still picture playback.
 *
 * Handles stop keys to end playback, checks for key-specific
 * command queues, and forwards other keys to the CEC device.
 *
 * @param key The pressed VDR key
 * @return OS state indicating playback continuation or exit
 */
eOSState cCECControl::ProcessKey(eKeys key)
{
    cKey k;
    if (key != kNone) {
        Dsyslog("cCECControl ProcessKey %d %s", key, k.ToString(key,false));
    }
    if (mMenuItem.isStopKey(key)) {
        Hide();
        return osEnd;
    }
    if (key == kNone) {
        return osContinue;
    }

    mCmdQueueKeyMap::iterator it = mMenuItem.mCmdQueueKey.find(key);
    if (it != mMenuItem.mCmdQueueKey.end())
    {
        mPlugin->PushCmdQueue(it->second);
    }
    else
    {
        key = (eKeys)((int)key & ~k_Repeat);
        cCmd cmd(CEC_VDRKEYPRESS, (int)key, &mMenuItem.mDevice);
        mPlugin->PushCmd(cmd);
    }
    return (osContinue);
}

} // namespace cecplugin

