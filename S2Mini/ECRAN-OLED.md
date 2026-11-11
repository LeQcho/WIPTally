# Écran d’information du master — version 1.7

Le firmware Master prend en charge un **SSD1306 I²C monochrome 128 × 64, 0,96 pouce**, à quatre broches. L’écran est facultatif et détecté automatiquement au démarrage, aux adresses **0x3C**, puis **0x3D**. Aucun menu ni bouton supplémentaire sur l’écran. Les durées se règlent dans Tally Atelier avant de flasher le master.

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

## Les quatre pages

| Page | Informations | Durée |
|---|---|---|
| 1 — ATEM | IP configurée, connecté/déconnecté, disponibilité du tally | Réglable ; 5 s par défaut |
| 2 — Wi-Fi | Nom du réseau configuré, connecté/déconnecté | Réglable ; 5 s par défaut |
| 3 — Luminosité LED | 25, 50, 75 ou 100 %, avec une barre de niveau | Réglable ; 5 s par défaut |
| 4 — Récepteurs | Nombre de récepteurs ayant envoyé un retour de présence récent | Réglable ; 5 s par défaut |

Dans le profil **Master** de Tally Atelier 1.7, saisir séparément une durée entière de **1 à 120 secondes** pour chaque page. Exemple : ATEM 5 s, Wi-Fi 10 s, luminosité 5 s et récepteurs 5 s. Les champs sont mémorisés par le navigateur et sont écrits dans le master lors du flash. Modifier le formulaire seul ne change pas le master déjà programmé.

Les durées sont stockées en secondes dans `tally-master/oled_atem`, `oled_wifi`, `oled_led` et `oled_rx`. Une clé absente ou invalide utilise 5 secondes. Le flash direct conserve les valeurs existantes ; le flash Atelier les remplace par celles choisies dans le formulaire.

Le cycle repart ensuite à la page ATEM. En fonctionnement normal, le réseau configuré est celui rejoint par le master. Si le Wi-Fi est déconnecté, son nom reste visible et la page indique **DECONNECTE**. L’état ATEM « CONNECTE » nécessite la réception de son état tally initialisé, comme pour l’envoi aux récepteurs. En démo, l’écran indique explicitement **MODE DEMO**, sans annoncer une connexion réelle à l’ATEM.

Le nom du réseau tient sur deux lignes de 21 caractères maximum. La police est ASCII : les accents français courants sont simplifiés (`é` → `e`), les autres caractères non pris en charge deviennent `?`. Le mot de passe n’est jamais affiché. La valeur de luminosité est celle du master ; l’écran ne confirme pas sa réception par les récepteurs.

Les données de la page visible sont vérifiées toutes les 250 ms. Un clic de luminosité met donc à jour la page Luminosité sans attendre le tour suivant, si celle-ci est déjà visible. Le clic ne change pas de page.

## Installer et tester

1. Relancer **Tally Atelier 1.7.0**, choisir les quatre durées dans le profil **Master** et préparer les réglages habituels.
2. Flasher chaque **Récepteur RGB** ou **Récepteur classique** en 1.7, puis le **Master**.
3. Brancher l’écran comme indiqué, puis alimenter le master.
4. Vérifier le cycle ATEM → Wi-Fi → Luminosité → Récepteurs, avec les durées choisies.
5. Quand la page Luminosité est visible, appuyer sur le bouton et contrôler les quatre niveaux sur l’écran et sur un récepteur.
6. Vérifier que Programme et Preview continuent à fonctionner pendant le changement de page et que les LED internes des récepteurs restent éteintes.
7. Allumer deux récepteurs : ils doivent apparaître au compteur après acquisition du canal et leur premier retour. En éteindre un, attendre au moins 8 secondes puis vérifier sa disparition au prochain affichage de la page. Le rallumer et contrôler qu’il revient.
8. Redémarrer le master et vérifier la conservation des durées. Le compteur repart à zéro et se reconstruit avec les prochains retours.

