#pragma once
#include <Arduino.h>
#include <functional>

struct OtaStatus {
    String currentVersion;
    String latestVersion;
    bool   updateAvailable;
    String downloadUrl;
    String error;
};

// Interroge l'API GitHub pour connaitre la derniere release
OtaStatus checkOtaUpdate();

// Telecharge et flashe le firmware — redemmarre si succes
// onStart    : appelee au debut du telechargement
// onProgress : appelee periodiquement avec (octets recus, total)
bool applyOtaUpdate(const String& url,
                    std::function<void()>       onStart    = nullptr,
                    std::function<void(int,int)> onProgress = nullptr);
