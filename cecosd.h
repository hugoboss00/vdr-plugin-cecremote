/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the OSD menu.
 */

#ifndef CECOSD_H_
#define CECOSD_H_

#include <vdr/menu.h>
#include <vdr/plugin.h>
#include "ceccontrol.h"

namespace cecplugin {

/**
 * @class cCECOsd
 * @brief Main OSD menu displaying available CEC control options.
 *
 * Creates a menu with entries for each configured CEC device/action
 * from the XML configuration file.
 */
class cCECOsd : public cOsdMenu {
public:
    static std::vector<cCECMenu> mMenuItems;  ///< Static storage of menu items

    /**
     * @brief Constructs the CEC OSD menu.
     * @param plugin Pointer to the parent plugin instance.
     */
    cCECOsd(cPluginCecremote *plugin);

    /** @brief Destructor. */
    virtual ~cCECOsd() {}
};

/**
 * @class cCECOsdItem
 * @brief Individual menu item for a CEC device or action.
 *
 * Each item represents one <menu> entry from the XML configuration.
 * Selecting an item either starts a player or toggles device power.
 */
class cCECOsdItem : public cOsdItem {
private:
    cCECControl *mControl;        ///< Associated control (for player mode)
    cPluginCecremote *mPlugin;    ///< Parent plugin instance
    cCECMenu mMenuItem;           ///< Menu configuration for this item

public:
    /**
     * @brief Constructs a CEC OSD menu item.
     * @param menuitem Menu configuration from XML.
     * @param menutxt Display text for the menu item.
     * @param plugin Pointer to the parent plugin instance.
     */
    cCECOsdItem(const cCECMenu &menuitem, const char *menutxt, cPluginCecremote *plugin);

    /** @brief Destructor. */
    ~cCECOsdItem() {}

    /**
     * @brief Processes a key press on this menu item.
     * @param key The VDR key that was pressed.
     * @return osPlugin to activate player, osContinue otherwise.
     */
    virtual eOSState ProcessKey(eKeys key);
};

} // namespace cecplugin

#endif /* CECOSD_H_ */
