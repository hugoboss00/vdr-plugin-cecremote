/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2019 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * configfileparser.h: Class for parsing the plugin configuration file.
 */

#ifndef CONFIGFILEPARSER_H_
#define CONFIGFILEPARSER_H_

#include <pugixml.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include <string>
#include <list>
#include <map>
#include <queue>
#include <set>

#include "cecremote.h"
#include "stringtools.h"

namespace cecplugin {

/**
 * @class cCECCommandHandler
 * @brief Handler for responding to specific CEC opcodes.
 *
 * Defines actions to take when a specific CEC command is received,
 * configured via <onceccommand> in XML.
 */
class cCECCommandHandler {
public:
    cCmdQueue mCommands;      ///< Commands to execute on opcode
    std::string mExecMenu;    ///< Menu to execute (optional)
    std::string mStopMenu;    ///< Menu's player to stop (optional)
    cec_opcode mCecOpCode;    ///< CEC opcode to handle
    cCECDevice mDevice;       ///< Initiator device filter
public:
    /** @brief Default constructor. */
    cCECCommandHandler() : mCecOpCode(CEC_OPCODE_NONE) {};
};

typedef std::multimap<cec_opcode, cCECCommandHandler> mapCommandHandler;
typedef mapCommandHandler::iterator mapCommandHandlerIterator;

typedef std::set<eKeys> keySet;

/**
 * @class cCECGlobalOptions
 * @brief Stores global configuration options from <global> XML element.
 *
 * Contains all plugin-wide settings including debug levels, command lists
 * for various events, key maps, and CEC adapter configuration.
 */
class cCECGlobalOptions {
public:
    int cec_debug = 7;                    ///< CEC debug level (see cec_log_level)
    uint32_t mComboKeyTimeoutMs = 1000;   ///< Combo key timeout in milliseconds
    int mHDMIPort = CEC_DEFAULT_HDMI_PORT; ///< HDMI port number
    int mStartupDelay = 0;                ///< Delay before CEC initialization (seconds)
    int32_t mPhysicalAddress = -1;        ///< Physical CEC address (-1 = auto)
    cec_logical_address mBaseDevice = CECDEVICE_UNKNOWN;  ///< Base device address
    cCECDevice mAudioDevice;              ///< Audio device for volume routing
    cCmdQueue mOnStart;                   ///< Commands on plugin start
    cCmdQueue mOnStop;                    ///< Commands on plugin stop
    cCmdQueue mOnVolumeUp;                ///< Commands on volume increase
    cCmdQueue mOnVolumeDown;              ///< Commands on volume decrease
    cCmdQueue mOnManualStart;             ///< Commands on manual start (not timer)
    cCmdQueue mOnSwitchToTV;              ///< Commands on switch to TV channel
    cCmdQueue mOnSwitchToRadio;           ///< Commands on switch to radio
    cCmdQueue mOnSwitchToReplay;          ///< Commands on replay start
    deviceTypeList mDeviceTypes;          ///< CEC device types to register
    std::string mCECKeymap = cKeyMaps::DEFAULTKEYMAP;    ///< Active CEC keymap ID
    std::string mVDRKeymap = cKeyMaps::DEFAULTKEYMAP;    ///< Active VDR keymap ID
    std::string mGLOBALKeymap = cKeyMaps::DEFAULTKEYMAP; ///< Active global keymap ID
    bool mShutdownOnStandby = false;      ///< Send standby on VDR shutdown
    bool mPowerOffOnStandby = false;      ///< Send power off on VDR shutdown
    bool mRTCDetect = true;               ///< Use RTC to detect manual start
    mapCommandHandler mCECCommandHandlers; ///< Handlers for CEC opcodes

