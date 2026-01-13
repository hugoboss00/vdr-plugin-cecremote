/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the configuration menu
 *
 */

#include "configmenu.h"

namespace cecplugin {


/**
 * @brief Constructs the plugin setup menu page.
 *
 * Creates menu items for plugin configuration options.
 */
cConfigMenu::cConfigMenu() : cMenuSetupPage()
{
    Add(new cMenuEditBoolItem(tr("Show in main menu"), &mShowMainMenu));
}

/**
 * @brief Parses setup values from VDR's setup.conf.
 *
 * @param Name Parameter name
 * @param Value Parameter value string
 * @return true if parameter was recognized, false otherwise
 */
const bool cConfigMenu::SetupParse(const char *Name, const char *Value)
{
    if (strcasecmp(Name, ENABLEMAINMENU) == 0) {
        mShowMainMenu = atoi(Value);
    }
    else {
        return false;
    }
    return true;
}

/**
 * @brief Stores current settings to VDR's setup.conf.
 */
void cConfigMenu::Store(void)
{
    SetupStore(ENABLEMAINMENU, (int)mShowMainMenu);
}

} // namespace cecplugin
