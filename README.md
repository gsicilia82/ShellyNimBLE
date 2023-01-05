# ShellyNimBLE

Dieses Projekt dient dazu, Shelly Devices mit ESP32 Microcontroller (Single-Core) als BLE Scanner zu verwenden. Es gibt tolle Projekte, wie espresense, die aber noch keine SingleCore ESP32 unterstützen.

Die Firmware kann dazu genutzt werden, die Shelly weiterhin zur STeuerung von Lichtern oder Rollläden zu verwenden. Die Kommuikation erfolgt über MQTT (optimiert für ioBroker).

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

***

## Flashen mit esptool

Das Flashen kann über esptool.py erfolgen, was von hier bezogen werden kann:

[GitHub - espressif/esptool: Espressif SoC serial bootloader utility](https://github.com/espressif/esptool)

Hilfreiche Befehle sind zum Beispiel:

* Löschen des Flash-Speicher:
  
   `esptool.py erase_flash`

* Schreiben des Flashspeichers:
  
   `esptool.py --baud 115200 write_flash 0x0 firmware.bin`

Ich selbst bevorzuge das Flashen mit PlatformIO innerhalb von VS-Code

***

## Flashen mit PlatformIO

PlatformIO ist ein Plugin, dass unter VSCode eingesetzt wird. Es ist zum Standard für größere Projekte geworden und bietet einige Vorteile gegenüber der Standard Arduino-IDE.

Wenn VSCode installiert wurde, kann PlatformIO als Plugin installiert werden:

![ ](pictures/vscode/010_vscode_install_platformio.png  "pio install")

Als nächstes wird dieses Projekt von Github heruntergeladen (als zip) und lokal entpackt. Dieser Ordner wird PlatformIO als Projekt hinzugefügt:

![ ](pictures/vscode/020_vscode_pio_add-project.png  "pio install")