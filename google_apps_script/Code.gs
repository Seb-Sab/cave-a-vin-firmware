/**
 * Cave a vin - API Google Apps Script
 * Deployez ce script comme "Web app" (Execute as: Me, Who has access: Anyone).
 * Copiez l'URL obtenue dans le code ESP32 (variable GSCRIPT_URL).
 *
 * Onglets attendus :
 *   - "Inventaire"  (UID, Nom, Domaine, Millesime, Region, Couleur, Quantite, Apogee, DateAchat, Commentaires)
 *   - "Conditions"  (Horodatage, Temperature, Humidite, Pression)
 *   - "Log"         (Horodatage, UID, Action, Avant, Apres)
 *
 * Actions HTTP supportees :
 *   ?action=lookup&uid=...       -> recherche un vin par UID
 *   ?action=register&uid=...     -> enregistre un nouvel UID (sans doublon)
 *   ?action=increment&uid=...    -> +1 bouteille
 *   ?action=decrement&uid=...    -> -1 bouteille
 *   ?action=env&t=..&h=..&p=..   -> log conditions cave
 */

// ============== CONFIGURATION ==============
const SHARED_TOKEN = "changez_moi_123";

// Seuils d'alerte cave
const TEMP_MIN = 10;
const TEMP_MAX = 16;
const HUM_MIN  = 60;
const HUM_MAX  = 80;
// ===========================================

function doGet(e)  { return handleRequest(e); }
function doPost(e) { return handleRequest(e); }

function handleRequest(e) {
  try {
    const params = (e && e.parameter) ? e.parameter : {};
    if (SHARED_TOKEN && params.token !== SHARED_TOKEN) {
      return jsonOut({ok: false, error: "token invalide"});
    }

    const action = params.action || "";
    switch (action) {
      case "lookup":    return jsonOut(lookup(params.uid));
      case "register":  return jsonOut(registerUid(params.uid));
      case "increment": return jsonOut(updateQty(params.uid, +1));
      case "decrement": return jsonOut(updateQty(params.uid, -1));
      case "env":       return jsonOut(logEnv(params.t, params.h, params.p));
      default:          return jsonOut({ok: false, error: "action inconnue"});
    }
  } catch (err) {
    return jsonOut({ok: false, error: String(err)});
  }
}

function jsonOut(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}

// ---------- INVENTAIRE ----------

function getInvSheet() {
  return SpreadsheetApp.getActive().getSheetByName("Inventaire");
}

function findRowByUid(uid) {
  const sh = getInvSheet();
  const last = sh.getLastRow();
  if (last < 2) return -1;
  const data = sh.getRange(2, 1, last - 1, 1).getValues();
  const target = String(uid).trim().toUpperCase();
  for (let i = 0; i < data.length; i++) {
    if (String(data[i][0]).trim().toUpperCase() === target) {
      return i + 2;
    }
  }
  return -1;
}

function lookup(uid) {
  if (!uid) return {ok: false, error: "uid manquant"};
  const sh = getInvSheet();
  const row = findRowByUid(uid);
  if (row < 0) {
    return {ok: true, found: false, uid: uid, nom: "Inconnu", qte: 0};
  }
  const r = sh.getRange(row, 1, 1, 10).getValues()[0];
  return {
    ok: true, found: true,
    uid: r[0], nom: r[1], domaine: r[2], millesime: r[3], region: r[4],
    couleur: r[5], qte: Number(r[6]) || 0, apogee: r[7], achat: r[8], note: r[9]
  };
}

/**
 * Enregistre une nouvelle etiquette RFID.
 * Si l'UID existe deja, ne cree pas de doublon et renvoie created=false.
 * Sinon cree une ligne avec nom "(a renseigner)", quantite 0, date du jour.
 */
function registerUid(uid) {
  if (!uid) return {ok: false, error: "uid manquant"};
  const sh = getInvSheet();
  const row = findRowByUid(uid);

  if (row > 0) {
    const nom = sh.getRange(row, 2).getValue();
    return {ok: true, created: false, uid: uid, nom: nom};
  }

  sh.appendRow([
    uid,              // A : UID
    "(a renseigner)", // B : Nom
    "",               // C : Domaine
    "",               // D : Millesime
    "",               // E : Region
    "",               // F : Couleur
    0,                // G : Quantite
    "",               // H : Apogee
    new Date(),       // I : DateAchat
    ""                // J : Commentaires
  ]);

  // Journal
  SpreadsheetApp.getActive().getSheetByName("Log").appendRow([
    new Date(), uid, "enregistrement", 0, 0
  ]);

  return {ok: true, created: true, uid: uid, nom: "(a renseigner)"};
}

function updateQty(uid, delta) {
  if (!uid) return {ok: false, error: "uid manquant"};
  const sh = getInvSheet();
  let row = findRowByUid(uid);

  // En mode normal : si l'UID n'existe pas, on refuse plutot que de creer
  // une ligne non enrolee (le mode "register" est fait pour ca).
  if (row < 0) {
    return {ok: false, error: "uid non enregistre"};
  }

  const avant = Number(sh.getRange(row, 7).getValue()) || 0;
  const apres = Math.max(0, avant + delta);
  sh.getRange(row, 7).setValue(apres);

  SpreadsheetApp.getActive().getSheetByName("Log").appendRow([
    new Date(), uid, (delta > 0 ? "ajout" : "retrait"), avant, apres
  ]);

  const nom = sh.getRange(row, 2).getValue();
  return {ok: true, uid: uid, nom: nom, qte: apres, avant: avant};
}

// ---------- CONDITIONS DE CAVE ----------

function logEnv(t, h, p) {
  const sh = SpreadsheetApp.getActive().getSheetByName("Conditions");
  const temp = parseFloat(t);
  const hum  = parseFloat(h);
  const pres = parseFloat(p);
  sh.appendRow([new Date(), temp, hum, pres]);

  const alerts = [];
  if (!isNaN(temp) && (temp < TEMP_MIN || temp > TEMP_MAX)) alerts.push("temperature");
  if (!isNaN(hum)  && (hum  < HUM_MIN  || hum  > HUM_MAX))  alerts.push("humidite");

  return {ok: true, alerts: alerts};
}
