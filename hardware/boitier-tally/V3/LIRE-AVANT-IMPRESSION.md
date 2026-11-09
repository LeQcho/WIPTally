# Boîtier Tally V3 — 62 × 52 × 32 mm

L’ESP se visse directement sur deux plots du socle. Les deux supports LED et les deux diffuseurs descendent par le dessus et sont retenus une fois le couvercle fermé. **Aucune colle structurelle n’est nécessaire** : seul l’adhésif du ruban LED sert à le fixer sur ses supports.

Le corps mesure **62 × 52 × 32 mm**, soit 3 mm de moins sur la dimension de 65 mm de la V2. Les LED sont reculées de **2 mm par rapport aux diffuseurs**, en comparaison de la V2. Le dessous reste plat pour le scratch.

## Six pièces à imprimer

Les fichiers sont en millimètres, à importer à **100 %**, en conservant l’orientation fournie.

| Fichier dans `STL` | Quantité | Couleur |
|---|---:|---|
| `01_socle_noir_ESP_visse.stl` | 1 | Noir |
| `02_couvercle_noir_verrouillage.stl` | 1 | Noir |
| `03_support_LED_x2.stl` | 2 | Noir |
| `04_diffuseur_captif_blanc_04mm_x2.stl` | 2 | Blanc |

Chaque fichier `_x2` contient une seule pièce : la dupliquer dans le trancheur. Les fichiers `TEST` et `OPTION` ne sont pas à ajouter au boîtier final.

Prévoir **2 vis M2 × 6 à tête cylindrique ou bombée pour l’ESP**, et **2 vis M2 × 8 à tête fraisée pour le couvercle**. Les avant-trous de 1,7 mm prennent les vis dans le plastique. La carte doit reposer sur les plots avant de serrer ; ne pas la tirer vers le fond en forçant sur les vis.

## Essais avant l’impression complète

La V3 a passé les contrôles numériques ; elle n’a pas encore été validée par une impression et un montage physiques. Les coupons permettent de vérifier les interfaces sans réimprimer tout le socle.

1. **`TEST_fixation_ESP_et_USB.stl`** : poser la S2 Mini et vérifier simultanément les deux trous, l’appui de la carte et le branchement USB. Le coupon contient les deux plots réels et le côté USB du socle.
2. **`TEST_logement_diffuseur.stl` + `TEST_verrou_diffuseur.stl` + un diffuseur** : descendre le diffuseur par le haut de l’ouverture, avec son grand bord arrière côté intérieur, puis poser le petit verrou par-dessus. Ce verrou reproduit la partie correspondante du couvercle ; le maintenir en place à la main pour l’essai. Vérifier le coulissement et la retenue, sans déformer la membrane.
3. **`TEST_glissiere_LED.stl` + un support LED** : essayer une oreille du support dans la glissière depuis le haut. La fente fait 2,6 mm pour une oreille de 2 mm.

Le plan officiel donne un entraxe de **20,4 mm** et deux trous de **2 mm**, centrés à ±10,2 mm sur la largeur de la carte. Leur distance au bord opposé à l’USB n’est pas cotée : **3,3 mm ont été estimés graphiquement**, et restent à confirmer sur ta carte avec le coupon. Ne pas percer ni modifier la carte pour compenser un éventuel écart ; corriger le paramètre du modèle si nécessaire.

