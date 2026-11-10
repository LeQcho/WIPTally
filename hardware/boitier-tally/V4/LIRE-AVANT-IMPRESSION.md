# Tally V4 — diffuseurs plats vissés et couvercle à quatre vis

La V4 corrige les deux points signalés après l’impression de la V3 : les diffuseurs se cassent au montage et le couvercle se soulève avec les câbles. Le corps reste **62 × 52 × 32 mm**. Les fixations de l’ESP, les supports LED, leur recul et les volumes réservés aux fils sont repris de la V3.

Pour un master souvent mis à jour, le [socle optionnel avec passage BOOT](acces-boot/README.md) donne accès au bouton avec le couvercle en place.

## Ce qu’il faut réimprimer

**Le socle, le couvercle et les deux diffuseurs.** Les deux supports LED V3 peuvent être conservés. Les anciens diffuseurs ne sont pas compatibles avec cette nouvelle fixation ; les nouveaux ont deux vis chacun et se posent depuis l’extérieur.

Le dossier `STL` contient des pièces en millimètres, à importer à **100 %** en conservant l’orientation fournie. Les STEP équivalents sont dans `STEP`.

| Fichier STL | Quantité pour un boîtier complet | Impression |
|---|---:|---|
| `01_socle_noir_4_vis.stl` | 1 | Noir, fond sur le plateau |
| `02_couvercle_noir_4_vis.stl` | 1 | Noir, face extérieure sur le plateau |
| `03_support_LED_x2.stl` | 2 | Noir ; réutiliser ceux de la V3 si disponibles |
| `04_diffuseur_plat_blanc_04mm_x2.stl` | 2 | Blanc, grande face plane sur le plateau |

Chaque fichier `_x2` contient une seule pièce : la dupliquer dans le trancheur. Le boîtier complet comprend toujours **six pièces imprimées**.

## Vis nécessaires

- **4 vis M2 × 8 à tête fraisée** pour le couvercle, une à chaque coin.
- **2 vis M2 × 6 à tête cylindrique ou bombée** pour l’ESP, comme en V3.
- **4 vis M2 × 5 à tête cylindrique ou bombée**, diamètre de tête d’environ 4 mm, pour les diffuseurs : deux vis par pièce blanche. Les vis sont accessibles depuis l’extérieur.

Les avant-trous dans le plastique font 1,7 mm. Les trous traversants des diffuseurs font **2,8 mm**, soit un jeu radial nominal de **0,4 mm** autour d’une vis M2. La fixation ne dépend plus d’un emboîtement serré. Les têtes des vis doivent porter sur le bord épais, sans rondelle dépassant dans la membrane. Utiliser des vis de la longueur indiquée : la place derrière les diffuseurs est réservée aux LED et au câblage.

## Nouveau diffuseur : pourquoi cette forme

La pièce est une **plaque de 40 × 20 mm**, avec une membrane centrale de **0,4 mm** et un bord de **1,6 mm**. Le bord fait nominalement 6,5 mm de large sur les côtés et 3,5 mm en haut et en bas. Il reçoit les deux vis et les efforts de montage.

La double collerette, les surplombs et la gorge de la V3 ont été supprimés. La pièce s’imprime entièrement à plat ; chaque couche repose sur celle d’en dessous. Elle se pose contre la face du boîtier depuis l’extérieur et se visse sans flexion volontaire. Il n’y a ni languette à encliqueter, ni glissière à forcer.

La face lumineuse reste au même endroit qu’en V3 : à **8,2 mm de la face du support LED**, environ **5,7 mm du pixel** pour l’enveloppe ruban + pixel supposée à 2,5 mm. La position réelle du pixel dépend de ton ruban. L’ESP, le recul des LED et le volume pour les fils restent identiques.

## Comment imprimer les pièces blanches

1. **Conserver l’orientation des fichiers** : grande face plane contre le plateau, bord relevé vers le haut. Ne pas imprimer la plaque debout ni sur la tranche.
2. Base de départ : buse 0,4 mm, **première couche de 0,2 mm et couches suivantes de 0,2 mm**. Vérifier que la membrane de 0,4 mm donne bien **deux couches pleines** dans l’aperçu du trancheur. Un remplissage intérieur ne remplace pas ces deux couches pleines.
3. **Aucun support d’impression pour le diffuseur.** Le modèle ne contient pas de face inférieure suspendue. La bordure et les trous montent directement depuis le plateau.
4. Employer un PLA blanc laissant passer la lumière. Une surface de plateau texturée peut modifier l’aspect de diffusion ; la transmission exacte dépend du filament et de l’impression.
5. Laisser refroidir le plateau avant de décoller la pièce. Décoller par les **bords épais**, sans soulever la membrane avec un outil.

Une variante **`OPTION_diffuseur_plat_blanc_06mm_x2.stl`** est fournie avec une membrane de **0,6 mm, soit trois couches de 0,2 mm**, et exactement le même bord et les mêmes fixations. Pour le premier essai après la casse de la V3, cette variante permet de tester une membrane plus épaisse ; comparer ensuite sa luminosité avec la version de 0,4 mm. La résistance et la transmission doivent être constatées sur l’impression réelle.

