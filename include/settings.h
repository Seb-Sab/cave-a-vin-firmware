#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct Settings {
    String  wifiSsid;
    String  wifiPass;
    String  userNom;
    String  userPrenom;
    String  userEmail;
    String  deviceToken;
    String  language;           // "fr" | "en" | "it"
    int     ldrThreshold;
    int     darkTimeoutS;
    int     inactivityTimeoutMin;
    float   tempMin;
    float   tempMax;
    float   humMin;
    float   humMax;
    // Notifications externes (alertes temperature/humidite)
    String  phone;              // format international, ex "+33612345678"
    String  telegramChatId;
    bool    notifyEmail;
    bool    notifyTelegram;
};

extern Settings settings;

void loadSettings();
void saveSettings();
void clearSettings();
bool hasWifiConfig();
