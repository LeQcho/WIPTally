# Tally ATEM · Wemos LOLIN S2 Mini

Prototype pour **ESP32-S2 / LOLIN S2 Mini**, avec un master Wi-Fi et des récepteurs ESP-NOW. Les binaires compilés se trouvent dans `firmware/`. Pas besoin de modifier le code pour le premier test.

```text
ATEM ── Ethernet ── Routeur 2,4 GHz ))) Wi-Fi ))) S2 Mini MASTER
                                                  ))) ESP-NOW ))) S2 Mini CAM 1
                                                  ))) ESP-NOW ))) S2 Mini CAM 2…
```

Le master est le seul ESP connecté au routeur et le seul client ATEM. Les récepteurs n'ont besoin ni du mot de passe Wi-Fi ni d'une IP. Le protocole transmet les flags tally fournis par l'ATEM (`TlIn`), ce qui permet de suivre les transitions et les compositions, plutôt que seulement les boutons Program/Preview. Le rouge a priorité si les deux flags sont actifs.

## Flasher sur Mac

Ouvrir Terminal dans ce dossier `S2Mini`, puis préparer l'outil une seule fois :

```sh
python3 -m venv .flash-venv
.flash-venv/bin/python -m pip install -r requirements-flash.txt
```

Brancher **une seule S2 Mini** avec un câble USB de données. Au besoin, maintenir **0 / BOOT**, appuyer puis relâcher **RST**, puis relâcher **0 / BOOT**. Cela force le mode téléchargement USB.

Pour le master :

```sh
.flash-venv/bin/python flash.py master
```

Débrancher cette carte, brancher une autre S2 Mini, puis :

```sh
.flash-venv/bin/python flash.py receiver
```

Appuyer sur **RST** après chaque flash si la carte reste en mode téléchargement. Les firmwares démarrent aussi sur batterie / alimentation USB sans ordinateur.

Si plusieurs ports existent :

```sh
.flash-venv/bin/python flash.py list
.flash-venv/bin/python flash.py receiver --port /dev/cu.usbmodemXXXX
```

Le script vérifie les sommes SHA-256 et cible explicitement **esp32s2**. Il écrit le bootloader à `0x1000`, les partitions à `0x8000`, `boot_app0` à `0xe000` et l'application à `0x10000`. Il conserve la zone NVS de configuration, sauf si un kit personnel fournit `firmware/network-settings.bin` : cette configuration est alors appliquée au master à `0x9000`. Le paquet public ne contient pas ce fichier. Changer de rôle n'efface donc pas les anciens réglages de ce rôle dans le paquet générique. Un flash remplace le firmware précédemment installé sur la carte.

## Premier test sans ATEM ni LED externe

1. Alimenter le master et un récepteur. Le récepteur est **caméra 1** par défaut.
2. Sur téléphone ou Mac, rejoindre le Wi-Fi **Tally-Setup-XX:XX**, mot de passe **tallysetup**. L'absence d'accès Internet est normale.
3. Ouvrir **http://192.168.4.1**, cocher **Mode démo**, puis enregistrer. Aucun SSID / IP n'est requis en démo.
4. Le master simule les caméras 1 et 2. Pour la caméra 1 : 2 s Program, 2 s Preview, 2 s repos, en boucle. La caméra 2 fait l'inverse pour Program et Preview.
5. La LED intégrée du récepteur indique les états dans le tableau ci-dessous. Elle est monochrome : elle ne devient pas rouge ou verte.
6. Couper le master : le récepteur doit quitter Program/Preview au bout de **1,8 s**, clignoter rapidement et chercher le master. Le rallumer : acquisition habituelle en quelques secondes.

## Connexion réelle à l'ATEM

1. Relier l'ATEM en Ethernet au routeur. Vérifier que son IP est accessible depuis le Wi-Fi, par exemple avec ATEM Software Control. Éviter le réseau invité et l'isolation des clients.
2. Sur un master configuré, maintenir **BOOT pendant 2 s après son démarrage** pour rouvrir le Wi-Fi de configuration. Ne pas maintenir BOOT pendant le reset, car cela ouvre le bootloader.
3. Rejoindre Tally-Setup, ouvrir http://192.168.4.1, renseigner le **SSID 2,4 GHz**, le mot de passe et l'IP ATEM. Décocher **Mode démo**, puis enregistrer.
4. Le master redémarre et se connecte au Wi-Fi. Son point d'accès de configuration est fermé lors de ce démarrage normal ; BOOT 2 s permet de le rouvrir.
5. Reconnecter le Mac au routeur. Mettre la caméra 1 en Preview puis en Program sur l'ATEM, tester un Cut et une transition Auto, puis tester une coupure réseau et le retour de connexion.

Le master suit le canal du point d'accès Wi-Fi. Les récepteurs parcourent automatiquement les canaux 1 à 13 lorsqu'ils n'entendent plus le master. Aucun réglage de canal ni appairage MAC n'est requis pour le test. Un canal fixe du routeur rend néanmoins le comportement plus prévisible. Le domaine radio du récepteur est CH (canaux 1–13), à adapter avant utilisation dans une autre région.

## Numéro de caméra et LED