## Compteur de récepteurs

**Le master et les récepteurs doivent être flashés en 1.7 pour le comptage.** Les récepteurs 1.5/1.6 continuent à recevoir les couleurs et la luminosité du nouveau master, mais n’envoient pas de présence et ne sont pas comptés. Les nouveaux récepteurs fonctionnent aussi avec les anciens masters compatibles ; ces derniers ignorent simplement le retour de présence.

Chaque récepteur qui entend le master envoie un petit message de 8 octets à son adresse MAC, toutes les 1,5 à 2 secondes. Un décalage propre à chaque démarrage étale les émissions. Le retour utilise Long Range 250 kbit/s ; si le pilote refuse ce débit, le journal indique la tentative de secours standard. Le récepteur cesse ces retours lorsqu’il ne reçoit plus les trames tally du master. L’état de l’ATEM n’intervient pas : une carte peut être active alors que l’ATEM est indisponible.

Le master compte les **adresses MAC distinctes**, jusqu’à 64 cartes, même si plusieurs suivent la même caméra. Sans retour valide depuis **8 secondes**, une carte expire. Le chiffre est donc une présence radio récente dans les deux sens, pas une connexion Wi-Fi ni une garantie sur chaque commande ou sur l’allumage des LED. En limite de portée, un récepteur peut entendre le master sans que son retour arrive : il ne sera alors plus compté.

La trame tally descendante de 55 octets reste inchangée. Les messages de présence ont un type distinct, valident le groupe et le numéro de caméra. Les retours sont mis en file dans le callback Wi-Fi puis traités dans la boucle principale. Un journal USB `RECEPTEURS actifs=...` permet aussi de contrôler le compteur. Si l’initialisation du comptage échoue, l’écran indique `-- / INDISPONIBLE` ; la diffusion tally reste active.

Sur les deux variantes de récepteur 1.7, **GPIO15 reste à LOW** pour éteindre la LED interne. Les WS2812, les sorties externes rouge/verte, le bouton BOOT et la sélection de caméra conservent leur fonctionnement.

## Écran absent ou en panne

Sans réponse aux deux adresses au démarrage, le master continue normalement et n’effectue plus d’accès I²C. Une panne I²C pendant l’envoi d’un écran désactive l’affichage jusqu’au prochain redémarrage du master. Pour ajouter ou rebrancher un écran, couper l’alimentation avant le câblage, puis redémarrer.

Les messages USB permettent de distinguer `OLED absent`, l’adresse détectée et une erreur I²C. Une erreur d’affichage n’arrête pas le tally.

## Développement et validation

Bibliothèques épinglées pour le profil Master : Adafruit SSD1306 **2.5.15**, GFX **1.12.4**, BusIO **1.17.4**. PlatformIO les télécharge au premier build. Les binaires d’Atelier sont précompilés et ne nécessitent pas ce téléchargement.

L’affichage utilise un tampon de 1 024 octets. Le firmware le transmet à 400 kHz par transactions de 31 octets de données maximum, une transaction par passage dans la boucle. Il n’utilise pas de pause entre pages ni le transfert complet bloquant `display()`. Le délai d’attente I²C est fixé à 5 ms et les accès cessent après une erreur.

Les tests hôtes vérifient les durées inégales et leur débordement `millis()`, les statuts, les noms longs/UTF-8, les niveaux, les deux adresses, les transferts fractionnés et l’arrêt des accès après panne. Ils couvrent aussi la validation des présences, les doublons MAC, plusieurs cartes sur une caméra, les expirations, la capacité, le retour après coupure et le rythme d’envoi. Les tests Atelier relisent les quatre durées dans le binaire NVS généré. Les trois firmwares ATEM sont compilés. L’affichage OLED 1.6 a été confirmé sur le montage par l’utilisateur ; **les nouvelles durées, le comptage radio et l’extinction des LED internes 1.7 restent à valider sur les cartes.**
