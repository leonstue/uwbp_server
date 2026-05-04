# UWBP Backend

Backend für das UWBP Indoor-Positioning-System.

Repos:

- Backend: <https://github.com/leonstue/uwbp_server>
- Frontend: <https://github.com/leonstue/uwbp_frontend>

Das Backend stellt die API bereit, verwaltet das WLAN für die ESP32-Geräte und wird auf dem Raspberry Pi als systemd-Service gestartet.

Das Frontend wird separat deployed und stellt das Dashboard bereit.

---

## Deploymentanleitung

Das Backend-Projekt muss bereits lokal auf dem Raspberry Pi vorhanden sein.

In dieser README wird folgender Platzhalter verwendet:

```text
<BACKEND_DIR> = lokaler Pfad zum Backend-Projekt
```

Das Backend wird als fertiges Artefakt unter folgendem Pfad deployed:

```text
/opt/uwbp/server
```

### Backend vollständig deployen

```bash
cd <BACKEND_DIR>

sudo make install-deps
make submodules
make bootstrap-vcpkg
make build
sudo make deploy
sudo make install-service
sudo reboot
```

Nach dem Neustart prüfen:

```bash
systemctl status uwbp-server.service --no-pager
journalctl -u uwbp-server.service -n 50 --no-pager
```

Wenn der Service `active (running)` ist, wurde das Backend erfolgreich gestartet.

---

## Make-Targets

| Target | Beschreibung |
| --- | --- |
| `make help` | Zeigt alle verfügbaren Make-Targets an. |
| `sudo make install-deps` | Installiert die benötigten Systempakete für den Backend-Build. |
| `make submodules` | Initialisiert und aktualisiert die Git-Submodules, insbesondere `external/vcpkg`. |
| `make bootstrap-vcpkg` | Bereitet `vcpkg` vor, falls `external/vcpkg/vcpkg` noch nicht vorhanden ist. |
| `make build` | Baut das Backend für die automatisch erkannte Plattform. |
| `make configure` | Führt nur den CMake-Configure-Schritt aus. |
| `make rebuild` | Löscht den aktuellen Build-Ordner und baut das Backend neu. |
| `make clean` | Löscht den Build-Ordner des aktuellen Presets. |
| `make wsl` | Baut explizit mit dem WSL-Preset. |
| `make pi` | Baut explizit mit dem Raspberry-Pi-Preset. |
| `sudo make deploy` | Kopiert das gebaute Backend-Binary nach `/opt/uwbp/server`. |
| `sudo make install-service` | Installiert den systemd-Service und aktiviert den Autostart. |
| `make run` | Baut und startet das Backend lokal mit `sudo`. |
| `make run-only` | Startet das bereits gebaute Backend lokal mit `sudo`, ohne neu zu bauen. |

---

## systemd-Service

Der Backend-Service wird durch `sudo make install-service` installiert.

Erwarteter Service-Name:

```text
uwbp-server.service
```

Status prüfen:

```bash
systemctl status uwbp-server.service --no-pager
```

Logs anzeigen:

```bash
journalctl -u uwbp-server.service -n 50 --no-pager
```