Le numéro du récepteur correspond à l'entrée caméra ATEM : **1 → entrée 1**, **2 → entrée 2**, jusqu'à **40** selon le mélangeur.

- Pour passer à la caméra suivante : maintenir BOOT **1 s**, puis le relâcher. Une pression longue ne fait avancer que d'une caméra.
- Pour choisir directement : ouvrir le port série à **115200 bauds**, envoyer `cam 2` suivi d'Entrée. Le numéro reste mémorisé après extinction.
- Avec PlatformIO : `pio device monitor -b 115200`, puis saisir la commande. Fermer le moniteur avant de flasher.

| État | LED rouge externe | LED verte externe | LED intégrée GPIO15 |
|---|---|---|---|
| Program | Allumée | Éteinte | Fixe |
| Preview | Éteinte | Allumée | Clignote, période 500 ms |
| Repos, liaison valide | Éteinte | Éteinte | Un bref flash toutes les 2 s |
| Master présent, ATEM pas prêt | Éteinte | Éteinte | Deux brefs flashes |
| Master absent / recherche | Éteinte | Éteinte | Clignote rapidement, période 200 ms |

Sur le **master**, la LED intégrée fixe signifie que les données tally sont prêtes ; elle clignote sinon. Le mode démo est considéré prêt.

Câblage facultatif pour petites LED séparées :

```text
GPIO16 ── résistance 330 Ω ── anode LED rouge ; cathode → GND
GPIO17 ── résistance 330 Ω ── anode LED verte ; cathode → GND
```

Utiliser les numéros GPIO sérigraphiés sur la S2 Mini. Sorties logiques 3,3 V, actives à l'état haut. Pour un ruban WS2812, une LED RGB à anode commune ou une lampe de puissance, il faut adapter la sortie ; ne pas brancher une lampe de puissance directement sur les GPIO.

## Compilation et provenance

```sh
python3 -m pip install platformio==6.1.18
pio run
python3 package_firmware.py
```

Ouvrir ce dossier dans PlatformIO et choisir l'environnement `master` ou `receiver` pour téléverser directement les sources. Plateforme épinglée : **espressif32 6.12.0**, Arduino ESP32 **2.0.17**, cible **lolin_s2_mini**. USB CDC natif activé.

Test hôte du format radio, des limites, de la priorité Program et du timeout :

```sh
c++ -std=c++11 -Wall -Wextra -Werror -I include test/protocol.cpp -o /tmp/tally-protocol-test
/tmp/tally-protocol-test
```

Les sources du portage `S2Mini` sont sous GPL-3.0-or-later ; voir `LICENSE`. Le reste du dépôt amont conserve ses mentions d'origine.

- Fork de [jkowalk/esp-now-atem-tally](https://github.com/jkowalk/esp-now-atem-tally), commit `a249e27ba2c1440b566d36be04cf3faeb8ea2261`. Les dossiers historiques Controller/Receiver sont conservés pour référence ; **utiliser S2Mini pour les nouvelles cartes**. Le nouveau protocole radio n'est pas compatible avec leurs anciens firmwares.
- Les bibliothèques ATEMbase, ATEMmin et SkaarhojPgmspace de `S2Mini/lib` viennent de [AronHetLam/ATEM_tally_light_with_ESP8266](https://github.com/AronHetLam/ATEM_tally_light_with_ESP8266), commit `9aeb2b0931d7b9a70cce16d6f3318f685799068f`, à l'origine de Kasper Skårhøj. Elles sont reprises avec leurs licences et notices.
- [Documentation Wemos S2 Mini](https://www.wemos.cc/en/latest/s2/s2_mini.html).
- [Documentation Espressif ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s2/api-reference/network/esp_now.html) : Wi-Fi et ESP-NOW partagent le canal radio.

## Limites de cette première version

Compilation des deux firmwares et tests hôtes effectués. **Pas encore de validation physique sur S2 Mini ni sur l'ATEM de l'utilisateur** : la compatibilité exacte du modèle / firmware ATEM et les performances radio restent à tester. Aucune mesure de portée ou de latence réelle n'est revendiquée.

Une trame complète est envoyée sur changement d'état (échantillonnage toutes les 20 ms) et au moins toutes les 250 ms. Le broadcast ESP-NOW est non chiffré, sans accusé de réception applicatif. Le récepteur éteint ses sorties si aucune trame valide n'arrive pendant 1,8 s. Une perte de l'ATEM avec le master toujours actif est détectée par la bibliothèque ATEM, après environ 5 s sans paquet, puis signalée aux récepteurs. Ce délai est distinct du timeout ESP-NOW.

Un seul master du même groupe doit être actif à proximité. Pour isoler deux systèmes, changer `kGroup` dans `include/tally.h` des deux firmwares. Un filtre d'adresse MAC du master est disponible dans `src/receiver.cpp` ; il filtre la source mais n'apporte pas de chiffrement. La reprise après changement de canal nécessite un nouveau balayage, soit jusqu'à environ 4 s en conditions favorables, plus les délais de reconnexion Wi-Fi / ATEM.

La bibliothèque fournit jusqu'à 40 index de tally. Les modèles / firmwares qui ne transmettent pas une table `TlIn` compatible nécessiteront une adaptation. Les grandes configurations multi-M/E et les entrées spéciales ne sont pas validées par cette version.