Source mécanique : [plan officiel Wemos S2 Mini V1.0.0](https://docs.wemos.cc/en/latest/_static/files/dim_s2_mini_v1.0.0.pdf), accessible depuis la [documentation Wemos](https://docs.wemos.cc/en/latest/s2/s2_mini.html).

## Impression

- Départ conseillé : PLA, buse 0,4 mm, couches 0,2 mm, 3 périmètres et 15–20 % de remplissage.
- **Socle** : fond sur le plateau, ouvertures en U vers le haut.
- **Couvercle** : face extérieure sur le plateau. Les languettes USB, les fermetures des fenêtres et les doigts de retenue LED montent vers le haut.
- **Supports LED** : dos plats sur le plateau, encoches vers le haut une fois montés.
- **Diffuseurs** : face lumineuse sur le plateau. La membrane centrale de **0,4 mm correspond à deux couches pleines de 0,2 mm**. Employer un PLA blanc laissant passer la lumière ; vérifier les deux couches dans l’aperçu du trancheur.
- Le diffuseur possède une **collerette arrière rigide de 1,6 mm**, qui déborde du col de 3 mm sur les côtés. Contrôler ce surplomb dans le trancheur : des supports localisés sous cette collerette peuvent être nécessaires. Ne pas en mettre sur la membrane centrale ni dans la gorge qui prend la paroi du socle. Ébavurer soigneusement les faces de coulissement.
- Vérifier aussi les petits surplombs des butées LED du socle. Une bordure peut aider les coupons étroits à tenir sur le plateau.
- `OPTION_diffuseur_captif_blanc_06mm_x2.stl` remplace les diffuseurs de 0,4 mm si une membrane plus épaisse est souhaitée. Elle est compatible avec le même logement ; sa transmission lumineuse dépend du filament.

## Pourquoi le diffuseur tient

Le bord extérieur du diffuseur et sa collerette intérieure prennent la paroi du socle entre eux. La pièce se monte **par coulissement vertical**, couvercle retiré ; elle ne se clipse pas en poussant depuis l’extérieur. Les butées inférieures la positionnent en bas. Les blocs du couvercle limitent sa remontée à environ **0,2 mm**.

La gorge entre les deux bords fait **2,25 mm pour une paroi de 2 mm**. Le col fait 28 mm pour une ouverture de 28,6 mm. Ces jeux sont nominaux et doivent être essayés sur l’imprimante. Aucune languette souple ne porte l’effort de retenue. Pour démonter, enlever le couvercle puis remonter le diffuseur par le haut.

## Câblage et montage

1. Présenter la S2 Mini sur le coupon, puis dans le socle : USB vers son ouverture, deux gros trous du côté opposé. Vérifier que les deux petits appuis côté USB tombent sous des zones libres du PCB et que les composants et soudures ne touchent pas le plastique. La hauteur nominale de la face inférieure de la carte est de 11 mm.
2. Préparer et souder les fils sur la carte **avant de la visser**, hors du boîtier. Les deux plots de fixation et les appuis côté USB restent libres de colle.
3. Coller, avec son propre adhésif, un segment d’un pixel sur chaque support LED. Centrer le pixel, LED orientée vers le diffuseur, encoches ouvertes vers le haut. Faire les soudures à l’extérieur en conservant le câblage et le sens DIN/DOUT déjà validés. Une zone d’environ 34 × 10 mm est prévue pour le segment ; contrôler la taille réelle du ruban et de ses pastilles.
4. Laisser assez de mou entre les trois éléments pour les poser séparément sur la table. Essayer l’électronique avant fermeture : bleu en attente du master, vert Preview, rouge Programme.
5. Descendre les deux diffuseurs depuis le haut des ouvertures en U, grand bord arrière vers l’intérieur, face fine vers l’extérieur, jusqu’aux butées basses.
6. Descendre les supports LED dans les glissières, LED vers les diffuseurs, jusqu’à leurs butées. Les encoches restent dégagées pour les fils. Les supports n’ont pas besoin de colle : les doigts du couvercle bloquent leur remontée.
7. Descendre l’ESP et son câblage en guidant l’USB dans l’ouverture en U. Visser la carte directement sur les deux plots avec les **M2 × 6**, sans serrage excessif.
8. Ranger le mou dans la **baie latérale de 16 × 26 × 22,5 mm**, du côté opposé à l’USB. Un volume central de **28 × 12 × 6 mm** reste aussi libre sous la carte. Un corridor de **6 × 36 × 6,5 mm** traverse le fond. Ces volumes ont été contrôlés sans obstruction dans le modèle nominal ; la place restante dépend de tes composants, soudures, fils et éventuels accessoires.
9. Garder les fils hors des doigts de retenue des LED, des blocs au-dessus des diffuseurs, de la languette USB et des vis. Poser le couvercle sans forcer, puis fermer avec les **M2 × 8 à tête fraisée**. Le couvercle assure la retenue mécanique des supports LED et des diffuseurs.
10. Mettre le scratch sous le fond plat.

## Dimensions et limites du modèle

- Corps noir : **62 × 52 × 32 mm**. Avec les deux diffuseurs de 0,4 mm : environ **62 × 55,2 × 32 mm**, hors USB et câble.
- S2 Mini nominale : 34,3 × 25,4 mm, PCB de 1,6 mm. Le volume violet représente le PCB avec ses trous, sans détails des composants.
- USB nominal : 9 × 3,3 mm, débord de 2,5 mm au bord de la carte. Sa saillie prévue est de **1 mm** hors du corps. Ouverture utile : **12,4 × 5,8 mm**. La forme réelle du câble et la position du connecteur restent à essayer.
- Écart entre la face des supports LED et l’intérieur de la membrane : **8,2 mm**, soit 2 mm de plus qu’en V2. Avec une épaisseur de ruban + pixel supposée à 2,5 mm, l’écart lumineux est d’environ **5,7 mm**. Cette épaisseur n’a pas été mesurée sur ton ruban.
- Cette version utilise ses propres socle, couvercle et diffuseurs. Ne pas mélanger les pièces avec celles de la V2.

## Fichiers et vérifications

- `STEP` et `STL` : mêmes pièces, STEP pour modification, STL orientés pour impression.
- `STEP/00_ASSEMBLAGE_reference_6_pieces.step` : montage complet de référence, **à ne pas imprimer en bloc**.
- `STEP/REFERENCE_electronique_non_imprimable.step` : montage avec enveloppes nominales de la carte, de l’USB et des rubans. Ces derniers ne sont pas des pièces imprimables.
- `apercu_geometrie_V3.png` : rendu des STL réels.
- `verification.json` : contrôles numériques détaillés.
- `sources` : CadQuery, paramètres et rendu VTK reproductibles.

Les STEP ont été relus et les STL vérifiés fermés, d’un seul tenant, orientés avec leur minimum à Z = 0. Les 15 paires de pièces imprimées ne se croisent pas en position finale, et les enveloppes nominales d’électronique restent dégagées. La descente des pièces a été vérifiée à plusieurs hauteurs, couvercle retiré. Des déplacements tests confirment qu’une extraction vers l’extérieur et une remontée des diffuseurs sont bloquées géométriquement, et que le couvercle bloque la remontée des supports LED. Les trois volumes réservés aux fils et les deux variantes de membrane sont vérifiés.

Ces contrôles portent sur la géométrie, pas sur la résistance du PLA, l’usure, l’effort de vissage, la diffusion lumineuse ou les tolérances d’impression. La tenue en service et l’ajustement sur ta carte se valident avec les impressions d’essai.