    /** @brief Default constructor. */
    cCECGlobalOptions() = default;
};

typedef std::map<std::string, cCECDevice> mCECDeviceMap;
typedef std::map<eKeys, cCmdQueue> mCmdQueueKeyMap;

/**
 * @class cCECMenu
 * @brief Configuration for a single menu entry from <menu> XML element.
 *
 * Stores all settings for a CEC device menu item, including the commands
 * to execute on selection, stop keys, key maps, and power toggle behavior.
 */
class cCECMenu {
    friend class cConfigFileParser;
public:
    /** @brief Menu operation mode. */
    typedef enum {
        UNDEFINED,      ///< Not yet configured
        USE_ONSTART,    ///< Uses onstart/onstop (player mode)
        USE_ONPOWER     ///< Uses onpoweron/onpoweroff (toggle mode)
    } PowerToggleState;

    std::string mMenuTitle;         ///< Display name in OSD menu
    std::string mStillPic;          ///< Path to still picture file
    keySet mStopKeys;               ///< Keys that stop the player
    cCECDevice mDevice;             ///< Target CEC device
    cCmdQueue mOnStart;             ///< Commands on menu selection
    cCmdQueue mOnStop;              ///< Commands on player stop
    cCmdQueue mOnPowerOn;           ///< Commands when device is off
    cCmdQueue mOnPowerOff;          ///< Commands when device is on
    mCmdQueueKeyMap mCmdQueueKey;   ///< Custom key->command mappings
    cCmdQueue mOnVolumeUp;          ///< Commands on volume up (player mode)
    cCmdQueue mOnVolumeDown;        ///< Commands on volume down (player mode)
    std::string mCECKeymap;         ///< CEC keymap ID for player
    std::string mVDRKeymap;         ///< VDR keymap ID for player

    /** @brief Default constructor. */
    cCECMenu() : mCECKeymap(cKeyMaps::DEFAULTKEYMAP),
                 mVDRKeymap(cKeyMaps::DEFAULTKEYMAP),
                 mPowerToggle(UNDEFINED) {};

    /**
     * @brief Checks if menu is in power toggle mode.
     * @return true if using onpoweron/onpoweroff.
     */
    bool isMenuPowerToggle() const { return (mPowerToggle == USE_ONPOWER); };

    /**
     * @brief Checks if a key should stop the player.
     * @param key Key to check.
     * @return true if key is configured as a stop key.
     */
    bool isStopKey(eKeys key) { return (mStopKeys.find(key) != mStopKeys.end()); };
private:
    PowerToggleState mPowerToggle;  ///< Current menu mode
};

typedef std::list<cCECMenu> cCECMenuList;
typedef cCECMenuList::const_iterator cCECMenuListIterator;

/**
 * @class cCECConfigException
 * @brief Exception thrown when a syntax error occurs in the config file.
 */
class cCECConfigException : public std::exception {
private:
    int mLineNr = -1;       ///< Line number where error occurred
    std::string mTxt;       ///< Error description
public:
    /**
     * @brief Constructs exception with line number and message.
     * @param linenr Line number where error occurred.
     * @param txt Error description.
     */
    explicit cCECConfigException(int linenr, const std::string &txt) {
        mLineNr = linenr;
        mTxt = txt;
    }

    /** @brief Destructor. */
    ~cCECConfigException() throw() {};

    /**
     * @brief Returns formatted error message.
     * @return Error text including line number.
     */
    const char *what() const throw() {
        char buf[10];
        sprintf(buf,"%d\n", mLineNr);
        static std::string s = "Syntax error in line ";
        s += buf + mTxt;
        return s.c_str();
    }
};

/**
 * @class cConfigFileParser
 * @brief Parses the XML configuration file for the CEC plugin.
 *
 * Reads and validates the XML configuration file (cecremote.xml),
 * populating cCECGlobalOptions, cCECMenuList, and cKeyMaps with
 * the parsed configuration data.
 */
class cConfigFileParser {
private:
    /**
     * @brief Throws an error if a node has unexpected child elements.
     * @param node XML node to check.
     */
    void checkSubElement(pugi::xml_node node);

    /**
     * @brief Gets line number from byte offset in XML file.
     * @param offset Byte offset from XML parser.
     * @return Corresponding line number.
     */
    int getLineNumber(long offset);

    /**
     * @brief Converts device type string to enum value.
     * @param s Device type string (e.g., "RECORDING_DEVICE").
     * @return Corresponding cec_device_type value.
     */
    cec_device_type getDeviceType(const std::string &s);

