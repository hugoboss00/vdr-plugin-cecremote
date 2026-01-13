/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements a VDR Player which displays a still-picture.
 */

#ifndef _CECREMOTE_STILLPICPLAYER_H_
#define _CECREMOTE_STILLPICPLAYER_H_

#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/player.h>
#include <string>
#include "cecremoteplugin.h"

namespace cecplugin {

// The maximum size of a single frame (up to HDTV 1920x1080):
#define TS_SIZE 188
#define CDMAXFRAMESIZE  (KILOBYTE(1024) / TS_SIZE * TS_SIZE) // multiple of TS_SIZE to avoid breaking up TS packets

/**
 * @class cStillPicPlayer
 * @brief VDR player that displays a still picture during CEC device control.
 *
 * This player loads and displays a still MPEG image while the user interacts
 * with external CEC devices (e.g., DVD/Blu-ray players). The still picture
 * provides visual feedback that VDR is waiting for the external device.
 */
class cStillPicPlayer: public cPlayer {
protected:
    /**
     * @brief Called when the player is activated or deactivated.
     * @param On true when activating, false when deactivating.
     *
     * Loads the still picture when activated.
     */
    void Activate(bool On);

    /**
     * @brief Loads a still picture from file into memory.
     * @param filename Path to the MPEG still picture file.
     */
    void LoadStillPicture (const std::string &filename);

    /**
     * @brief Displays the loaded still picture on screen.
     */
    void DisplayStillPicture ();

    cMutex mPlayerMutex;          ///< Mutex protecting player state
    uchar *pStillBuf = nullptr;   ///< Buffer containing the still picture data
    ssize_t mStillBufLen = 0;     ///< Length of still picture data in bytes
    cCECMenu mConfig;             ///< Menu configuration for this player
public:
    /**
     * @brief Constructs a still picture player.
     * @param config Menu configuration containing the still picture path.
     */
    explicit cStillPicPlayer(const cCECMenu &config) : mConfig(config) {};

    /**
     * @brief Destructor - frees the still picture buffer.
     */
    virtual ~cStillPicPlayer();

    /**
     * @brief Gets the configuration for this player.
     * @return Copy of the menu configuration.
     */
    const cCECMenu getConfig() const {return mConfig;};
};

} // namespace cecplugin

#endif /* _CECREMOTE_STILLPICPLAYER_H_ */
