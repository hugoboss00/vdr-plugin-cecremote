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


#include <getopt.h>
#include <stdlib.h>

#include "cecremoteplugin.h"
#include "ceclog.h"
#include "cecosd.h"
#include "stringtools.h"
#include "keymaps.h"
#include "configmenu.h"
#include "rtcwakeup.h"

namespace cecplugin {

static const char *VERSION        = "2.0.1";
static const char *DESCRIPTION    = "Send/Receive CEC commands";
static const char *MAINMENUENTRY  = "CECremote";
using namespace std;

/**
 * @brief Destructor that cleans up CEC resources.
 *
 * Deletes the CEC remote handler and status monitor if they exist.
 */
cPluginCecremote::~cPluginCecremote()
{
    if (mCECRemote != nullptr) {
        delete mCECRemote;
        mCECRemote = nullptr;
    }
    if (mStatusMonitor != nullptr) {
        delete mStatusMonitor;
        mStatusMonitor = nullptr;
    }
}

/**
 * @brief Returns the plugin version string.
 * @return Pointer to static version string
 */
const char *cPluginCecremote::Version(void)
{
    return VERSION;
}

/**
 * @brief Returns a brief description of the plugin.
 * @return Pointer to static description string
 */
const char *cPluginCecremote::Description(void)
{
    return DESCRIPTION;
}

/**
 * @brief Returns the main menu entry text.
 *
 * Returns the menu entry text if the main menu option is enabled,
 * or nullptr to hide from the main menu.
 *
 * @return Main menu entry string or nullptr
 */
const char *cPluginCecremote::MainMenuEntry(void)
{
    if (cConfigMenu::GetShowMainMenu()) {
        return tr(MAINMENUENTRY);
    }
    return nullptr;
}

/**
 * @brief Returns command line help text.
 * @return String describing available command line options
 */
const char *cPluginCecremote::CommandLineHelp(void)
{
    return "-c  --configdir <dir>     Directory for config files : cecremote\n"
           "-x  --configfile <file>   Config file : cecremote.xml\n"
           "-l  --loglevel <level>    Log level (0-3, not specified: VDR's log level)";
}

/**
 * @brief Processes command line arguments.
 *
 * Parses -c/--configdir, -x/--configfile, and -l/--loglevel options.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return true on success, false on error
 */
bool cPluginCecremote::ProcessArgs(int argc, char *argv[])
{
    static struct option long_options[] =
    {
            { "configdir",      required_argument, nullptr, 'c' },
            { "configfile",     required_argument, nullptr, 'x' },
            { "loglevel",       required_argument, nullptr, 'l' },
            { nullptr }
    };
    int c, option_index = 0;

    while ((c = getopt_long(argc, argv, "c:x:l:",
            long_options, &option_index)) != -1) {
        switch (c) {
        case 'c':
            mCfgDir.assign(optarg);
            break;
        case 'x':
            mCfgFile.assign(optarg);
            break;
        case 'l':
            cecplugin_loglevel = atoi(optarg);
            break;
        default:
            Esyslog("CECRemotePlugin unknown option %c", c);
            return false;
        }
    }

    return true;
}

/**
 * @brief Initializes the plugin.
 *
 * Parses the configuration file, determines startup mode (manual vs timed),
 * creates the CEC remote handler, and sets default keymaps.
 *
 * @return true on success, false if config parsing fails
 */
bool cPluginCecremote::Initialize(void)
{
    string file = GetConfigFile();
    rtcwakeup::RTC_WAKEUP_TYPE rtcwakeup = rtcwakeup::RTC_ERROR;

    if (!mConfigFileParser.Parse(file, mKeyMaps)) {
        Esyslog("Error on parsing config file file %s", file.c_str());
        return false;
    }
    mCECLogLevel = mConfigFileParser.mGlobalOptions.cec_debug;
    if (mConfigFileParser.mGlobalOptions.mRTCDetect) {
        Dsyslog("Use RTC wakeup detection");
        rtcwakeup = rtcwakeup::check();
        mStartManually = (rtcwakeup != rtcwakeup::RTC_WAKEUP);
    }
    // Either rtc wakeup is disabled or not available, so fall back
    // to "old" manual start detection
    if (rtcwakeup == rtcwakeup::RTC_ERROR) {
        Dsyslog("Use VDR wakeup detection: Next Wakeup %d",
                Setup.NextWakeupTime);
        if (Setup.NextWakeupTime > 0) {
            // 600 comes from vdr's MANUALSTART constant in vdr.c
            if (abs(Setup.NextWakeupTime - time(nullptr)) < 600) {
                mStartManually = false;
            }
        }
    }
    if (mStartManually) {
        Dsyslog("manual start");
    }
    else {
        Dsyslog("timed start");
    }
    mCECRemote = new cCECRemote(mConfigFileParser.mGlobalOptions, this);
    SetDefaultKeymaps();

    return true;
}

/**
 * @brief Starts the plugin operation.
 *
 * Starts the CEC remote worker thread and creates the status monitor.
 *
 * @return true always
 */
bool cPluginCecremote::Start(void)
{
    mCECRemote->Startup();
    mStatusMonitor = new cStatusMonitor(this);
    return true;
}

/**
 * @brief Stops the plugin operation.
 *
 * Stops the status monitor and CEC remote handler, executing
 * any configured onStop commands.
 */
void cPluginCecremote::Stop(void)
{
    Dsyslog("Stop Plugin");
    delete mStatusMonitor;
    mStatusMonitor = nullptr;
    mCECRemote->Stop();
    delete mCECRemote;
    mCECRemote = nullptr;
}

/**
 * @brief Performs periodic housekeeping tasks.
 *
 * Called periodically by VDR. Currently not used.
 */
void cPluginCecremote::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

/**
 * @brief Main thread hook for time-critical actions.
 *
 * Called from VDR's main loop. Currently not used.
 * @warning Use with great care - see PLUGINS.html!
 */
void cPluginCecremote::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    // WARNING: Use with great care - see PLUGINS.html!
}

