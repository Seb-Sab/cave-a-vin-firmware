#include "portal.h"
#include "settings.h"
#include "config.h"
#include "ota.h"
#include <Arduino.h>

extern void showOtaProgress(int cur, int total, const String& fromVer, const String& toVer);
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

static WebServer server(80);
static DNSServer dns;
static bool      portalActive    = false;
static bool      portalMsgNeeded = false;
static OtaStatus lastOta;

static std::function<void()>        otaStartCb    = nullptr;
static std::function<void(int,int)> otaProgressCb = nullptr;

void setOtaCallbacks(std::function<void()> onStart, std::function<void(int,int)> onProgress) {
    otaStartCb    = onStart;
    otaProgressCb = onProgress;
}

// ---- Page HTML embarquee ----
static const char HTML_PAGE[] = R"rawhtml(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Clever Cellar</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:#111;color:#ddd;padding:16px;max-width:520px;margin:auto}
.logo-wrap{text-align:center;margin:16px 0 8px}
.logo-wrap svg{width:72px;height:auto}
h1{text-align:center;color:#a876c8;margin:0 0 20px;font-size:1.4em;font-weight:500}
section{background:#1e1e2e;border-radius:10px;padding:16px;margin-bottom:16px}
h2{color:#a876c8;font-size:1em;margin-bottom:12px;border-bottom:1px solid #3d2456;padding-bottom:6px}
label{display:block;font-size:.82em;color:#aaa;margin-top:10px;margin-bottom:3px}
input,select{width:100%;padding:8px 10px;background:#2a2a3e;border:1px solid #444;color:#eee;border-radius:5px;font-size:.95em}
input:focus,select:focus{outline:none;border-color:#a876c8}
.row2{display:flex;gap:10px}
.row2>div{flex:1}
.reco{font-size:.78em;color:#7ba;background:#152525;border-radius:4px;padding:6px 8px;margin-top:8px;line-height:1.5;white-space:pre-line}
.hint{font-size:.75em;color:#777;margin-top:3px}
.ldr-box{display:flex;align-items:center;gap:12px;background:#161626;border-radius:6px;padding:10px 14px;margin-top:8px}
.ldr-num{font-size:1.8em;font-weight:bold;color:#a876c8;min-width:64px}
.ldr-ind{font-size:.9em;padding:4px 10px;border-radius:4px;font-weight:bold}
.ldr-ind.light{background:#3a2a00;color:#fa0}
.ldr-ind.dark{background:#001a3a;color:#6af}
button{width:100%;padding:10px;border:none;border-radius:5px;cursor:pointer;font-size:.95em;margin-top:8px}
.btn-scan{background:#2a2a3e;color:#bbb;border:1px solid #444}
.btn-scan:hover{background:#3a3a4e}
.btn-save{background:#5b2f7c;color:#fff;font-weight:bold;font-size:1.1em;margin-top:20px;padding:14px}
.btn-save:hover{background:#7a4aa0}
.msg{text-align:center;margin-top:12px;font-size:.9em;min-height:1.4em}
.lang-bar{display:flex;gap:8px;margin-top:8px}
.lang-btn{flex:1;padding:9px 4px;background:#2a2a3e;color:#aaa;border:1px solid #444;border-radius:5px;cursor:pointer;font-size:.9em;text-align:center}
.lang-btn.active{background:#5b2f7c;color:#fff;border-color:#a876c8}
.lang-btn:hover:not(.active){background:#3a3a4e}
.ota-row{display:flex;justify-content:space-between;align-items:center;font-size:.85em;margin-top:6px}
.ota-ver{color:#a876c8;font-weight:bold;font-size:1em}
.ota-new{color:#7bc87b;font-weight:bold;font-size:1em}
.ota-status{font-size:.82em;margin-top:8px;min-height:1.2em}
.btn-ota{background:#1e2a1e;color:#7bc87b;border:1px solid #3a5a3a;border-radius:5px;cursor:pointer;font-size:.9em;padding:8px;width:100%;margin-top:8px}
.btn-ota:hover{background:#2a3a2a}
.btn-ota:disabled{opacity:.5;cursor:default}
.btn-ota-update{background:#2a1a3a;color:#a876c8;border-color:#5b2f7c;margin-top:6px}
.btn-ota-update:hover{background:#3a2a4a}
.btn-request{background:#1e2a3a;color:#7ba8c8;border:1px solid #3a6a9a;border-radius:5px;cursor:pointer;font-size:.9em;padding:8px;width:100%;margin-top:8px}
.btn-request:hover{background:#2a3a4a}
.btn-request:disabled{opacity:.5;cursor:default}
.danger-section{border:1px solid #5a2020;background:#1a1010}
.danger-section h2{color:#c55;border-color:#5a2020}
.btn-reset{background:#5a1010;color:#ff9999;border:1px solid #8a3030;font-weight:bold;margin-top:8px}
.btn-reset:hover{background:#7a1a1a}
.danger-warn{font-size:.8em;color:#a66;margin-top:8px;line-height:1.5}
.pw-wrap{position:relative}
.pw-toggle{position:absolute;right:6px;top:6px;width:auto;background:none;border:none;padding:5px 6px;cursor:pointer;margin:0;color:#888;display:flex;align-items:center}
.pw-toggle:hover{color:#bbb}
.pw-wrap input{padding-right:34px}
.chk-row{display:flex;align-items:center;gap:8px;font-size:.9em;color:#ddd;margin-top:10px;cursor:pointer}
.chk-row input{width:auto}
.chk-row.disabled{opacity:.45;cursor:default}
.btn-tg{display:block;text-align:center;text-decoration:none;background:#1e2a3a;color:#7ba8c8;border:1px solid #3a6a9a;border-radius:5px;padding:10px;font-size:.9em;margin-top:10px}
.btn-tg:hover{background:#2a3a4a}
</style>
</head>
<body>
<div class="logo-wrap">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 400">
<defs><radialGradient id="g" cx="35%" cy="30%"><stop offset="0%" stop-color="#A876C8"/><stop offset="100%" stop-color="#5B2F7C"/></radialGradient></defs>
<circle cx="200" cy="200" r="150" fill="#1e1e2e" stroke="#3D2456" stroke-width="3"/>
<circle cx="200" cy="200" r="140" fill="none" stroke="#3D2456" stroke-width="1.4"/>
<g stroke="#2B7A9B" fill="none" stroke-linecap="round" stroke-width="8"><path d="M 155 114 Q 200 80 245 114"/></g>
<g stroke="#2B7A9B" fill="none" stroke-linecap="round" stroke-width="7"><path d="M 169 127 Q 200 101 231 127"/></g>
<g stroke="#2B7A9B" fill="none" stroke-linecap="round" stroke-width="6"><path d="M 183 140 Q 200 126 217 140"/></g>
<circle cx="200" cy="148" r="5" fill="#2B7A9B"/>
<path d="M 206 170 Q 246 132 292 144 Q 288 170 252 180 Q 226 180 206 170 Z" fill="#7A9B5C" stroke="#4A6B3C" stroke-width="2.5"/>
<line x1="200" y1="154" x2="200" y2="168" stroke="#6B4E3D" stroke-width="3.5" stroke-linecap="round"/>
<g stroke="#3D2456" stroke-width="2">
<circle cx="170" cy="182" r="18" fill="url(#g)"/><circle cx="200" cy="182" r="18" fill="url(#g)"/><circle cx="230" cy="182" r="18" fill="url(#g)"/>
<circle cx="155" cy="212" r="18" fill="url(#g)"/><circle cx="185" cy="212" r="18" fill="url(#g)"/><circle cx="215" cy="212" r="18" fill="url(#g)"/><circle cx="245" cy="212" r="18" fill="url(#g)"/>
<circle cx="170" cy="242" r="18" fill="url(#g)"/><circle cx="200" cy="242" r="18" fill="url(#g)"/><circle cx="230" cy="242" r="18" fill="url(#g)"/>
<circle cx="185" cy="272" r="18" fill="url(#g)"/><circle cx="215" cy="272" r="18" fill="url(#g)"/>
<circle cx="200" cy="302" r="18" fill="url(#g)"/>
</g>
</svg>
</div>
<h1>Clever Cellar</h1>

<section>
<h2 data-i18n="lang_title">Langue</h2>
<div class="lang-bar">
  <button id="btn-fr" class="lang-btn active" onclick="setLang('fr')">&#127467;&#127479; Fran&#231;ais</button>
  <button id="btn-en" class="lang-btn" onclick="setLang('en')">&#127468;&#127463; English</button>
  <button id="btn-it" class="lang-btn" onclick="setLang('it')">&#127470;&#127481; Italiano</button>
</div>
<input type="hidden" id="lang" value="fr">
</section>

<section>
<h2 data-i18n="wifi_title">WiFi</h2>
<label data-i18n="wifi_network">R&eacute;seau</label>
<select id="ssid"></select>
<button class="btn-scan" id="scanBtn" onclick="scanWifi()" data-i18n="wifi_scan_btn">Scanner les r&eacute;seaux</button>
<label data-i18n="wifi_pass">Mot de passe</label>
<div class="pw-wrap">
  <input type="password" id="pass" data-i18n-ph="wifi_pass_ph" placeholder="Laisser vide pour conserver l'actuel">
  <button type="button" class="pw-toggle" onclick="togglePw('pass',this)"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8Z"/><circle cx="12" cy="12" r="3"/></svg></button>
</div>
</section>

<section>
<h2 data-i18n="user_title">Utilisateur</h2>
<label data-i18n="user_name">Nom</label>
<input type="text" id="nom" placeholder="Dupont">
<label data-i18n="user_firstname">Pr&eacute;nom</label>
<input type="text" id="prenom" placeholder="Jean">
<label data-i18n="user_email">Email</label>
<input type="email" id="email" placeholder="jean@exemple.com">
<label data-i18n="user_token">Token device</label>
<input type="text" id="devToken" placeholder="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx">
<p class="hint" data-i18n="user_token_hint">Identifiant unique de cette cave dans la base de donn&eacute;es.</p>
</section>

<section id="requestSection" style="display:none">
<h2 data-i18n="request_title">Demande d&apos;activation</h2>
<p class="hint" data-i18n="request_hint">Pas encore de token ? Remplissez vos informations ci-dessus puis envoyez une demande. L&apos;administrateur vous enverra votre token par email.</p>
<button class="btn-request" id="requestBtn" onclick="requestActivation()" data-i18n="request_btn">Demander l&apos;activation</button>
<p class="msg" id="requestMsg"></p>
</section>

<section>
<h2 data-i18n="ldr_title">Capteur lumi&egrave;re (LDR)</h2>
<label data-i18n="ldr_realtime">Valeur LDR en temps r&eacute;el</label>
<div class="ldr-box">
  <div class="ldr-num" id="ldrNum">-</div>
  <div class="ldr-ind" id="ldrInd">...</div>
</div>
<p class="hint" data-i18n="ldr_hint1">Placez la cave en situation r&eacute;elle pour calibrer le seuil.</p>
<label data-i18n="ldr_threshold">Seuil obscurit&eacute; (0-4095)</label>
<input type="number" id="ldrThr" min="0" max="4095">
<label data-i18n="ldr_dark_time">Dur&eacute;e obscurit&eacute; avant veille (secondes)</label>
<input type="number" id="darkS" min="5" max="3600">
<label data-i18n="ldr_inact">Inactivit&eacute; avant veille (minutes)</label>
<input type="number" id="inactM" min="1" max="120">
</section>

<section>
<h2 data-i18n="alarm_title">Alarmes cave</h2>
<div class="reco" id="alarmReco" data-i18n="alarm_reco">Recommandations &#8212; Rouges : 12-14&#176;C | Blancs : 8-12&#176;C | Plage : 10-16&#176;C
Humidit&#233; optimale : 65-75% | Plage : 60-80%
En dessous de 50% : bouchons secs. Au-dessus de 85% : risque de moisissures.</div>
<label data-i18n="alarm_tmin">Temp&eacute;rature min (&#176;C)</label>
<input type="number" id="tMin" min="0" max="30" step="0.5">
<label data-i18n="alarm_tmax">Temp&eacute;rature max (&#176;C)</label>
<input type="number" id="tMax" min="0" max="30" step="0.5">
<div class="row2">
  <div>
    <label data-i18n="alarm_hmin">Humidit&eacute; min (%)</label>
    <input type="number" id="hMin" min="0" max="100">
  </div>
  <div>
    <label data-i18n="alarm_hmax">Humidit&eacute; max (%)</label>
    <input type="number" id="hMax" min="0" max="100">
  </div>
</div>
</section>

<section>
<h2 data-i18n="notif_title">Notifications d&apos;alerte</h2>
<p class="hint" data-i18n="notif_hint">Re&ccedil;evez une alerte si la temp&eacute;rature ou l&apos;humidit&eacute; sort de la plage d&eacute;finie ci-dessus.</p>
<label data-i18n="notif_phone">T&eacute;l&eacute;phone (format international)</label>
<input type="tel" id="phone" placeholder="+33612345678">
<label class="chk-row"><input type="checkbox" id="notifyEmail"> <span data-i18n="notif_email">Email</span></label>
<label class="chk-row"><input type="checkbox" id="notifyTelegram"> <span data-i18n="notif_telegram">Telegram</span></label>
<a id="tgConnectBtn" class="btn-tg" href="#" target="_blank" data-i18n="notif_tg_connect">Connecter Telegram</a>
<p class="hint" data-i18n="notif_tg_hint">Ouvrez ce lien depuis votre t&eacute;l&eacute;phone : Telegram s&apos;ouvre et associe automatiquement votre compte, sans rien copier.</p>
<label class="chk-row disabled"><input type="checkbox" disabled> <span data-i18n="notif_whatsapp">WhatsApp (bient&ocirc;t disponible)</span></label>
</section>

<section>
<h2 data-i18n="ota_title">Mise &agrave; jour firmware</h2>
<div class="ota-row">
  <span data-i18n="ota_current_lbl">Version install&eacute;e :</span>
  <span class="ota-ver" id="otaCurrent">---</span>
</div>
<button class="btn-ota" id="otaCheckBtn" onclick="checkOta()" data-i18n="ota_check_btn">V&eacute;rifier les mises &agrave; jour</button>
<div class="ota-status" id="otaStatus"></div>
<div id="otaUpdateSection" style="display:none">
  <div class="ota-row" style="margin-top:8px">
    <span data-i18n="ota_latest_lbl">Nouvelle version :</span>
    <span class="ota-new" id="otaLatest"></span>
  </div>
  <button class="btn-ota btn-ota-update" id="otaApplyBtn" onclick="applyOta()" data-i18n="ota_update_btn">Mettre &agrave; jour maintenant</button>
</div>
</section>

<button class="btn-save" onclick="save()" data-i18n="save_btn">Enregistrer et red&eacute;marrer</button>
<p class="msg" id="msg"></p>

<section class="danger-section">
<h2 data-i18n="reset_title">R&eacute;initialisation</h2>
<p class="danger-warn" data-i18n="reset_warning">Efface d&eacute;finitivement WiFi, token, utilisateur et tous les r&eacute;glages. Le bo&icirc;tier red&eacute;marre en mode configuration comme lors de la premi&egrave;re utilisation.</p>
<button class="btn-reset" onclick="doReset()" data-i18n="reset_btn">R&eacute;initialiser le bo&icirc;tier</button>
<p class="msg" id="resetMsg"></p>
</section>

<script>
var currentLang='fr';
var cs='';

var i18n={
  fr:{
    lang_title:'Langue',
    wifi_title:'WiFi',
    wifi_network:'Réseau',
    wifi_scan_btn:'Scanner les réseaux',
    wifi_scan_busy:'Scan en cours...',
    wifi_pass:'Mot de passe',
    wifi_pass_ph:"Laisser vide pour conserver l'actuel",
    user_title:'Utilisateur',
    user_name:'Nom',
    user_firstname:'Prénom',
    user_email:'Email',
    user_token:'Token device (fourni par l\'administrateur)',
    user_token_hint:'Identifiant unique de cette cave dans la base de données.',
    ldr_title:'Capteur lumière (LDR)',
    ldr_realtime:'Valeur LDR en temps réel',
    ldr_hint1:'Placez la cave en situation réelle pour calibrer le seuil.',
    ldr_threshold:'Seuil obscurité (0-4095)',
    ldr_dark_time:'Durée obscurité avant veille (secondes)',
    ldr_inact:'Inactivité avant veille (minutes)',
    alarm_title:'Alarmes cave',
    alarm_reco:'Recommandations — Rouges : 12-14°C | Blancs : 8-12°C | Plage : 10-16°C\nHumidité optimale : 65-75% | Plage : 60-80%\nEn dessous de 50% : bouchons secs. Au-dessus de 85% : risque de moisissures.',
    alarm_tmin:'Température min (°C)',
    alarm_tmax:'Température max (°C)',
    alarm_hmin:'Humidité min (%)',
    alarm_hmax:'Humidité max (%)',
    save_btn:'Enregistrer et redémarrer',
    saving:'Enregistrement...',
    saved:'Sauvegardé ! Redémarrage en cours...',
    error_prefix:'Erreur : ',
    conn_lost:'Connexion perdue (redémarrage en cours ?)',
    ldr_dark:'Obscur',
    ldr_light:'Éclairé',
    notif_title:'Notifications d\'alerte',
    notif_hint:'Reçevez une alerte si la température ou l\'humidité sort de la plage définie ci-dessus.',
    notif_phone:'Téléphone (format international)',
    notif_email:'Email',
    notif_telegram:'Telegram',
    notif_tg_connect:'Connecter Telegram',
    notif_tg_hint:'Ouvrez ce lien depuis votre téléphone : Telegram s\'ouvre et associe automatiquement votre compte, sans rien copier.',
    notif_whatsapp:'WhatsApp (bientôt disponible)',
    ota_title:'Mise à jour firmware',
    ota_current_lbl:'Version installée :',
    ota_latest_lbl:'Nouvelle version :',
    ota_check_btn:'Vérifier les mises à jour',
    ota_checking:'Vérification en cours...',
    ota_up_to_date:'✓ Firmware à jour',
    ota_update_avail:'Mise à jour disponible !',
    ota_update_btn:'Mettre à jour maintenant',
    ota_confirm:'Mettre à jour le firmware ? L\'appareil va redémarrer.',
    ota_updating:'Téléchargement... Ne pas couper l\'alimentation.',
    ota_success:'✓ Mise à jour réussie. Redémarrage en cours...',
    ota_error:'Erreur',
    ota_no_wifi:'WiFi non connecté. Si vous venez de configurer le réseau, enregistrez et redémarrez d\'abord.',
    reset_title:'Réinitialisation usine',
    reset_warning:'Efface définitivement WiFi, token, utilisateur et tous les réglages. Le boîtier redémarre en mode configuration comme lors de la première utilisation.',
    reset_btn:'Réinitialiser le boîtier',
    reset_confirm:'Confirmer la réinitialisation ? Toutes les données seront effacées définitivement.',
    reset_done:'Réinitialisation en cours... Le boîtier redémarre en mode configuration.',
    request_title:"Demande d'activation",
    request_hint:"Pas encore de token ? Remplissez vos informations ci-dessus puis envoyez une demande. L'administrateur vous enverra votre token par email.",
    request_btn:"Demander l'activation",
    request_sending:'Envoi en cours...',
    request_sent:'✓ Demande envoyée ! Vous recevrez votre token par email.',
    request_error:"Erreur lors de l'envoi",
    request_no_wifi:'Enregistrez d\'abord vos identifiants WiFi.',
    request_fill:'Remplissez Nom, Prénom et Email.'
  },
  en:{
    lang_title:'Language',
    wifi_title:'WiFi',
    wifi_network:'Network',
    wifi_scan_btn:'Scan networks',
    wifi_scan_busy:'Scanning...',
    wifi_pass:'Password',
    wifi_pass_ph:'Leave blank to keep current',
    user_title:'User',
    user_name:'Last name',
    user_firstname:'First name',
    user_email:'Email',
    user_token:'Device token (provided by administrator)',
    user_token_hint:'Unique identifier for this cellar in the database.',
    ldr_title:'Light sensor (LDR)',
    ldr_realtime:'Real-time LDR value',
    ldr_hint1:'Place the cellar in real conditions to calibrate the threshold.',
    ldr_threshold:'Dark threshold (0-4095)',
    ldr_dark_time:'Dark duration before sleep (seconds)',
    ldr_inact:'Inactivity before sleep (minutes)',
    alarm_title:'Cellar alarms',
    alarm_reco:'Recommendations — Reds: 12-14°C | Whites: 8-12°C | Range: 10-16°C\nOptimal humidity: 65-75% | Range: 60-80%\nBelow 50%: corks dry out. Above 85%: mould risk.',
    alarm_tmin:'Min temperature (°C)',
    alarm_tmax:'Max temperature (°C)',
    alarm_hmin:'Min humidity (%)',
    alarm_hmax:'Max humidity (%)',
    save_btn:'Save and restart',
    saving:'Saving...',
    saved:'Saved! Restarting...',
    error_prefix:'Error: ',
    conn_lost:'Connection lost (restarting?)',
    ldr_dark:'Dark',
    ldr_light:'Lit',
    notif_title:'Alert notifications',
    notif_hint:'Get notified if temperature or humidity goes outside the range defined above.',
    notif_phone:'Phone (international format)',
    notif_email:'Email',
    notif_telegram:'Telegram',
    notif_tg_connect:'Connect Telegram',
    notif_tg_hint:'Open this link from your phone: Telegram opens and links your account automatically, nothing to copy.',
    notif_whatsapp:'WhatsApp (coming soon)',
    ota_title:'Firmware update',
    ota_current_lbl:'Installed version:',
    ota_latest_lbl:'New version:',
    ota_check_btn:'Check for updates',
    ota_checking:'Checking...',
    ota_up_to_date:'✓ Firmware up to date',
    ota_update_avail:'Update available!',
    ota_update_btn:'Update now',
    ota_confirm:'Update firmware? The device will restart.',
    ota_updating:'Downloading... Do not cut power.',
    ota_success:'✓ Update successful. Restarting...',
    ota_error:'Error',
    ota_no_wifi:'WiFi not connected. If you just configured the network, save and restart first.',
    reset_title:'Factory reset',
    reset_warning:'Permanently erases WiFi, token, user info and all settings. The device will restart in configuration mode as if it were first use.',
    reset_btn:'Reset device',
    reset_confirm:'Confirm factory reset? All data will be permanently erased.',
    reset_done:'Resetting... The device is restarting in configuration mode.',
    request_title:'Activation request',
    request_hint:'No token yet? Fill in your details above and send a request. The administrator will send you your token by email.',
    request_btn:'Request activation',
    request_sending:'Sending...',
    request_sent:'✓ Request sent! You will receive your token by email.',
    request_error:'Error sending request',
    request_no_wifi:'Save your WiFi credentials first.',
    request_fill:'Fill in Name, First name and Email.'
  },
  it:{
    lang_title:'Lingua',
    wifi_title:'WiFi',
    wifi_network:'Rete',
    wifi_scan_btn:'Cerca reti',
    wifi_scan_busy:'Ricerca...',
    wifi_pass:'Password',
    wifi_pass_ph:"Lascia vuoto per mantenere l'attuale",
    user_title:'Utente',
    user_name:'Cognome',
    user_firstname:'Nome',
    user_email:'Email',
    user_token:"Token device (fornito dall'amministratore)",
    user_token_hint:'Identificatore unico di questa cantina nel database.',
    ldr_title:'Sensore luce (LDR)',
    ldr_realtime:'Valore LDR in tempo reale',
    ldr_hint1:'Posiziona la cantina in condizioni reali per calibrare la soglia.',
    ldr_threshold:'Soglia oscurità (0-4095)',
    ldr_dark_time:'Durata buio prima dello standby (secondi)',
    ldr_inact:'Inattività prima dello standby (minuti)',
    alarm_title:'Allarmi cantina',
    alarm_reco:'Raccomandazioni — Rossi: 12-14°C | Bianchi: 8-12°C | Gamma: 10-16°C\nUmidità ottimale: 65-75% | Gamma: 60-80%\nSotto il 50%: i tappi si seccano. Sopra l\'85%: rischio di muffa.',
    alarm_tmin:'Temperatura min (°C)',
    alarm_tmax:'Temperatura max (°C)',
    alarm_hmin:'Umidità min (%)',
    alarm_hmax:'Umidità max (%)',
    save_btn:'Salva e riavvia',
    saving:'Salvataggio...',
    saved:'Salvato! Riavvio in corso...',
    error_prefix:'Errore: ',
    conn_lost:'Connessione persa (riavvio in corso?)',
    ldr_dark:'Buio',
    ldr_light:'Illuminato',
    notif_title:'Notifiche di allerta',
    notif_hint:'Ricevi un avviso se la temperatura o l\'umidità esce dall\'intervallo definito sopra.',
    notif_phone:'Telefono (formato internazionale)',
    notif_email:'Email',
    notif_telegram:'Telegram',
    notif_tg_connect:'Connetti Telegram',
    notif_tg_hint:'Apri questo link dal tuo telefono: Telegram si apre e collega automaticamente il tuo account, senza copiare nulla.',
    notif_whatsapp:'WhatsApp (presto disponibile)',
    ota_title:'Aggiornamento firmware',
    ota_current_lbl:'Versione installata:',
    ota_latest_lbl:'Nuova versione:',
    ota_check_btn:'Controlla aggiornamenti',
    ota_checking:'Verifica in corso...',
    ota_up_to_date:'✓ Firmware aggiornato',
    ota_update_avail:'Aggiornamento disponibile!',
    ota_update_btn:'Aggiorna adesso',
    ota_confirm:'Aggiornare il firmware? Il dispositivo si riavvierà.',
    ota_updating:'Download in corso... Non togliere alimentazione.',
    ota_success:'✓ Aggiornamento riuscito. Riavvio in corso...',
    ota_error:'Errore',
    ota_no_wifi:'WiFi non connesso. Se hai appena configurato la rete, salva e riavvia prima.',
    reset_title:'Ripristino impostazioni',
    reset_warning:'Cancella definitivamente WiFi, token, utente e tutte le impostazioni. Il dispositivo si riavvierà in modalità configurazione come al primo utilizzo.',
    reset_btn:'Ripristina dispositivo',
    reset_confirm:'Confermare il ripristino? Tutti i dati saranno cancellati definitivamente.',
    reset_done:'Ripristino in corso... Il dispositivo si riavvia in modalità configurazione.',
    request_title:'Richiesta di attivazione',
    request_hint:"Nessun token? Compila i tuoi dati qui sopra e invia una richiesta. L'amministratore ti invierà il token via email.",
    request_btn:'Richiedi attivazione',
    request_sending:'Invio in corso...',
    request_sent:'✓ Richiesta inviata! Riceverai il tuo token via email.',
    request_error:"Errore durante l'invio",
    request_no_wifi:'Salva prima le credenziali WiFi.',
    request_fill:'Compila Nome, Cognome ed Email.'
  }
};

var EYE_OPEN='<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8Z"/><circle cx="12" cy="12" r="3"/></svg>';
var EYE_OFF='<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17.94 17.94A10.94 10.94 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94"/><path d="M9.9 4.24A10.94 10.94 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19"/><path d="M14.12 14.12a3 3 0 1 1-4.24-4.24"/><line x1="1" y1="1" x2="23" y2="23"/></svg>';

function togglePw(id,btn){
  var inp=document.getElementById(id);
  var show=inp.type==='password';
  inp.type=show?'text':'password';
  btn.innerHTML=show?EYE_OFF:EYE_OPEN;
}

function setLang(lang){
  currentLang=lang;
  document.getElementById('lang').value=lang;
  var tr=i18n[lang];
  var els=document.querySelectorAll('[data-i18n]');
  for(var i=0;i<els.length;i++){
    var k=els[i].getAttribute('data-i18n');
    if(tr[k]!==undefined)els[i].textContent=tr[k];
  }
  var phs=document.querySelectorAll('[data-i18n-ph]');
  for(var i=0;i<phs.length;i++){
    var k=phs[i].getAttribute('data-i18n-ph');
    if(tr[k]!==undefined)phs[i].placeholder=tr[k];
  }
  var btns=['fr','en','it'];
  for(var i=0;i<btns.length;i++){
    document.getElementById('btn-'+btns[i]).className='lang-btn'+(btns[i]===lang?' active':'');
  }
}

function rssiIcon(r){
  if(r>=-50)return'●●●●';
  if(r>=-65)return'●●●○';
  if(r>=-75)return'●●○○';
  return'●○○○';
}

function scanWifi(){
  var b=document.getElementById('scanBtn');
  b.textContent=i18n[currentLang].wifi_scan_busy;b.disabled=true;
  fetch('/scan').then(function(r){return r.json();}).then(function(n){
    var s=document.getElementById('ssid'),p=s.value||cs;
    s.innerHTML='';
    n.sort(function(a,b){return b.rssi-a.rssi;}).forEach(function(x){
      var o=document.createElement('option');
      o.value=x.ssid;o.textContent=rssiIcon(x.rssi)+' '+x.ssid+' ('+x.rssi+' dBm)';
      if(x.ssid===p)o.selected=true;
      s.appendChild(o);
    });
    if(!s.value&&p){
      var o=document.createElement('option');o.value=p;o.textContent=p;o.selected=true;
      s.insertBefore(o,s.firstChild);
    }
    b.textContent=i18n[currentLang].wifi_scan_btn;b.disabled=false;
  }).catch(function(){b.textContent=i18n[currentLang].wifi_scan_btn;b.disabled=false;});
}

function updateLdr(){
  fetch('/ldr').then(function(r){return r.json();}).then(function(d){
    document.getElementById('ldrNum').textContent=d.value;
    var ind=document.getElementById('ldrInd');
    if(d.dark){ind.textContent=i18n[currentLang].ldr_dark;ind.className='ldr-ind dark';}
    else{ind.textContent=i18n[currentLang].ldr_light;ind.className='ldr-ind light';}
  }).catch(function(){});
}
setInterval(updateLdr,2000);
updateLdr();

fetch('/config').then(function(r){return r.json();}).then(function(c){
  cs=c.ssid||'';
  var s=document.getElementById('ssid');
  s.innerHTML='';
  if(cs){var o=document.createElement('option');o.value=cs;o.textContent=cs;s.appendChild(o);}
  ['nom','prenom','email','devToken','phone'].forEach(function(k){if(c[k])document.getElementById(k).value=c[k];});
  var keys=['ldrThr','darkS','inactM','tMin','tMax','hMin','hMax'];
  keys.forEach(function(k){if(c[k]!=null)document.getElementById(k).value=c[k];});
  document.getElementById('notifyEmail').checked=!!c.notifyEmail;
  document.getElementById('notifyTelegram').checked=!!c.notifyTelegram;
  if(c.devToken&&c.tgBot){
    var payload=c.devToken.replace(/-/g,'');
    document.getElementById('tgConnectBtn').href='https://t.me/'+c.tgBot+'?start='+payload;
  }
  if(c.version)document.getElementById('otaCurrent').textContent=c.version;
  if(!c.devToken){document.getElementById('requestSection').style.display='';}
  setLang(c.lang||'fr');
});

function checkOta(){
  var tr=i18n[currentLang];
  var btn=document.getElementById('otaCheckBtn');
  var st=document.getElementById('otaStatus');
  btn.disabled=true;
  st.style.color='#aaa';
  st.textContent=tr.ota_checking;
  document.getElementById('otaUpdateSection').style.display='none';
  fetch('/ota').then(function(r){return r.json();}).then(function(d){
    document.getElementById('otaCurrent').textContent=d.current||'?';
    btn.disabled=false;
    if(d.error==='no_wifi'){
      st.style.color='#aa7';st.textContent=tr.ota_no_wifi;
    }else if(d.error){
      st.style.color='#a44';st.textContent=tr.ota_error+': '+d.error;
    }else if(d.available){
      document.getElementById('otaLatest').textContent=d.latest;
      document.getElementById('otaUpdateSection').style.display='';
      st.style.color='#aa7';st.textContent=tr.ota_update_avail;
    }else{
      st.style.color='#7a7';st.textContent=tr.ota_up_to_date;
    }
  }).catch(function(e){
    btn.disabled=false;
    st.style.color='#a44';st.textContent=tr.ota_error+': '+e;
  });
}

function applyOta(){
  var tr=i18n[currentLang];
  if(!confirm(tr.ota_confirm))return;
  var applyBtn=document.getElementById('otaApplyBtn');
  var checkBtn=document.getElementById('otaCheckBtn');
  var st=document.getElementById('otaStatus');
  applyBtn.disabled=true;checkBtn.disabled=true;
  st.style.color='#aa7';st.textContent=tr.ota_updating;
  fetch('/ota/update',{method:'POST'}).then(function(r){return r.json();}).then(function(d){
    if(d.ok){st.style.color='#7a7';st.textContent=tr.ota_success;}
    else{applyBtn.disabled=false;checkBtn.disabled=false;st.style.color='#a44';st.textContent=tr.ota_error+': '+(d.error||'');}
  }).catch(function(){
    // L'ESP32 redemmarre => connexion perdue = succes attendu
    st.style.color='#7a7';st.textContent=tr.ota_success;
  });
}

function requestActivation(){
  var tr=i18n[currentLang];
  var nom=document.getElementById('nom').value.trim();
  var prenom=document.getElementById('prenom').value.trim();
  var email=document.getElementById('email').value.trim();
  var msg=document.getElementById('requestMsg');
  if(!nom||!prenom||!email){msg.style.color='#a44';msg.textContent=tr.request_fill;return;}
  var btn=document.getElementById('requestBtn');
  btn.disabled=true;
  msg.style.color='#aaa';msg.textContent=tr.request_sending;
  fetch('/request',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({nom:nom,prenom:prenom,email:email})
  }).then(function(r){return r.json();}).then(function(d){
    btn.disabled=false;
    if(d.ok){msg.style.color='#7a7';msg.textContent=tr.request_sent;}
    else if(d.error==='no_wifi'){msg.style.color='#aa7';msg.textContent=tr.request_no_wifi;}
    else{msg.style.color='#a44';msg.textContent=tr.request_error+(d.error?': '+d.error:'');}
  }).catch(function(){btn.disabled=false;msg.style.color='#a44';msg.textContent=tr.request_error;});
}

function doReset(){
  var tr=i18n[currentLang];
  if(!confirm(tr.reset_confirm))return;
  var msg=document.getElementById('resetMsg');
  msg.style.color='#aa7';msg.textContent=tr.reset_done;
  fetch('/reset',{method:'POST'}).catch(function(){
    msg.style.color='#7a7';msg.textContent=tr.reset_done;
  });
}

function save(){
  var d={
    ssid:document.getElementById('ssid').value,
    pass:document.getElementById('pass').value,
    nom:document.getElementById('nom').value,
    prenom:document.getElementById('prenom').value,
    email:document.getElementById('email').value,
    devToken:document.getElementById('devToken').value,
    lang:document.getElementById('lang').value,
    ldrThr:parseInt(document.getElementById('ldrThr').value)||1000,
    darkS:parseInt(document.getElementById('darkS').value)||30,
    inactM:parseInt(document.getElementById('inactM').value)||30,
    tMin:parseFloat(document.getElementById('tMin').value)||10,
    tMax:parseFloat(document.getElementById('tMax').value)||14,
    hMin:parseFloat(document.getElementById('hMin').value)||60,
    hMax:parseFloat(document.getElementById('hMax').value)||80,
    phone:document.getElementById('phone').value,
    notifyEmail:document.getElementById('notifyEmail').checked,
    notifyTelegram:document.getElementById('notifyTelegram').checked
  };
  var tr=i18n[currentLang];
  document.getElementById('msg').textContent=tr.saving;
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
    .then(function(r){return r.json();}).then(function(r){
      if(r.ok){document.getElementById('msg').style.color='#7a7';document.getElementById('msg').textContent=tr.saved;}
      else{document.getElementById('msg').style.color='#a44';document.getElementById('msg').textContent=tr.error_prefix+(r.error||'');}
    }).catch(function(){
      document.getElementById('msg').style.color='#888';
      document.getElementById('msg').textContent=tr.conn_lost;
    });
}
</script>
</body>
</html>
)rawhtml";

// ---- Handlers HTTP ----

static void handleRoot() {
    server.send(200, "text/html; charset=utf-8", HTML_PAGE);
}

static void handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"";
        json += WiFi.SSID(i);
        json += "\",\"rssi\":";
        json += WiFi.RSSI(i);
        json += "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

static void handleLdr() {
    int val  = analogRead(PIN_LDR);
    bool dark = (val < settings.ldrThreshold);
    String json = "{\"value\":";
    json += val;
    json += ",\"dark\":";
    json += dark ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
}

static void handleOtaCheck() {
    if (WiFi.status() != WL_CONNECTED) {
        server.send(200, "application/json", "{\"current\":\"" FIRMWARE_VERSION "\",\"available\":false,\"error\":\"no_wifi\"}");
        return;
    }
    lastOta = checkOtaUpdate();
    StaticJsonDocument<256> doc;
    doc["current"]   = lastOta.currentVersion;
    doc["latest"]    = lastOta.latestVersion;
    doc["available"] = lastOta.updateAvailable;
    if (!lastOta.error.isEmpty()) doc["error"] = lastOta.error;
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handleOtaUpdate() {
    if (!lastOta.updateAvailable || lastOta.downloadUrl.isEmpty()) {
        server.send(200, "application/json", "{\"ok\":false,\"error\":\"No update available\"}");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true}");
    delay(200);
    String fromV = lastOta.currentVersion;
    String toV   = lastOta.latestVersion;
    applyOtaUpdate(lastOta.downloadUrl,
        [fromV, toV]() { showOtaProgress(0, 1, fromV, toV); },
        [fromV, toV](int c, int t) { showOtaProgress(c, t, fromV, toV); });
}

static void handleConfig() {
    StaticJsonDocument<768> doc;
    doc["ssid"]     = settings.wifiSsid;
    doc["nom"]      = settings.userNom;
    doc["prenom"]   = settings.userPrenom;
    doc["email"]    = settings.userEmail;
    doc["devToken"] = settings.deviceToken;
    doc["lang"]     = settings.language;
    doc["version"]  = FIRMWARE_VERSION;
    doc["ldrThr"]   = settings.ldrThreshold;
    doc["darkS"]    = settings.darkTimeoutS;
    doc["inactM"]   = settings.inactivityTimeoutMin;
    doc["tMin"]     = settings.tempMin;
    doc["tMax"]     = settings.tempMax;
    doc["hMin"]     = settings.humMin;
    doc["hMax"]     = settings.humMax;
    doc["phone"]          = settings.phone;
    doc["notifyEmail"]    = settings.notifyEmail;
    doc["notifyTelegram"] = settings.notifyTelegram;
    doc["tgBot"]          = TELEGRAM_BOT_USERNAME;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handleSave() {
    String body = server.arg("plain");
    StaticJsonDocument<768> doc;
    if (deserializeJson(doc, body)) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"JSON invalide\"}");
        return;
    }
    const char* newSsid = doc["ssid"] | "";
    if (strlen(newSsid) > 0) settings.wifiSsid = newSsid;
    const char* newPass = doc["pass"] | "";
    if (strlen(newPass) > 0) settings.wifiPass = newPass;
    settings.userNom              = doc["nom"]      | settings.userNom.c_str();
    settings.userPrenom           = doc["prenom"]   | settings.userPrenom.c_str();
    settings.userEmail            = doc["email"]    | settings.userEmail.c_str();
    settings.deviceToken          = doc["devToken"] | settings.deviceToken.c_str();
    settings.language             = doc["lang"]     | settings.language.c_str();
    settings.ldrThreshold         = doc["ldrThr"]   | settings.ldrThreshold;
    settings.darkTimeoutS         = doc["darkS"]    | settings.darkTimeoutS;
    settings.inactivityTimeoutMin = doc["inactM"]   | settings.inactivityTimeoutMin;
    settings.tempMin              = doc["tMin"]     | settings.tempMin;
    settings.tempMax              = doc["tMax"]     | settings.tempMax;
    settings.humMin               = doc["hMin"]     | settings.humMin;
    settings.humMax               = doc["hMax"]     | settings.humMax;
    settings.phone                = doc["phone"]          | settings.phone.c_str();
    settings.notifyEmail          = doc["notifyEmail"]    | settings.notifyEmail;
    settings.notifyTelegram       = doc["notifyTelegram"] | settings.notifyTelegram;
    saveSettings();
    server.send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

static void handleRequest() {
    if (WiFi.status() != WL_CONNECTED) {
        server.send(200, "application/json", "{\"ok\":false,\"error\":\"no_wifi\"}");
        return;
    }
    String body = server.arg("plain");
    StaticJsonDocument<256> inDoc;
    if (deserializeJson(inDoc, body)) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"JSON invalide\"}");
        return;
    }
    String nom    = inDoc["nom"]    | "";
    String prenom = inDoc["prenom"] | "";
    String email  = inDoc["email"]  | "";
    if (nom.isEmpty() || prenom.isEmpty() || email.isEmpty()) {
        server.send(200, "application/json", "{\"ok\":false,\"error\":\"champs manquants\"}");
        return;
    }
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    String url = String(SUPABASE_URL) + "?action=request";
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY));
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> outDoc;
    outDoc["nom"] = nom; outDoc["prenom"] = prenom; outDoc["email"] = email;
    String outBody; serializeJson(outDoc, outBody);
    int code = http.POST(outBody);
    String response = (code == 200) ? http.getString() : "";
    http.end();
    if (response.isEmpty()) {
        server.send(200, "application/json", "{\"ok\":false,\"error\":\"API injoignable\"}");
        return;
    }
    server.send(200, "application/json", response);
}

static void handleReset() {
    server.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    clearSettings();
    delay(200);
    ESP.restart();
}

static void handleNotFound() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

// ---- API publique ----

void startPortal() {
    WiFi.disconnect(true);
    delay(200);
    if (settings.wifiSsid.length() > 0) {
        // Identifiants connus : dual mode AP+STA pour avoir internet (OTA)
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());
    } else {
        WiFi.mode(WIFI_AP);
    }
    WiFi.softAP(PORTAL_AP_SSID);
    delay(100);
    dns.start(53, "*", WiFi.softAPIP());
    server.on("/",          HTTP_GET,  handleRoot);
    server.on("/scan",      HTTP_GET,  handleScan);
    server.on("/ldr",       HTTP_GET,  handleLdr);
    server.on("/config",    HTTP_GET,  handleConfig);
    server.on("/save",      HTTP_POST, handleSave);
    server.on("/ota",       HTTP_GET,  handleOtaCheck);
    server.on("/ota/update",HTTP_POST, handleOtaUpdate);
    server.on("/reset",     HTTP_POST, handleReset);
    server.on("/request",   HTTP_POST, handleRequest);
    server.onNotFound(handleNotFound);
    server.begin();
    portalActive    = true;
    portalMsgNeeded = true;
    Serial.printf("Portail: SSID=%s  IP=%s\n",
                  PORTAL_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void handlePortal() {
    dns.processNextRequest();
    server.handleClient();
}

bool isPortalActive() {
    return portalActive;
}

bool isPortalMsgNeeded() {
    if (portalMsgNeeded) { portalMsgNeeded = false; return true; }
    return false;
}
