# Tally Atelier — préparer et flasher les cartes

Tally Atelier est une application locale en français, avec une interface dans le navigateur de l'ordinateur et un moteur Python qui écrit sur la carte USB sélectionnée. Elle prépare les réglages et utilise les firmwares déjà compilés du kit. Aucun hébergement ni compte n'est nécessaire.

## Ouvrir l'application

**Sur Mac :** double-cliquer sur **Tally Atelier.app**, en la laissant dans son dossier avec le reste du kit. Le lanceur **Lancer-Tally.command** ouvre la même application si besoin. Python 3 doit être installé ; les outils de flash sont installés automatiquement au premier lancement si nécessaire. Cette première installation nécessite Internet. Les lancements suivants et les flashes fonctionnent hors ligne.

**Sur Windows :** installer Python 3 avec son lanceur `py`, puis double-cliquer sur **Lancer-Tally.bat**. Ce lanceur est fourni ; la validation de cette version a été effectuée sur Mac.

L'application ouvre `http://127.0.0.1:8764` et reste accessible uniquement depuis cet ordinateur. Cliquer **Quitter** pour arrêter son moteur local ; le bouton est désactivé pendant une opération. Garder l'application ouverte pendant le flash. Réouvrir son lanceur ramène à la même application si elle est déjà démarrée.

Si macOS bloque le lancement d'une application locale non signée, le lanceur `.command` reste disponible. Aucun certificat ni installateur système n'est fourni.

## Préparer un master

1. Choisir **Master**.
2. Saisir le SSID **2,4 GHz**, le mot de passe Wi-Fi, l'adresse IPv4 de l'ATEM et le **nombre d'entrées caméra utilisées**, entre 1 et 40. Le mot de passe est requis, sauf si la case « Réseau ouvert, sans mot de passe » est explicitement cochée.
3. Brancher la S2 Mini destinée à devenir master et sélectionner son port USB.
4. Cliquer **Flasher le master**. Attendre la vérification et le redémarrage automatique. Si le port USB de la S2 Mini ne revient pas, couper puis rétablir son alimentation USB.

Les paramètres sont écrits dans la mémoire de configuration pendant le flash. Le master démarre directement avec ces réglages. Le champ « Entrées caméra utilisées » limite les entrées diffusées par le master : pour 8, seules les entrées 1 à 8 sont utilisées ; les flags 9 à 40 sont remis à zéro. Ce champ ne reconfigure pas la topologie ou les entrées du mélangeur Blackmagic. Une entrée supérieure au nombre configuré ne donnera pas de tally sur un récepteur.

Les anciennes cartes sans valeur `inputs` mémorisée conservent la limite historique de 40. Pour utiliser ce nouveau réglage sur le master, flasher la nouvelle version via l'application. La ligne série `CONFIG camera_inputs=...` permet de contrôler la valeur après démarrage.

## Préparer un récepteur / slave

1. Choisir **Récepteur**.
2. Choisir sa **caméra de 1 à 40**, avec les boutons rapides ou le champ numérique.
3. Choisir **Deux LED RGB WS2812** ou **LED intégrée / sorties rouge et verte**. Pour les WS2812, choisir l’ordre des couleurs RGB ou GRB et la luminosité. Les choix 25, 50, 75 et 100 % correspondent à 64, 130, 191 et 255 sur 255. Le réglage par défaut est 50 %.
4. Brancher la carte concernée, vérifier le port et cliquer **Flasher le récepteur**.
5. Attendre le redémarrage automatique après la vérification du flash.

La carte démarre avec le numéro exact choisi, mémorisé sous `tally-rx/camera`. Aucun mot de passe Wi-Fi n'est écrit sur un récepteur. L’intensité est écrite dans les réglages du récepteur au moment du flash. Les boutons BOOT et la commande série `cam N` restent disponibles pour changer de caméra plus tard. Pour les WS2812, suivre [le guide de câblage](LED-RGB.md).

## Vérifier et lire le résultat

**Vérifier la préparation sans flasher** contrôle les champs, les sommes SHA-256 des images et la génération de la configuration. Cette action fonctionne même sans carte connectée et n'écrit rien sur l'USB.

Le journal affiche la connexion, l’écriture et les vérifications effectuées par esptool. Le redémarrage watchdog est demandé dans la même session, après l’écriture. Atelier cherche le port USB de la S2 Mini en mode application, y compris si son nom a changé. Sur la C3, le port USB identique dans les deux modes permet de signaler l’envoi de la commande, mais la confirmation du démarrage se fait sur le voyant. Une opération à la fois est autorisée. La S2 Mini peut disparaître brièvement du port pendant son passage en mode téléchargement : l'application retente la connexion une fois, uniquement si aucune opération d'effacement ou d'écriture n'a commencé.

Si la carte ne répond pas : fermer les moniteurs série, maintenir BOOT, appuyer puis relâcher RST, relâcher BOOT, actualiser les ports et relancer. Un flash remplace le firmware et la partition de réglages de la carte sélectionnée. Les autres cartes alimentées ailleurs continuent à fonctionner.

## Données locales

Le navigateur mémorise les réglages non secrets du formulaire pour les prochains lancements. **Le mot de passe Wi-Fi n'est pas conservé dans ce stockage et n'est pas journalisé.** Il est envoyé uniquement au moteur local, placé dans la configuration temporaire puis écrit sur le master quand on clique sur Flash. Les fichiers temporaires sont supprimés après l'opération. Pour vérifier puis flasher, le champ du mot de passe reste rempli après la simple vérification ; il est vidé dans l'interface après lancement du flash.


## Développement et validation

```sh
python tally_app.py --simulate        # aucun appel au flash matériel
python tally_app.py --port 8764       # usage normal
python -m unittest discover -s test -p test_desktop.py -v
sh test/run_host_tests.sh
```

La préparation NVS repose sur le générateur officiel Espressif ESP-IDF **v4.4.7**, inclus dans `desktop/vendor/nvs_partition_gen.py`, licence Apache-2.0. Son ancien appel à `distutils.dir_util.mkpath` est remplacé par `pathlib.Path.mkdir` pour Python récent ; le format NVS n'est pas réimplémenté.

Tests : lecture des valeurs caméra et nombre d'entrées dans les binaires NVS générés, échappement CSV, validation des limites et du réseau, suppression des fichiers temporaires, absence du mot de passe dans le journal, refus des requêtes d'un autre site, opération unique et contrôle de la commande esptool sans exécuter de flash physique. Le parcours Master a été vérifié sans écriture ; le parcours Récepteur caméra 6 a été exercé en simulation dans l'interface, en formats étroit et ordinateur. Les trois firmwares compilent. **Aucune carte n'a été reflashée pour la création de l'application ; la première utilisation de ce nouveau parcours sur une carte reste à contrôler.**

## Profil Sony REC

Tally Atelier 1.3.0 permet de flasher une LOLIN C3 Mini pour écouter les notifications REC/STOP d’un Sony A7 IV en Bluetooth LE. La validation sur caméra reste à faire. [Guide Sony](SONY.md)
