
**Changelog 0.1.2 (08.02.2022)**

* bugfix autoredirect to /update instead of /webserial

**Changelog 0.1.1 (07.02.2022)**

* added debug build option "-D VERBOSE_MQTT"
* bugfix: do not send power "0" if ADE7953 disabled
* Disable unnececessary subscribtion for cover topics if in LIGHT mode
* Wait for MQTT connection before onDisconnect() is set. Otherwise instant boot on error
* bugfix: Shelly1 crash after opening relay cause of power send process

**Changelog 0.1.0 (04.02.2022)**

* iBeacon als Filter Möglichkeit hinzugefügt
* Aliase bei Filter möglich
* Auto*Kalibrierung für Rollläden
* Leistungsmessung bei Shelly Plus 2PM
* Sende Info Status