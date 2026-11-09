# Tally Sony REC — prototype 1.3.0

L’onglet **Tally Sony** prépare et flashe un voyant d’enregistrement pour Sony A7 IV, en Bluetooth LE. Les profils ATEM restent disponibles pour les S2 Mini. Le firmware Sony est compilé pour **Wemos LOLIN C3 Mini, ESP32-C3, flash 4 Mo**. Il n’a pas encore été validé sur une caméra physique. Il ne certifie pas l’intégrité des fichiers enregistrés.

## Matériel et câblage

La **S2 Mini n’a pas de Bluetooth**. Ce profil nécessite une C3 Mini. Il ne convient pas à toutes les cartes portant le nom ESP32. Le format nominal de la LOLIN C3 Mini est de 34,3 × 25,4 mm comme la S2 Mini ; vérifier le connecteur USB et les composants avant de réutiliser le boîtier imprimé.

Pour les deux LED WS2812 actuelles : alimentation adaptée au ruban, masse commune, DATA vers **GPIO4** par défaut. Le GPIO, le nombre de pixels et l’ordre RGB/GRB sont réglables dans Atelier. Une résistance série de 330–470 Ω et un adaptateur de niveau logique si nécessaire restent recommandés pour DATA. **Ne pas conserver GPIO18 : il appartient à l’USB de la C3.** Respecter le sens DIN des LED. Ce profil pilote le ruban externe et pas la LED intégrée de la C3 Mini.

## Préparer et appairer

1. Ouvrir Tally Atelier, puis **Tally Sony**.
2. Sélectionner LOLIN C3 Mini. Le nom Bluetooth par défaut est `ILCE-7M4`.
3. Pour plusieurs Sony du même modèle, saisir l’adresse Bluetooth du boîtier cible (menu d’affichage de l’adresse sur le Sony). Sinon, n’allumer qu’une caméra au premier appairage. Le firmware refuse de choisir parmi plusieurs candidats visibles de même nom, mais une adresse explicite est préférable en multicam.
4. Choisir les LED, brancher la C3 Mini et flasher. Le contrôle du type de puce par esptool empêche d’écrire un binaire C3 sur une S2 ; aucun mode `--force` n’est utilisé.
5. Sur le Sony, activer Bluetooth **et** la commande à distance Bluetooth, puis ouvrir Appairage. Démarrer le tally et confirmer la demande sur la caméra. Les intitulés des menus peuvent varier avec le firmware Sony.
6. Démarrer puis arrêter un enregistrement avec le bouton de la caméra. Vérifier que le voyant suit réellement ces changements.

Après un appairage réussi, l’adresse du Sony est mémorisée et le tally recherche cette caméra aux prochains démarrages. Maintenir BOOT **4 secondes après le démarrage** pour effacer les liens côté tally. Si nécessaire, supprimer également l’ancien périphérique dans le menu Sony. Un nouveau flash avec réglages réinitialise le NVS, donc l’appairage.

## Voyants

| Couleur | Signification |
|---|---|
| Rouge fixe | Dernier état reçu : enregistrement démarré |
| Éteint | Dernier état reçu : enregistrement arrêté |
| Bleu clignotant | Recherche, perte de liaison ou état REC inconnu |

Une connexion Bluetooth seule n’est jamais interprétée comme un arrêt de REC. Après reconnexion, le tally attend une nouvelle notification REC/STOP. Si aucun état initial n’est envoyé, il reste bleu jusqu’au prochain changement REC reçu.

Le firmware s’abonne à la caractéristique Sony `FF02` du service `8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF`. Seuls les messages de trois octets `02 D5 20` (REC) et `02 D5 00` (STOP) sont interprétés. Il ignore les messages de mise au point et d’obturateur. Il n’envoie **aucune commande REC**, ni aucune écriture vers `FF01`.

Le protocole ne fournit ici aucun battement périodique certifiant REC. Si la liaison reste établie mais qu’un message d’arrêt manque, la dernière indication peut rester rouge. La fiabilité des notifications, les arrêts imprévus et les comportements après reconnexion doivent être éprouvés sur le Sony utilisé avant toute confiance en tournage. Ce prototype ne remplace pas la vérification à l’écran et des fichiers.

## Validation sur caméra à effectuer

- REC/STOP déclenchés sur le Sony, plusieurs cycles ; filmer les LED et l’écran pour mesurer le délai.
- Démarrage du tally pendant un REC déjà en cours ; l’état doit être confirmé ou rester bleu.
- Arrêt effectif pour limite, surchauffe ou absence d’espace, selon les conditions de test disponibles.
- Extinction ou éloignement de la caméra pendant REC : le rouge doit disparaître après détection de perte BLE.
- Redémarrage caméra/tally et reconnexion au même boîtier.
- Deux Sony proches : vérifier le verrouillage sur l’adresse voulue.
- Vérifier les interactions avec les autres télécommandes et applications Sony utilisées sur le tournage.

## Sources et compilation

- [Sony : voyant de la télécommande pendant REC](https://helpguide.sony.net/ilc/1820/v1/en/contents/TP0002240387.html)
- [Démonstration ESP32 / A7 IV](https://github.com/dzwiedziu-nkg/esp32-a7iv-rc) : preuve de connexion, pas validation de ce firmware.
- [Freemote : messages FF02 Sony](https://github.com/coral/freemote#answers-from-camera-remotecommand-0xff02) : protocole issu de rétro-ingénierie.
- [LOLIN C3 Mini : caractéristiques officielles](https://docs.wemos.cc/en/latest/c3/c3_mini.html)

Compilation : `pio run -e sony-c3`, puis `python package_firmware.py --roles sony-c3`. Dépendances : espressif32 6.12.0, Arduino 2.0.17, NimBLE-Arduino 1.4.3, NeoPixel 1.12.3 inclus dans le kit. Les binaires précompilés évitent d’installer le compilateur sur la machine servant au flash.