/**
 * @brief Checks if shutdown should be postponed.
 * @return nullptr to allow shutdown, or message string to postpone
 */
cString cPluginCecremote::Active(void)
{
    // Return a message string if shutdown should be postponed
    return nullptr;
}

/**
 * @brief Returns custom wakeup time for shutdown script.
 * @return 0 (no custom wakeup time)
 */
time_t cPluginCecremote::WakeupTime(void)
{
    // Return custom wakeup time for shutdown script
    return 0;
}

/**
 * @brief Starts a CEC player for a menu item.
 *
 * If the menu item has no stillpic, executes the onStart commands only.
 * Otherwise creates and launches a new still picture player.
 *
 * @param menuitem Reference to the menu configuration to start
 */
void cPluginCecremote::StartPlayer(const cCECMenu &menuitem)
{
    // If no <stillpic> is used, execute only onStart section
    if (menuitem.mStillPic.empty()) {
        Isyslog("Executing: %s", menuitem.mMenuTitle.c_str());
        if (menuitem.isMenuPowerToggle()) {
            ExecToggle(menuitem);
        }
        else {
            PushCmdQueue(menuitem.mOnStart);
        }
    }
    // otherwise start a new player
    else {
        Isyslog("starting player: %s", menuitem.mMenuTitle.c_str());
        cControl::Launch(new cCECControl(menuitem, this));
        cControl::Attach();
    }
}

/**
 * @brief Creates the main menu action.
 *
 * If only one menu item exists, executes it directly.
 * Otherwise returns a new OSD menu for selection.
 *
 * @return OSD object for menu display, or nullptr if single action was executed
 */
cOsdObject *cPluginCecremote::MainMenuAction(void)
{
    if (cCECOsd::mMenuItems.size() == 1) {
        StartPlayer(cCECOsd::mMenuItems[0]);
        return nullptr;
    }
    return new cCECOsd(this);
}

/**
 * @brief Creates the setup menu for plugin configuration.
 * @return New configuration menu page
 */
cMenuSetupPage *cPluginCecremote::SetupMenu(void)
{
    return new cConfigMenu();
}

/**
 * @brief Parses setup parameters from VDR's setup.conf.
 *
 * @param Name Parameter name
 * @param Value Parameter value
 * @return true if parameter was handled, false otherwise
 */
bool cPluginCecremote::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    return cConfigMenu::SetupParse(Name, Value);
}

