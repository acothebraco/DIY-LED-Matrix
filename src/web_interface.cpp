#include "web_interface.h"

#include <WebServer.h>
#include <WiFi.h>
#include "config.h"
#include "app_state.h"
#include "settings.h"
#include "matrix_display.h"
#include "wifi_manager.h"
#include "ota_update.h"

static WebServer server(80);

static String htmlEscape(const String &input) {
  String output = input;
  output.replace("&", "&amp;");
  output.replace("\"", "&quot;");
  output.replace("'", "&#39;");
  output.replace("<", "&lt;");
  output.replace(">", "&gt;");
  return output;
}

static String urlEncode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String output;

  for (size_t i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input[i];

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~') {
      output += (char)c;
    } else {
      output += '%';
      output += hex[(c >> 4) & 0x0F];
      output += hex[c & 0x0F];
    }
  }

  return output;
}

static String getWifiSecurityName(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN: return "Offen";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    default: return "Gesch&uuml;tzt";
  }
}

static String getWifiSignalName(int32_t rssi) {
  if (rssi >= -55) return "Sehr gut";
  if (rssi >= -67) return "Gut";
  if (rssi >= -75) return "OK";
  return "Schwach";
}

static String htmlButton(const String &label, const String &url) {
  return "<a class='btn' href='" + url + "'>" + label + "</a>";
}

static void redirectHome() {
  server.sendHeader("Location", "/");
  server.send(303);
}

