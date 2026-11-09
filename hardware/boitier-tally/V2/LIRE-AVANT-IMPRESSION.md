# Boîtier Tally V2 — 65 × 52 × 32 mm

Cette V2 privilégie le câblage hors du boîtier : l’ESP et les deux morceaux de ruban LED se collent sur des supports amovibles. Les supports se descendent ensuite par le dessus. Le passage USB est ouvert en haut, puis refermé par une languette du couvercle. Le dessous reste plat pour le scratch.

Les fichiers sont en **millimètres, échelle 100 %**. Le dossier `STL` contient les pièces orientées pour l’impression. Le dossier `STEP` contient leurs équivalents modifiables et un assemblage complet de référence.

## À imprimer pour un boîtier : 7 pièces

| Fichier dans `STL` | Quantité | Matière / couleur |
|---|---:|---|
| `01_socle_noir.stl` | 1 | PLA noir |
| `02_couvercle_noir.stl` | 1 | PLA noir |
| `03_support_ESP_a_coller.stl` | 1 | PLA noir |
| `04_support_LED_a_coller_x2.stl` | 2 | PLA noir |
| `05_diffuseur_blanc_04mm_x2.stl` | 2 | PLA blanc |

Le suffixe **x2** indique la quantité à dupliquer dans le trancheur : chaque fichier ne contient qu’une pièce.

Quincaillerie prévue : **2 vis M2 × 8 à tête fraisée** pour le couvercle et **2 vis M2 × 6** pour le support ESP. Les avant-trous de 1,7 mm sont prévus pour visser dans le plastique, sans insert. Contrôler l’effort de vissage et ne pas forcer. Éviter des vis plus longues qui risquent d’atteindre le fond.

## Impression

- Conserver les orientations fournies : fond du socle sur le plateau ; face extérieure du couvercle sur le plateau ; dos plats des supports sur le plateau ; face lumineuse des diffuseurs sur le plateau, clips vers le haut.
- Base de départ : buse 0,4 mm, couches de 0,2 mm, 3 périmètres et 15–20 % de remplissage. Vérifier l’aperçu du trancheur autour des encoches et des petites butées des glissières ; ces petits surplombs peuvent demander un ajustement selon l’imprimante.
- Les diffuseurs ont une **membrane centrale de 0,4 mm**, soit **deux couches de 0,2 mm**. Utiliser du PLA blanc peu opaque et vérifier que ces deux couches sont pleines. La tenue mécanique est assurée par le pourtour et les languettes, pas par la membrane seule.
- Une variante `OPTION_diffuseur_blanc_06mm_x2.stl` est fournie si la membrane de 0,4 mm est trop fragile. Elle remplace les diffuseurs de 0,4 mm et laisse généralement moins passer la lumière ; la transmission dépend du filament.
- Éviter les supports d’impression dans les glissières et sous les languettes des diffuseurs : ils peuvent bloquer leur fonctionnement. Une bordure peut aider les petites éprouvettes étroites à adhérer.

## Faire d’abord les petits essais

La géométrie a été vérifiée numériquement ; **cette V2 n’a pas encore été imprimée et assemblée physiquement**. Les jeux réels dépendent de l’imprimante, du pied d’éléphant, des vis et de tes éléments électroniques.

1. `TEST_logement_diffuseur.stl` + un diffuseur : vérifier que la collerette entre et que les deux languettes retiennent la pièce sans plier sa face. Soutenir le pourtour, pas la membrane. Si ça force, arrêter et ajuster le jeu ; ne pas enfoncer de force.
2. `TEST_glissiere_LED.stl` + un support LED : présenter une de ses oreilles dans la glissière, depuis le haut. Le passage nominal est de 2,6 mm pour une oreille de 2 mm ; le jeu latéral extérieur est de 0,3 mm.
3. `TEST_entree_USB.stl` + le support ESP : vérifier à blanc la position de la carte, du connecteur et de ton câble USB avant tout collage définitif. Cette éprouvette ne reproduit que le côté USB du socle.

Ces pièces `TEST` ne font pas partie des 7 pièces du boîtier final.

## Montage et câblage

