/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2024 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the remote receiving and processing the CEC commands.
 */

#include <sys/wait.h>
#include "cecremote.h"
#include "ceclog.h"
#include "cecremoteplugin.h"
#include "ceccontrol.h"

using namespace std;
using namespace cecplugin;

namespace cecplugin {

/**
 * @brief Sends a VDR key press to a CEC device.
 *
 * Translates the VDR key to CEC key codes using the active
 * keymap and sends keypress/keyrelease commands to the target device.
 *
 * @param cmd Reference to the command containing key and device info
 */
void cCECRemote::ActionKeyPress(cCmd &cmd)
{
    cec_logical_address addr;
    cCECList ceckmap;
    cec_user_control_code ceckey;

    addr = getLogical(cmd.mDevice);
    if (addr != CECDEVICE_UNKNOWN) {
        ceckmap = mPlugin->mKeyMaps.VDRtoCECKey((eKeys)cmd.mVal);

        for (cCECListIterator ci = ceckmap.begin(); ci != ceckmap.end();
                ++ci) {
            ceckey = *ci;
            Dsyslog ("Send Keypress VDR %d - > CEC 0x%02x", cmd.mVal, ceckey);
            if (ceckey != CEC_USER_CONTROL_CODE_UNKNOWN) {
                if (!mCECAdapter->SendKeypress(addr, ceckey, true)) {
                    Esyslog("Keypress to %d %s failed",
                            addr, mCECAdapter->ToString(addr));
                    return;
                }
                cCondWait::SleepMs(50);
                if (!mCECAdapter->SendKeyRelease(addr, true)) {
                    Esyslog("SendKeyRelease to %d %s failed",
                            addr, mCECAdapter->ToString(addr));
                }
            }
        }
    }
}

/**
 * @brief Sends a TEXT_VIEW_ON CEC command.
 *
 * Instructs a TV to switch to this device's input and
 * turn on if in standby.
 *
 * @param address Logical address of the target device
 * @return true on success, false on failure
 */
bool cCECRemote::TextViewOn(cec_logical_address address)
{
    cec_command data;
    if (mCECAdapter == NULL) {
        Esyslog ("TextViewOn CEC Adapter disconnected");
        return false;
    }

    cec_command::Format(data, mCECConfig.baseDevice, address, CEC_OPCODE_TEXT_VIEW_ON);
    Dsyslog("Text View on : %02x %02x %02x", data.initiator, data.destination, data.opcode);
    return mCECAdapter->Transmit(data);
}

/**
 * @brief Executes power toggle based on device state.
 *
 * Queries the device power status and executes either the
 * poweron or poweroff command queue accordingly.
 *
 * @param dev Reference to the target device
 * @param poweron Command queue to execute if device is off
 * @param poweroff Command queue to execute if device is on
 */
void cCECRemote::ExecToggle(cCECDevice dev,
                            const cCmdQueue &poweron, const cCmdQueue &poweroff)
{
    bool repeat;
    cec_power_status status;
    cec_logical_address addr;
    cCondWait w;

    if (mCECAdapter == NULL) {
        Esyslog ("ExecToggle CEC Adapter disconnected");
        return;
    }

    addr = getLogical(dev);
    if (addr == CECDEVICE_UNKNOWN) {
        status = CEC_POWER_STATUS_UNKNOWN;
    }
    else
    {
        do {
            repeat = false;
            status = mCECAdapter->GetDevicePowerStatus(addr);
            Dsyslog("ExecToggle: %s", mCECAdapter->ToString(status));
            // If currently in any transition state, wait.
            if ((status == CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON) ||
                    (status == CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY)) {
                w.Wait(1000);
                repeat = true;
            }
        } while (repeat);
    }

    if (status == CEC_POWER_STATUS_ON) {
        PushCmdQueue(poweroff);
    }
    else {
        PushCmdQueue(poweron);
    }
}

/**
 * @brief Processes CEC commands received from the bus.
 *
 * Looks up registered handlers for the CEC opcode and executes
 * matching actions (start/stop menus, run command queues).
 *
 * @param cmd Reference to the received CEC command
 */
void cCECRemote::CECCommand(const cCmd &cmd) {
    mapCommandHandler *h = mPlugin->GetCECCommandHandlers();

    std::pair<mapCommandHandlerIterator, mapCommandHandlerIterator> range;
    range = h->equal_range(cmd.mCecOpcode);

    for (mapCommandHandlerIterator i = range.first; i != range.second; i++) {
        cCECCommandHandler handler = i->second;
        cec_logical_address devaddr = getLogical(handler.mDevice);
        Csyslog("Handler for CEC Command %d test %d %d\n",
                cmd.mCecOpcode, cmd.mCecLogicalAddress, devaddr);
        if (cmd.mCecLogicalAddress == devaddr) {
            Dsyslog("Handler for CEC Command %d found %d %d\n", cmd.mCecOpcode,
                    cmd.mCecLogicalAddress, devaddr);

            // First stop the defined player if running
            if (!handler.mStopMenu.empty()) {
                // Get current running control
            	cMutexLock lock;
                cControl *c = cControl::Control(lock);
                if (c != NULL) {
                    if (cCECControl* cont = dynamic_cast<cCECControl*>(c)) {
                        Dsyslog("Stillpic Player running %s %s",
                                cont->getMenuTitle().c_str(),
                                handler.mStopMenu.c_str());
                        if (cont->getMenuTitle() == handler.mStopMenu) {
                            cont->Shutdown();
                        }
                    }
                }
            }
            // Startup a new menu/player if defined
            if (!handler.mExecMenu.empty()) {
                cCECMenu menuitem;
                if (mPlugin->FindMenu(handler.mExecMenu, menuitem)) {
                    mPlugin->StartPlayer(menuitem);
                }
            }
            // Now Push the command queue
            PushCmdQueue(handler.mCommands);
        }
    }
}

} // namespace cecplugin

