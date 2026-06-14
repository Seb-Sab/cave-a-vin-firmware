#include "lang.h"

const LangStrings LANG_FR = {
    "Cave a vin",           // appName
    "Initialisation...",    // initializing
    "Pret.",                // ready
    "Posez une bouteille",  // placeBottle
    "== Vin scanne ==",     // wineScanned
    "Qte: ",                // qty
    "Recherche...",         // searching
    "Non enregistre",       // notRegistered
    "+/- pour modifier",    // plusMinusModify
    "+ long = enregistrer", // longPlusRegister
    "== ENREGISTREMENT ==", // enrollTitle
    "Posez une etiquette",  // placeLabel
    "- long = annuler",     // longMinusCancel
    "Envoi...",             // sending
    "Enregistre !",         // registered
    "Completez le profil",  // completeOnDash
    "Deja present:",        // alreadyPresent
    "Annule",               // cancelled
    "+1 enregistre",        // added
    "-1 enregistre",        // removed
    "Connexion WiFi...",    // connectingWifi
    "WiFi OK",              // wifiOk
    "WiFi echoue",          // wifiFailed
    "mode hors ligne",      // offlineMode
    "Erreur reseau",        // networkError
    "Erreur API",           // apiError
    "Ajout...",             // adding
    "Retrait...",           // removing
    "- Annuler",            // cancelMinus
    "+ Ajouter",            // plusToAdd
    "- Ignorer",            // minusToIgnore
    "ALERTE: ",             // alert
    "Btn pour acquitter",   // pressBtnAck
    "Mesure cave...",       // caveMeasure
    "Stabilisation BME:",   // bmeStab
    "Veille...",            // sleeping
    "Mode config",          // configMode
    "Token invalide !",     // tokenError
    "Verification token...", // tokenChecking
    "Mise a jour...",       // otaUpdating
    "Verif. mise a jour",  // otaChecking
    "MAJ disponible !",    // otaAvailable
    "+ Mettre a jour",     // otaUpdate
    "+ Associer",          // linkAssociate
    "Associe !",           // linkDone
    "Aucun vin sans UID",  // linkNone
    "Assoc. etiquette",    // linkTitle
    "+/- defiler",         // linkNavHelp
    "+l=OK  -l=sortir"     // linkLongHelp
};

const LangStrings LANG_EN = {
    "Clever Cellar",        // appName
    "Initializing...",      // initializing
    "Ready.",               // ready
    "Place a bottle",       // placeBottle
    "== Wine scanned ==",   // wineScanned
    "Qty: ",                // qty
    "Searching...",         // searching
    "Not registered",       // notRegistered
    "+/- to update",        // plusMinusModify
    "+ hold = register",    // longPlusRegister
    "== ENROLLMENT ==",     // enrollTitle
    "Place a label",        // placeLabel
    "- hold = cancel",      // longMinusCancel
    "Sending...",           // sending
    "Registered!",          // registered
    "Complete profile",     // completeOnDash
    "Already exists:",      // alreadyPresent
    "Cancelled",            // cancelled
    "+1 saved",             // added
    "-1 saved",             // removed
    "Connecting WiFi...",   // connectingWifi
    "WiFi OK",              // wifiOk
    "WiFi failed",          // wifiFailed
    "offline mode",         // offlineMode
    "Network error",        // networkError
    "API error",            // apiError
    "Adding...",            // adding
    "Removing...",          // removing
    "- Cancel",             // cancelMinus
    "+ Add",                // plusToAdd
    "- Ignore",             // minusToIgnore
    "ALERT: ",              // alert
    "Press btn to ack",     // pressBtnAck
    "Cave measure...",      // caveMeasure
    "BME stabilizing:",     // bmeStab
    "Sleeping...",          // sleeping
    "Config mode",          // configMode
    "Invalid token!",       // tokenError
    "Checking token...",    // tokenChecking
    "Updating...",          // otaUpdating
    "Checking update...",  // otaChecking
    "Update available!",   // otaAvailable
    "+ Update now",        // otaUpdate
    "+ Link",              // linkAssociate
    "Linked!",             // linkDone
    "No wine w/o UID",     // linkNone
    "Link label",          // linkTitle
    "+/- browse",          // linkNavHelp
    "+l=OK  -l=quit"       // linkLongHelp
};

const LangStrings LANG_IT = {
    "Clever Cellar",        // appName
    "Avvio...",             // initializing
    "Pronto.",              // ready
    "Metti una bottiglia",  // placeBottle
    "== Vino letto ==",     // wineScanned
    "Qta: ",                // qty
    "Ricerca...",           // searching
    "Non registrato",       // notRegistered
    "+/- per modif.",       // plusMinusModify
    "+ tieni = aggiungi",   // longPlusRegister
    "== REGISTRAZIONE ==",  // enrollTitle
    "Metti un'etichetta",   // placeLabel
    "- tieni = annulla",    // longMinusCancel
    "Invio...",             // sending
    "Registrato!",          // registered
    "Completa il profilo",  // completeOnDash
    "Gia' presente:",       // alreadyPresent
    "Annullato",            // cancelled
    "+1 salvato",           // added
    "-1 salvato",           // removed
    "Connessione WiFi...",  // connectingWifi
    "WiFi OK",              // wifiOk
    "WiFi fallito",         // wifiFailed
    "modalita' offline",    // offlineMode
    "Errore di rete",       // networkError
    "Errore API",           // apiError
    "Aggiunta...",          // adding
    "Rimozione...",         // removing
    "- Annulla",            // cancelMinus
    "+ Aggiungi",           // plusToAdd
    "- Ignora",             // minusToIgnore
    "AVVISO: ",             // alert
    "Premi tasto conf.",    // pressBtnAck
    "Misura cantina...",    // caveMeasure
    "Stab. BME:",           // bmeStab
    "In standby...",        // sleeping
    "Modalita' config",     // configMode
    "Token non valido!",    // tokenError
    "Verifica token...",    // tokenChecking
    "Aggiornamento...",     // otaUpdating
    "Verifica aggiorn.",   // otaChecking
    "Aggiorn. disponib!",  // otaAvailable
    "+ Aggiorna ora",      // otaUpdate
    "+ Associa",           // linkAssociate
    "Associato!",          // linkDone
    "Nessun vino lib.",    // linkNone
    "Assoc. etichetta",    // linkTitle
    "+/- sfoglia",         // linkNavHelp
    "+l=OK  -l=esci"       // linkLongHelp
};

const LangStrings* T = &LANG_FR;

void setLang(const String& code) {
    if      (code == "en") T = &LANG_EN;
    else if (code == "it") T = &LANG_IT;
    else                   T = &LANG_FR;
}
