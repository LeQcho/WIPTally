# Tally Atelier 1.6.0 — démarrer ici

Double-cliquer sur **Tally Atelier.app** (Mac), **Lancer-Tally.command** (Terminal Mac) ou **Lancer-Tally.bat** (Windows). Python 3 et Internet sont nécessaires au premier lancement pour installer les dépendances de flash ; les firmwares sont déjà compilés.

## Choisir le profil

- **Master** : S2 Mini, connexion Wi-Fi à l’ATEM. Renseigner le réseau, son mot de passe, l’adresse ATEM et la luminosité de départ.
- **Récepteur** : S2 Mini, choisir le numéro de caméra et les LED. Liaison ESP-NOW au master.
- **Tally Sony** : **LOLIN C3 Mini**, voyant REC autonome en Bluetooth LE pour Sony A7 IV. La S2 Mini n’a pas de Bluetooth et ne convient pas à ce profil.

Renseigner les réglages, brancher une seule carte compatible, puis utiliser **Vérifier la préparation sans flasher** ou le bouton de flash. Le type de puce est vérifié avant écriture. Après vérification, la carte reçoit sa commande de redémarrage automatique. Les profils Sony et ATEM ont des binaires distincts.

Le profil Sony est un **prototype à valider sur caméra**. Voir [SONY.md](SONY.md) pour le câblage GPIO4, l’appairage et les essais REC/STOP. Il indique rouge pour REC confirmé, éteint pour arrêt confirmé, bleu clignotant pour état inconnu ou liaison perdue.

Ce paquet ne contient aucune partition de réglages Wi-Fi personnels. Utiliser l’interface pour préparer les cartes. Les anciens scripts de flash ATEM restent disponibles pour les usages avancés ; sans partition personnelle, ils conservent les réglages NVS existants.

[Guide de l’application](APPLICATION.md) · [Guide ATEM](README.fr.md) · [LED RGB ATEM](LED-RGB.md) · [Sony REC](SONY.md)

Le [bouton de luminosité du master](BOUTON-LUMINOSITE.md) fait passer tous les récepteurs WS2812 entre 25, 50, 75 et 100 %. Pour la mise à jour 1.5, flasher les récepteurs avant le master.

La version **1.6** permet d’ajouter un [OLED SSD1306 au master](ECRAN-OLED.md), sans configuration supplémentaire. Il affiche trois pages de 15 secondes : ATEM, Wi-Fi et luminosité. Mettre à jour uniquement le master si les récepteurs sont déjà en 1.5.
