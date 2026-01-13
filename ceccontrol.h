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

#ifndef CECONTROL_H_
#define CECONTROL_H_

#include <vdr/player.h>

#include "cecremoteplugin.h"
#include "stillpicplayer.h"

namespace cecplugin {

/**
 * @class cCECControl
 * @brief VDR control class for managing CEC player interaction.
 *
 * This class handles user input while a still picture player is active,
 * translating VDR keys to CEC commands based on the configured key maps.
 * It manages the player lifecycle and handles stop keys to return to VDR.
 */
class cCECControl: public cControl {
private:
   
    cPluginCecremote *mPlugin = nullptr;  ///< Parent plugin instance
    cCECMenu mMenuItem;  ///< Menu item that started this control
    cCECMenu mConfig;    ///< Configuration for the active player
public:
    /** @brief Deleted default constructor - menu item and plugin are required. */
    cCECControl() = delete;

    /**
     * @brief Constructs a CEC control for a menu item.
     * @param menuitem Menu configuration containing player settings.
     * @param plugin Pointer to the parent plugin instance.
     */
    explicit cCECControl(const cCECMenu &menuitem, cPluginCecremote *plugin);

    /**
     * @brief Destructor - executes onstop commands and restores key maps.
     */
    virtual ~cCECControl();

    /**
     * @brief Hides any OSD elements (not used).
     */
    virtual void Hide(void);

    /**
     * @brief Returns info object for the control (not used).
     * @return Always nullptr.
     */
    virtual cOsdObject *GetInfo(void) { return nullptr; }

    /**
     * @brief Processes a key press while player is active.
     * @param Key The VDR key that was pressed.
     * @return osEnd if stop key pressed, osContinue otherwise.
     *
     * Handles stop keys to end playback, volume keys, and
     * custom key mappings from the player configuration.
     */
    virtual eOSState ProcessKey(eKeys Key);

    /**
     * @brief Gets the menu title for this control.
     * @return The menu title string.
     */
    std::string getMenuTitle() const {
        return mMenuItem.mMenuTitle;
    }

    /**
     * @brief Gets the configuration for this control.
     * @return Copy of the menu configuration.
     */
    const cCECMenu getConfig(void) const {return mConfig;};
};

} // namespace cecplugin

#endif /* CECONTROL_H_ */