    /**
     * @brief Checks if a node has child elements.
     * @param node XML node to check.
     * @return true if node contains element children.
     */
    bool hasElements(const pugi::xml_node node);

    /**
     * @brief Parses a device reference from text.
     * @param text Device name or logical address.
     * @param device Output device structure.
     * @param linenr Line number for error reporting.
     */
    void getDevice(const char *text, cCECDevice &device, ptrdiff_t linenr);

    /**
     * @brief Converts "true"/"false" string to boolean.
     * @param text String to convert.
     * @param val Output boolean value.
     * @return true if conversion succeeded.
     */
    bool textToBool(const char *text, bool &val);

    /** @brief Converts text to int. */
    bool textToInt(const char *text, int &val, int base = 0) {
            int v;
            std::string s = text;
            bool ret = StringTools::TextToInt(s, v, base);
            val = v;
            return ret;
        };

    /** @brief Converts text to uint16_t. */
    bool textToInt(const char *text, uint16_t &val, int base = 0) {
        int v;
        std::string s = text;
        bool ret = StringTools::TextToInt(s, v, base);
        val = v;
        return ret;
    };

    /** @brief Converts text to uint32_t. */
    bool textToInt(const char *text, uint32_t &val, int base = 0) {
        int v;
        std::string s = text;
        bool ret = StringTools::TextToInt(s, v, base);
        val = v;
        return ret;
    };

    /** @brief Converts text to cec_logical_address. */
    bool textToInt(const char *text, cec_logical_address &val, int base = 0) {
        int v;
        std::string s = text;
        bool ret = StringTools::TextToInt(s, v, base);
        val = (cec_logical_address)v;
        return ret;
    };

    /** @brief Converts text to cec_opcode. */
    bool textToInt(std::string text, cec_opcode &val, int base = 0) {
        int v;
        bool ret = StringTools::TextToInt(text, v, base);
        val = (cec_opcode)v;
        return ret;
    };

    /** @brief Parses <vdrkeymap> element. */
    void parseVDRKeymap(const pugi::xml_node node, cKeyMaps &keymaps);

    /** @brief Parses <ceckeymap> element. */
    void parseCECKeymap(const pugi::xml_node node, cKeyMaps &keymaps);

    /** @brief Parses <globalkeymap> element. */
    void parseGLOBALKeymap(const pugi::xml_node node, cKeyMaps &keymaps);

    /** @brief Parses <global> element and its children. */
    void parseGlobal(const pugi::xml_node node);

    /** @brief Parses <menu> element and its children. */
    void parseMenu(const pugi::xml_node node);

    /**
     * @brief Parses a command list (onstart, onstop, etc.).
     * @param node XML node containing the command list.
     * @param cmdlist Output command queue.
     */
    void parseList(const pugi::xml_node node, cCmdQueue &cmdlist);

    /**
     * @brief Parses <player> element within a menu.
     * @param node XML player node.
     * @param menu Menu to update with player settings.
     */
    void parsePlayer(const pugi::xml_node node, cCECMenu &menu);

    /** @brief Parses <device> element. */
    void parseDevice(const pugi::xml_node node);

    /** @brief Parses <onceccommand> element. */
    void parseOnCecCommand(const pugi::xml_node node);

