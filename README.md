# VDR CECRemote Plugin

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![VDR](https://img.shields.io/badge/VDR-%E2%89%A5%202.0.0-green.svg)](https://www.tvdr.de/)
[![libCEC](https://img.shields.io/badge/libCEC-%E2%89%A5%204.0.0-orange.svg)](https://libcec.pulse-eight.com/)

A plugin for the [Video Disk Recorder (VDR)](https://www.tvdr.de/) that enables **HDMI-CEC control**. Control your TV, AV receiver, and other CEC-enabled devices directly from VDR, and use your TV remote to control VDR.

> **Originally written by:** Ulrich Eckhardt \<uli-vdr@uli-eckhardt.de\>

---

## 📑 Table of Contents

- [Features](#-features)
- [Requirements](#-requirements)
- [Installation](#-installation)
  - [Ubuntu/Debian](#ubuntudebian)
  - [Building from Source](#building-from-source)
- [Quick Start](#-quick-start)
- [Configuration Reference](#-configuration-reference)
  - [Global Options](#global-options)
  - [Device Definitions](#device-definitions)
  - [Key Mappings](#key-mappings)
  - [Menu Definitions](#menu-definitions)
  - [CEC Command Handlers](#cec-command-handlers)
  - [Command Lists](#command-lists)
- [SVDRP Commands](#-svdrp-commands)
- [Command Line Arguments](#-command-line-arguments)
- [Troubleshooting](#-troubleshooting)
- [Examples](#-examples)

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| 🎮 **Remote Control** | Receive remote control commands from your TV via HDMI-CEC |
| 📺 **Device Control** | Send CEC commands to control TVs, receivers, and players |
| ⚡ **Power Sync** | Automatic power on/off synchronization with your TV |
| ⌨️ **Key Mapping** | Customizable key mappings (CEC ↔ VDR) |
| 🔧 **Scripting** | Execute shell scripts on CEC events |
| 🖼️ **Still Picture** | Still picture player for device control screens |
| ⏰ **RTC Detection** | Manual vs. timed start detection |
| 🔊 **Volume Control** | Volume forwarding to audio devices |

---

## 📋 Requirements

| Dependency | Version | Description |
|------------|---------|-------------|
| [VDR](https://www.tvdr.de/) | ≥ 2.0.0 | Video Disk Recorder |
| [libCEC](https://libcec.pulse-eight.com/) | ≥ 4.0.0 | HDMI-CEC library |
| [PugiXML](https://pugixml.org/) | ≥ 1.5 | XML parser for C++ |

**Hardware:** A CEC-compatible USB adapter (e.g., Pulse-Eight) or built-in CEC (Raspberry Pi, some graphics cards)

> 💡 **Raspberry Pi:** For PugiXML packages, see the [Raspbian archive](http://distribution-us.hexxeh.net/raspbian/archive/raspbian/pool/main/p/pugixml/)

---

## 📦 Installation

### Ubuntu/Debian

Install the required development packages:

```bash
sudo apt-get install libcec-dev libp8-platform-dev libpugixml-dev vdr-dev
```

<details>
<summary>📦 What gets installed?</summary>

| Package | Purpose |
|---------|---------|
| `libcec-dev` | libCEC development files for HDMI-CEC communication |
| `libp8-platform-dev` | Platform support library required by libCEC |
| `libpugixml-dev` | PugiXML development files for XML parsing |
| `vdr-dev` | VDR development headers |

</details>

**Optional:** Install CEC utilities for testing:

```bash
sudo apt-get install cec-utils
```

### Building from Source

```bash
make
sudo make install
```

The plugin will be installed to VDR's plugin directory.

---

## 🚀 Quick Start

### 1️⃣ Scan for CEC Devices

Before configuring the plugin, scan your CEC bus to find devices:

```bash
echo "scan" | cec-client -s -d 1
```

<details>
<summary>📺 Example output</summary>

```
opening a connection to the CEC adapter...
requesting CEC bus information ...
CEC bus information
===================
device #0: TV
address:       0.0.0.0
active source: no
vendor:        LG
osd string:    TV
CEC version:   1.4
power status:  on
language:      ger

device #4: Playback 1
address:       2.0.0.0
active source: no
vendor:        Pulse Eight
osd string:    CECTester
CEC version:   1.4
power status:  on
language:      ???
```

</details>

Or with the plugin already running:

```bash
svdrpsend plug cecremote LSTD
```

### 2️⃣ Create Configuration File

```bash
mkdir -p /etc/vdr/plugins/cecremote
cp contrib/cecremote.xml /etc/vdr/plugins/cecremote/
```

Edit the file to match your device setup.

### 3️⃣ Prevent Device Conflicts

Install the udev rule to prevent ModemManager from grabbing the adapter:

```bash
sudo cp contrib/20-libcec.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

### 4️⃣ Start VDR

```bash
vdr -P cecremote
```

---

## ⚙️ Configuration Reference

The configuration file uses XML format. Root element is `<config>`.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<config>
    <global>...</global>
    <device>...</device>
    <ceckeymap>...</ceckeymap>
    <vdrkeymap>...</vdrkeymap>
    <globalkeymap>...</globalkeymap>
    <menu>...</menu>
    <onceccommand>...</onceccommand>
</config>
```

### Global Options

```xml
<global>
    <cecdebug>7</cecdebug>
    <hdmiport>1</hdmiport>
    <basedevice>0</basedevice>
    <rtcdetect>true</rtcdetect>
    <shutdownonstandby>false</shutdownonstandby>
    <poweroffonstandby>false</poweroffonstandby>
    <startupdelay>0</startupdelay>
    <physical>1000</physical>
    <cecdevicetype>RECORDING_DEVICE</cecdevicetype>
    <audiodevice>TV</audiodevice>
    <keymaps cec="default" vdr="default" globalvdr="default"/>
    <onstart>...</onstart>
    <onmanualstart>...</onmanualstart>
    <onstop>...</onstop>
    <onswitchtotv>...</onswitchtotv>
    <onswitchtoradio>...</onswitchtoradio>
    <onswitchtoreplay>...</onswitchtoreplay>
    <onvolumeup>...</onvolumeup>
    <onvolumedown>...</onvolumedown>
</global>
```

| Option | Description |
|--------|-------------|
| `<cecdebug>` | CEC library debug level (see `cec_log_level` in cectypes.h). Values: `1`=errors, `2`=warnings, `4`=notice, `8`=traffic, `16`=debug. Combine with OR (e.g., `7` = errors+warnings+notice) |
| `<hdmiport>` | HDMI port number where the adapter is connected (1-15) |
| `<basedevice>` | Logical address of device adapter connects to (`0`=TV) |
| `<rtcdetect>` | Use RTC to detect manual vs. timed start (`true`/`false`) |
| `<shutdownonstandby>` | Set devices to standby on VDR shutdown (`true`/`false`) |
| `<poweroffonstandby>` | Power off devices on VDR shutdown (`true`/`false`) |
| `<startupdelay>` | Seconds to wait before CEC initialization |
| `<physical>` | Physical address override (hex, e.g., `1000` = 1.0.0.0) |
| `<cecdevicetype>` | Device type: `RECORDING_DEVICE`, `TUNER`, `TV`, `PLAYBACK_DEVICE`, `AUDIO_SYSTEM` |
| `<audiodevice>` | Device for volume key forwarding |
| `<keymaps>` | Default keymaps (`cec`, `vdr`, `globalvdr` attributes) |

**Event Handlers:**

| Handler | Triggered When |
|---------|---------------|
| `<onstart>` | Plugin starts |
| `<onmanualstart>` | Manual start (not timer) |
| `<onstop>` | Plugin shuts down |
| `<onswitchtotv>` | Switching to TV channel |
| `<onswitchtoradio>` | Switching to radio channel |
| `<onswitchtoreplay>` | Starting replay |
| `<onvolumeup>` | Volume increase |
| `<onvolumedown>` | Volume decrease |

> ⚠️ **Raspberry Pi:** libCEC may crash when registering multiple device types.

---

### Device Definitions

Define CEC devices by physical and/or logical address:

```xml
<device id="blueray">
    <physical>2000</physical>
    <logical>8</logical>
</device>
```

| Element | Description |
|---------|-------------|
| `<physical>` | Physical address in hex (`2000` = HDMI port 2 on TV) |
| `<logical>` | Logical address fallback (0-15) |

The plugin tries physical address first, then falls back to logical. Device IDs can be referenced elsewhere (e.g., in command lists).

> 💡 **Predefined device:** `TV` (logical address 0)

---

### Key Mappings

Three types of keymaps:

| Keymap | Direction | Use Case |
|--------|-----------|----------|
| `<ceckeymap>` | CEC → VDR | Incoming remote commands |
| `<vdrkeymap>` | VDR → CEC | Outgoing commands in player |
| `<globalkeymap>` | VDR → CEC | Outgoing commands globally |

**Example - Remap SELECT to Menu:**

```xml
<ceckeymap id="mymap">
    <key code="SELECT">
        <value>Menu</value>
    </key>
    <key code="RIGHT_UP">
        <value>Right</value>  <!-- Only Right, not Right+Up -->
    </key>
</ceckeymap>
```

**Example - Remap OK to ROOT_MENU:**

```xml
<vdrkeymap id="tvcontrol">
    <key code="Ok">
        <value>ROOT_MENU</value>
    </key>
</vdrkeymap>
```

> 💡 Use SVDRP to list available key codes: `svdrpsend plug cecremote LSTK`

---

### Menu Definitions

Create OSD menu entries for controlling devices:

```xml
<menu name="Blu-Ray Player" address="blueray">
    <onstart>
        <poweron>blueray</poweron>
        <makeactive/>
    </onstart>
    <onstop>
        <makeinactive/>
    </onstop>
</menu>
```

**Power Toggle Mode** (alternative to onstart/onstop):

```xml
<menu name="TV Power" address="TV">
    <onpoweron>
        <poweroff>TV</poweroff>
    </onpoweron>
    <onpoweroff>
        <poweron>TV</poweron>
        <makeactive/>
    </onpoweroff>
</menu>
```

**With Still Picture Player:**

```xml
<menu name="Blu-Ray" address="blueray">
    <player file="/path/to/blueray.mpg">
        <stop>Back</stop>
        <stop>Menu</stop>
        <keymaps cec="default" vdr="blueray"/>
        <onkey code="Red">
            <exec>/usr/local/bin/blueray-eject.sh</exec>
        </onkey>
        <onvolumeup>...</onvolumeup>
        <onvolumedown>...</onvolumedown>
    </player>
    <onstart>
        <poweron>blueray</poweron>
    </onstart>
    <onstop>
        <poweroff>blueray</poweroff>
    </onstop>
</menu>
```

---

### CEC Command Handlers

React to CEC commands from other devices:

```xml
<onceccommand command="STANDBY" initiator="TV">
    <commandlist>
        <exec>/usr/local/bin/vdr-shutdown.sh</exec>
    </commandlist>
</onceccommand>

<onceccommand command="SET_STREAM_PATH" initiator="TV">
    <execmenu>Blu-Ray Player</execmenu>
</onceccommand>
```

| Attribute | Description |
|-----------|-------------|
| `command` | CEC opcode name (without `CEC_OPCODE_` prefix) or numeric value. Examples: `STANDBY`, `0x36`, `54` |
| `initiator` | Source device |

| Child Element | Description |
|---------------|-------------|
| `<stopmenu>` | Stop the named menu's still picture player |
| `<execmenu>` | Execute the named menu entry |
| `<commandlist>` | Execute a command list |

---

### Command Lists

Command lists are used in `<onstart>`, `<onstop>`, `<onpoweron>`, etc.

| Command | Description |
|---------|-------------|
| `<poweron>device</poweron>` | Power on the device |
| `<poweroff>device</poweroff>` | Power off / standby the device |
| `<textviewon>device</textviewon>` | Send TextViewOn (wake + switch input) |
| `<makeactive/>` | Make VDR the active source |
| `<makeinactive/>` | Release active source |
| `<exec>command</exec>` | Execute shell command |

**Example:**

```xml
<onstart>
    <poweron>TV</poweron>
    <textviewon>TV</textviewon>
    <makeactive/>
    <exec>/usr/local/bin/notify-started.sh</exec>
</onstart>
```

---

## 📡 SVDRP Commands

Query and control the plugin via SVDRP:

```bash
svdrpsend plug cecremote <COMMAND>
```

| Command | Description |
|---------|-------------|
| `LSTD` | List active CEC devices |
| `LSTK` | List all supported CEC key codes |
| `KEYM` | List available key maps |
| `VDRK <id>` | Display VDR→CEC key map |
| `CECK <id>` | Display CEC→VDR key map |
| `GLOK <id>` | Display Global VDR→CEC key map |
| `CONN` | Connect to CEC adapter |
| `DISC` | Disconnect from CEC adapter (for other apps to use it) |
| `STAT` | Show plugin status |

---

## 🖥️ Command Line Arguments

```bash
vdr -P "cecremote [OPTIONS]"
```

| Option | Description | Default |
|--------|-------------|---------|
| `-c, --configdir <dir>` | Configuration directory (relative paths are relative to VDR config dir) | `cecremote` |
| `-x, --configfile <file>` | Configuration file name | `cecremote.xml` |
| `-l, --loglevel <level>` | Plugin log level: `0`=none, `1`=errors, `2`=errors+warnings, `3`=errors+warnings+debug | VDR's log level |

**Example:**

```bash
vdr -P "cecremote -c /etc/vdr/cec -x myconfig.xml -l 2"
```

---

## 🔧 Troubleshooting

<details>
<summary>❌ <b>Adapter not found</b></summary>

- Check USB connection
- Verify with: `ls -l /dev/ttyACM*`
- Check dmesg for USB device detection: `dmesg | tail -20`

</details>

<details>
<summary>🔒 <b>Permission denied</b></summary>

- Add user to dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
- Log out and back in
- Install udev rules from `contrib/20-libcec.rules`

</details>

<details>
<summary>📱 <b>ModemManager interference</b></summary>

- Install the udev rule to blacklist the adapter:
  ```bash
  sudo cp contrib/20-libcec.rules /etc/udev/rules.d/
  sudo udevadm control --reload-rules
  ```
- Check logs: `grep -i "modem\|mtp" /var/log/syslog`

</details>

<details>
<summary>📺 <b>No CEC devices found</b></summary>

- Ensure TV supports CEC (often called **Anynet+**, **Bravia Sync**, **SimpLink**, **Viera Link**, etc.)
- Enable CEC in TV settings menu
- Try a different HDMI port
- Test with: `echo "scan" | cec-client -s -d 1`

</details>

<details>
<summary>⌨️ <b>Keys not working</b></summary>

- Check key mappings: `svdrpsend plug cecremote LSTK`
- Verify with verbose logging: `vdr -P "cecremote -l 3"`

</details>

<details>
<summary>💥 <b>Crash on startup (Raspberry Pi)</b></summary>

- Use only **one** `<cecdevicetype>` in configuration

</details>

---

## 📝 Examples

See [`contrib/cecremote.xml`](contrib/cecremote.xml) for a complete example configuration.

### Minimal Configuration

```xml
<?xml version="1.0" encoding="UTF-8"?>
<config>
    <global>
        <hdmiport>1</hdmiport>
        <onstart>
            <poweron>TV</poweron>
            <makeactive/>
        </onstart>
        <onstop>
            <makeinactive/>
        </onstop>
    </global>
</config>
```

### Power Sync with TV

```xml
<?xml version="1.0" encoding="UTF-8"?>
<config>
    <global>
        <hdmiport>1</hdmiport>
        <shutdownonstandby>true</shutdownonstandby>
        <onstart>
            <poweron>TV</poweron>
            <makeactive/>
        </onstart>
        <onstop>
            <poweroff>TV</poweroff>
        </onstop>
    </global>
    <onceccommand command="STANDBY" initiator="TV">
        <commandlist>
            <exec>poweroff</exec>
        </commandlist>
    </onceccommand>
</config>
```

---

## 📄 License

This program is free software; you can redistribute it and/or modify it under the terms of the **GNU General Public License** as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

See the [COPYING](COPYING) file for more information.
