#include "ota.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

// GitHub API : recupere la derniere release et l'URL du firmware.bin
OtaStatus checkOtaUpdate() {
    OtaStatus status;
    status.currentVersion  = FIRMWARE_VERSION;
    status.updateAvailable = false;

    WiFiClientSecure client;
    client.setInsecure(); // accepte tout certificat (suffisant pour usage prive)

    HTTPClient http;
    String url = "https://api.github.com/repos/"
                 GITHUB_REPO_OWNER "/" GITHUB_REPO_NAME "/releases/latest";

    if (!http.begin(client, url)) {
        status.error = "Connexion impossible";
        return status;
    }
    http.addHeader("User-Agent",  "Cave-a-vin-ESP32");
    http.addHeader("Accept",      "application/vnd.github.v3+json");

    int code = http.GET();
    if (code != 200) {
        status.error = "HTTP " + String(code);
        http.end();
        return status;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        status.error = "JSON: " + String(err.c_str());
        return status;
    }

    String tag = doc["tag_name"] | "";
    status.latestVersion = tag;

    // Cherche l'asset firmware.bin dans la liste des fichiers de la release
    JsonArray assets = doc["assets"].as<JsonArray>();
    for (JsonObject asset : assets) {
        if (String(asset["name"] | "") == "firmware.bin") {
            status.downloadUrl = String(asset["browser_download_url"] | "");
            break;
        }
    }

    status.updateAvailable = (tag != String(FIRMWARE_VERSION)) &&
                             !status.downloadUrl.isEmpty()      &&
                             !tag.isEmpty();
    return status;
}

// Telecharge et applique le firmware — l'ESP32 redemmarre automatiquement
bool applyOtaUpdate(const String& url,
                    std::function<void()>        onStart,
                    std::function<void(int,int)> onProgress) {
    WiFiClientSecure client;
    client.setInsecure();

    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(true);

    if (onStart)    httpUpdate.onStart(onStart);
    if (onProgress) httpUpdate.onProgress(onProgress);

    t_httpUpdate_return ret = httpUpdate.update(client, url);
    // Si on arrive ici, c'est une erreur (le succes provoque un reboot)
    return false;
}
