# ShellyNimBLE

Dieses Projekt dient dazu, Shelly Devices mit ESP32 Microcontroller (Single-Core) als BLE Scanner zu verwenden. Es gibt tolle Projekte, wie espresense, die aber noch keine SingleCore ESP32 unterstützen.

Die Firmware kann dazu genutzt werden, die Shelly weiterhin zur Steuerung von Lichtern oder Rollläden zu verwenden. Die Kommuikation erfolgt über MQTT (optimiert für ioBroker).

Die States innerhalb von ioBroker werden automatisch erstellt, wenn eine MQTT Instanz läuft. Ich bevorzuge unter ioBroker den MQTT-Client, da ich Mosquitto als MQTT-Server verwende. Wenn ein Shelly Plus 2PM als Cover eingerichtet wird, wären nachfolgend die ioBroker States zu sehen:

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



Mit der aktuellen Version können nur BLE Geräte über ihre MAC gefiltert werden (Funktion als Whitelist). Im ioBroker State `mqtt-client.0.shellyscanner.devices.master.Filter` können bis zu 10 MAC Adressen eingegeben werden. Die MAC Adressen müssen über ein Komma getrennt werden.



***

## Erstmalig Flashen mit esptool

Das Flashen kann über esptool.py erfolgen, was von hier bezogen werden kann:

[GitHub - espressif/esptool: Espressif SoC serial bootloader utility](https://github.com/espressif/esptool)

Hilfreiche Befehle sind zum Beispiel:

* Löschen des Flash-Speicher:
  
   `esptool.py erase_flash`

* Schreiben des Flashspeichers:
  
   `esptool.py --baud 115200 write_flash 0x0 firmware.bin`

Ich selbst bevorzuge das Flashen mit PlatformIO innerhalb von VS-Code. Zur weiteren Anwendung mit esptool kann kein Support geleistet werden.

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

## OTA Flasen mit *.bin über Webportal

Wenn auf dem Shelly bereits diese Firmware geflasht wurde, kann über die IP des Shellys auf die Web OTA Funktion zugegriffen werden.

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

Für SwitchX_Mode sind folgende Inhalte möglich: Switch, Button, Detached

```
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

```
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

```
{  
"Config": "Shelly Plus 1PM v0.1.9",
"ButtonReset": 27,
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

Für eine einfache Einrichtung sollte der Rollladen in voll geöffneter Stellung sein, wenn der Shelly eingebaut wird!

Eine automatische Kalibrierung ist aktuell nicht möglich. Daher wird die Position eines Rollladen über die Zeit ermittelt. Die benötigte Zeit zwischen den beiden Endpositionen wird in ioBroker unter dem State `mqtt-client.0.shellyscanner.devices.master.SetMaxCoverTime` angegeben. Zeit manuell messen und hier angeben.

Ein Rollladen kann über die drei States CoverUp, CoverDown und CoverStop gesteuert werden. Außerdem ist eine Sollvorgabe über SetPosition möglich.

Hinweis: Ein Schalten der beiden Ausgänge gleichzeitig ist seitens Software verriegelt.

---

## Reset-Taste am Shelly

* Wird die Taste zwischen 0,2s und 10s betätigt, wird es neu gestartet

* Wird die Taste länger als 10s betätigt, erfolgt ein HardReset. Hinterlegte Daten für WIFI und MQTT werden gelöscht. Das Gerät bootet nun in den AP-Mode.

---

## Geplant

* Powermessung für Shelly Plus 2PM über integriertem ADE7593

* Automatische Kalibrierung der Rollladen über Powermessung

* WebSerial zur Ausgabe von weiteren Informationen über die Website

* Test ob OTA flash direkt auf Shelly Firmware funktioniert

---

## Changelog

**Changelog 0.0.1 (05.01.2022)**

- Erstes Release