static String htmlPage() {
  String autoStatus = autoModeDemo ? "AKTIV" : "AUS";
  String wifiSsidValue = homeWifiSsid;
  bool wifiSsidFromScan = false;

  if (server.hasArg("ssid")) {
    wifiSsidValue = server.arg("ssid");
    wifiSsidValue.trim();
    wifiSsidFromScan = wifiSsidValue.length() > 0;
  }

  String page;
  page += "<!DOCTYPE html><html lang='de'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<meta name='theme-color' content='#050812'>";
  page += "<title>SmartFix Matrix</title>";
  page += R"rawliteral(
<style>
:root{--bg:#050812;--panel:#0d1320;--panel2:#111827;--line:#223149;--text:#e5e7eb;--muted:#94a3b8;--green:#22c55e;--green2:#16a34a;--blue:#38bdf8;--blue2:#2563eb;--danger:#ef4444;}
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at 18% 0%,rgba(34,197,94,.20),transparent 34%),radial-gradient(circle at 88% 10%,rgba(56,189,248,.16),transparent 32%),linear-gradient(180deg,#050812 0%,#08111f 55%,#050812 100%);color:var(--text);min-height:100vh;}
a{color:inherit}.wrap{max-width:940px;margin:0 auto;padding:22px}.hero{position:relative;overflow:hidden;border:1px solid rgba(148,163,184,.22);border-radius:26px;background:linear-gradient(145deg,rgba(17,24,39,.93),rgba(2,6,23,.92));box-shadow:0 22px 70px rgba(0,0,0,.45);padding:22px;margin-bottom:16px}.hero:before{content:'';position:absolute;inset:0;background:linear-gradient(90deg,rgba(34,197,94,.10),transparent 40%,rgba(56,189,248,.10));pointer-events:none}.topbar{position:relative;display:flex;align-items:center;gap:15px;margin-bottom:18px}.logo-badge{width:52px;height:52px;border-radius:16px;display:grid;place-items:center;font-weight:900;font-size:20px;color:white;background:linear-gradient(135deg,var(--green),var(--blue));box-shadow:0 0 32px rgba(34,197,94,.28)}.title-block h1{margin:0;font-size:31px;letter-spacing:-.8px}.title-block .sub{margin:5px 0 0;color:var(--muted)}.version-chip{margin-left:auto;background:rgba(34,197,94,.12);border:1px solid rgba(34,197,94,.36);color:#bbf7d0;padding:8px 12px;border-radius:999px;font-size:13px;font-weight:bold;white-space:nowrap}.status{position:relative;display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.pill{background:rgba(2,6,23,.72);border:1px solid rgba(148,163,184,.18);border-radius:16px;padding:12px;box-shadow:inset 0 1px 0 rgba(255,255,255,.03)}.pill.good{border-color:rgba(34,197,94,.32)}.pill.blue{border-color:rgba(56,189,248,.28)}.label{font-size:11px;letter-spacing:.5px;text-transform:uppercase;color:var(--muted)}.value{font-size:16px;font-weight:bold;color:#f8fafc;margin-top:5px;word-break:break-word}.card{background:linear-gradient(145deg,rgba(17,24,39,.92),rgba(12,18,30,.92));border:1px solid rgba(148,163,184,.18);border-radius:22px;margin-bottom:12px;box-shadow:0 12px 36px rgba(0,0,0,.28)}details.card{overflow:hidden}summary{cursor:pointer;list-style:none;padding:18px 20px;display:flex;align-items:center;gap:12px;user-select:none}summary::-webkit-details-marker{display:none}.section-icon{width:34px;height:34px;border-radius:12px;display:grid;place-items:center;background:rgba(56,189,248,.12);border:1px solid rgba(56,189,248,.25);color:#7dd3fc;font-size:18px}.summary-text{font-size:18px;font-weight:800;color:#f8fafc}.sumvalue{margin-left:auto;color:var(--muted);font-size:13px;font-weight:bold;text-align:right}.chev{margin-left:8px;color:var(--green);font-size:22px;line-height:1;transition:.18s transform}details[open] .chev{transform:rotate(45deg)}.detail-body{padding:0 20px 20px;border-top:1px solid rgba(148,163,184,.11)}h2{margin:18px 0 12px;font-size:15px;color:#7dd3fc;text-transform:uppercase;letter-spacing:.5px}.sub{color:var(--muted);font-size:14px;line-height:1.45}.hint{margin-top:12px;padding:12px;border:1px solid rgba(34,197,94,.24);background:rgba(34,197,94,.08);border-radius:14px;color:#bbf7d0}.buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}.btn{display:flex;align-items:center;justify-content:center;min-height:46px;text-align:center;text-decoration:none;border:1px solid rgba(148,163,184,.18);background:linear-gradient(135deg,rgba(37,99,235,.95),rgba(14,165,233,.82));color:white;padding:12px;border-radius:14px;font-weight:800;box-shadow:0 10px 22px rgba(37,99,235,.17);transition:.16s transform,.16s filter,.16s border-color}.btn:hover{filter:brightness(1.12);transform:translateY(-1px);border-color:rgba(125,211,252,.45)}.btn.green{background:linear-gradient(135deg,var(--green2),var(--green));box-shadow:0 10px 22px rgba(34,197,94,.17)}.btn.dark{background:rgba(15,23,42,.92)}.btn.danger{background:linear-gradient(135deg,#b91c1c,var(--danger))}button.btn{border:0;width:100%;cursor:pointer;font-size:15px}input{width:100%;background:rgba(2,6,23,.78);color:var(--text);border:1px solid rgba(148,163,184,.22);border-radius:15px;padding:14px 15px;font-size:16px;margin-bottom:12px;outline:none}input:focus{border-color:rgba(56,189,248,.65);box-shadow:0 0 0 3px rgba(56,189,248,.12)}.small{font-size:13px;color:var(--muted);margin:18px 0 4px;text-align:center}.row-title{display:flex;align-items:center;justify-content:space-between;margin:18px 0 10px}.mini-chip{font-size:12px;color:#bbf7d0;background:rgba(34,197,94,.10);border:1px solid rgba(34,197,94,.25);border-radius:999px;padding:5px 9px}@media(max-width:760px){.wrap{padding:14px}.hero{padding:18px;border-radius:22px}.topbar{align-items:flex-start}.version-chip{display:none}.status{grid-template-columns:1fr 1fr}.buttons{grid-template-columns:1fr}.sumvalue{display:none}.title-block h1{font-size:27px}}@media(max-width:460px){.status{grid-template-columns:1fr}.logo-badge{width:46px;height:46px}.summary-text{font-size:16px}}
</style>
<script>
(function(){
  var key='sf_open_sections_v2';
  function ids(){var a=[];document.querySelectorAll('details.config-section').forEach(function(d){if(d.open&&d.id)a.push(d.id);});return a;}
  function save(){try{localStorage.setItem(key,JSON.stringify(ids()));sessionStorage.setItem('sf_scroll',String(window.scrollY||0));}catch(e){}}
  function openByHash(){var h=location.hash?location.hash.substring(1):'';if(!h)return false;var el=document.getElementById(h);if(el&&el.tagName&&el.tagName.toLowerCase()==='details'){el.open=true;setTimeout(function(){el.scrollIntoView({block:'start'});},80);return true;}return false;}
  window.addEventListener('load',function(){
    var restored=false;
    try{var raw=localStorage.getItem(key);if(raw){var arr=JSON.parse(raw);document.querySelectorAll('details.config-section').forEach(function(d){d.open=arr.indexOf(d.id)>=0;});restored=true;}}catch(e){}
    if(!restored){var d=document.getElementById('section-mode');if(d)d.open=true;}
    var hashOpened=openByHash();
    document.querySelectorAll('details.config-section').forEach(function(d){d.addEventListener('toggle',save);});
    document.querySelectorAll('a.btn,button.btn,form').forEach(function(el){el.addEventListener(el.tagName.toLowerCase()==='form'?'submit':'click',save);});
    if(!hashOpened){var y=sessionStorage.getItem('sf_scroll');if(y!==null){setTimeout(function(){window.scrollTo(0,parseInt(y)||0);sessionStorage.removeItem('sf_scroll');},40);}}
  });
  window.addEventListener('beforeunload',save);
})();
</script>
)rawliteral";
  page += "</head><body><div class='wrap'>";

  page += "<div class='hero'>";
  page += "<div class='topbar'><div class='logo-badge'>SF</div><div class='title-block'>";
  page += "<h1>SmartFix Matrix</h1>";
  page += "<div class='sub'>SmartFix Elektronikservice &bull; ESP32-S3 HUB75 Matrix Sign</div>";
  page += "</div><div class='version-chip'>Firmware v";
  page += FIRMWARE_VERSION;
  page += "</div></div>";
  page += "<div class='status'>";

  page += "<div class='pill good'><div class='label'>Firmware</div><div class='value'>v";
  page += FIRMWARE_VERSION;
  page += "</div></div>";

  page += "<div class='pill blue'><div class='label'>Mode</div><div class='value'>";
  page += getModeName(currentMode);
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Auto Demo</div><div class='value'>";
  page += autoStatus;
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Brightness</div><div class='value'>";
  page += String(matrixBrightness);
  page += " / 255</div></div>";

  page += "<div class='pill'><div class='label'>Scroll</div><div class='value'>";
  page += getSpeedName();
  page += " / ";
  page += getScrollTextEffectName();
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Logo</div><div class='value'>";
  page += getLogoEffectName();
  page += " / ";
  page += getLogoColorName();
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Textfarbe</div><div class='value'>";
  page += getScrollTextColorName();
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>Home WiFi</div><div class='value'>";
  page += getWiFiStatusText();
  page += "<br>";
  page += htmlEscape(getStaIpText());
  page += "</div></div>";

  page += "<div class='pill'><div class='label'>mDNS Adresse</div><div class='value'>";
  page += htmlEscape(getMdnsAddressText());
  page += "</div></div>";

  page += "</div></div>";

  page += "<details class='card config-section' id='section-mode'>";
  page += "<summary><span class='section-icon'>&#9881;</span><span class='summary-text'>Modus ausw&auml;hlen</span><span class='sumvalue'>";
  page += getModeName(currentMode);
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='buttons'>";
  page += htmlButton("Laufschrift", "/mode?m=0");
  page += htmlButton("Static Logo", "/mode?m=1");
  page += htmlButton("Pixel Art", "/mode?m=2");
  page += htmlButton("Random FX", "/mode?m=3");
  page += "</div></div></details>";

  page += "<details class='card config-section' id='section-scroll'>";
  page += "<summary><span class='section-icon'>&#9998;</span><span class='summary-text'>Laufschrift</span><span class='sumvalue'>";
  page += getScrollTextEffectName();
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='row-title'><h2>Text</h2><span class='mini-chip'>max. 160 Zeichen</span></div>";
  page += "<form action='/set-text' method='GET'>";
  page += "<input name='t' maxlength='160' value='" + htmlEscape(scrollText) + "'>";
  page += "<button class='btn green' type='submit'>Text speichern</button>";
  page += "</form>";
  page += "<h2>Geschwindigkeit</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Langsam", "/speed?v=70");
  page += htmlButton("Mittel", "/speed?v=35");
  page += htmlButton("Schnell", "/speed?v=18");
  page += htmlButton("Turbo", "/speed?v=8");
  page += "</div>";
  page += "<h2>Farbe</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Weiss", "/text-color?c=0");
  page += htmlButton("Gruen", "/text-color?c=1");
  page += htmlButton("Blau", "/text-color?c=2");
  page += htmlButton("Gelb", "/text-color?c=3");
  page += htmlButton("Rot", "/text-color?c=4");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<h2>Text Effekt</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Normal", "/scroll-effect?e=0");
  page += htmlButton("Rainbow", "/scroll-effect?e=1");
  page += htmlButton("Wave", "/scroll-effect?e=2");
  page += htmlButton("Sparkle", "/scroll-effect?e=3");
  page += htmlButton("Comet Trail", "/scroll-effect?e=4");
  page += htmlButton("Flash", "/scroll-effect?e=5");
  page += "</div>";
  page += "<div class='hint'>Effekte betreffen nur die laufende Textzeile unten.</div>";
  page += "</div></details>";

  page += "<details class='card config-section' id='section-logo'>";
  page += "<summary><span class='section-icon'>SF</span><span class='summary-text'>Logo / Header</span><span class='sumvalue'>";
  page += getLogoEffectName();
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='row-title'><h2>Logo Text</h2><span class='mini-chip'>max. 32 Zeichen</span></div>";
  page += "<form action='/set-logo-text' method='GET'>";
  page += "<input name='t' maxlength='32' value='" + htmlEscape(logoText) + "'>";
  page += "<button class='btn green' type='submit'>Logo Text speichern</button>";
  page += "</form>";
  page += "<div class='hint'>Der Logo-Text kann einfarbig, zweifarbig oder wortweise mehrfarbig dargestellt werden.</div>";
  page += "<h2>Logo Effekt</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Statisch", "/logo-effect?e=0");
  page += htmlButton("Buchstabe", "/logo-effect?e=1");
  page += htmlButton("Fade", "/logo-effect?e=2");
  page += htmlButton("Slide", "/logo-effect?e=3");
  page += htmlButton("Shimmer", "/logo-effect?e=4");
  page += htmlButton("Sparkle", "/logo-effect?e=5");
  page += htmlButton("Pulse", "/logo-effect?e=6");
  page += htmlButton("Wave", "/logo-effect?e=7");
  page += htmlButton("Bounce", "/logo-effect?e=8");
  page += htmlButton("Glitch", "/logo-effect?e=9");
  page += htmlButton("Scanline", "/logo-effect?e=10");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<h2>Logo Farbe</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("Auto / Brand", "/logo-color?c=0");
  page += htmlButton("Gruen", "/logo-color?c=1");
  page += htmlButton("Blau", "/logo-color?c=2");
  page += htmlButton("Weiss", "/logo-color?c=3");
  page += htmlButton("Gelb", "/logo-color?c=4");
  page += htmlButton("Rot", "/logo-color?c=5");
  page += htmlButton("2-farbig nach Wort", "/logo-color?c=6");
  page += htmlButton("Mehrfarbig nach Wort", "/logo-color?c=7");
  page += "</div>";
  page += "</div></details>";

  page += "<details class='card config-section' id='section-demo'>";
  page += "<summary><span class='section-icon'>&#9728;</span><span class='summary-text'>Auto Demo &amp; Helligkeit</span><span class='sumvalue'>";
  page += autoStatus;
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<h2>Auto Demo</h2><div class='buttons'>";
  page += "<a class='btn green' href='/auto'>Auto Demo starten</a>";
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<h2>Helligkeit</h2>";
  page += "<div class='buttons'>";
  page += htmlButton("25%", "/brightness?v=40");
  page += htmlButton("50%", "/brightness?v=80");
  page += htmlButton("75%", "/brightness?v=130");
  page += htmlButton("100%", "/brightness?v=200");
  page += "</div></div></details>";

  page += "<details class='card config-section' id='wifi'>";
  page += "<summary><span class='section-icon'>&#128246;</span><span class='summary-text'>Heim WLAN</span><span class='sumvalue'>";
  page += getWiFiStatusText();
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<form action='/wifi-save' method='POST'>";
  page += "<input name='ssid' maxlength='64' placeholder='SSID' value='" + htmlEscape(wifiSsidValue) + "'>";
  page += "<input name='pass' maxlength='64' placeholder='WLAN Passwort' type='password' value='" + htmlEscape(homeWifiPassword) + "'>";
  page += "<button class='btn green' type='submit'>Mit Heim WLAN verbinden</button>";
  page += "</form>";
  page += "<div class='buttons' style='margin-top:10px;'>";
  page += htmlButton("WLAN scannen", "/wifi-scan");
  page += htmlButton("Heim WLAN l&ouml;schen", "/wifi-forget");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  if (wifiSsidFromScan) {
    page += "<div class='hint'>SSID aus Scan &uuml;bernommen. Bitte WLAN Passwort eingeben und speichern.</div>";
  }
  page += "<div class='sub' style='margin-top:12px;'>Der SmartFix-Matrix Access Point bleibt zus&auml;tzlich aktiv.</div>";
  page += "</div></details>";

  page += "<details class='card config-section' id='section-ota'>";
  page += "<summary><span class='section-icon'>&#9889;</span><span class='summary-text'>GitHub OTA Firmware</span><span class='sumvalue'>";
  page += htmlEscape(lastOtaStatus);
  page += "</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<form action='/ota-save' method='POST'>";
  page += "<input name='url' maxlength='220' value='" + htmlEscape(otaUrl) + "'>";
  page += "<button class='btn green' type='submit'>OTA URL speichern</button>";
  page += "</form>";
  page += "<div class='buttons' style='margin-top:10px;'>";
  page += htmlButton("OTA Update starten", "/ota-start");
  page += htmlButton("Refresh", "/");
  page += "</div>";
  page += "<div class='hint'>Status: ";
  page += htmlEscape(lastOtaStatus);
  page += "</div></div></details>";

  page += "<details class='card config-section' id='section-system'>";
  page += "<summary><span class='section-icon'>&#128295;</span><span class='summary-text'>System</span><span class='sumvalue'>Reset</span><span class='chev'>+</span></summary><div class='detail-body'>";
  page += "<div class='buttons'>";
  page += "<a class='btn danger' href='/factory-reset'>Werkseinstellungen</a>";
  page += htmlButton("Refresh", "/");
  page += "</div></div></details>";

  page += "<div class='small'>SmartFix Elektronikservice &bull; Designed for 64x32 HUB75 RGB Matrix</div>";
  page += "</div></body></html>";

  return page;
}

