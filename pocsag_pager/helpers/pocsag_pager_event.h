#pragma once

typedef enum {
    //PCSGCustomEvent
    PCSGCustomEventStartId = 100,

    PCSGCustomEventSceneSettingLock,

    PCSGCustomEventViewReceiverOK,
    PCSGCustomEventViewReceiverConfig,
    PCSGCustomEventViewReceiverBack,
    PCSGCustomEventViewReceiverOffDisplay,
    PCSGCustomEventViewReceiverUnlock,

    PCSGCustomEventTransmitEditRic,
    PCSGCustomEventTransmitEditMsg,
    PCSGCustomEventTransmitSend,
    PCSGCustomEventTransmitDone,
} PCSGCustomEvent;
