# ShellyNimBLE

Dieses Projekt dient dazu, Shelly Devices mit ESP32 Microcontroller (Single-Core) als BLE Scanner zu verwenden. Es gibt favorisierte Projekte, wie espresense, die aber noch keine SingleCore ESP32 unterstützen.

Die Firmware kann dazu genutzt werden, die Shelly weiterhin zur Steuerung von Lichtern oder Rollläden zu verwenden. Die Kommunikation erfolgt über MQTT (optimiert für ioBroker).
Mit der aktuellen Version können BLE Geräte über ihre MAC oder, wenn vorhanden, über ihre iBeacon UUID gescannt und gefiltert werden (Funktion als Whitelist). 

* [ioBroker States Übersicht](#ioBroker-States-Übersicht)
* [MQTT Konfiguration](#mqtt-konfiguration)
* [Firmware Binaries unter Releases](#Firmware-Binaries-unter-Releases)
* [Erstmalig Flashen mit esptool](#Erstmalig-Flashen-mit-esptool)
* [Erstmalig Flashen mit PlatformIO](#Erstmalig-Flashen-mit-PlatformIO)
* [OTA Flashen über Webportal](#OTA-Flashen-über-Webportal)
* [Konfiguration des Shelly](#Konfiguration-des-Shelly)
* [Rollladen / COVER](#Rollladen-/-COVER)
* [Reset-Taste am Shelly](#Reset-Taste-am-Shelly)
* [JavaScript für Optimierung der MQTT States](#JavaScript-für-Optimierung-der-MQTT-States)
* [Changelog](#Changelog)

## ioBroker States Übersicht

Die States innerhalb von ioBroker werden automatisch erstellt, wenn eine MQTT Instanz läuft. Ich bevorzuge unter ioBroker den MQTT-Client, da ich Mosquitto als MQTT-Server verwende. Nachfolgend sind als Beispiel zwei Shellies zu sehen; ein ShellyPlus 2PM als COVER und ein ShellyPlus 1 als LIGHT:

![](pictures/iobroker/020_iobroker_states_overview.png  "ioBroker States")

Im State `mqtt-client.0.shellyscanner.devices.*.Filter` können bis zu 10 MAC-Adressen / iBeacon-UUIDs eingegeben werden. Die Eingaben müssen über ein Komma getrennt werden. Außerdem können optional Aliase vergeben werden. Nach dem Flashprozess ist ein Beispiel im State hinterlegt.

## MQTT Konfiguration

Damit die States von ioBroker erkannt werden, muss unter der MQTT Instanz eine Subscription auf `shellyscanner/#` eingestellt werden:

![ ](pictures/iobroker/010_iobroker_mqtt_subscriptions.png  "ioBroker MQTT")

Nach dem Flashprozess - beschrieben in nachfolgenden Kapiteln - kann die Steuerung des Shelly über die automatisch erstellten ioBroker States erfolgen. Für ausgehende States muss "publish" aktiviert werden. Hier ein Beispiel für einen beliebigen State:

* Zunächst auf das Zahnradsymbol klicken
* MQTT Instanz aufklappen und Checkbox publish aktivieren

![ ](pictures/iobroker/025_iobroker_publish_Zahnrad.png  "ioBroker publish1")
![ ](pictures/iobroker/030_iobroker_publish_config.png  "ioBroker publish2")

Dies sollte für alle, außer den nachfolgenden States erledigt werden:

* IP_Address (Zur Info, welche IP das Gerät hat)
* Info (Boot-Zeit, FW-Version)
* Message (Empfang von zuätzlichen Informationen)
* Online (Anzeige des Online-Zustandes)
* Switch1 und ggf. Switch2 (Anzeige ob Inputs am Shelly anliegen)

## Firmware Binaries unter Releases

Zu jedem Release werden zwei Dateien hinzugefügt:

* firmware_full.bin

* firmware_update.bin

Wenn **erstmalig** mit esptool geflasht wird, muss **firmware_full.bin** verwendet werden. Hier sind alle benötigten Partitionen vorhanden, weshalb diese Binary eine Größe von 4MB aufweist.

Zukünftige **Updates** können über die **WebUI** des Shelly erfolgen. Es ist eine simple OTA Funktionalität integriert. Hier wird dann die **firmware_update.bin** verwendet, damit Einstellungen, wie die Konfiguration, Filter, WIFI etc. beibehalten werden.

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

## Erstmalig Flashen mit PlatformIO

<details>
  <summary>Für mehr Infos hier klicken</summary>
  <br>

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

</details>

## OTA Flashen über Webportal

Wenn auf dem Shelly diese Firmware über eine der oben beschriebenen Wege bereits geflasht wurde, kann über die IP des Shellys auf die Web OTA Funktion zugegriffen werden.
Für den OTA Flashvorgang muss aus den Releases die `firmware_update.bin` verwendet werden. 

## Konfiguration des Shelly

Wenn der Shelly erfolgreich geflasht wurde, wird mit dem ersten boot ein AccessPoint mit dem Namen `esp32-Shelly` gestartet. Bitte mit diesem verbinden, eine Weiterleitung sollte automatisch erfolgen, ansonsten im Browser die IP `192.168.4.1` aufrufen.

<u>Hier müssen alle notwendigen Daten eingegeben werden</u>

(Die Felder sind für schnellere Tests fast alle vorbelegt)

![CaptivePortal.png](pictures/config/CaptivePortal.png)

* **WIFI und MQTT** Daten sind selbsterklärend

* Der **Device Name** wird als Bestandteil der MQTT Topics verwendet. Dieser sagt somit aus, unter welchem Namen dieses Gerät später zu sehen ist.

* In **Device Mode** kann zwischen LIGHT und COVER gewählt werden. Bei Shelly Plus 1(PM) ist nur der Modus LIGHT sinnvoll. Bei den 2PM Modellen ist es je nach Einsatzzweck zu wählen.

Die bis hier angegebenen Daten können später nicht mehr geändert werden. Dies ist dann nur nach einem HardReset möglich (oder nach Erase-Flash und neu Flashen über PlatformIO o.ä.)

Im Eingabebereich **Config** muss ein Konfigurationsprofil hinterlegt, je nachdem welches Shelly Modell verwendet wird. Wird hier keine gültige Config hinterlegt, wird automatisch "Shelly Plus 2PM v0.1.9" gewählt.

Nachfolgend dargestellte Profile können aktuell eingesetzt werden. Einfach das passende kopieren und in den Config-Bereich eingeben.

Diese Profile können auch später im Einsatz über MQTT angepasst werden. Es kann der Bedarf bestehen, dass die Eingangs- oder Ausgangs-Pins vertauscht werden; dies kann über diese Config geschehen.

In ioBroker wird die Config z.B. über `mqtt-client.0.shellyscanner.devices.*.Config`dargestellt und kann auch hierüber angepasst werden.

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

## Rollladen / COVER

Für Rollläden kann über MQTT eine Kalibrierung gestartet werden. Solange keine Kalibrierung durchgeführt wurde, kann über die Taster hoch- und runtergefahren werden. Eine Positionsvorgabe kann nicht durchgeführt werden. 

Ein Rollladen kann über die drei States CoverUp, CoverDown und CoverStop gesteuert werden. Außerdem ist nach erfolgter Kalibrierung eine Sollvorgabe über SetPosition möglich.

Hinweis: Ein Schalten der beiden Ausgänge gleichzeitig ist seitens Software verriegelt.

## Reset-Taste am Shelly

* Wird die Taste zwischen 0,2s und 10s betätigt, wird es neu gestartet

* Wird die Taste länger als 10s betätigt, erfolgt ein HardReset. Hinterlegte Daten für WIFI und MQTT werden gelöscht. Das Gerät bootet nun in den AP-Mode.

## JavaScript für Optimierung der MQTT States

Wenn MQTT States von ioBroker automatisch generiert werden, sind diese grundsätzlich beschreibbar, publish ist deaktiviert. Ich habe ein kleines Skript geschrieben, dass die States gemäß den Anforderungen konfiguriert, wobei auch die State-Namen den Device-Namen als Präfix erhalten. Dafür muss das Skript in der ioBroker Skript-Engine einmalig ausgeführt werden. Es beendet sich selbst, wenn es durchlief ud muss nochmals gestartet werden, wenn später weitere Shellys hinzukommen.

Dieses Skript funktioniert, wie hier hinterlegt, ausschließlich über die Instanz 0 vm MQTT-Client Adapter. Wenn andere Adapter verwendet werden, muss das Skript entsprechend angepasst werden.

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
    "Switch2",
    "Info"
]

$('mqtt-client.0.shellyscanner.devices.*.*').each(function ( id, i) {
    let splittedID = id.split(".");
    let lastItem = splittedID.pop();
    let deviceName = splittedID.pop();
    let obj = getObject( id);
    if ( !obj.common.name.includes( deviceName) ) obj.common.name = deviceName + " " + obj.common.name; // change name
    if ( arrReadOnly.includes( lastItem) ){
        // disable publish and write access
        obj.common.write = false;
        obj.common.custom["mqtt-client.0"].publish = false;
    }
    else {
        // enable publish and write access
        obj.common.write = true;
        obj.common.custom["mqtt-client.0"].publish = true;
    }

    if ( getState( id).val == "true" || getState( id).val == "false"){
        // set boolean mode
        obj.common.type = "boolean";
    }

    setObject( id, obj);
    if( lastItem=="Restart") cld( obj);
});

stopScript();
```

---

## Changelog

**Changelog 0.1.0 (04.02.2022)**

- iBeacon als Filter Möglichkeit hinzugefügt
- Auto-Kalibrierung für Rollläden
- Leistungsmessung bei Shelly Plus 2PM
- Sende Info Status

**Changelog 0.0.1 (05.01.2022)**

- Erstes Release
