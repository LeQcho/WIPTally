# Sources V4

Unités : millimètres. Python 3.12. Depuis le dossier V4 :

```sh
python -m venv .venv
.venv/bin/pip install -r sources/requirements.txt
.venv/bin/python sources/generer.py
.venv/bin/python sources/render_vtk.py
```

La V4 conserve les dimensions et l’implantation ESP/LED de la V3. Le couvercle est fixé par quatre M2 × 8. Chaque diffuseur est une plaque plane avec membrane centrale de 0,4 mm (variante 0,6 mm), cadre de 1,6 mm et deux trous traversants de 2,8 mm pour M2 × 5 à tête ronde. Les fenêtres sont fermées en haut. Les double collerettes et blocs de retenue des diffuseurs de V3 sont supprimés.

Les paramètres principaux sont dans `parametres.json` ; certains détails sont définis directement dans `generer.py`. Le script régénère les STEP/STL et le rapport de vérification. Les coordonnées des plots ESP sont reprises de la V3 testée par l’utilisateur ; l’estimation initiale de leur cote longitudinale reste décrite dans les paramètres historiques de fixation.

Les vérifications de retenue des diffuseurs utilisent une enveloppe nominale de tête M2 de 4 mm de diamètre et un corps de vis de 2 mm, long de 5 mm sous tête. Le corps coupe volontairement les avant-trous de 1,7 mm du socle, puisque les vis prennent dans le plastique ; cette interférence de filetage n’est pas interprétée comme une collision interdite. Les vis ne sont pas fournies comme pièces imprimables ni incorporées aux six solides du montage de référence.

Le rendu nécessite VTK et les polices Arial de macOS. Adapter les chemins de polices sur une autre plateforme. Les STL de `rendus` sont positionnés pour les images ; utiliser ceux de `STL` pour imprimer.
