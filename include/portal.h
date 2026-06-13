#pragma once
#include <functional>

void startPortal();
void handlePortal();
bool isPortalActive();
bool isPortalMsgNeeded();
void setOtaCallbacks(std::function<void()> onStart, std::function<void(int,int)> onProgress);
