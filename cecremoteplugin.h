/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the main VDR plugin code.
 */

#ifndef CECREMOTEPLUGIN_H
#define CECREMOTEPLUGIN_H

#include <string>

#include <vdr/plugin.h>
#include "cecremote.h"
#include "configmenu.h"
#include "configfileparser.h"
#include "statusmonitor.h"

namespace cecplugin {

class cCECOsd;
class cStatusMonitor;

/**
 * @class cPluginCecremote
 * @brief Main VDR plugin class for CEC remote control functionality.
 *
 * This class is the entry point for the VDR plugin system. It handles:
 * - Plugin lifecycle (initialization, start, stop)
 * - Configuration file parsing
 * - OSD menu integration
 * - SVDRP command interface
 * - Command routing to the CEC remote handler
 */
class cPluginCecremote : public cPlugin {
    friend class cStatusMonitor;
protected:

    int mCECLogLevel = CEC_LOG_ERROR | CEC_LOG_WARNING | CEC_LOG_DEBUG;

    std::string mCfgDir = "cecremote";   ///< Configuration directory name
    std::string mCfgFile = "cecremote.xml";  ///< Configuration file name

    cConfigFileParser mConfigFileParser;  ///< XML configuration parser
    cCECRemote *mCECRemote = nullptr;     ///< CEC communication handler
    cStatusMonitor *mStatusMonitor = nullptr;  ///< VDR status event monitor
    bool mStartManually = true;  ///< true if VDR was started manually (not by timer)

    /**
     * @brief Gets the full path to the configuration directory.
     * @return Absolute path to the config directory.
     */
    const std::string GetConfigDir(void) {
        const std::string cfdir = ConfigDirectory();
        if (mCfgDir[0] == '/') {
            return mCfgDir  + "/";
        }
        return cfdir + "/" + mCfgDir + "/";
    }

    /**
     * @brief Gets the full path to the configuration file.
     * @return Absolute path to cecremote.xml.
     */
    const std::string GetConfigFile(void) {
        const std::string cf = GetConfigDir() + mCfgFile;
        return cf;
    }

    /**
     * @brief Executes a power toggle command for a menu item.
     * @param menu The menu configuration containing power on/off commands.
     */
    void ExecToggle(cCECMenu menu) {
        cCmd cmd(CEC_EXECTOGGLE, menu.mDevice, menu.mOnPowerOn, menu.mOnPowerOff);
        mCECRemote->PushWaitCmd(cmd);
    }

    /**
     * @brief Gets the current plugin status as a string.
     * @return Status information including queue sizes and connection state.
     */
    cString getStatus(void);
public:
    cKeyMaps mKeyMaps;  ///< Key mapping tables (CEC <-> VDR)

    /** @brief Default constructor. */
    explicit cPluginCecremote() {};

    /** @brief Destructor - cleans up CEC remote and status monitor. */
    virtual ~cPluginCecremote();

    /** @brief Returns the plugin version string. */
    virtual const char *Version();

    /** @brief Returns the plugin description. */
    virtual const char *Description();

    /** @brief Returns command line help text. */
    virtual const char *CommandLineHelp();

    /**
     * @brief Processes command line arguments.
     * @param argc Argument count.
     * @param argv Argument values.
     * @return true if arguments were valid.
     */
    virtual bool ProcessArgs(int argc, char *argv[]);

    /**
     * @brief Initializes the plugin (parses config file).
     * @return true if initialization was successful.
     */
    virtual bool Initialize();

    /**
     * @brief Starts the plugin (creates CEC remote handler and status monitor).
     * @return true if startup was successful.
     */
    virtual bool Start();

    /** @brief Stops the plugin and releases resources. */
    virtual void Stop();

    /** @brief Called periodically for housekeeping tasks. */
    virtual void Housekeeping();

    /** @brief Called from VDR's main thread loop. */
    virtual void MainThreadHook();

    /**
     * @brief Checks if plugin is still active (has pending commands).
     * @return Message if active, nullptr if idle.
     */
    virtual cString Active();

    /**
     * @brief Returns the next wakeup time.
     * @return Always returns 0 (no scheduled wakeup).
     */
    virtual time_t WakeupTime();

    /**
     * @brief Returns the main menu entry text.
     * @return Menu entry string or nullptr if disabled.
     */
    virtual const char *MainMenuEntry();

    /**
     * @brief Creates the main menu OSD object.
     * @return New OSD menu instance.
     */
    virtual cOsdObject *MainMenuAction();

    /**
     * @brief Creates the setup menu page.
     * @return New setup menu instance.
     */
    virtual cMenuSetupPage *SetupMenu();

    /**
     * @brief Parses a setup parameter from VDR's setup.conf.
     * @param Name Parameter name.
     * @param Value Parameter value.
     * @return true if parameter was recognized.
     */
    virtual bool SetupParse(const char *Name, const char *Value);

    /**
     * @brief Handles service calls from other plugins.
     * @param Id Service identifier.
     * @param Data Service data (optional).
     * @return true if service was handled.
     */
    virtual bool Service(const char *Id, void *Data = nullptr);

    /**
     * @brief Returns SVDRP help pages.
     * @return Array of help strings.
     */
    virtual const char **SVDRPHelpPages();

    /**
     * @brief Handles SVDRP commands.
     * @param Command The SVDRP command.
     * @param Option Command options.
     * @param ReplyCode Reply code to return.
     * @return Command response.
     */
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);

    /** @brief Initializes default key mappings. */
    void SetDefaultKeymaps();

    /**
     * @brief Starts the still picture player for a menu item.
     * @param menuitem Menu configuration for the player.
     */
    void StartPlayer(const cCECMenu &menuitem);

    /**
     * @brief Pushes a command to the CEC remote queue.
     * @param cmd Command to execute.
     */
    void PushCmd(const cCmd &cmd) {mCECRemote->PushCmd(cmd);}

    /**
     * @brief Pushes multiple commands to the CEC remote queue.
     * @param cmdList List of commands to execute.
     */
    void PushCmdQueue(const cCmdQueue &cmdList) {mCECRemote->PushCmdQueue(cmdList);}

    /**
     * @brief Gets the list of configured menu items.
     * @return Pointer to the menu list.
     */
    cCECMenuList *GetMenuList() {return &mConfigFileParser.mMenuList;}

    /**
     * @brief Checks if VDR was started manually.
     * @return true if manual start, false if timer-triggered.
     */
    bool GetStartManually() {return mStartManually;}

    /**
     * @brief Gets the map of CEC command handlers.
     * @return Pointer to the command handler map.
     */
    mapCommandHandler *GetCECCommandHandlers() {
        return &mConfigFileParser.mGlobalOptions.mCECCommandHandlers;
    }

    /**
     * @brief Finds a menu configuration by name.
     * @param menuname Name of the menu to find.
     * @param menu Output parameter for the found menu.
     * @return true if menu was found.
     */
    bool FindMenu(const std::string &menuname, cCECMenu &menu) {
        return mConfigFileParser.FindMenu(menuname, menu);
    }
};

} // namespace cecplugin

#endif
