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

#include "cecremote.h"
#include "cecremoteplugin.h"
#include "cmd.h"

namespace cecplugin {

/**
 * @brief Constructs a command with optional device and exec string.
 *
 * @param cmd The command type
 * @param val Integer value (e.g., key code)
 * @param dev Optional pointer to target device
 * @param exec Optional shell command string
 */
cCmd::cCmd(CECCommand cmd, int val, cCECDevice *dev, std::string exec) {
    mCmd = cmd;
    mVal = val;
    if (dev != nullptr) {
        mDevice = *dev;
    }
    mExec = exec;
}

/**
 * @brief Constructs a toggle command with power on/off queues.
 *
 * @param cmd The command type (typically CEC_EXECTOGGLE)
 * @param dev Target device
 * @param poweron Command queue for power on
 * @param poweroff Command queue for power off
 */
cCmd::cCmd(CECCommand cmd, const cCECDevice dev,
        const cCmdQueue poweron, const cCmdQueue poweroff) {
    mCmd = cmd;
    mDevice = dev;
    mPoweron = poweron;
    mPoweroff = poweroff;
}

/**
 * @brief Constructs a CEC opcode command.
 *
 * @param cmd The command type (typically CEC_COMMAND)
 * @param opcode The CEC opcode
 * @param logicaladdress Source logical address
 */
cCmd::cCmd(CECCommand cmd, cec_opcode opcode,
        cec_logical_address logicaladdress) {
    mCmd = cmd;
    mCecOpcode = opcode;
    mCecLogicalAddress = logicaladdress;
}

/**
 * @brief Gets a unique serial number for command tracking.
 *
 * Thread-safe serial number generator that wraps at 10000.
 *
 * @return Unique serial number (1-10000)
 */
int cCmd::getSerial(void) {
    static int serial = 1;
    static cMutex mSerialMutex;
    int ret;
    mSerialMutex.Lock();
    serial++;
    if (serial > 10000) {
        serial = 1;
    }
    ret = serial;
    mSerialMutex.Unlock();
    return ret;
}

} // namespace cecplugin
