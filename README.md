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
/opt/uwbp/server          # aktive version (laufender service)
/opt/uwbp/server.next     # staging fuer den naechsten service-start
```

`make deploy` legt neue Builds in `server.next` ab und lässt einen laufenden Service unangetastet. Beim nächsten Service-Start (Reboot oder `make start`) promotet ein `ExecStartPre`-Hook das Staging-Verzeichnis atomar zur aktiven Version.

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

`make deploy` installiert die benötigten Pakete, initialisiert Submodules, bereitet `vcpkg` vor, baut das Backend, legt das Binary unter `/opt/uwbp/server.next` ab und installiert/aktiviert den systemd-Service.

**Wichtig:** Der laufende Service wird **nicht** angefasst. Er läuft mit der bisher aktiven Version weiter. Die neue Version wird erst beim nächsten Service-Start aktiv (Reboot oder `make start` / `sudo systemctl restart uwbp-server.service`).

Beim Service-Start prüft ein `ExecStartPre`-Hook ob `server.next` existiert und benennt es atomar zu `server` um, bevor der Backend-Prozess startet. Mehrere `make deploy` hintereinander überschreiben das Staging — beim nächsten Start wird nur die letzte Version aktiv.

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
| `make deploy` | Baut das Backend und stagt das neue Artefakt nach `/opt/uwbp/server.next`. Der laufende Service wird **nicht** restartet — die neue Version wird erst beim nächsten Service-Start aktiv (Reboot oder `make start`). |
| `make clean` | Stoppt und entfernt den Service, löscht aktives und gestagtes Artefakt (`/opt/uwbp/server` und `/opt/uwbp/server.next`) und entfernt lokale Build- und vcpkg-Artefakte. |
| `make clean-artifacts` | Löscht aktives und gestagtes Artefakt unter `/opt/uwbp/`. Der Service bleibt registriert und kann danach fehlschlagen, bis erneut deployed wurde. |
| `make logs` | Zeigt die letzten Logs des Backend-Service an. |
| `make install-deps` | Installiert die benötigten Systempakete. |
| `make submodules` | Initialisiert und aktualisiert die Git-Submodules. |
| `make bootstrap-vcpkg` | Bereitet `vcpkg` vor, falls `external/vcpkg/vcpkg` noch nicht vorhanden ist. |
| `make configure` | Führt nur den CMake-Configure-Schritt aus. |
| `make build` | Baut das Backend für die automatisch erkannte Plattform. |
| `make rebuild` | Löscht den aktuellen Build-Ordner und baut neu. |
| `make deploy-artifact` | Kopiert das gebaute Backend-Binary in das Staging-Verzeichnis `/opt/uwbp/server.next`. |
| `make install-service` | Installiert und aktiviert den systemd-Service. |
| `make start` | Startet bzw. restartet den Backend-Service sofort. Falls Staging-Artefakt vorhanden, wird es vor dem Start atomar zur aktiven Version promoviert. |
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
