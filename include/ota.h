#pragma once
#include <Arduino.h>

struct OtaStatus {
    String currentVersion;
    String latestVersion;
    bool   updateAvailable;
    String downloadUrl;
    String error;
};

// Interroge l'API GitHub pour connaitre la derniere release
OtaStatus checkOtaUpdate();

// Telecharge et flashe le firmware depuis l'URL fournie — redemmarre si succes
bool applyOtaUpdate(const String& url);