1. **Présenter l’ESP sur son support hors du boîtier.** Le connecteur USB se place du côté sans les deux trous de vis. Les quatre petits plots servent de zones de collage ; ils ne doivent toucher que des zones libres sous la carte. Vérifier les composants et les soudures présents sur ta carte avant de coller. Employer un adhésif isolant adapté à l’électronique, en petite quantité, et laisser les broches accessibles.
2. **Coller un morceau de ruban avec une seule LED sur chacun des deux supports LED.** La face adhésive du ruban va sur la face large opposée au dos d’impression ; la LED regarde vers le diffuseur. Centrer le pixel. La zone prévue convient à un segment d’environ 10 mm de large et jusqu’à 34 mm de long, sous réserve de la position de ses pastilles. Les deux encoches doivent rester vers le haut et disponibles pour le passage des fils. Vérifier les marquages 5V, GND et DIN/DOUT du ruban et conserver le câblage électronique déjà validé.
3. **Souder et essayer l’ensemble à l’extérieur.** Prévoir du mou entre les trois supports pour pouvoir les poser séparément sur la table, puis ranger ce mou dans le boîtier. Vérifier bleu en attente du master, vert en Preview et rouge en Programme avec le firmware déjà fonctionnel.
4. Descendre les deux supports LED par le haut dans leurs glissières, jusqu’aux butées, LED vers l’extérieur. Ne pas emprisonner les fils entre une oreille et la glissière.
5. Descendre le support ESP et sa carte, en guidant l’USB dans l’ouverture en U. Fixer le support avec les deux M2 × 6. Les deux logements de vis sont à l’opposé du connecteur.
6. Ranger les fils et le mou dans la baie libre, côté opposé à l’USB, et dans l’ouverture du support ESP. Un corridor de **6 mm de large × 6,5 mm de haut sur 36 mm de long** a été conservé dans le modèle à droite du support. Garder les fils hors des glissières, des vis et de la trajectoire de la languette USB du couvercle.
7. Clipser les deux diffuseurs depuis l’extérieur, **languettes vers le haut**. Ils ne nécessitent pas de colle ; ils restent ainsi démontables. Si nécessaire, soutenir leurs languettes depuis l’intérieur pour les déposer.
8. Poser le couvercle, sa languette du côté USB, sans pincer de fil. Fixer avec les deux M2 × 8 à tête fraisée. Coller le scratch sous le fond plat.

## Encombrement et hypothèses à contrôler

- **Corps noir fermé : 65 × 52 × 32 mm.** Avec les deux diffuseurs de 0,4 mm : environ **65 × 55,2 × 32 mm**, hors USB et câble.
- Carte nominale : **34,3 × 25,4 mm**, circuit imprimé de **1,6 mm**, face inférieure à 11 mm du dessous du boîtier. Les composants de la carte ne sont pas modélisés dans le volume violet de l’aperçu.
- USB nominal : largeur 9 mm, hauteur 3,3 mm, débord de 2,5 mm par rapport au bord de carte. Il dépasse alors d’environ **1 mm du corps**. L’ouverture utile fait **12,4 × 5,8 mm** ; une tête de câble très volumineuse peut demander un ajustement.
- L’épaisseur de colle, les composants sous la carte, les soudures et la gaine du ruban ne sont pas mesurés. Contrôler leurs encombrements avec les éprouvettes avant de lancer toutes les pièces. Coller les supports ne doit pas relever la carte au point de décaler son USB.
- Diffuseurs et supports V2 : ne pas mélanger avec les pièces V1.

## Contenu du dossier

- `STEP/00_ASSEMBLAGE_ne_pas_imprimer_en_bloc.step` : les 7 pièces positionnées pour consulter le montage ; **ne pas imprimer l’ensemble fusionné**.
- `STEP/REFERENCE_carte_USB_non_imprimable.step` : volumes nominaux carte et USB pour vérifier le placement. Ce sont des références de conception, pas des pièces à imprimer.
- `STL/` : seules les pièces imprimables, dont les variantes et éprouvettes nommées explicitement.
- `apercu_geometrie_V2.png` : vues calculées à partir des vrais STL.
- `verification.json` : résultats des contrôles de géométrie.
- `sources/` : script CadQuery, paramètres et script de rendu.

Chaque pièce possède un solide STEP valide, relu après export, et un STL fermé d’un seul tenant. Les 21 paires de pièces du montage ne présentent pas d’intersection volumique. Les enveloppes nominales de la carte et du connecteur ne croisent aucune pièce imprimée. La descente verticale des supports a été contrôlée à plusieurs hauteurs, ainsi que le corridor de câblage et les épaisseurs centrales de 0,4 et 0,6 mm. Ces contrôles ne simulent ni la souplesse des clips, ni la résistance du PLA, ni la diffusion lumineuse, ni les tolérances de fabrication.
