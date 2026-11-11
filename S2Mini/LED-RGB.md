# Deux WS2812 5 V sur un récepteur S2 Mini

Le firmware `receiver-rgb` commande deux pixels RGB WS2812/NeoPixel en chaîne à partir de **GPIO18**. Les deux pixels affichent le même état. Les récepteurs 1.4 acceptent ESP-NOW Long Range et les trames du master standard.

| État | Les deux LED |
|---|---|
| Démarrage / attente ou perte du master | Bleu fixe |
| Caméra en Preview | Vert fixe |
| Caméra en Programme | Rouge fixe |
| Master reçu, ATEM pas prêt | Bleu clignotant, période 1 s |
| Caméra ni en Preview ni en Programme | Éteintes |

Programme reste prioritaire si les deux flags sont actifs. Une perte du master fait revenir au bleu au bout de 1,8 s. La détection d'une coupure ATEM par le master prend environ 5 s sans réponse. Dans Tally Atelier, choisir la luminosité de départ dans le profil **Master**. Son bouton sur GPIO7 permet ensuite de passer entre 25, 50, 75 et 100 %, soit 64, 130, 191 et 255 par canal. Le master transmet ce niveau aux récepteurs et le mémorise. [Câblage du bouton et mise à jour](BOUTON-LUMINOSITE.md).

## Matériel

- Deux modules/pixels **WS2812 RGB 5 V**, avec entrée DIN et sortie DOUT pour le premier, ou deux pixels découpés d'un ruban. Le firmware utilise 800 kHz. La valeur par défaut est désormais RGB pour corriger l’inversion rouge/vert observée sur tes LED. L’application permet aussi de choisir GRB pour les modules standard. Les modèles RGBW ne sont pas couverts.
- Un convertisseur de niveau **74AHCT125** alimenté en 5 V, ou un module équivalent adapté aux signaux NeoPixel 3,3 → 5 V.
- Une résistance **330 Ω** en série sur les données, près de DIN de la première LED.
- Un condensateur de **500 à 1000 µF, tension nominale ≥ 6,3 V**, entre 5 V et GND près des LED, en respectant sa polarité. Pour un circuit nu 74AHCT125, ajouter un découplage 100 nF à son alimentation.
- Une alimentation USB 5 V correcte pour le récepteur et les deux LED.

## Câblage hors tension

Pour le premier montage, alimenter la S2 Mini par son USB et prendre le 5 V sur sa broche **VBUS** (parfois notée 5V sur les cartes compatibles). Il s'agit du 5 V USB, pas du 3V3.

```text
S2 Mini GPIO18 ── entrée A du 74AHCT125
                      sortie Y ── 330 Ω ── DIN LED 1
                                         DOUT LED 1 ── DIN LED 2

VBUS / 5 V USB ──┬── VCC du 74AHCT125
                ├── +5V LED 1
                └── +5V LED 2

GND S2 Mini ────┬── GND du 74AHCT125
                ├── /OE du canal utilisé (activation à 0)
                ├── GND LED 1
                └── GND LED 2

Condensateur : + sur le rail 5 V, − sur GND, près des LED.
DOUT de la dernière LED : non connecté.
```

Les sorties GPIO de la S2 Mini sont en **3,3 V** : ne pas y appliquer le 5 V. Les deux LED partagent les rails 5 V/GND ; seul le signal de données se chaîne. Respecter le sens DIN → DOUT, habituellement indiqué par une flèche. Garder les fils de données courts pour le premier test. Sur un 74AHCT125 nu, ne pas laisser les entrées des canaux inutilisés flottantes ; les fixer à GND et désactiver ces canaux via leurs /OE à 5 V. Se référer au brochage du boîtier exact.

Ce schéma utilise une seule source 5 V USB commune. Ne pas rajouter une seconde alimentation 5 V sur VBUS en parallèle de l'USB du Mac.

## Flash et choix de caméra

Pour un flash avec les réglages déjà mémorisés, lancer **Flasher-RECEPTEUR-RGB.command** et attendre le redémarrage automatique. Le script écrit le firmware RGB en conservant le numéro de caméra déjà mémorisé. Une carte neuve est caméra 1.

En ligne de commande :

```sh
.flash-venv/bin/python flash.py receiver-rgb
```

Pour choisir caméra 2 directement : ouvrir le port série à 115200 bauds et envoyer `cam 2` suivi d'Entrée. Sinon, sur une carte actuellement caméra 1, maintenir BOOT une seconde après démarrage puis relâcher. Le choix est mémorisé. Les boutons et la LED monochrome intégrée gardent leurs fonctions précédentes.

## Premier contrôle

1. Alimenter le récepteur seul : les deux LED doivent être bleues.
2. Alimenter le master connecté à l'ATEM. Une caméra au repos donne les deux LED éteintes.
3. Passer cette entrée en Preview : les deux LED deviennent vertes ; en Programme : rouges.
4. Couper le master : les deux LED doivent revenir au bleu après environ 1,8 s.
5. Rallumer le master et vérifier le retour automatique au bon état.

Si rouge et vert sont inversés, choisir l’autre ordre RGB/GRB dans Tally Atelier puis reflasher le récepteur. Le code conserve toujours Programme = rouge et Preview = vert. Si une seule LED fonctionne, vérifier DOUT de la première vers DIN de la seconde et les alimentations. Un échec du test matériel ne doit pas être compensé par l'application de 5 V sur un GPIO.

## Validation et provenance

Les trois environnements `master`, `receiver` et `receiver-rgb` compilent sur LOLIN S2 Mini / Arduino ESP32 2.0.17. Les tests hôtes couvrent le mapping des couleurs depuis les vrais états tally, la priorité Programme, le timeout de perte du master, le clignotement ATEM hors ligne et le repos. **Le montage avec les deux WS2812 reste à valider physiquement.** Aucun firmware n'a été téléversé pendant cette préparation RGB.

La bibliothèque officielle **Adafruit NeoPixel 1.12.3**, commit `aa798ff5e9bb9d7299190627d2454ccd6f599ae8`, est incluse dans `lib/Adafruit_NeoPixel` avec ses notices et sa licence LGPLv3. Le backend ESP32 utilise le périphérique RMT.

Références : [câblage Adafruit](https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections), [alimentation et condensateur](https://learn.adafruit.com/adafruit-neopixel-uberguide/powering-neopixels), [schéma officiel Wemos S2 Mini](https://www.wemos.cc/en/latest/_static/files/sch_s2_mini_v1.0.0.pdf), [sources NeoPixel 1.12.3](https://github.com/adafruit/Adafruit_NeoPixel/tree/1.12.3).
