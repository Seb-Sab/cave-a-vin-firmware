#pragma once
#include <Arduino.h>

// Toutes les chaines affichees sur l'OLED doivent etre en ASCII pur (0x20-0x7E)
// Le portail web (HTML UTF-8) peut utiliser des caracteres accentues
struct LangStrings {
    // Ecran principal
    const char* appName;
    const char* initializing;
    const char* ready;
    const char* placeBottle;
    // Scan vin
    const char* wineScanned;
    const char* qty;
    const char* searching;
    const char* notRegistered;
    const char* plusMinusModify;
    const char* longPlusRegister;
    // Mode enregistrement
    const char* enrollTitle;
    const char* placeLabel;
    const char* longMinusCancel;
    const char* sending;
    const char* registered;
    const char* completeOnDash;
    const char* alreadyPresent;
    const char* cancelled;
    // Hints compteur
    const char* added;
    const char* removed;
    // Reseau
    const char* connectingWifi;
    const char* wifiOk;
    const char* wifiFailed;
    const char* offlineMode;
    // Erreurs
    const char* networkError;
    const char* apiError;
    // Actions
    const char* adding;
    const char* removing;
    // Countdown auto-decrement
    const char* cancelMinus;
    const char* plusToAdd;
    const char* minusToIgnore;
    // Alertes
    const char* alert;
    const char* pressBtnAck;
    // Mesure quotidienne
    const char* caveMeasure;
    const char* bmeStab;
    // Veille
    const char* sleeping;
    // Portail
    const char* configMode;
    // Verification token
    const char* tokenError;
    const char* tokenChecking;
    // OTA
    const char* otaUpdating;
    const char* otaChecking;
    const char* otaAvailable;
    const char* otaUpdate;
    // Association RFID
    const char* linkAssociate;
    const char* linkDone;
    const char* linkNone;
    const char* linkTitle;
    const char* linkNavHelp;
    const char* linkLongHelp;
    // Changement d'etiquette (retag)
    const char* retagConfirm;
    const char* retagConfirmHint;
    const char* retagHolding;
    const char* retagScan;
    const char* retagDone;
    const char* retagUsed;
};

extern const LangStrings LANG_FR;
extern const LangStrings LANG_EN;
extern const LangStrings LANG_IT;

extern const LangStrings* T;

void setLang(const String& code);
