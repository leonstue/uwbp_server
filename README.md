# UWBP Backend

Backend für das UWBP Indoor-Positioning-System.

Das Projekt besteht aus zwei Teilen:

- Backend: <https://github.com/leonstue/uwbp_server>
- Frontend: <https://github.com/leonstue/uwbp_frontend>

Das Backend läuft auf einem Raspberry Pi, stellt die API für Frontend und ESP32-Geräte bereit und verwaltet das WLAN für das UWBP-System. Zusätzlich wird ein Watchdog verwendet, der das Netzwerk beendet, wenn das Backend nicht mehr läuft.

Das Frontend wird separat gebaut und deployed. Für den vollständigen Betrieb müssen Backend und Frontend eingerichtet sein.

---

## Voraussetzungen

Backend- und Frontend-Projekt müssen bereits lokal auf dem Raspberry Pi vorhanden sein.

In dieser README wird folgender Platzhalter verwendet:

```text
<BACKEND_DIR> = lokaler Pfad zum Backend-Projekt
```

Das Backend wird nach dem Build als fertiges Artefakt unter folgendem Pfad deployed:

```text
/opt/uwbp/server
```

---

## Systempakete installieren

```bash
sudo apt update
sudo apt upgrade -y

sudo apt install -y git build-essential cmake ninja-build
```

`git` wird benötigt, wenn das eingebundene `vcpkg`-Submodule initialisiert werden muss.

---

## Backend-Submodules vorbereiten

Das Backend verwendet `vcpkg` als Git-Submodule unter:

```text
<BACKEND_DIR>/external/vcpkg
```

Falls das Backend-Projekt über Git bereitgestellt wurde, muss das Submodule initialisiert werden:

```bash
cd <BACKEND_DIR>
git submodule update --init --recursive
```

Alternativ muss das Verzeichnis `external/vcpkg` bereits vollständig im Backend-Projektverzeichnis vorhanden sein.

---

## vcpkg vorbereiten

```bash
cd <BACKEND_DIR>
./external/vcpkg/bootstrap-vcpkg.sh
```

---

## Backend bauen

Die verfügbaren Make-Targets können angezeigt werden mit:

```bash
make help
```

Das Backend wird gebaut mit:

```bash
make build
```

Nach erfolgreichem Build liegt die ausführbare Datei unter:

```text
<BACKEND_DIR>/out/build/pi-arm64-debug/uwbp_server
```

---

## Backend-Artefakt deployen

Es wird nur das fertig gebaute Backend-Artefakt nach `/opt/uwbp/server` kopiert.

```bash
sudo rm -rf /opt/uwbp/server
sudo mkdir -p /opt/uwbp/server

sudo cp <BACKEND_DIR>/out/build/pi-arm64-debug/uwbp_server /opt/uwbp/server/uwbp_server
```

---

## Backend-Service-Datei

Im Backend-Repository liegt die systemd-Service-Datei unter:

```text
<BACKEND_DIR>/deploy/uwbp-server.service
```

Die Service-Datei verwendet den festen Deployment-Pfad `/opt/uwbp/server`:

```ini
[Unit]
Description=UWBP Backend Server
Wants=NetworkManager.service
After=dbus.service NetworkManager.service

[Service]
Type=simple
WorkingDirectory=/opt/uwbp/server
ExecStart=/opt/uwbp/server/uwbp_server
Restart=always
RestartSec=5

User=root

[Install]
WantedBy=multi-user.target
```

---

## Backend-Service installieren

```bash
sudo cp <BACKEND_DIR>/deploy/uwbp-server.service /etc/systemd/system/uwbp-server.service
sudo systemctl daemon-reload
sudo systemctl enable uwbp-server.service
```

Danach wird der Raspberry Pi neu gestartet:

```bash
sudo reboot
```

---

## Nach dem Reboot testen

Nach dem Neustart kann geprüft werden, ob der Backend-Service erfolgreich automatisch gestartet wurde:

```bash
systemctl status uwbp-server.service --no-pager
journalctl -u uwbp-server.service -n 50 --no-pager
```

Wenn der Service `active (running)` ist, wurde das Backend erfolgreich gestartet.
