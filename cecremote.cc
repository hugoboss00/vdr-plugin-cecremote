/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2019 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the remote receiving and processing the CEC commands.
 */

#include "cecremote.h"
#include "ceclog.h"
#include "cecremoteplugin.h"
#include <sys/wait.h>
#include <unistd.h>
// close_range() requires glibc >= 2.34 and Linux >= 5.9
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 34))
#include <linux/close_range.h>
#define HAVE_CLOSE_RANGE 1
#endif
// We need this for cecloader.h
#include <iostream>
#include <csignal>
using namespace std;
#include <cecloader.h>

using namespace cecplugin;

namespace cecplugin {
const char *cCECRemote::VDRNAME = "VDR";

/**
 * @brief Callback when libCEC receives a key press.
 *
 * Called by libCEC when a CEC key press event is received. Filters
 * duplicate events based on key code and duration, then queues the
 * key for processing by the worker thread.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param key Pointer to the received key press information
 * @note Thread-safe via static mutex for lastkey tracking.
 */
static void CecKeyPressCallback(void *cbParam, const cec_keypress* key)
{
    static cec_user_control_code lastkey = CEC_USER_CONTROL_CODE_UNKNOWN;
    static cMutex lastkeyMutex;  // Protect static lastkey from concurrent access
    cCECRemote *rem = (cCECRemote *)cbParam;

    Dsyslog("key pressed %02x (%d)", key->keycode, key->duration);

    cMutexLock lock(&lastkeyMutex);
    if (
        ((key->keycode >= 0) && (key->keycode <= CEC_USER_CONTROL_CODE_MAX)) &&
        ((key->duration == 0) || (key->keycode != lastkey))
       )
    {
        lastkey = key->keycode;
        cCmd cmd(CEC_KEYRPRESS, (int)key->keycode);
        rem->PushCmd(cmd);
    }
}

/**
 * @brief Callback function for libCEC command messages.
 *
 * Called by libCEC when a CEC command is received. Logs the command
 * details and queues a CEC_COMMAND for processing by the worker thread.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param command Pointer to the received CEC command structure
 * @note Safely handles case where adapter is disconnected during callback.
 */
static void CecCommandCallback(void *cbParam, const cec_command *command)
{
    cCECRemote *rem = (cCECRemote *)cbParam;
    // Safety check: adapter may be disconnected during callback
    if (rem->mCECAdapter == nullptr) {
        Dsyslog("CEC Command ignored - adapter disconnected");
        return;
    }
    Dsyslog("CEC Command %d : %s Init %d Dest %d", command->opcode,
                                   rem->mCECAdapter->ToString(command->opcode),
                                   command->initiator, command->destination);
    cCmd cmd(CEC_COMMAND, command->opcode, command->initiator);
    rem->PushCmd(cmd);
}

/**
 * @brief Callback function for libCEC alert messages.
 *
 * Called by libCEC when an alert condition occurs. Handles connection
 * loss by triggering automatic reconnection. Other alerts are logged
 * for informational purposes.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param type Type of alert received
 * @param param Additional parameter for the alert (type-dependent)
 */
static void CecAlertCallback(void *cbParam, const libcec_alert type,
                            const libcec_parameter param)
{
    cCECRemote *rem = (cCECRemote *)cbParam;
    Dsyslog("CecAlert %d", type);
    switch (type)
    {
    case CEC_ALERT_CONNECTION_LOST:
        Esyslog("Connection lost");
        rem->Reconnect();
        break;
    case CEC_ALERT_TV_POLL_FAILED:
        Isyslog("TV Poll failed");
        break;
    case CEC_ALERT_SERVICE_DEVICE:
        Isyslog("CEC_ALERT_SERVICE_DEVICE");
        break;
    case CEC_ALERT_PERMISSION_ERROR:
        Isyslog("CEC_ALERT_PERMISSION_ERROR");
        break;
    case CEC_ALERT_PORT_BUSY:
        Isyslog("CEC_ALERT_PORT_BUSY");
        break;
    case CEC_ALERT_PHYSICAL_ADDRESS_ERROR:
        Isyslog("CEC_ALERT_PHYSICAL_ADDRESS_ERROR");
        break;
    default:
        break;
    }
}

/**
 * @brief Callback function for libCEC log messages.
 *
 * Called by libCEC when log messages are generated. Filters messages
 * based on configured log level and routes them to appropriate VDR
 * syslog functions.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param message Pointer to the log message structure
 */
static void CecLogMessageCallback(void *cbParam, const cec_log_message *message)
{
    cCECRemote *rem = (cCECRemote *)cbParam;
    if ((message->level & rem->getCECLogLevel()) == message->level)
    {
        string strLevel;
        switch (message->level)
        {
        case CEC_LOG_ERROR:
            strLevel = "ERROR:   ";
            break;
        case CEC_LOG_WARNING:
            strLevel = "WARNING: ";
            break;
        case CEC_LOG_NOTICE:
            strLevel = "NOTICE:  ";
            break;
        case CEC_LOG_TRAFFIC:
            strLevel = "TRAFFIC: ";
            break;
        case CEC_LOG_DEBUG:
            strLevel = "DEBUG:   ";
            break;
        default:
            break;
        }

        char strFullLog[MAXSYSLOGBUF];
        snprintf(strFullLog, MAXSYSLOGBUF-1, "CEC %s %s", strLevel.c_str(), message->message);
        if (message->level == CEC_LOG_ERROR)
        {
            Esyslog(strFullLog);
        }
        else
        {
            Dsyslog(strFullLog);
        }
    }
}

/**
 * @brief Callback function for libCEC source activation events.
 *
 * Called by libCEC when a device is activated as the active source.
 * Currently used for verbose debug logging only.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param address Logical address of the activated device
 * @param activated Activation status (1 = activated, 0 = deactivated)
 */
static void CECSourceActivatedCallback (void *cbParam,
                                        const cec_logical_address address,
                                        const uint8_t activated)
{
    Csyslog("CECSourceActivatedCallback address %d activated %d", address, activated);
}

/**
 * @brief Callback function for libCEC configuration changes.
 *
 * Called by libCEC when the adapter configuration changes.
 * Currently used for verbose debug logging only.
 *
 * @param cbParam Pointer to the cCECRemote instance
 * @param config Pointer to the new configuration structure
 */
static void CECConfigurationCallback (void *cbParam,
                                      const libcec_configuration *config)
{
    Csyslog("CECConfiguration");
}

/**
 * @brief Main worker thread that processes the CEC command queue.
 *
 * This is the main processing loop for the CEC remote. It waits for
 * commands in the worker queue and executes them sequentially. Handles
 * all CEC operations including power control, key presses, active source
 * management, and shell command execution.
 *
 * @note Runs as a background VDR thread, started via Start().
 */
void cCECRemote::Action(void)
{
    cCmd cmd;
    cCECList ceckmap;
    cec_logical_address addr;

    // Allow some delay before the first connection to the CEC Adapter.
    if (mStartupDelay > 0) {
        sleep(mStartupDelay);
    }
    Connect();

    Dsyslog("cCECRemote start worker thread");
    while (Running()) {
        cmd = WaitCmd();
        Dsyslog ("(%d) Action %d Val %d Phys Addr %d Logical %04x %04x Op %d",
                 cmd.mSerial,
                 cmd.mCmd, cmd.mVal, cmd.mDevice.mPhysicalAddress,
                 cmd.mDevice.mLogicalAddressDefined,
                 cmd.mDevice.mLogicalAddressUsed,
                 cmd.mCecOpcode);
        switch (cmd.mCmd)
        {
        case CEC_KEYRPRESS:
            if ((cmd.mVal >= 0) && (cmd.mVal <= CEC_USER_CONTROL_CODE_MAX)) {
                Isyslog("Key Press %d", cmd.mVal);
                const cKeyList &inputKeys =
                        mPlugin->mKeyMaps.CECtoVDRKey((cec_user_control_code)cmd.mVal);
                cKeyListIterator ikeys;
                for (const auto k : inputKeys) {
                    Put(k);
                    Dsyslog ("   Put(%d)", k);
                }
            }
            break;
        case CEC_POWERON:
            if (mCECAdapter != nullptr) {
                Isyslog("Power on");
                addr = getLogical(cmd.mDevice);
                if ((addr != CECDEVICE_UNKNOWN) &&
                    (!mCECAdapter->PowerOnDevices(addr))) {
                    Esyslog("PowerOnDevice failed for %s",
                            mCECAdapter->ToString(addr));
                }
                else {
                    WaitForPowerStatus(addr, CEC_POWER_STATUS_ON);
                }
            }
            else {
                Esyslog("PowerOnDevice ignored");
            }
            break;
        case CEC_POWEROFF:
            if (mCECAdapter != nullptr) {
                Isyslog("Power off");
                addr = getLogical(cmd.mDevice);
                if ((addr != CECDEVICE_UNKNOWN) &&
                    (!mCECAdapter->StandbyDevices(addr))) {
                    Esyslog("StandbyDevices failed for %s",
                            mCECAdapter->ToString(addr));
                }
                else {
                    WaitForPowerStatus(addr, CEC_POWER_STATUS_STANDBY);
                }
            }
            else {
                Esyslog("StandbyDevices ignored");
            }
            break;
        case CEC_TEXTVIEWON:
            if (mCECAdapter != nullptr) {
                Isyslog("Textviewon");
                addr = getLogical(cmd.mDevice);
                if ((addr != CECDEVICE_UNKNOWN) &&
                    (TextViewOn(addr) != 0)) {
                    Esyslog("TextViewOn failed for %s",
                            mCECAdapter->ToString(addr));
                }
            }
            else {
                Esyslog("Textviewon ignored");
            }
            break;
        case CEC_MAKEACTIVE:
            if (mCECAdapter != nullptr) {
                Isyslog ("Make active");
                if (!mCECAdapter->SetActiveSource()) {
                    Esyslog("SetActiveSource failed");
                }
            }
            else {
                Esyslog("SetActiveSource ignored");
            }
            break;
        case CEC_MAKEINACTIVE:
            if (mCECAdapter != nullptr) {
                Isyslog ("Make inactive");
                if (!mCECAdapter->SetInactiveView()) {
                    Esyslog("SetInactiveView failed");
                }
            }
            else {
                Esyslog("SetInactiveView ignored");
            }
            break;
        case CEC_VDRKEYPRESS:
            if (mCECAdapter != nullptr) {
                ActionKeyPress(cmd);
            }
            else {
                Esyslog("Keypress ignored");
            }
            break;
        case CEC_EXECSHELL:
            Isyslog ("Exec: %s", cmd.mExec.c_str());
            Exec(cmd);
            break;
        case CEC_EXIT:
            Isyslog("cCECRemote exit worker thread");
            Cancel(-1);
            Disconnect();
            break;
        case CEC_RECONNECT:
            Isyslog("cCECRemote reconnect");
            Disconnect();
            sleep(1);
            Connect();
            break;
        case CEC_CONNECT:
            Isyslog("cCECRemote connect");
            Connect();
            break;
        case CEC_DISCONNECT:
            Isyslog("cCECRemote disconnect");
            Disconnect();
            break;
        case CEC_COMMAND:
            Dsyslog("cCECRemote command %d", cmd.mCecOpcode);
            CECCommand(cmd);
            break;
        case CEC_EXECTOGGLE:
            Isyslog("cCECRemote exec_toggle");
            ExecToggle(cmd.mDevice, cmd.mPoweron, cmd.mPoweroff);
            break;
        default:
            Esyslog("Unknown action %d Val %d", cmd.mCmd, cmd.mVal);
            break;
        }
        Csyslog ("(%d) Action finished", cmd.mSerial);
        if (cmd.mSerial != -1) {
            mProcessedSerial = cmd.mSerial;
            mCmdReady.Signal();
        }
    }
    Dsyslog("cCECRemote stop worker thread");
}

/**
 * @brief Constructs a new CEC remote handler.
 *
 * Initializes libCEC callbacks and configuration based on the provided
 * global options. Does not connect to the CEC adapter; call Connect()
 * separately after construction.
 *
 * @param options Global CEC configuration options from the XML config file
 * @param plugin Pointer to the parent plugin instance
 */
cCECRemote::cCECRemote(const cCECGlobalOptions &options, cPluginCecremote *plugin):
        cRemote("CEC"),
        cThread("CEC receiver"),
        mPlugin(plugin)
{
    mHDMIPort = options.mHDMIPort;
    mBaseDevice = options.mBaseDevice;
    mCECLogLevel = options.cec_debug;
    mOnStart = options.mOnStart;
    mOnStop = options.mOnStop;
    mOnVolumeUp = options.mOnVolumeUp;
    mOnVolumeDown = options.mOnVolumeDown;
    mOnManualStart = options.mOnManualStart;
    mComboKeyTimeoutMs = options.mComboKeyTimeoutMs;
    mDeviceTypes = options.mDeviceTypes;
    mShutdownOnStandby = options.mShutdownOnStandby;
    mPowerOffOnStandby = options.mPowerOffOnStandby;
    mStartupDelay = options.mStartupDelay;
    mPhysAddress = options.mPhysicalAddress;
    SetDescription("CEC Thread");
}

/**
 * @brief Starts the CEC remote worker thread and executes startup commands.
 *
 * Starts the background worker thread and executes configured startup
 * command queues. If the CEC adapter is not yet connected (deferred
 * startup), the startup commands will be executed after connection.
 */
void cCECRemote::Startup()
{
    Start();

    Csyslog("cCECRemote Init");

    if (mCECAdapter == nullptr) {
        Csyslog("cCECRemote Delayed Startup");
        mDeferredStartup = true;
    }
    else {
        Csyslog("cCECRemote Startup");
        if (mPlugin->GetStartManually()) {
            PushCmdQueue(mOnManualStart);
        }
        PushCmdQueue(mOnStart);
    }
}

/**
 * @brief Connects to the CEC adapter and initializes libCEC.
 *
 * Sets up CEC callbacks, configuration, and attempts to open the
 * first detected CEC adapter. Scans for active CEC devices on the
 * bus and logs their information.
 *
 * @note Safe to call multiple times; returns immediately if already connected.
 */
void cCECRemote::Connect()
{
    Dsyslog("cCECRemote::Connect");
    if (mCECAdapter != nullptr) {
        Csyslog("Ignore Connect");
        return;
    }
    // Initialize Callbacks
    mCECCallbacks.Clear();
    mCECCallbacks.logMessage  = &::CecLogMessageCallback;
    mCECCallbacks.keyPress    = &::CecKeyPressCallback;
    mCECCallbacks.commandReceived     = &::CecCommandCallback;
    mCECCallbacks.alert       = &::CecAlertCallback;
    mCECCallbacks.sourceActivated = &::CECSourceActivatedCallback;
    mCECCallbacks.configurationChanged = &::CECConfigurationCallback;
    // Setup CEC configuration
    mCECConfig.Clear();
    strncpy(mCECConfig.strDeviceName, VDRNAME, sizeof(mCECConfig.strDeviceName)-1);
    mCECConfig.clientVersion      = LIBCEC_VERSION_CURRENT;
    mCECConfig.bActivateSource    = CEC_FALSE;
    mCECConfig.iComboKeyTimeoutMs = mComboKeyTimeoutMs;
    mCECConfig.iHDMIPort = mHDMIPort;
    mCECConfig.wakeDevices.Clear();
    mCECConfig.powerOffDevices.Clear();
    mCECConfig.bPowerOffOnStandby = mPowerOffOnStandby;
    mCECConfig.baseDevice = mBaseDevice;
    // If no <cecdevicetype> is specified in the <global>, set default
    if (mDeviceTypes.empty())
    {
        mCECConfig.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
    }
    else {
        // Add all device types as specified in <cecdevicetype>
        deviceTypeListIterator idev;
        for (idev = mDeviceTypes.begin();
                idev != mDeviceTypes.end(); ++idev) {
            cec_device_type t = *idev;
            mCECConfig.deviceTypes.Add(t);
            Dsyslog ("   Add device %d", t);
        }
    }

    // Setup callbacks
    mCECConfig.callbackParam = this;
    mCECConfig.callbacks = &mCECCallbacks;
    // Initialize libcec
    mCECAdapter = LibCecInitialise(&mCECConfig);
    if (mCECAdapter == nullptr) {
        Esyslog("Can not initialize libcec");
        return;
    }
    // init video on targets that need this
    mCECAdapter->InitVideoStandalone();
    Dsyslog("LibCEC %s", mCECAdapter->GetLibInfo());

    mDevicesFound = mCECAdapter->DetectAdapters(mCECAdapterDescription,
                                                MAX_CEC_ADAPTERS, nullptr, true);
    if (mDevicesFound <= 0)
    {
        Esyslog("No adapter found");
        UnloadLibCec(mCECAdapter);
        mCECAdapter = nullptr;
        mDevicesFound = 0;
        return;
    }

    for (int i = 0; i < mDevicesFound; i++)
    {
        Dsyslog("Device %d path: %s port: %s", i,
                mCECAdapterDescription[i].strComPath,
                mCECAdapterDescription[i].strComName);
    }

    if (!mCECAdapter->Open(mCECAdapterDescription[0].strComName, 5000))
    {
        Esyslog("Unable to open the device on port %s",
                mCECAdapterDescription[0].strComName);
        UnloadLibCec(mCECAdapter);
        mCECAdapter = nullptr;
        mDevicesFound = 0;
        return;
    }
    Csyslog("END cCECRemote::Open OK");

    if (mPhysAddress != 0) {
        Dsyslog("Set new physical address %d", mPhysAddress);
        if (!mCECAdapter->SetPhysicalAddress(mPhysAddress)) {
            Esyslog("Unable to set new physical address %d", mPhysAddress);
        }
    }
    cec_logical_addresses devices = mCECAdapter->GetActiveDevices();
    for (int j = 0; j < 16; j++)
    {
        if (devices[j])
        {
            cec_logical_address logical_addres = (cec_logical_address) j;

            uint16_t phaddr = mCECAdapter->GetDevicePhysicalAddress(logical_addres);

            cec_vendor_id vendor = (cec_vendor_id)mCECAdapter->GetDeviceVendorId(logical_addres);
            string name = mCECAdapter->GetDeviceOSDName(logical_addres);
            Dsyslog("   %15.15s %d@%04x %15.15s %15.15s",
                    mCECAdapter->ToString(logical_addres), logical_addres,
                    phaddr, name.c_str(), mCECAdapter->ToString(vendor));
        }
    }
    Csyslog("END cCECRemote::Initialize");

    if (mDeferredStartup) {
        mDeferredStartup = false;
        Startup();
    }
}

/**
 * @brief Disconnects from the CEC adapter.
 *
 * Sets the device to inactive, closes the adapter connection,
 * and unloads the libCEC library.
 */
void cCECRemote::Disconnect()
{
    if (mCECAdapter != nullptr) {
        mCECAdapter->SetInactiveView();
        mCECAdapter->Close();
        UnloadLibCec(mCECAdapter);
    }
    mCECAdapter = nullptr;
    Dsyslog("cCECRemote::Disconnect");
}

/**
 * @brief Gracefully stops the CEC remote handler.
 *
 * Executes the configured onStop command queue and sends an exit
 * command to the worker thread, waiting for it to complete.
 */
void cCECRemote::Stop()
{
    Dsyslog("Executing onStop");
    PushCmdQueue(mOnStop);
    // Send exit command to worker thread
    cCmd cmd(CEC_EXIT);
    PushWaitCmd(cmd);
    Csyslog("onStop OK");
}

/**
 * @brief Destructor that stops the worker thread and unloads libCEC.
 */
cCECRemote::~cCECRemote()
{
    Cancel(3);
    Disconnect();
}

/**
 * @brief Gets the number of pending commands in the worker queue.
 *
 * Thread-safe implementation using mutex protection.
 *
 * @return Number of commands waiting to be processed.
 */
int cCECRemote::GetWorkQueueSize()
{
    cMutexLock lock(&mWorkerQueueMutex);
    return mWorkerQueue.size();
}

/**
 * @brief Gets the number of pending commands in the exec queue.
 *
 * Thread-safe implementation using mutex protection.
 *
 * @return Number of commands in the execution queue.
 */
int cCECRemote::GetExecQueueSize()
{
    cMutexLock lock(&mExecQueueMutex);
    return mExecQueue.size();
}

/**
 * @brief Lists all active CEC devices on the bus.
 *
 * Queries the CEC adapter for all active devices and returns
 * a formatted string suitable for SVDRP output.
 *
 * @return cString containing the device list in human-readable format
 */
cString cCECRemote::ListDevices()
{
    cString s = "Available CEC Devices:";
    uint16_t phaddr;
    string name;
    cec_vendor_id vendor;
    cec_power_status powerstatus;

    if (mCECAdapter == nullptr) {
        Esyslog ("ListDevices CEC Adapter disconnected");
        s = "CEC Adapter disconnected";
        return s;
    }

    for (int i = 0; i < mDevicesFound; i++)
    {
        s = cString::sprintf("%s\n  Device %d path: %s port: %s Firmware %04d",
                             *s, i,
                             mCECAdapterDescription[0].strComPath,
                             mCECAdapterDescription[0].strComName,
                             mCECAdapterDescription[0].iFirmwareVersion);
    }

    s = cString::sprintf("%s\n\nActive Devices:", *s);
    cec_logical_addresses devices = mCECAdapter->GetActiveDevices();
    cec_logical_addresses own = mCECAdapter->GetLogicalAddresses();

    for (int j = 0; j < 16; j++)
    {
        if (devices[j])
        {
            cec_logical_address logical_addres = (cec_logical_address)j;

            phaddr = mCECAdapter->GetDevicePhysicalAddress(logical_addres);
            name = mCECAdapter->GetDeviceOSDName(logical_addres);
            vendor = (cec_vendor_id)mCECAdapter->GetDeviceVendorId(logical_addres);

            if (own[j]) {
                s = cString::sprintf("%s\n   %d# %-15.15s@%04x %-15.15s %-14.14s %-15.15s", *s,
                        logical_addres,
                        mCECAdapter->ToString(logical_addres),
                        phaddr, name.c_str(),
                        VDRNAME, VDRNAME);
            }
            else {
                powerstatus = mCECAdapter->GetDevicePowerStatus(logical_addres);
                s = cString::sprintf("%s\n   %d# %-15.15s@%04x %-15.15s %-14.14s %-15.15s %-15.15s", *s,
                        logical_addres,
                        mCECAdapter->ToString(logical_addres),
                        phaddr, name.c_str(),
                        mCECAdapter->GetDeviceOSDName(logical_addres).c_str(),
                        mCECAdapter->ToString(vendor),
                        mCECAdapter->ToString(powerstatus));
            }
        }
    }
    return s;
}

/**
 * @brief Resolves a device to its logical CEC address.
 *
 * Attempts to find the logical address for a device, using physical
 * address mapping first, then falling back to configured logical address.
 * Verifies that the resolved address is not the VDR's own address.
 *
 * @param dev Reference to the device structure (may be modified with found logical address)
 * @return The resolved logical address, or CECDEVICE_UNKNOWN on failure
 */
cec_logical_address cCECRemote::getLogical(cCECDevice &dev)
{
    if (mCECAdapter == nullptr) {
        Esyslog ("getLogical CEC Adapter disconnected");
        return CECDEVICE_UNKNOWN;
    }
    cec_logical_address found = CECDEVICE_UNKNOWN;
    // Logical address already available
    if (dev.mLogicalAddressUsed != CECDEVICE_UNKNOWN) {
        return dev.mLogicalAddressUsed;
    }
    // Try to get logical address from physical address.
    // It may be possible that more than one logical address is available
    // at a physical address!
    if (dev.mPhysicalAddress != 0) {
        cec_logical_addresses devices = mCECAdapter->GetActiveDevices();
        for (int j = 0; j < 16; j++)
        {
            if (devices[j])
            {
                cec_logical_address logical_addres = (cec_logical_address)j;
                if (dev.mPhysicalAddress ==
                        mCECAdapter->GetDevicePhysicalAddress(logical_addres)) {
                    dev.mLogicalAddressUsed = logical_addres;
                    Dsyslog ("Mapping Physical %04x->Logical %d",
                            dev.mPhysicalAddress, logical_addres);
                    found = logical_addres;
                    // Exact match
                    if (dev.mLogicalAddressDefined == logical_addres) {
                        return logical_addres;
                    }
                }
            }
        }
    }
    if (found != CECDEVICE_UNKNOWN) {
        return found;
    }

    // No mapping available, so try as last attempt the defined logical address,
    // if available.
    if (dev.mLogicalAddressDefined == CECDEVICE_UNKNOWN) {
        Esyslog("No fallback logical address for %04x configured", dev.mPhysicalAddress);
        return CECDEVICE_UNKNOWN;
    }
    // Ensure that we don't send accidentally to the own VDR address.

    cec_logical_addresses own = mCECAdapter->GetLogicalAddresses();
    if (own[dev.mLogicalAddressDefined]) {
        Esyslog("Logical address of physical %04x is the VDR", dev.mPhysicalAddress);
        return CECDEVICE_UNKNOWN;
    }
    // Check if device is available.
    if (!mCECAdapter->PollDevice(dev.mLogicalAddressDefined)) {
        Esyslog("Logical address not available", dev.mLogicalAddressDefined);
        return CECDEVICE_UNKNOWN;
    }

    dev.mLogicalAddressUsed = dev.mLogicalAddressDefined;
    return dev.mLogicalAddressDefined;
}


/**
 * @brief Waits for a device to reach a specific power status.
 *
 * Polls the device power status at 100ms intervals until the
 * expected status is reached or a timeout occurs.
 *
 * @param addr Logical address of the device to monitor
 * @param newstatus The expected power status to wait for
 */
void cCECRemote::WaitForPowerStatus(cec_logical_address addr, cec_power_status newstatus)
{
    cec_power_status status;
    int cnt = 0;
    cCondWait w;

    do {
        w.Wait(100);
        status = mCECAdapter->GetDevicePowerStatus(addr);
        cnt++;
    } while ((status != newstatus) && (cnt < 50) && (status != CEC_POWER_STATUS_UNKNOWN));
}

/**
 * @brief Executes a shell command with special SVDRP handling.
 *
 * Forks a child process to execute the command. While the script
 * runs, monitors the exec queue for SVDRP CONN/DISC commands
 * that may come from the script itself.
 *
 * @param execcmd Reference to the command containing the shell script to execute
 */
void cCECRemote::Exec(cCmd &execcmd)
{
    cCmd cmd;
    Dsyslog("Execute script %s", execcmd.mExec.c_str());

    pid_t pid = fork();
    if (pid < 0) {
        Esyslog("fork failed");
        return;
    }
    else if (pid == 0) {
    	pid = setsid();
    	if (pid < 0) {
    		Esyslog("Sid failed");
    		abort();
    	}
    	// Close all file descriptors >= 4 to prevent leaking to child process
#ifdef HAVE_CLOSE_RANGE
    	close_range(4, UINT_MAX, CLOSE_RANGE_UNSHARE);
#else
    	// Fallback for older glibc (< 2.34) / Linux (< 5.9)
    	int maxfd = sysconf(_SC_OPEN_MAX);
    	if (maxfd < 0) maxfd = 1024;
    	for (int fd = 4; fd < maxfd; fd++) {
    	    close(fd);
    	}
#endif
        execl("/bin/sh", "sh", "-c", execcmd.mExec.c_str(), nullptr);
        Esyslog("Exec failed");
        abort();
    }

    mInExec = true;
    do {
        cmd = WaitExec(pid);
        Dsyslog ("(%d) ExecAction %d Val %d",
                 cmd.mSerial, cmd.mCmd, cmd.mVal);
        switch (cmd.mCmd) {
        case CEC_EXIT:
            Dsyslog("cCECRemote Exec script stopped");
            break;
        case CEC_RECONNECT:
            Dsyslog("cCECRemote Exec reconnect");
            Disconnect();
            sleep(1);
            Connect();
            break;
        case CEC_CONNECT:
            Dsyslog("cCECRemote Exec connect");
            Connect();
            break;
        case CEC_DISCONNECT:
            Dsyslog("cCECRemote Exec disconnect");
            Disconnect();
            break;
        default:
            Esyslog("cCECRemote Exec Unexpected action %d Val %d", cmd.mCmd, cmd.mVal);
            break;
        }
        Csyslog ("(%d) Action finished", cmd.mSerial);
        if (cmd.mSerial != -1) {
            mProcessedSerial = cmd.mSerial;
            mCmdReady.Signal();
        }
    } while (cmd.mCmd != CEC_EXIT);
    mInExec = false;
}

/**
 * @brief Waits for a command in the exec queue during script execution.
 *
 * Monitors both the exec queue and the running process. Returns when
 * either a command is received or the process terminates.
 *
 * @param pid Process ID of the running script
 * @return The received command, or CEC_EXIT if the process terminated
 */
cCmd cCECRemote::WaitExec(pid_t pid)
{
    Csyslog("WaitExec");
    int stat_loc = 0;
    mExecQueueMutex.Lock();
    while (mExecQueue.empty()) {
        mExecQueueMutex.Unlock();
        if (mExecQueueWait.Wait(250)) {
            Csyslog("  Signal");
        }
        else {
            if (waitpid (pid, &stat_loc, WNOHANG) == pid) {
                Dsyslog("  Script exit with %d", WEXITSTATUS(stat_loc));
                cCmd cmd(CEC_EXIT);
                return cmd;
            }
        }
        mExecQueueMutex.Lock();
    }

    cCmd cmd = mExecQueue.front();
    mExecQueue.pop_front();
    mExecQueueMutex.Unlock();
    return cmd;
}


/**
 * @brief Pushes an entire command queue for execution.
 *
 * Adds all commands from the given queue to the worker queue
 * for sequential execution. Thread-safe.
 *
 * @param cmdList Reference to the command queue to push
 */
void cCECRemote::PushCmdQueue(const cCmdQueue &cmdList)
{
    if (mCECAdapter == nullptr) {
        Esyslog ("PushCmdQueue CEC Adapter disconnected");
        return;
    }
    Csyslog("cCECRemote::PushCmdQueue");
    mWorkerQueueMutex.Lock();
    for (cCmdQueueIterator i = cmdList.begin();
           i != cmdList.end(); i++) {
        mWorkerQueue.push_back(*i);
    }
    mWorkerQueueMutex.Unlock();
    mWorkerQueueWait.Signal();
}

/**
 * @brief Pushes a single command for asynchronous execution.
 *
 * Adds the command to the worker queue and signals the worker
 * thread. Returns immediately without waiting for execution.
 * Thread-safe.
 *
 * @param cmd Reference to the command to push
 */
void cCECRemote::PushCmd(const cCmd &cmd)
{
    Csyslog("cCECRemote::PushCmd %d (size %d)", cmd.mCmd, mWorkerQueue.size());

    mWorkerQueueMutex.Lock();
    mWorkerQueue.push_back(cmd);
    mWorkerQueueMutex.Unlock();
    mWorkerQueueWait.Signal();
}

/**
 * @brief Pushes a command and waits for its execution.
 *
 * Adds the command to the appropriate queue and blocks until
 * execution is complete or timeout occurs. Used for synchronous
 * operations like SVDRP commands.
 *
 * @param cmd Reference to the command to execute
 * @param timeout Maximum time to wait in milliseconds (default: 3000)
 */
void cCECRemote::PushWaitCmd(cCmd &cmd, int timeout)
{
    int serial = cmd.getSerial();
    cmd.mSerial = serial;
    bool signaled = false;

    Csyslog("cCECRemote::PushWaitCmd %d ID %d (WQ %d EQ %d)",
            cmd.mCmd, serial, mWorkerQueue.size(), mExecQueue.size());

    // Special handling for CEC_CONNECT and CEC_DISCONNECT when called
    // from exec state (used for out of band processing of svdrp commands
    // coming from a script, executed by a command queue.
    if (((cmd.mCmd == CEC_CONNECT) || (cmd.mCmd == CEC_DISCONNECT)) && mInExec){
        Csyslog("ExecQueue");
        mExecQueueMutex.Lock();
        mExecQueue.push_back(cmd);
        mExecQueueMutex.Unlock();
        mExecQueueWait.Signal();
    }
    // Normal handling
    else {
        mWorkerQueueMutex.Lock();
        mWorkerQueue.push_back(cmd);
        mWorkerQueueMutex.Unlock();
        mWorkerQueueWait.Signal();
    }

    // Wait until this command is processed.
    do {
        signaled = mCmdReady.Wait(timeout);
    } while ((mProcessedSerial.load() != serial) && (signaled));
    if (!signaled) {
        Esyslog("cCECRemote::PushWaitCmd timeout %d %d", mProcessedSerial.load(), serial);
    }
    else {
        Csyslog("cCECRemote %d %d", mProcessedSerial.load(), serial);
    }
}

/**
 * @brief Waits for and retrieves the next command from the worker queue.
 *
 * Blocks until a command is available in the queue, then removes
 * and returns it. Thread-safe.
 *
 * @param timeout Maximum time to wait in milliseconds (default: 2000)
 * @return The next command to process
 */
cCmd cCECRemote::WaitCmd(int timeout)
{
    Csyslog("Wait");
    mWorkerQueueMutex.Lock();
    while (mWorkerQueue.empty()) {
        mWorkerQueueMutex.Unlock();
        if (mWorkerQueueWait.Wait(timeout)) {
            Csyslog("  Signal");
        }
        mWorkerQueueMutex.Lock();
    }

    cCmd cmd = mWorkerQueue.front();
    mWorkerQueue.pop_front();
    mWorkerQueueMutex.Unlock();

    return cmd;
}

/**
 * @brief Requests a reconnection to the CEC adapter.
 *
 * Pushes a reconnect command to the front of the appropriate queue
 * for immediate execution. Used primarily by the alert callback
 * when connection is lost.
 */
void cCECRemote::Reconnect()
{
    Dsyslog("cCECRemote::Reconnect");
    cCmd cmd(CEC_RECONNECT);
    // coming from a script, executed by a command queue.
    if (mInExec) {
        mExecQueueMutex.Lock();
        mExecQueue.push_front(cmd); // Ensure that command is executed ASAP.
        mExecQueueMutex.Unlock();
        mExecQueueWait.Signal();
    }
    else {
        mWorkerQueueMutex.Lock();
        mWorkerQueue.push_front(cmd); // Ensure that command is executed ASAP.
        mWorkerQueueMutex.Unlock();
        mWorkerQueueWait.Signal();
    }
}

} // namespace cecplugin