static void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

static void handleModeChange() {
  if (server.hasArg("m")) {
    int mode = server.arg("m").toInt();

    if (mode >= MODE_SCROLL_TEXT && mode <= MODE_RANDOM_FX) {
      autoModeDemo = false;
      setMode((DisplayMode)mode, true);
    }
  }

  redirectHome();
}

static void handleAutoDemo() {
  autoModeDemo = true;
  lastModeChange = millis();
  saveModeSettings();
  Serial.println("Auto mode demo enabled from web");
  redirectHome();
}

static void handleBrightness() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 255) value = 255;

    matrixBrightness = value;
    display->setBrightness8(matrixBrightness);

    saveBrightnessSetting();

    Serial.print("Brightness changed to: ");
    Serial.println(matrixBrightness);
  }

  redirectHome();
}

static void handleTextColor() {
  if (server.hasArg("c")) {
    int value = server.arg("c").toInt();

    if (value < 0) value = 0;
    if (value > 4) value = 4;

    scrollTextColorMode = (uint8_t)value;
    saveTextColorSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Text color changed to: ");
    Serial.println(getScrollTextColorName());
  }

  redirectHome();
}

static void handleScrollEffect() {
  if (server.hasArg("e")) {
    int value = server.arg("e").toInt();

    if (value < SCROLL_EFFECT_NORMAL) value = SCROLL_EFFECT_NORMAL;
    if (value > SCROLL_EFFECT_FLASH) value = SCROLL_EFFECT_FLASH;

    scrollTextEffectMode = (uint8_t)value;
    saveScrollEffectSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Scroll effect changed to: ");
    Serial.println(getScrollTextEffectName());
  }

  redirectHome();
}