## Essai court avant de réimprimer le socle

Imprimer **`TEST_fenetre_et_vis_diffuseur.stl`** en noir et **un diffuseur**. Le coupon reproduit la fenêtre et les deux logements de vis du vrai socle. Il est déjà orienté avec la face extérieure sur le plateau.

Poser le diffuseur depuis l’extérieur, aligner les deux trous et engager les deux M2 × 5 à la main avant de serrer. Serrer juste assez pour que le bord épais soit en appui. Le diffuseur ne doit pas avoir à se courber pour rejoindre le coupon. Si les trous semblent décalés ou si la pièce force, corriger l’impression ou les jeux ; ne pas chercher à compenser en serrant fort. Ce test vérifie le montage réel, que les contrôles numériques ne peuvent pas garantir.

Les coupons ESP/USB et glissière LED sont aussi inclus pour une nouvelle installation. Si ces éléments sont déjà validés sur ta V3, ils n’ont pas besoin d’être réessayés pour cette modification.

## Montage

1. Vérifier à blanc les fenêtres et les diffuseurs avec le coupon. Monter les deux diffuseurs depuis l’extérieur du nouveau socle avec **deux M2 × 5 chacun**. Engager les deux vis avant de serrer ; soutenir le bord épais de la plaque.
2. Reprendre les deux supports LED équipés et câblés de la V3. Les descendre dans leurs glissières, LED vers les diffuseurs, encoches vers le haut. Ils n’ont pas besoin de colle dans le socle : les doigts du couvercle les retiennent. Le ruban conserve son adhésif sur chaque support.
3. Reposer l’ESP avec l’USB dans son ouverture en U et les deux gros trous sur les plots. Utiliser les **M2 × 6** déjà prévues en V3 ; la carte doit reposer sur ses appuis avant serrage.
4. Ranger le mou dans la baie latérale de **16 × 26 × 22,5 mm** et, si nécessaire, dans le volume central de **28 × 12 × 6 mm** sous l’ESP. Le corridor de fond de **6 × 36 × 6,5 mm** reste dégagé dans le modèle nominal. Garder les fils hors du plan de joint, des vis et des doigts de retenue LED.
5. Poser le couvercle, languette du côté USB. Engager les **quatre M2 × 8** puis serrer progressivement en croix, à la main. Les quatre points répartissent le maintien du couvercle. Le câblage doit être rangé avant la fermeture, sans utiliser les vis pour écraser les fils.
6. Conserver le scratch sous le fond plat. Vérifier les deux LED avec les états déjà validés : bleu en attente du master, vert Preview et rouge Programme.

Les diffuseurs peuvent désormais être déposés depuis l’extérieur en retirant leurs deux vis, sans démonter l’ESP.

## Encombrement et compatibilité

- Corps noir : **62 × 52 × 32 mm**, identique à la V3.
- Avec les diffuseurs : environ **62 × 55,2 × 32 mm**, comme en V3, hors USB, câble et têtes de vis des diffuseurs. Les têtes rondes dépassent de leur propre hauteur.
- Position de l’ESP, dimensions de la carte de référence, USB, supports LED et position des LED : repris de la V3.
- Les fenêtres sont maintenant fermées sur le dessus : cela rétablit une paroi continue sous le couvercle. Les diffuseurs se montent par l’extérieur, et non par le dessus.
- Le nouveau socle et le nouveau couvercle se montent ensemble. Le socle V3 ne possède ni les deux nouveaux plots de couvercle ni les quatre logements de vis des diffuseurs.
- Le firmware et le câblage électronique ne sont pas modifiés.

## Vérifications et fichiers

`verification.json` détaille les contrôles : validité des solides STEP et relecture après export, STL fermés d’un seul tenant avec minimum à Z = 0, absence de collision entre les six pièces, dégagement des enveloppes électroniques nominales, accès des pièces, jeux des vis de diffuseur, retenue nominale par leurs têtes et volumes réservés aux fils. Les diffuseurs ont aussi été vérifiés sans face inférieure suspendue au-dessus du plateau.

Les tests portent sur la géométrie. Cette nouvelle V4 n’a pas encore été imprimée physiquement ; la résistance réelle, la qualité du vissage, les jeux obtenus et la diffusion lumineuse se vérifient sur le coupon et les impressions.

- `STEP/00_ASSEMBLAGE_reference_6_pieces.step` : montage de consultation ; ne pas imprimer les six pièces assemblées en bloc.
- `STEP/REFERENCE_electronique_non_imprimable.step` : montage avec enveloppes de carte, USB et ruban ; les références électroniques ne sont pas à imprimer.
- `STL/` : seules les pièces imprimables, variantes et coupons explicitement nommés.
- `apercu_geometrie_V4.png` : rendu des vrais modèles.
- `sources/` : script CadQuery, paramètres et rendu VTK.