    // Keywords used in the XML config file
    static constexpr char const *XML_GLOBAL = "global";
    static constexpr char const *XML_MENU = "menu";
    static constexpr char const *XML_CECKEYMAP = "ceckeymap";
    static constexpr char const *XML_VDRKEYMAP = "vdrkeymap";
    static constexpr char const *XML_GLOBALKEYMAP = "globalkeymap";
    static constexpr char const *XML_ONSTART = "onstart";
    static constexpr char const *XML_ONSTOP = "onstop";
    static constexpr char const *XML_ONPOWERON = "onpoweron";
    static constexpr char const *XML_ONPOWEROFF = "onpoweroff";
    static constexpr char const *XML_ID  = "id";
    static constexpr char const *XML_KEY  = "key";
    static constexpr char const *XML_CODE = "code";
    static constexpr char const *XML_VALUE = "value";
    static constexpr char const *XML_STOP = "stop";
    static constexpr char const *XML_KEYMAPS = "keymaps";
    static constexpr char const *XML_FILE = "file";
    static constexpr char const *XML_CEC = "cec";
    static constexpr char const *XML_VDR = "vdr";
    static constexpr char const *XML_GLOBALVDR  = "globalvdr";
    static constexpr char const *XML_POWERON = "poweron";
    static constexpr char const *XML_POWEROFF = "poweroff";
    static constexpr char const *XML_MAKEACTIVE = "makeactive";
    static constexpr char const *XML_MAKEINACTIVE = "makeinactive";
    static constexpr char const *XML_EXEC = "exec";
    static constexpr char const *XML_TEXTVIEWON = "textviewon";
    static constexpr char const *XML_COMBOKEYTIMEOUTMS = "combokeytimeoutms";
    static constexpr char const *XML_CECDEBUG = "cecdebug";
    static constexpr char const *XML_CECDEVICETYPE = "cecdevicetype";
    static constexpr char const *XML_DEVICE = "device";
    static constexpr char const *XML_PHYSICAL = "physical";
    static constexpr char const *XML_LOGICAL = "logical";
    static constexpr char const *XML_ONMANUALSTART = "onmanualstart";
    static constexpr char const *XML_ONSWITCHTOTV = "onswitchtotv";
    static constexpr char const *XML_ONSWITCHTORADIO = "onswitchtoradio";
    static constexpr char const *XML_ONSWITCHTOREPLAY = "onswitchtoreplay";
    static constexpr char const *XML_ONACTIVESOURCE = "onactivesource";
    static constexpr char const *XML_HDMIPORT = "hdmiport";
    static constexpr char const *XML_SHUTDOWNONSTANDBY = "shutdownonstandby";
    static constexpr char const *XML_POWEROFFONSTANDBY = "poweroffonstandby";
    static constexpr char const *XML_BASEDEVICE = "basedevice";
    static constexpr char const *XML_ONCECCOMMAND = "onceccommand";
    static constexpr char const *XML_EXECMENU = "execmenu";
    static constexpr char const *XML_STOPMENU = "stopmenu";
    static constexpr char const *XML_COMMANDLIST = "commandlist";
    static constexpr char const *XML_COMMAND = "command";
    static constexpr char const *XML_INITIATOR = "initiator";
    static constexpr char const *XML_RTCDETECT = "rtcdetect";
    static constexpr char const *XML_STARTUPDELAY = "startupdelay";
    static constexpr char const *XML_ONKEY = "onkey";
    static constexpr char const *XML_ONVOLUMEUP = "onvolumeup";
    static constexpr char const *XML_ONVOLUMEDOWN = "onvolumedown";
    static constexpr char const *XML_AUDIODEVICE = "audiodevice";

    const char* mXmlFile = nullptr;  ///< Path to the configuration file

public:
    cCECGlobalOptions mGlobalOptions;  ///< Parsed global options
    cCECMenuList mMenuList;            ///< Parsed menu items
    mCECDeviceMap mDeviceMap;          ///< Parsed device definitions

    /** @brief Default constructor. */
    cConfigFileParser() = default;

    /**
     * @brief Parses the XML configuration file.
     * @param filename Path to the configuration file.
     * @param keymaps Key maps to populate.
     * @return true if parsing succeeded, false on error.
     *
     * Populates mGlobalOptions, mMenuList, mDeviceMap, and the provided
     * keymaps with data from the XML file.
     */
    bool Parse(const std::string &filename, cKeyMaps &keymaps);

    /**
     * @brief Finds a menu configuration by name.
     * @param menuname Name of the menu to find.
     * @param menu Output parameter for the found menu.
     * @return true if menu was found.
     */
    bool FindMenu(const std::string &menuname, cCECMenu &menu);
};

} // namespace cecplugin
#endif /* CONFIGFILEPARSER_H_ */