static void handleLogoEffect() {
  if (server.hasArg("e")) {
    int value = server.arg("e").toInt();

    if (value < LOGO_EFFECT_STATIC) value = LOGO_EFFECT_STATIC;
    if (value > LOGO_EFFECT_SCANLINE) value = LOGO_EFFECT_SCANLINE;

    logoEffectMode = (uint8_t)value;
    saveLogoEffectSetting();

    clearDisplay();

    Serial.print("Logo effect changed to: ");
    Serial.println(getLogoEffectName());
  }

  redirectHome();
}

static void handleLogoColor() {
  if (server.hasArg("c")) {
    int value = server.arg("c").toInt();

    if (value < LOGO_COLOR_BRAND) value = LOGO_COLOR_BRAND;
    if (value > LOGO_COLOR_RAINBOW) value = LOGO_COLOR_RAINBOW;

    logoColorMode = (uint8_t)value;
    saveLogoColorSetting();

    clearDisplay();

    Serial.print("Logo color changed to: ");
    Serial.println(getLogoColorName());
  }

  redirectHome();
}

static void handleSpeed() {
  if (server.hasArg("v")) {
    int value = server.arg("v").toInt();

    if (value < 5) value = 5;
    if (value > 200) value = 200;

    scrollInterval = (uint16_t)value;
    saveSpeedSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("Scroll speed changed to: ");
    Serial.print(scrollInterval);
    Serial.println(" ms");
  }

  redirectHome();
}

