# Écran d’information du master — version 1.6

Le firmware Master prend en charge un **SSD1306 I²C monochrome 128 × 64, 0,96 pouce**, à quatre broches. L’écran est facultatif et détecté automatiquement au démarrage, aux adresses **0x3C**, puis **0x3D**. Aucun menu ni réglage supplémentaire dans Tally Atelier.

## Câblage

Couper l’alimentation avant de câbler. Utiliser un module compatible **3,3 V**, en vérifiant sa fiche vendeur. Les modules génériques n’ont pas tous le même ordre de broches : suivre leurs inscriptions.

| Broche du module OLED | Broche de la S2 Mini du master |
|---|---|
| GND | GND |
| VCC / VDD | 3V3 |
| SDA | GPIO33 |
| SCL / SCK | GPIO35 |

Ces broches I²C sont celles de la variante Arduino LOLIN S2 Mini. Le bouton de luminosité reste entre **GPIO7 et GND**. Alimenter l’OLED en 3,3 V pour conserver des niveaux I²C compatibles avec l’ESP32-S2 ; ne pas tirer SDA ou SCL vers 5 V.

[Carte et schéma Wemos](https://docs.wemos.cc/en/latest/s2/s2_mini.html) · [Bibliothèque SSD1306 Adafruit](https://github.com/adafruit/Adafruit_SSD1306/tree/2.5.15)

## Les trois pages

| Page | Informations | Durée |
|---|---|---|
| 1 — ATEM | IP configurée, connecté/déconnecté, disponibilité du tally | 15 secondes |
| 2 — Wi-Fi | Nom du réseau configuré, connecté/déconnecté | 15 secondes |
| 3 — Luminosité LED | 25, 50, 75 ou 100 %, avec une barre de niveau | 15 secondes |

Le cycle repart ensuite à la page ATEM. En fonctionnement normal, le réseau configuré est celui rejoint par le master. Si le Wi-Fi est déconnecté, son nom reste visible et la page indique **DECONNECTE**. L’état ATEM « CONNECTE » nécessite la réception de son état tally initialisé, comme pour l’envoi aux récepteurs. En démo, l’écran indique explicitement **MODE DEMO**, sans annoncer une connexion réelle à l’ATEM.

Le nom du réseau tient sur deux lignes de 21 caractères maximum. La police est ASCII : les accents français courants sont simplifiés (`é` → `e`), les autres caractères non pris en charge deviennent `?`. Le mot de passe n’est jamais affiché. La valeur de luminosité est celle du master ; l’écran ne confirme pas sa réception par les récepteurs.

Les données de la page visible sont vérifiées toutes les 250 ms. Un clic de luminosité met donc à jour la page Luminosité sans attendre le tour suivant, si celle-ci est déjà visible. Le clic ne change pas de page.

## Installer et tester

1. Relancer **Tally Atelier 1.6.0**, puis flasher le profil **Master** avec les réglages habituels.
2. Brancher l’écran comme indiqué, puis alimenter le master.
3. Vérifier le cycle ATEM → Wi-Fi → Luminosité, avec 15 secondes par page.
4. Quand la page Luminosité est visible, appuyer sur le bouton et contrôler les quatre niveaux sur l’écran et sur un récepteur.
5. Vérifier que Programme et Preview continuent à fonctionner pendant le changement de page.

Si les récepteurs sont déjà en version 1.5 avec la luminosité reçue du master, **aucun nouveau flash des récepteurs n’est nécessaire** pour cet ajout OLED. Le protocole ESP-NOW et leur firmware restent identiques.

## Écran absent ou en panne

Sans réponse aux deux adresses au démarrage, le master continue normalement et n’effectue plus d’accès I²C. Une panne I²C pendant l’envoi d’un écran désactive l’affichage jusqu’au prochain redémarrage du master. Pour ajouter ou rebrancher un écran, couper l’alimentation avant le câblage, puis redémarrer.

Les messages USB permettent de distinguer `OLED absent`, l’adresse détectée et une erreur I²C. Une erreur d’affichage n’arrête pas le tally.

## Développement et validation

Bibliothèques épinglées pour le profil Master : Adafruit SSD1306 **2.5.15**, GFX **1.12.4**, BusIO **1.17.4**. PlatformIO les télécharge au premier build. Les binaires d’Atelier sont précompilés et ne nécessitent pas ce téléchargement.

L’affichage utilise un tampon de 1 024 octets. Le firmware le transmet à 400 kHz par transactions de 31 octets de données maximum, une transaction par passage dans la boucle. Il n’utilise pas de pause de 15 secondes ni le transfert complet bloquant `display()`. Le délai d’attente I²C est fixé à 5 ms et les accès cessent après une erreur.

Les tests hôtes vérifient la cadence et son débordement `millis()`, les statuts, les noms longs/UTF-8, les niveaux, les deux adresses, les transferts fractionnés et l’arrêt des accès après panne. Le build Master vérifie la compilation avec les vraies bibliothèques Adafruit. **Le câblage et l’affichage sur le module physique restent à valider sur le montage.**
