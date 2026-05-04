# UWBP Backend

Backend für das UWBP Indoor-Positioning-System.

Repos:

- Backend: <https://github.com/leonstue/uwbp_server>
- Frontend: <https://github.com/leonstue/uwbp_frontend>

Das Backend stellt die API bereit, verwaltet das WLAN für die ESP32-Geräte und läuft auf dem Raspberry Pi als systemd-Service.

---

## Deploymentanleitung

Das Backend-Projekt muss bereits lokal auf dem Raspberry Pi vorhanden sein.

```text
<BACKEND_DIR> = lokaler Pfad zum Backend-Projekt
```

Das Backend wird als fertiges Artefakt unter folgendem Pfad deployed:

```text
/opt/uwbp/server
```

Für die Deployment-Befehle wird `make` benötigt. Falls `make` noch nicht installiert ist:

```bash
sudo apt update
sudo apt install -y make
```

### Deployment ausführen

```bash
cd <BACKEND_DIR>
make deploy
```

`make deploy` installiert die benötigten Pakete, initialisiert Submodules, bereitet `vcpkg` vor, baut das Backend, kopiert das Binary nach `/opt/uwbp/server`, installiert den systemd-Service und startet ihn direkt.

### Deployment testen

```bash
systemctl status uwbp-server.service --no-pager
make logs
```

Wenn der Service `active (running)` ist, wurde das Backend erfolgreich gestartet.

---

## Make-Targets

| Target | Beschreibung |
| --- | --- |
| `make help` | Zeigt alle verfügbaren Make-Targets an. |
| `make deploy` | Installiert Abhängigkeiten, bereitet Submodules und `vcpkg` vor, baut das Backend, deployed das Binary, installiert den Service und startet ihn direkt. |
| `make clean` | Stoppt und entfernt den Service, löscht das deployte Artefakt und entfernt lokale Build- und vcpkg-Artefakte. |
| `make clean-artifacts` | Löscht nur das deployte Artefakt unter `/opt/uwbp/server`. Der Service bleibt registriert und kann danach fehlschlagen, bis erneut deployed wurde. |
| `make logs` | Zeigt die letzten Logs des Backend-Service an. |
| `make install-deps` | Installiert die benötigten Systempakete. |
| `make submodules` | Initialisiert und aktualisiert die Git-Submodules. |
| `make bootstrap-vcpkg` | Bereitet `vcpkg` vor, falls `external/vcpkg/vcpkg` noch nicht vorhanden ist. |
| `make configure` | Führt nur den CMake-Configure-Schritt aus. |
| `make build` | Baut das Backend für die automatisch erkannte Plattform. |
| `make rebuild` | Löscht den aktuellen Build-Ordner und baut neu. |
| `make deploy-artifact` | Kopiert das gebaute Backend-Binary nach `/opt/uwbp/server`. |
| `make install-service` | Installiert und aktiviert den systemd-Service. |
| `make start` | Startet bzw. restartet den Backend-Service sofort. |
| `make run` | Baut und startet das Backend lokal mit `sudo`. |
| `make run-only` | Startet das bereits gebaute Backend lokal mit `sudo`, ohne neu zu bauen. |
| `make wsl` | Baut explizit mit dem WSL-Preset. |
| `make pi` | Baut explizit mit dem Raspberry-Pi-Preset. |

---

## Service

Service-Name:

```text
uwbp-server.service
```

Status prüfen:

```bash
systemctl status uwbp-server.service --no-pager
```

Logs anzeigen:

```bash
make logs
```
