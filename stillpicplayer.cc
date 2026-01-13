/*
 * CECRemote PlugIn for VDR
 *
 * Copyright (C) 2015 Ulrich Eckhardt <uli-vdr@uli-eckhardt.de>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 * This class implements a VDR Player which displays a still-picture.
 */

#include "stillpicplayer.h"
#include "ceclog.h"

using namespace std;
using namespace cecplugin;

namespace cecplugin {

/**
 * @brief Destructor that frees the still picture buffer.
 */
cStillPicPlayer::~cStillPicPlayer() {
    if (pStillBuf != nullptr) {
        free (pStillBuf);
        pStillBuf = nullptr;
    }
}

/**
 * @brief Displays the loaded still picture on the device.
 *
 * Calls DeviceStillPicture with the loaded image buffer.
 */
void cStillPicPlayer::DisplayStillPicture (void)
{
    if (pStillBuf != nullptr) {
        DeviceStillPicture((const uchar *)pStillBuf, mStillBufLen);
    }
}

/**
 * @brief Loads a still picture from a file.
 *
 * Reads the image file into memory, dynamically allocating
 * buffer space as needed. Thread-safe via mutex protection.
 *
 * @param FileName Path to the still picture file (MPEG I-frame)
 */
void cStillPicPlayer::LoadStillPicture (const string &FileName)
{
    int fd;
    ssize_t len;
    int size = CDMAXFRAMESIZE;
    cMutexLock MutexLock(&mPlayerMutex);

    if (pStillBuf != nullptr) {
        free (pStillBuf);
    }
    pStillBuf = nullptr;
    mStillBufLen = 0;
    fd = open(FileName.c_str(), O_RDONLY);
    if (fd < 0) {
        string errtxt = tr("Can not open still picture: ");
        errtxt += FileName;
        Skins.QueueMessage(mtError, errtxt.c_str());
        Esyslog("%s %d Can not open still picture %s",
                __FILE__, __LINE__, FileName.c_str());
        return;
    }

    pStillBuf = (uchar *)malloc ((CDMAXFRAMESIZE * 2)+1);
    if (pStillBuf == nullptr) {
        Esyslog("%s %d Out of memory", __FILE__, __LINE__);
        close(fd);
        return;
    }
    do {
        len = read(fd, &pStillBuf[mStillBufLen], CDMAXFRAMESIZE);
        if (len < 0) {
            Esyslog ("%s %d read error %d", __FILE__, __LINE__, errno);
            close(fd);
            free (pStillBuf);
            pStillBuf = nullptr;
            mStillBufLen = 0;
            return;
        }
        if (len > 0) {
            mStillBufLen += len;
            if (mStillBufLen >= size) {
                size += CDMAXFRAMESIZE;
                pStillBuf = (uchar *) realloc(pStillBuf, size + CDMAXFRAMESIZE);
                if (pStillBuf == nullptr) {
                    close(fd);
                    mStillBufLen = 0;
                    Esyslog("%s %d Out of memory", __FILE__, __LINE__);
                    return;
                }
            }
        }
    } while (len > 0);
    close(fd);
    DisplayStillPicture();
}

/**
 * @brief Activates or deactivates the player.
 *
 * When activated, loads and displays the configured still picture.
 *
 * @param On true to activate, false to deactivate
 */
void cStillPicPlayer::Activate(bool On) {
    if (On) {
        LoadStillPicture(mConfig.mStillPic);
    }
}

} // namespace cecplugin

