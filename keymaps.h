/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015-2016 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements the keymap handling class
 *
 */


#ifndef _CECKEYMAPS_H_
#define _CECKEYMAPS_H_

#include <vdr/plugin.h>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <cectypes.h>
#include <cec.h>

using namespace CEC;
namespace cecplugin {

// Key map CEC key->VDR keys (one CEC key can map to multiple VDR keys)
typedef std::list<eKeys> cKeyList;
typedef std::vector<cKeyList> cKeyMap;
typedef cKeyList::const_iterator cKeyListIterator;

// Key map VDR key->CEC keys (one VDR key can map to multiple CEC keys)
typedef std::list<cec_user_control_code>cCECList;
typedef std::vector<cCECList> cVDRKeyMap;
typedef cCECList::const_iterator cCECListIterator;

/**
 * @class cKeyMaps
 * @brief Manages bidirectional key mappings between CEC and VDR key codes.
 *
 * This class handles three types of key maps:
 * - CEC key map: Maps CEC remote keys to VDR keys (for receiving)
 * - VDR key map: Maps VDR keys to CEC keys (for sending in player mode)
 * - Global key map: Maps VDR keys to CEC keys (for sending outside player mode)
 *
 * Key maps can be customized via XML configuration and are identified by ID strings.
 * The "default" keymap provides standard mappings.
 */
class cKeyMaps {
private:
    eKeys mDefaultKeyMap[CEC_USER_CONTROL_CODE_MAX+1][3];  ///< Default CEC->VDR mappings
    const char *mCECKeyNames[CEC_USER_CONTROL_CODE_MAX+1]; ///< CEC key code names

    std::map<std::string, cVDRKeyMap> mVDRKeyMap;    ///< Named VDR->CEC key maps
    std::map<std::string, cKeyMap> mCECKeyMap;       ///< Named CEC->VDR key maps
    std::map<std::string, cVDRKeyMap> mGLOBALKeyMap; ///< Named global VDR->CEC maps
    cVDRKeyMap mActiveVdrKeyMap;     ///< Currently active VDR->CEC map
    cKeyMap mActiveCecKeyMap;        ///< Currently active CEC->VDR map
    cVDRKeyMap mActiveGlobalKeyMap;  ///< Currently active global map

    /**
     * @brief Gets the first CEC key code mapped to a VDR key.
     * @param key VDR key to look up.
     * @return First matching CEC key code.
     */
    cec_user_control_code getFirstCEC(eKeys key);
public:
    /** @brief Constructor - initializes default key mappings. */
    cKeyMaps();

    /**
     * @brief Initializes a VDR key map from default mappings.
     * @param id Identifier for the new key map.
     */
    void InitVDRKeyFromDefault(std::string id);

    /**
     * @brief Initializes a CEC key map from default mappings.
     * @param id Identifier for the new key map.
     */
    void InitCECKeyFromDefault(std::string id);

    /**
     * @brief Initializes a global key map from default mappings.
     * @param id Identifier for the new key map.
     */
    void InitGLOBALKeyFromDefault(std::string id);

    /**
     * @brief Clears all VDR key mappings for a CEC key.
     * @param id Key map identifier.
     * @param k CEC key code to clear.
     */
    void ClearCECKey(std::string id, cec_user_control_code k);

    /**
     * @brief Clears all CEC key mappings for a VDR key.
     * @param id Key map identifier.
     * @param k VDR key to clear.
     */
    void ClearVDRKey(std::string id, eKeys k);

    /**
     * @brief Clears all CEC key mappings for a VDR key in global map.
     * @param id Key map identifier.
     * @param k VDR key to clear.
     */
    void ClearGLOBALKey(std::string id, eKeys k);

    /**
     * @brief Adds a CEC->VDR key mapping.
     * @param id Key map identifier.
     * @param k CEC key code.
     * @param c VDR key to map to.
     */
    void AddCECKey(std::string id, cec_user_control_code k, eKeys c);

    /**
     * @brief Adds a VDR->CEC key mapping.
     * @param id Key map identifier.
     * @param k VDR key.
     * @param c CEC key code to map to.
     */
    void AddVDRKey(std::string id, eKeys k, cec_user_control_code c);

    /**
     * @brief Adds a VDR->CEC key mapping in global map.
     * @param id Key map identifier.
     * @param k VDR key.
     * @param c CEC key code to map to.
     */
    void AddGLOBALKey(std::string id, eKeys k, cec_user_control_code c);

    /**
     * @brief Converts a CEC key code to VDR key(s).
     * @param code CEC user control code.
     * @return List of mapped VDR keys.
     */
    cKeyList CECtoVDRKey(cec_user_control_code code);

    /**
     * @brief Converts a VDR key to CEC key(s).
     * @param key VDR key code.
     * @return List of mapped CEC keys.
     */
    cCECList VDRtoCECKey(eKeys key);

    /**
     * @brief Converts a CEC key name string to key code.
     * @param s Key name (e.g., "SELECT", "UP").
     * @return CEC user control code, or CEC_USER_CONTROL_CODE_UNKNOWN if not found.
     */
    cec_user_control_code StringToCEC(const std::string &s);

    /**
     * @brief Sets the active key maps for key translation.
     * @param vdrkeymapid ID of VDR key map to activate.
     * @param ceckeymapid ID of CEC key map to activate.
     * @param globalkeymapid ID of global key map to activate.
     */
    void SetActiveKeymaps(const std::string &vdrkeymapid,
                          const std::string &ceckeymapid,
                          const std::string &globalkeymapid);

    /**
     * @brief Lists all available key map IDs.
     * @return Formatted string for SVDRP output.
     */
    cString ListKeymaps();

    /**
     * @brief Lists all CEC key codes and their names.
     * @return Formatted string for SVDRP output.
     */
    cString ListKeycodes();

    /**
     * @brief Lists mappings in a CEC key map.
     * @param id Key map identifier.
     * @return Formatted string for SVDRP output.
     */
    cString ListCECKeyMap(const std::string &id);

    /**
     * @brief Lists mappings in a VDR key map.
     * @param id Key map identifier.
     * @return Formatted string for SVDRP output.
     */
    cString ListVDRKeyMap(const std::string &id);

    /**
     * @brief Lists mappings in a global key map.
     * @param id Key map identifier.
     * @return Formatted string for SVDRP output.
     */
    cString ListGLOBALKeyMap(const std::string &id);

    static constexpr char const *DEFAULTKEYMAP = "default";  ///< Default key map ID
};

} // namespace cecplugin

#endif /* _CECKEYMAPS_H_ */
