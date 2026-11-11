# Luminosité commandée depuis le master

Le master 1.5 règle la luminosité de tous ses récepteurs WS2812. Chaque pression sur le bouton fait avancer d’un niveau. Le cycle est **25 → 50 → 75 → 100 → 25 %**, avec les valeurs numériques **64, 130, 191 et 255 sur 255**.

Le dernier niveau est enregistré dans le master et revient après extinction. Les récepteurs reçoivent le niveau dans les trames tally, y compris dans les trames périodiques de 250 ms. Un récepteur qui démarre plus tard récupère donc le réglage courant. Les deux pixels utilisent ce niveau pour le rouge, le vert et le bleu.

## Brancher le bouton

Employer un **bouton poussoir momentané normalement ouvert**, qui revient au repos quand on le relâche. Sur la **S2 Mini du master**, câbler une borne sur **GPIO7** et l’autre sur **GND**.

```text
GPIO7 du master ───── bouton poussoir NO ───── GND du master
```

La résistance de rappel interne est activée par le firmware. Utiliser le numéro **GPIO7** du brochage de la carte pour identifier la pastille. Le bouton relie simplement l’entrée à la masse. Les fils peuvent passer vers un poussoir monté sur le boîtier.

L’entrée utilise un anti-rebond de 40 ms. Une pression maintenue compte pour un seul changement. Un bouton déjà maintenu pendant la mise sous tension doit être relâché avant la première pression prise en compte.

[Brochage de la S2 Mini](https://docs.wemos.cc/en/latest/s2/s2_mini.html) · [GPIO de l’ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s2/api-reference/peripherals/gpio.html)

## Configurer dans Tally Atelier

Dans le profil **Master**, choisir la **luminosité de départ des récepteurs** avant le flash. La valeur proposée est 50 %. Après installation, le bouton permet les changements en service. Lors d’un nouveau flash du master via Atelier, le niveau de départ choisi dans le formulaire remplace le niveau mémorisé.

Dans le profil **Récepteur RGB**, choisir sa caméra et l’ordre RGB/GRB. La luminosité suit le master. Avant la première trame, le récepteur utilise sa valeur locale de secours. Le niveau reçu est conservé en RAM pour l’affichage de recherche si la liaison est ensuite perdue. Les récepteurs écrivent leur numéro de caméra dans leur propre mémoire, et le master écrit le niveau lors d’un clic sur son bouton.

Les sorties à LED classiques fonctionnent en tout-ou-rien. Les quatre niveaux concernent les WS2812 du profil Récepteur RGB.

## Mettre à jour les cartes

1. Flasher les récepteurs avec la version 1.5 de leur profil.
2. Flasher le master en choisissant son niveau de départ.
3. Brancher le bouton et alimenter le système.
4. Mettre une caméra en Preview ou Programme et appuyer sur le bouton. Vérifier les quatre niveaux, puis le retour à 25 %.
5. Éteindre puis rallumer le master et vérifier le niveau conservé. Allumer ensuite un autre récepteur et vérifier qu’il récupère ce même niveau.

Le nouveau protocole version 2 ajoute un octet de luminosité et porte les trames de 54 à 55 octets. Les nouveaux récepteurs acceptent aussi les anciennes trames version 1 et utilisent alors leur luminosité locale. Les anciens récepteurs doivent être mis à jour pour lire les trames du nouveau master.

Le journal USB du master affiche le pourcentage et la valeur sur 255 lors d’un clic, puis les reprend dans ses lignes de configuration périodiques.

## Vérification

Les tests hôtes couvrent le cycle complet, les rebonds, le maintien du bouton, la mise sous tension avec bouton maintenu et le débordement de millis. Ils vérifient aussi le décodage des trames nouvelles et anciennes, les valeurs invalides et la couleur rendue quand la luminosité change sans changement d’état tally. Les tests de l’application vérifient les quatre valeurs enregistrées dans les réglages du master.

Le fonctionnement du bouton et la luminosité restent à contrôler sur le montage physique.