static void handleSetText() {
  if (server.hasArg("t")) {
    String newText = server.arg("t");

    newText.trim();

    if (newText.length() == 0) {
      newText = "SMARTFIX ELEKTRONIKSERVICE";
    }

    if (newText.length() > MAX_SCROLL_TEXT_LEN) {
      newText = newText.substring(0, MAX_SCROLL_TEXT_LEN);
    }

    scrollText = newText + "   ";

    saveScrollTextSetting();

    autoModeDemo = false;
    setMode(MODE_SCROLL_TEXT, true);

    Serial.print("New scroll text from web: ");
    Serial.println(scrollText);
  }

  redirectHome();
}

static void handleSetLogoText() {
  if (server.hasArg("t")) {
    String newLogoText = server.arg("t");

    newLogoText.trim();

    if (newLogoText.length() == 0) {
      newLogoText = "SmartFix";
    }

    if (newLogoText.length() > MAX_LOGO_TEXT_LEN) {
      newLogoText = newLogoText.substring(0, MAX_LOGO_TEXT_LEN);
    }

    logoText = newLogoText;

    saveLogoTextSetting();
    clearDisplay();

    Serial.print("New logo text from web: ");
    Serial.println(logoText);
  }

  redirectHome();
}

static String wifiScanPage() {
  String page;
  page += "<!DOCTYPE html><html lang='de'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<meta name='theme-color' content='#050812'>";
  page += "<title>SmartFix Matrix WLAN Scan</title>";
  page += R"rawliteral(
<style>
:root{--bg:#050812;--panel:#0d1320;--line:#223149;--text:#e5e7eb;--muted:#94a3b8;--green:#22c55e;--blue:#38bdf8}*{box-sizing:border-box}body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at 18% 0%,rgba(34,197,94,.20),transparent 34%),radial-gradient(circle at 88% 10%,rgba(56,189,248,.16),transparent 32%),linear-gradient(180deg,#050812 0%,#08111f 55%,#050812 100%);color:var(--text);min-height:100vh}.wrap{max-width:940px;margin:0 auto;padding:22px}.card{background:linear-gradient(145deg,rgba(17,24,39,.93),rgba(2,6,23,.92));border:1px solid rgba(148,163,184,.22);border-radius:26px;padding:22px;margin-bottom:16px;box-shadow:0 22px 70px rgba(0,0,0,.45)}.topbar{display:flex;align-items:center;gap:15px;margin-bottom:18px}.logo-badge{width:52px;height:52px;border-radius:16px;display:grid;place-items:center;font-weight:900;font-size:20px;color:white;background:linear-gradient(135deg,var(--green),var(--blue));box-shadow:0 0 32px rgba(34,197,94,.28)}h1{margin:0;font-size:31px;letter-spacing:-.8px}h2{margin:0 0 14px;font-size:18px;color:#7dd3fc}.sub{color:var(--muted);line-height:1.45;margin-bottom:18px}.net{display:grid;grid-template-columns:1.4fr .7fr .8fr .8fr;gap:10px;align-items:center;background:rgba(2,6,23,.72);border:1px solid rgba(148,163,184,.18);border-radius:16px;padding:12px;margin-bottom:10px}.ssid{font-weight:bold;color:#f8fafc;word-break:break-word}.meta{font-size:13px;color:var(--muted)}.btn{display:flex;align-items:center;justify-content:center;text-align:center;text-decoration:none;min-height:42px;background:linear-gradient(135deg,#2563eb,#0ea5e9);color:white;padding:10px 12px;border-radius:13px;font-weight:800}.btn.green{background:linear-gradient(135deg,#16a34a,#22c55e)}.btn:hover{filter:brightness(1.12)}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:16px}@media(max-width:650px){.net{grid-template-columns:1fr}.actions{grid-template-columns:1fr}.wrap{padding:14px}}
</style>
)rawliteral";
  page += "</head><body><div class='wrap'><div class='card'>";
  page += "<div class='topbar'><div class='logo-badge'>SF</div><div><h1>SmartFix Matrix</h1><div class='sub' style='margin:5px 0 0;'>WLAN Scan</div></div></div>";
  page += "<h2>Gefundene WLAN-Netzwerke</h2>";
  page += "<div class='sub'>W&auml;hle eine SSID aus, gib danach das Passwort ein und speichere die Verbindung.</div>";

  WiFi.scanDelete();
  int networkCount = WiFi.scanNetworks(false, true);

  if (networkCount <= 0) {
    page += "<div class='sub'>Keine WLAN-Netzwerke gefunden. Bitte pr&uuml;fe die Antenne/Position und scanne erneut.</div>";
  } else {
    for (int i = 0; i < networkCount; i++) {
      String ssid = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);
      wifi_auth_mode_t security = WiFi.encryptionType(i);

      String ssidLabel = ssid.length() > 0 ? htmlEscape(ssid) : String("<i>Verstecktes Netzwerk</i>");
      String selectUrl = "/?ssid=" + urlEncode(ssid) + "#wifi";

      page += "<div class='net'>";
      page += "<div><div class='ssid'>" + ssidLabel + "</div><div class='meta'>Kanal ";
      page += String(WiFi.channel(i));
      page += "</div></div>";
      page += "<div class='meta'>";
      page += String(rssi);
      page += " dBm<br>";
      page += getWifiSignalName(rssi);
      page += "</div>";
      page += "<div class='meta'>";
      page += getWifiSecurityName(security);
      page += "</div>";
      if (ssid.length() > 0) {
        page += "<a class='btn green' href='" + selectUrl + "'>SSID &uuml;bernehmen</a>";
      } else {
        page += "<span class='meta'>Manuell eingeben</span>";
      }
      page += "</div>";
    }
  }

  WiFi.scanDelete();

  page += "<div class='actions'>";
  page += "<a class='btn green' href='/wifi-scan'>Erneut scannen</a>";
  page += "<a class='btn' href='/#wifi'>Zur&uuml;ck</a>";
  page += "</div>";
  page += "</div></div></body></html>";

  return page;
}