/**
 * @brief Handles custom service requests from other plugins.
 *
 * @param Id Service identifier
 * @param Data Service data
 * @return false (no services implemented)
 */
bool cPluginCecremote::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    return false;
}

/**
 * @brief Returns SVDRP help pages.
 * @return Array of help strings for each SVDRP command
 */
const char **cPluginCecremote::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
            "LSTK\nList known CEC keycodes\n",
            "LSTD\nList CEC devices\n",
            "KEYM\nList available key map\n",
            "VDRK [id]\nDisplay VDR->CEC key map with id\n",
            "CECK [id]\nDisplay CEC->VDR key map with id\n",
            "GLOK [id]\nDisplay Global VDR -> CEC key map with id\n",
            "DISC\nDisconnect CEC",
            "CONN\nConnect CEC",
            "STAT\nPlugin status",
            nullptr
    };
    return HelpPages;
}

/**
 * @brief Processes SVDRP commands.
 *
 * Handles LSTD, LSTK, KEYM, VDRK, CECK, GLOK, DISC, CONN, and STAT commands.
 *
 * @param Command Command name
 * @param Option Command option/argument
 * @param ReplyCode Reference to reply code (set on error)
 * @return Command response string
 */
cString cPluginCecremote::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    ReplyCode = 214;
    if (strcasecmp(Command, "STAT") == 0) {
        return getStatus();
    }
    else if (strcasecmp(Command, "LSTD") == 0) {
        return mCECRemote->ListDevices();
    }
    else if (strcasecmp(Command, "KEYM") == 0) {
        return mKeyMaps.ListKeymaps();
    }
    else if (strcasecmp(Command, "LSTK") == 0) {
        return mKeyMaps.ListKeycodes();
    }
    else if (strcasecmp(Command, "VDRK") == 0) {
        if (Option == nullptr) {
            ReplyCode = 901;
            return "Error: Keymap ID required";
        }
        string s = Option;
        return mKeyMaps.ListVDRKeyMap(s);
    }
    else if (strcasecmp(Command, "CECK") == 0) {
        if (Option == nullptr) {
            ReplyCode = 901;
            return "Error: Keymap ID required";
        }
        string s = Option;
        return mKeyMaps.ListCECKeyMap(s);
    }
    else if (strcasecmp(Command, "GLOK") == 0) {
        if (Option == nullptr) {
            ReplyCode = 901;
            return "Error: Keymap ID required";
        }
        string s = Option;
        return mKeyMaps.ListGLOBALKeyMap(s);
    }
    else if (strcasecmp(Command, "DISC") == 0) {
        cCmd cmd(CEC_DISCONNECT);
        mCECRemote->PushWaitCmd(cmd);
        return "Disconnected";
    }
    else if (strcasecmp(Command, "CONN") == 0) {
        cCmd cmd(CEC_CONNECT);
        mCECRemote->PushWaitCmd(cmd);
        return "Connected";
    }

    ReplyCode = 901;
    return "Error: Unexpected option";
}

/**
 * @brief Sets the default keymaps from configuration.
 *
 * Activates the globally configured VDR, CEC, and GLOBAL keymaps.
 */
void cPluginCecremote::SetDefaultKeymaps()
{
    mKeyMaps.SetActiveKeymaps(mConfigFileParser.mGlobalOptions.mVDRKeymap,
                              mConfigFileParser.mGlobalOptions.mCECKeymap,
                              mConfigFileParser.mGlobalOptions.mGLOBALKeymap);
}

/**
 * @brief Returns plugin status information.
 *
 * Returns log level, queue sizes, and adapter connection state.
 *
 * @return Formatted status string
 */
cString cPluginCecremote::getStatus(void)
{
    cString s;
    const char *buf;
    if (mCECRemote->IsConnected()) {
        buf = "Connected";
    }
    else {
        buf = "Disconnected";
    }
    s = cString::sprintf("Log Level %d\nWork Queue %d\nExec Queue %d\nAdapter %s",
            SysLogLevel,
            mCECRemote->GetWorkQueueSize(),
            mCECRemote->GetExecQueueSize(),
            buf);

    return s;
}

} // namespace cecplugin

VDRPLUGINCREATOR(cecplugin::cPluginCecremote); // Don't touch this!

