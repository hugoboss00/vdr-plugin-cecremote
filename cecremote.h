/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2025 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the remote receiving and processing the CEC commands.
 */

#ifndef CECREMOTE_H_
#define CECREMOTE_H_

#include <vdr/plugin.h>
#include <vdr/remote.h>
#include <vdr/thread.h>
#include <vdr/keys.h>
#include <cectypes.h>
#include <cec.h>
#include <stdint.h>
#include <atomic>
#include <queue>
#include <list>
#include <vector>
#include <map>
#include <string>

#include "keymaps.h"
#include "cmd.h"

namespace cecplugin {

class cPluginCecremote;
class cCECGlobalOptions;

/**
 * @class cCECRemote
 * @brief Main CEC communication handler running as a background thread.
 *
 * This class manages the connection to the CEC adapter via libCEC, processes
 * incoming CEC commands, and executes queued commands. It inherits from
 * VDR's cRemote for key handling and cThread for background processing.
 *
 * Commands are processed through two queues:
 * - mWorkerQueue: Normal command processing queue
 * - mExecQueue: Special queue used during shell script execution
 */
class cCECRemote : public cRemote, private cThread {
public:
    /**
     * @brief Constructs the CEC remote handler.
     * @param options Global configuration options from XML config file.
     * @param plugin Pointer to the main plugin instance.
     */
    cCECRemote(const cCECGlobalOptions &options, cPluginCecremote *plugin);

    /**
     * @brief Destructor - stops the thread and disconnects from CEC adapter.
     */
    ~cCECRemote();

    /**
     * @brief Dummy initialization (required by cRemote interface).
     * @return Always returns false.
     */
    bool Initialize() {return false;};

    /**
     * @brief Pushes a command to the worker queue for asynchronous execution.
     * @param cmd The command to execute.
     */
    void PushCmd(const cCmd &cmd);

    /**
     * @brief Pushes multiple commands to the worker queue.
     * @param cmdList List of commands to execute in order.
     */
    void PushCmdQueue(const cCmdQueue &cmdList);

    /**
     * @brief Pushes a command and waits for its completion.
     * @param cmd The command to execute (may be modified with serial number).
     * @param timeout Maximum wait time in milliseconds (default: 5000ms).
     */
    void PushWaitCmd(cCmd &cmd, int timeout = 5000);

    /**
     * @brief Gets the current CEC logging level.
     * @return The CEC log level bitmask.
     */
    int getCECLogLevel() {return mCECLogLevel;}

    /**
     * @brief Lists all detected CEC devices.
     * @return Formatted string with device information for SVDRP output.
     */
    cString ListDevices();

    /**
     * @brief Reconnects to the CEC adapter (disconnect then connect).
     */
    void Reconnect();

    /**
     * @brief Stops the background thread and disconnects from CEC adapter.
     */
    void Stop();

    /**
     * @brief Starts the CEC adapter connection and background thread.
     */
    void Startup();

    /**
     * @brief Gets the number of pending commands in worker queue.
     * @return Number of commands waiting to be processed.
     * @note Thread-safe via mutex protection.
     */
    int GetWorkQueueSize();

    /**
     * @brief Gets the number of pending commands in exec queue.
     * @return Number of commands in the execution queue.
     * @note Thread-safe via mutex protection.
     */
    int GetExecQueueSize();

    /**
     * @brief Checks if connected to a CEC adapter.
     * @return true if connected, false otherwise.
     */
    bool IsConnected() {return (mCECAdapter != nullptr);}

    ICECAdapter            *mCECAdapter = nullptr;  ///< libCEC adapter interface
private:
    static constexpr const int MAX_CEC_ADAPTERS = 10;
    static const char      *VDRNAME;
    int                    mCECLogLevel;
    std::atomic<int>       mProcessedSerial{-1};  ///< Thread-safe command completion tracking
    int                    mStartupDelay;
    uint8_t                mDevicesFound = 0;
    uint8_t                mHDMIPort;
    cec_logical_address    mBaseDevice;
    uint32_t               mPhysAddress = 0;
    uint32_t               mComboKeyTimeoutMs;
    libcec_configuration   mCECConfig;
    ICECCallbacks          mCECCallbacks;
    cec_adapter_descriptor mCECAdapterDescription[MAX_CEC_ADAPTERS];

    // Queue for normal worker thread
    cMutex                 mWorkerQueueMutex;
    cCondWait              mWorkerQueueWait;
    cCmdQueue              mWorkerQueue;

    // Queue for special commands when shell script is executed
    cMutex                 mExecQueueMutex;
    cCondWait              mExecQueueWait;
    cCmdQueue              mExecQueue;

    cCondWait              mCmdReady;
    deviceTypeList         mDeviceTypes;
    bool                   mShutdownOnStandby;
    bool                   mPowerOffOnStandby;
    std::atomic<bool>      mInExec{false};        ///< Thread-safe exec state flag
    std::atomic<bool>      mDeferredStartup{false}; ///< Thread-safe deferred startup flag
    cPluginCecremote       *mPlugin;

    /** @brief Establishes connection to the CEC adapter. */
    void Connect();

    /** @brief Disconnects from the CEC adapter. */
    void Disconnect();

    /**
     * @brief Processes a CEC key press command.
     * @param cmd Command containing key press information.
     */
    void ActionKeyPress(cCmd &cmd);

    /** @brief Main thread action loop - processes commands from queues. */
    void Action();

    /**
     * @brief Executes a CEC command (power on/off, make active, etc.).
     * @param cmd The command to execute.
     */
    void CECCommand(const cCmd &cmd);

    /**
     * @brief Waits for a command with matching serial to complete.
     * @param timeout Maximum wait time in milliseconds.
     * @return The completed command.
     */
    cCmd WaitCmd(int timeout = 5000);

    /**
     * @brief Waits for a shell process to complete.
     * @param pid Process ID of the shell command.
     * @return Command result after process completion.
     */
    cCmd WaitExec(pid_t pid);

    /**
     * @brief Executes a shell command.
     * @param cmd Command containing the shell command string.
     */
    void Exec(cCmd &cmd);

    /**
     * @brief Toggles power state of a device (on/off based on current state).
     * @param dev The target CEC device.
     * @param poweron Commands to execute when powering on.
     * @param poweroff Commands to execute when powering off.
     */
    void ExecToggle(cCECDevice dev, const cCmdQueue &poweron,
                    const cCmdQueue &poweroff);

    /**
     * @brief Waits for a device to reach a specific power status.
     * @param addr Logical address of the CEC device.
     * @param newstatus The expected power status to wait for.
     */
    void WaitForPowerStatus(cec_logical_address addr, cec_power_status newstatus);

    /**
     * @brief Sends TextViewOn command to a CEC device.
     * @param address Logical address of the target device.
     * @return true if command was sent successfully.
     */
    bool TextViewOn(cec_logical_address address);

    /**
     * @brief Resolves the logical address for a CEC device.
     * @param dev Device with physical/logical address information.
     * @return The resolved logical address.
     */
    cec_logical_address getLogical(cCECDevice &dev);

    cCmdQueue mOnStart;        ///< Commands to execute on plugin start
    cCmdQueue mOnStop;         ///< Commands to execute on plugin stop
    cCmdQueue mOnVolumeUp;     ///< Commands to execute on volume up
    cCmdQueue mOnVolumeDown;   ///< Commands to execute on volume down
    cCmdQueue mOnManualStart;  ///< Commands to execute on manual start (not timer)
};

} // namespace cecplugin

#endif /* CECREMOTE_H_ */
