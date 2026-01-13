/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2014-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the configuration menu
 *
 */

#ifndef CONFIGMENU_H_
#define CONFIGMENU_H_

#include <vdr/plugin.h>

namespace cecplugin {

/**
 * @class cConfigMenu
 * @brief VDR setup menu page for plugin configuration.
 *
 * Provides a setup interface in VDR's plugin settings menu
 * for configuring plugin options like main menu visibility.
 */
class cConfigMenu: public cMenuSetupPage {
private:
    static inline int mShowMainMenu = true;  ///< Whether to show main menu entry
    static constexpr char const *ENABLEMAINMENU = "EnableMainMenu";  ///< Setup key

protected:
    /**
     * @brief Stores the current settings to VDR's setup.conf.
     */
    virtual void Store(void);

public:
    /**
     * @brief Constructs the configuration menu page.
     */
    cConfigMenu(void);

    /**
     * @brief Gets whether the main menu entry should be shown.
     * @return true if main menu entry is enabled.
     */
    static const bool GetShowMainMenu(void) { return mShowMainMenu; }

    /**
     * @brief Parses a setup parameter from setup.conf.
     * @param Name Parameter name.
     * @param Value Parameter value as string.
     * @return true if parameter was recognized and parsed.
     */
    static const bool SetupParse(const char *Name, const char *Value);
};

} // namespace cecplugin

#endif /* CONFIGMENU_H_ */