static void handleWifiScan() {
  server.send(200, "text/html", wifiScanPage());
}

static void handleWifiSave() {
  if (server.hasArg("ssid")) {
    homeWifiSsid = server.arg("ssid");
    homeWifiSsid.trim();

    homeWifiPassword = server.arg("pass");
    homeWifiEnabled = homeWifiSsid.length() > 0;

    saveWiFiSettings();

    server.send(200, "text/html",
                "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
                "<h1>SmartFix Matrix</h1>"
                "<p>WLAN gespeichert. Neustart...</p>"
                "</body></html>");

    delay(1000);
    ESP.restart();
    return;
  }

  redirectHome();
}

static void handleWifiForget() {
  homeWifiEnabled = false;
  homeWifiSsid = "";
  homeWifiPassword = "";
  saveWiFiSettings();
  disconnectHomeWiFi();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Heim WLAN gel&ouml;scht. Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

static void handleOtaSave() {
  if (server.hasArg("url")) {
    otaUrl = server.arg("url");
    otaUrl.trim();

    if (otaUrl.length() == 0) {
      otaUrl = DEFAULT_OTA_URL;
    }

    saveOtaSettings();
  }

  redirectHome();
}

static void handleOtaStart() {
  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix OTA</h1>"
              "<p>OTA Update wurde gestartet.</p>"
              "<p>Bitte warten. Bei Erfolg startet das Ger&auml;t automatisch neu.</p>"
              "</body></html>");

  delay(500);
  startOtaUpdateFromSavedUrl();
}

