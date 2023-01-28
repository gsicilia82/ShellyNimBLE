# ShellyNimBLE

Dieses Projekt dient dazu, Shelly Devices mit ESP32 Microcontroller (Single-Core) als BLE Scanner zu verwenden. Es gibt favorisierte Projekte, wie espresense, die aber noch keine SingleCore ESP32 unterstützen.

Die Firmware kann dazu genutzt werden, die Shelly weiterhin zur Steuerung von Lichtern oder Rollläden zu verwenden. Die Kommunikation erfolgt über MQTT (optimiert für ioBroker).
Das Scannen funktioniert für statische MAC-Adressen und iBeacon UUIDs.

* [ioBroker States](#ioBroker-States)
* [Firmware Binaries unter Releases](#Firmware-Binaries-unter-Releases)
* [Erstmalig Flashen mit esptool](#Erstmalig-Flashen-mit-esptool)
* [Erstmalig Flashen mit PlatformIO](#Erstmalig-Flashen-mit-PlatformIO)
* [OTA Flashen über Webportal](#OTA-Flashen-über-Webportal)
* [Konfiguration des Shelly](#Konfiguration-des-Shelly)
* [Rollladen / COVER](#Rollladen-/-COVER)
* [Reset-Taste am Shelly](#Reset-Taste-am-Shelly)
* [JavaScript für Optimierung der MQTT States](#JavaScript-für-Optimierung-der-MQTT-States)
* [Changelog](#Changelog)

## ioBroker States

Die States innerhalb von ioBroker werden automatisch erstellt, wenn eine MQTT Instanz läuft. Ich bevorzuge unter ioBroker den MQTT-Client, da ich Mosquitto als MQTT-Server verwende. Wenn ein Shelly Plus 2PM als Cover eingerichtet wird, wären nachfolgend die ioBroker States zu sehen:

(muss nicht letztem Stand entsprechen)

![ ](pictures/iobroker/020_iobroker_states_overview.png  "ioBroker States")

Damit die States von ioBroker erkannt werden, muss unter der MQTT Instanz eine Subscription auf `shellyscanner/#` eingestellt werden:

![ ](pictures/iobroker/010_iobroker_mqtt_subscriptions.png  "ioBroker MQTT")

Für die Steuerung des Shelly, muss publish (1) in ioBroker für ausgehend States aktiviert werden. Hier ein Beispiel:

![ ](pictures/iobroker/030_iobroker_publish_config.png  "ioBroker publish")

Dies sollte für alle, außer nachfolgende States erledigt werden:

* IP_Address (Zur Info, welche IP das Gerät hat)
* Message (Empfang von zuätzlichen Informationen)
* Online (Anzeige des Online-Zustandes)
* Switch1 und ggf. Switch2 (Anzeige ob Inputs am Shelly anliegen)

Mit der aktuellen Version können BLE Geräte über ihre MAC oder, wenn vorhanden, über ihre iBeacon UUID gefiltert werden (Funktion als Whitelist). Im ioBroker State `mqtt-client.0.shellyscanner.devices.master.Filter` können bis zu 10 MAC-Adressen / iBeacon-UUIDs eingegeben werden. Die Eingaben müssen über ein Komma getrennt werden.

***

## Firmware Binaries unter Releases

Zu jedem Release werden zwei Dateien hinzugefügt:

* firmware_full.bin

* firmware_update.bin

Wenn **erstmalig** mit esptool geflasht wird, muss **firmware_full.bin** verwendet werden. Hier sind alle benötigten Partitionen vorhanden, weshalb diese Binary eine Größe von 4MB aufweist.

Zukünftige **Updates** können über die **WebUI** des Shelly erfolgen. Es ist eine simple OTA Funktionalität integriert. Hier wird dann die **firmware_update.bin** verwendet, damit Einstellungen, wie die Konfiguration, Filter, WIFI etc. beibehalten werden.

---

## Erstmalig Flashen mit esptool

Das Flashen kann über esptool.py erfolgen, was von hier bezogen werden kann:

[GitHub - espressif/esptool: Espressif SoC serial bootloader utility](https://github.com/espressif/esptool)

Hilfreiche Befehle sind zum Beispiel:

* Backup der original Firmware
  
  `esptool.py read_flash 0x0 0x400000 fw-backup-4M.bin`

* Löschen des Flash-Speicher:
  
   `esptool.py erase_flash`

* Schreiben des Flashspeichers:
  
   `esptool.py --baud 115200 write_flash 0x0 firmware_full.bin`

***

## Erstmalig Flashen mit PlatformIO

PlatformIO ist ein Plugin, dass unter VSCode eingesetzt wird. Es ist zum Standard für größere Projekte geworden und bietet einige Vorteile gegenüber der Standard Arduino-IDE.

Wenn VSCode installiert wurde, kann PlatformIO als Plugin installiert werden:

(es schadet nicht, VSCode einmalig neu zu starten, wenn PlatformIO installiert wurde)

![ ](pictures/vscode/010_vscode_install_platformio.png  "pio install")

Als nächstes wird dieses Projekt als zip von Github heruntergeladen und lokal entpackt. Dieser Ordner wird PlatformIO als Projekt hinzugefügt:

Meistens wird das Fenster "PIO Home" automatisch geöffnet, wenn dies nicht der Fall ist, unten über das Haus-Icon manuell öffnen.

![ ](pictures/vscode/020_vscode_pio_add-project.png  "pio add project")

Da mehrere Projekte in PlatformIO existieren können, müssen wir unser Projekt nun innerhalb vom PIO-Home Fenster auswählen:

![ ](pictures/vscode/030_vscode_pio_open-project.png  "pio open project")

Das kompilieren und hochladen der Firmware erfolgt entweder mit angeschlossenem Flasher. Hier bitte zunächst den Schritt "Erase Flash" ausführen, wenn erstmalig geflasht wird.

![ ](pictures/vscode/040_vscode_pio_build-upload.png  "pio build upload")

Mit PlatformIO kann auch direkt OTA geflasht werden. Für den OTA Upload muss die platformio.ini aus dem Projekt geöffnet werden und in der entsprechenden Section die IP des Shellys angepasst werden.

![ ](pictures/vscode/050_vscode_pio_ota_direct.png  "pio build ota upload")

***

## OTA Flashen über Webportal

Wenn auf dem Shelly diese Firmware über eine der oben beschriebenen Wege bereits geflasht wurde, kann über die IP des Shellys auf die Web OTA Funktion zugegriffen werden.
Für den OTA Flashvorgang muss aus den Releases die `firmware_update.bin` verwendet werden. 

***

## Konfiguration des Shelly

Wenn der Shelly erfolgreich geflasht wurde, wird mit dem ersten boot ein AccessPoint mit dem Namen `esp32-Shelly` gestartet. Bitte mit diesem verbinden, eine Weiterleitung sollte automatisch erfolgen, ansonsten im Browser die IP `192.168.4.1` aufrufen.

<u>Hier müssen alle notwendigen Daten eingegeben werden</u>

(Die Felder sind für schnellere Tests fast alle vorbelegt)

![CaptivePortal.png](pictures/config/CaptivePortal.png)

* **WIFI und MQTT** Daten sind selbsterklärend

* Der **Device Name** wird als Bestandteil der MQTT Topics verwendet. Dieser sagt somit aus, unter welchem Namen dieses Gerät später zu sehen ist.

* In **Device Mode** kann zwischen LIGHT und COVER gewählt werden. Bei Shelly Plus 1(PM) ist nur der Modus LIGHT sinnvoll. Bei den 2PM Modellen ist es je nach Einsatzzweck zu wählen.

Die bis hier angegebenen Daten können später nicht mehr geändert werden. Dies ist dann nur nach einem HardReset möglich (oder nach Erase-Flash und neu Flashen über PlatformIO o.ä.)

Im Eingabebereich **Config** wird ein Konfigurationsprofil hinterlegt, je nachdem welches Shelly Modell verwendet wird. Wird hier keine gültige Config hinterlegt, wird automatisch "Shelly Plus 2PM v0.1.9" gewählt.

Nachfolgend dargestellte Profile können aktuell eingesetzt werden. Einfach das passende kopieren und in den Config-Bereich eingeben.

Diese Profile können auch später im Einsatz über MQTT angepasst werden. Es kann der Bedarf bestehen, dass die Eingangs- oder Ausgangs-Pins vertauscht werden; dies kann über diese Config geschehen.

In ioBroker wird die Config z.B. über `mqtt-client.0.shellyscanner.devices.master.Config`dargestellt und kann auch hierüber angepasst werden.

Für SwitchX_Mode sind folgende Inhalte möglich: Switch, Button, Detached.

Wird eine fehlerhafte Config übergeben, wird die Standard Config für Shelly Plus 2PM geladen.

```javascript
{  
"Config": "Shelly Plus 2PM v0.1.9",
"ButtonReset": 4,
"Switch1": 5,
"Switch1_Mode": "Switch",
"Switch2": 18,
"Switch2_Mode": "Switch",
"Relay1": 13,
"Relay2": 12,
"I2C_SCL": 25,
"I2C_SDA": 26,
"ADE7953": 27
}
```

```javascript
{  
"Config": "Shelly Plus 2PM v0.1.5",
"ButtonReset": 27,
"Switch1": 2,
"Switch1_Mode": "Switch",
"Switch2": 18,
"Switch2_Mode": "Switch",
"Relay1": 13,
"Relay2": 12,
"I2C_SCL": 25,
"I2C_SDA": 33,
"ADE7953": 36
}
```

```javascript
{  
"Config": "Shelly Plus 1(PM) v0.1.9",
"ButtonReset": 25,
"Switch1": 4,
"Switch1_Mode": "Switch",
"Switch2": -1,
"Switch2_Mode": "Switch",
"Relay1": 26,
"Relay2": -1,
"I2C_SCL": -1,
"I2C_SDA": -1,
"ADE7953": -1
}
```

---

## Rollladen / COVER

Für Rollläden kann über MQTT eine Kalibrierung gestartet werden. Solange keine Kalibrierung durchgeführt wurde, kann über die Taster hoch- und runtergefahren werden. Eine Positionsvorgabe kann nicht durchgeführt werden. 

Ein Rollladen kann über die drei States CoverUp, CoverDown und CoverStop gesteuert werden. Außerdem ist nach erfolgter Kalibrierung eine Sollvorgabe über SetPosition möglich.

Hinweis: Ein Schalten der beiden Ausgänge gleichzeitig ist seitens Software verriegelt.

---

## Reset-Taste am Shelly

* Wird die Taste zwischen 0,2s und 10s betätigt, wird es neu gestartet

* Wird die Taste länger als 10s betätigt, erfolgt ein HardReset. Hinterlegte Daten für WIFI und MQTT werden gelöscht. Das Gerät bootet nun in den AP-Mode.

---

## JavaScript für Optimierung der MQTT States

Wenn MQTT States von ioBroker automatisch generiert werden, sind diese grundsätzlich beschreibbar, publish ist deaktiviert. Ich habe ein kleines Skript geschrieben, dass die States gemäß den Anforderungen konfiguriert, wobei auch die State-Namen den Device-Namen als Präfix erhalten. Dafür muss das Skript in der ioBroker Skript-Engine einmalig ausgeführt werden. Es beendet sich selbst, wenn es durchlief ud muss nochmals gestartet werden, wenn später weitere Shellys hinzukommen.

```javascript
let arrReadOnly = [
    "CoverState",
    "Message",
    "Online",
    "IP_Address",
    "Power1",
    "Power2",
    "PowerAcc",
    "Switch1",
    "Switch2"
]

$('mqtt-client.0.shellyscanner.devices.*.*').each(function ( id, i) {
    let splittedID = id.split(".");
    let lastItem = splittedID.pop();
    let deviceName = splittedID.pop();
    let obj = getObject( id);
    obj.common.name = deviceName + " " + obj.common.name;
    if ( arrReadOnly.includes( lastItem) ){
        obj.common.write = false;
        obj.common.custom.publish = false;
    }
    else {
        obj.common.write = true;
        obj.common.custom.publish = true;
    }
    setObject( id, obj);
});
stopScript();
```

---

## Changelog

**Changelog 0.1.0 (25.01.2022)**

- iBeacon als Filter Möglichkeit hinzugefügt
- Auto-Kalibrierung für Rollläden 
- Leistungsmessung bei Shelly Plus 2PM 

**Changelog 0.0.1 (05.01.2022)**

- Erstes Release
