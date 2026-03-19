#pragma once

#include <furi.h>
#include <furi_hal.h>

#define PCSG_VERSION_APP FAP_VERSION
#define PCSG_DEVELOPED \
    "@xMasterX & @Shmuma\nProtocol improvements by:\n@htotoo\nIcons by:\n@Svaarich\nTransmit feature by:\nShchuchkin Evgenii Yurievich"
#define PCSG_GITHUB "https://github.com/xMasterX/flipper-pager"
#define PCSG_GITHUB_TX "https://github.com/shchuchkin-pkims"

#define PCSG_KEY_FILE_VERSION 1
#define PCSG_KEY_FILE_TYPE "Flipper POCSAG Pager Key File"

/** PCSGRxKeyState state */
typedef enum {
    PCSGRxKeyStateIDLE,
    PCSGRxKeyStateBack,
    PCSGRxKeyStateStart,
    PCSGRxKeyStateAddKey,
} PCSGRxKeyState;

/** PCSGHopperState state */
typedef enum {
    PCSGHopperStateOFF,
    PCSGHopperStateRunnig,
    PCSGHopperStatePause,
    PCSGHopperStateRSSITimeOut,
} PCSGHopperState;

/** PCSGLock */
typedef enum {
    PCSGLockOff,
    PCSGLockOn,
} PCSGLock;

typedef enum {
    POCSAGPagerViewVariableItemList,
    POCSAGPagerViewSubmenu,
    POCSAGPagerViewReceiver,
    POCSAGPagerViewReceiverInfo,
    POCSAGPagerViewWidget,
    POCSAGPagerViewTextInput,
} POCSAGPagerView;

/** POCSAGPagerTxRx state */
typedef enum {
    PCSGTxRxStateIDLE,
    PCSGTxRxStateRx,
    PCSGTxRxStateTx,
    PCSGTxRxStateSleep,
} PCSGTxRxState;
