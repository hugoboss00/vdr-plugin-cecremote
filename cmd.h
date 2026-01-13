/*
 *
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements a data storage for CEC commands.
 */

#ifndef _CCECCMD_H_
#define _CCECCMD_H_

using namespace CEC;

namespace cecplugin {

/**
 * @class cCECDevice
 * @brief Represents a CEC device with physical and logical addresses.
 *
 * Stores device addressing information from the <device> XML tag.
 * A device can be identified by either its physical address (HDMI topology)
 * or logical address (CEC device type).
 */
class cCECDevice {
public:
    uint16_t mPhysicalAddress;  ///< Physical HDMI address (e.g., 0x1000 for HDMI port 1)
    cec_logical_address mLogicalAddressDefined;  ///< Logical address from config
    cec_logical_address mLogicalAddressUsed;     ///< Actually resolved logical address

    /** @brief Default constructor - initializes to unknown device. */
    cCECDevice() : mPhysicalAddress(0),
                   mLogicalAddressDefined(CECDEVICE_UNKNOWN),
                   mLogicalAddressUsed(CECDEVICE_UNKNOWN) {};

    /**
     * @brief Assignment operator.
     * @param c Source device to copy from.
     * @return Reference to this device.
     */
    cCECDevice &operator=(const cCECDevice &c) {
        mPhysicalAddress = c.mPhysicalAddress;
        mLogicalAddressDefined = c.mLogicalAddressDefined;
        mLogicalAddressUsed = c.mLogicalAddressUsed;
        return *this;
    }
};

typedef std::list<cec_device_type> deviceTypeList;
typedef deviceTypeList::const_iterator deviceTypeListIterator;

/**
 * @enum CECCommand
 * @brief Types of commands that can be queued for execution.
 */
typedef enum {
    CEC_INVALID = -1,      ///< Invalid/uninitialized command
    CEC_EXIT = 0,          ///< Exit the command processing thread
    CEC_KEYRPRESS,         ///< Send a CEC key press
    CEC_MAKEACTIVE,        ///< Make VDR the active CEC source
    CEC_MAKEINACTIVE,      ///< Remove VDR as active CEC source
    CEC_POWERON,           ///< Power on a CEC device
    CEC_POWEROFF,          ///< Power off a CEC device
    CEC_VDRKEYPRESS,       ///< Inject a VDR key press
    CEC_EXECSHELL,         ///< Execute a shell command
    CEC_EXECTOGGLE,        ///< Toggle device power state
    CEC_TEXTVIEWON,        ///< Send TextViewOn CEC command
    CEC_RECONNECT,         ///< Reconnect to CEC adapter
    CEC_CONNECT,           ///< Connect to CEC adapter
    CEC_DISCONNECT,        ///< Disconnect from CEC adapter
    CEC_COMMAND            ///< Generic CEC command
} CECCommand;

class cCmd;

typedef std::list<cCmd> cCmdQueue;
typedef cCmdQueue::const_iterator cCmdQueueIterator;

/**
 * @class cCmd
 * @brief Represents a command to be executed by the CEC remote handler.
 *
 * Commands are queued and processed asynchronously by cCECRemote.
 * Each command has a type and associated data depending on the command type.
 */
class cCmd {
private:
    

public:
    CECCommand mCmd = CEC_INVALID;  ///< Command type
    int mVal = -1;                   ///< Integer value (key code, etc.)
    cCECDevice mDevice;              ///< Target device for the command
    std::string mExec;               ///< Shell command string (for CEC_EXECSHELL)
    int mSerial = -1;                ///< Serial number for synchronous commands
    cCmdQueue mPoweron;              ///< Commands to run on power on (for toggle)
    cCmdQueue mPoweroff;             ///< Commands to run on power off (for toggle)
    cec_opcode mCecOpcode = CEC_OPCODE_NONE;  ///< CEC opcode (for CEC_COMMAND)
    cec_logical_address mCecLogicalAddress = CECDEVICE_UNKNOWN;  ///< Source device

    /** @brief Default constructor. */
    cCmd() = default;

    /**
     * @brief Constructs a basic command.
     * @param cmd Command type.
     * @param val Optional integer value.
     * @param dev Optional target device.
     * @param exec Optional shell command string.
     */
    explicit cCmd(CECCommand cmd, int val = -1,
                  cCECDevice *dev = nullptr, std::string exec="");

    /**
     * @brief Constructs a power toggle command.
     * @param cmd Should be CEC_EXECTOGGLE.
     * @param dev Target device.
     * @param poweron Commands to execute when powering on.
     * @param poweroff Commands to execute when powering off.
     */
    explicit cCmd(CECCommand cmd, const cCECDevice dev,
                  const cCmdQueue poweron, const cCmdQueue poweroff);

    /**
     * @brief Constructs a CEC opcode command.
     * @param cmd Should be CEC_COMMAND.
     * @param opcode The CEC opcode to send.
     * @param logicaladdress Source logical address.
     */
    explicit cCmd(CECCommand cmd, cec_opcode opcode, cec_logical_address logicaladdress);

    /**
     * @brief Gets or generates a serial number for this command.
     * @return Unique serial number for tracking command completion.
     */
    int getSerial(void);

    /**
     * @brief Assignment operator.
     * @param c Source command to copy.
     * @return Reference to this command.
     */
    cCmd& operator=(const cCmd &c) {
        mCmd = c.mCmd;
        mVal = c.mVal;
        mDevice = c.mDevice;
        mExec = c.mExec;
        mSerial = c.mSerial;
        mPoweron = c.mPoweron;
        mPoweroff = c.mPoweroff;
        mCecOpcode = c.mCecOpcode;
        mCecLogicalAddress = c.mCecLogicalAddress;
        return *this;
    }
};

} // namespace cecplugin

#endif /* PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_CCECCMD_H_ */
