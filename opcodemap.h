/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2016 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * opcode.h: Class for converting CEC commands as string to the
 *           corresponding CEC opcode.
 */

#ifndef PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_OPCODE_H_
#define PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_OPCODE_H_

#include <string>
#include <map>
#include <cec.h>
#include <cectypes.h>

namespace cecplugin {

/**
 * @class opcodeMap
 * @brief Maps CEC opcode names to their numeric values.
 *
 * Provides conversion from string representations of CEC opcodes
 * (e.g., "STANDBY", "IMAGE_VIEW_ON") to their corresponding
 * cec_opcode enum values. Used for parsing XML configuration.
 */
class opcodeMap {
private:
    typedef std::map<std::string, CEC::cec_opcode> mapOp;
    typedef mapOp::iterator mapOpIterator;
    static mapOp *map;  ///< Lazy-initialized opcode map

    /**
     * @brief Initializes the opcode map with all known CEC opcodes.
     */
    static void initMap();

public:
    /**
     * @brief Converts an opcode name string to its enum value.
     * @param name Opcode name (e.g., "STANDBY", without CEC_OPCODE_ prefix).
     * @param opcode Output parameter for the converted opcode.
     * @return true if the name was found and converted, false otherwise.
     */
    static bool getOpcode(std::string name, CEC::cec_opcode &opcode);
};

} // namespace cecplugin
#endif /* PLUGINS_SRC_VDR_PLUGIN_CECREMOTE_OPCODE_H_ */