static void handleFactoryReset() {
  factoryResetSettings();

  server.send(200, "text/html",
              "<html><body style='background:#0b0f14;color:white;font-family:Arial;text-align:center;padding-top:40px;'>"
              "<h1>SmartFix Matrix</h1>"
              "<p>Einstellungen wurden gel&ouml;scht.</p>"
              "<p>Neustart...</p>"
              "</body></html>");

  delay(1000);
  ESP.restart();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mode", HTTP_GET, handleModeChange);
  server.on("/auto", HTTP_GET, handleAutoDemo);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/speed", HTTP_GET, handleSpeed);
  server.on("/text-color", HTTP_GET, handleTextColor);
  server.on("/scroll-effect", HTTP_GET, handleScrollEffect);
  server.on("/logo-effect", HTTP_GET, handleLogoEffect);
  server.on("/logo-color", HTTP_GET, handleLogoColor);
  server.on("/set-text", HTTP_GET, handleSetText);
  server.on("/set-logo-text", HTTP_GET, handleSetLogoText);
  server.on("/wifi-scan", HTTP_GET, handleWifiScan);
  server.on("/wifi-save", HTTP_POST, handleWifiSave);
  server.on("/wifi-forget", HTTP_GET, handleWifiForget);
  server.on("/ota-save", HTTP_POST, handleOtaSave);
  server.on("/ota-start", HTTP_GET, handleOtaStart);
  server.on("/factory-reset", HTTP_GET, handleFactoryReset);

  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Not found");
  });

  server.begin();
  Serial.println("Webserver started");
}

void handleWebServer() {
  server.handleClient();
}
