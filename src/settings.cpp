#include "settings.h"
#include "config.h"

Settings settings;

void loadSettings() {
    Preferences p;
    p.begin("cave", true);
    settings.wifiSsid             = p.getString("ssid",     WIFI_SSID);
    settings.wifiPass             = p.getString("pass",     WIFI_PASS);
    settings.userNom              = p.getString("nom",      "");
    settings.userPrenom           = p.getString("prenom",   "");
    settings.userEmail            = p.getString("email",    "");
    settings.deviceToken          = p.getString("devToken", DEVICE_TOKEN_DEFAULT);
    settings.language             = p.getString("lang",     "fr");
    settings.ldrThreshold         = p.getInt(  "ldrThr",   LDR_DARK_THRESHOLD);
    settings.darkTimeoutS         = p.getInt(  "darkS",    (int)(DARK_TIMEOUT_MS / 1000UL));
    settings.inactivityTimeoutMin = p.getInt(  "inactM",   (int)(INACTIVITY_TIMEOUT_MS / 60000UL));
    settings.tempMin              = p.getFloat("tMin",     TEMP_MIN_DEFAULT);
    settings.tempMax              = p.getFloat("tMax",     TEMP_MAX_DEFAULT);
    settings.humMin               = p.getFloat("hMin",     HUM_MIN_DEFAULT);
    settings.humMax               = p.getFloat("hMax",     HUM_MAX_DEFAULT);
    p.end();
}

void saveSettings() {
    Preferences p;
    p.begin("cave", false);
    p.putString("ssid",     settings.wifiSsid);
    p.putString("pass",     settings.wifiPass);
    p.putString("nom",      settings.userNom);
    p.putString("prenom",   settings.userPrenom);
    p.putString("email",    settings.userEmail);
    p.putString("devToken", settings.deviceToken);
    p.putString("lang",     settings.language);
    p.putInt(  "ldrThr",   settings.ldrThreshold);
    p.putInt(  "darkS",    settings.darkTimeoutS);
    p.putInt(  "inactM",   settings.inactivityTimeoutMin);
    p.putFloat("tMin",     settings.tempMin);
    p.putFloat("tMax",     settings.tempMax);
    p.putFloat("hMin",     settings.humMin);
    p.putFloat("hMax",     settings.humMax);
    p.end();
}

void clearSettings() {
    Preferences p;
    p.begin("cave", false);
    p.clear();
    p.end();
}

bool hasWifiConfig() {
    return settings.wifiSsid.length() > 0;
}
