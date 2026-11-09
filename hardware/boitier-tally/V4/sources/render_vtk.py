"""Render the V4 meshes and lay out the GitHub illustrations."""
import argparse
from pathlib import Path

import vtk
from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--models", type=Path, default=ROOT / "rendus")
args = parser.parse_args()
OUTPUT = ROOT / "rendus"
OUTPUT.mkdir(exist_ok=True)

PAPER = (239, 243, 248)
INK = (18, 32, 47)
MUTED = (66, 85, 103)
ACCENT = (29, 98, 127)
BLACK = (.23, .27, .31)
CARRIER = (.37, .43, .48)
WHITE = (.97, .95, .88)
ZERO = (0, 0, 0)

def font(size, bold=False):
    name = "Arial Bold.ttf" if bold else "Arial.ttf"
    try:
        return ImageFont.truetype(f"/System/Library/Fonts/Supplemental/{name}", size)
    except OSError:
        return ImageFont.truetype("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf", size)

def mesh_actor(name, color, offset):
    reader = vtk.vtkSTLReader()
    reader.SetFileName(str(args.models / f"{name}.stl"))
    reader.Update()
    if not reader.GetOutput().GetNumberOfCells():
        raise ValueError(f"Maillage introuvable ou vide : {name}")
    normals = vtk.vtkPolyDataNormals()
    normals.SetInputConnection(reader.GetOutputPort())
    normals.SetFeatureAngle(30)
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(normals.GetOutputPort())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.SetPosition(*offset)
    material = actor.GetProperty()
    material.SetColor(*color)
    material.SetAmbient(.31)
    material.SetDiffuse(.69)
    material.SetSpecular(.14)
    material.SetSpecularPower(30)
    return actor

def render(name, items, eye, anchors=()):
    width, height = 1340, 900
    renderer = vtk.vtkRenderer()
    renderer.SetBackground(*(c / 255 for c in PAPER))
    for item in items:
        renderer.AddActor(mesh_actor(*item))
    camera = renderer.GetActiveCamera()
    camera.ParallelProjectionOn()
    camera.SetPosition(*eye)
    camera.SetViewUp(0, 0, 1)
    window = vtk.vtkRenderWindow()
    window.SetOffScreenRendering(1)
    window.SetSize(width, height)
    window.SetMultiSamples(8)
    window.AddRenderer(renderer)
    renderer.ResetCamera()
    camera.Zoom(1.08)
    window.Render()

    capture = vtk.vtkWindowToImageFilter()
    capture.SetInput(window)
    capture.ReadFrontBufferOff()
    capture.Update()
    writer = vtk.vtkPNGWriter()
    writer.SetFileName(str(OUTPUT / f"{name}.png"))
    writer.SetInputConnection(capture.GetOutputPort())
    writer.Write()

    with Image.open(OUTPUT / f"{name}.png") as saved:
        scene = saved.convert("RGB")
    difference = ImageChops.difference(scene, Image.new("RGB", scene.size, scene.getpixel((0, 0))))
    bounds = difference.getbbox()
    draw = ImageDraw.Draw(scene)
    for number, point, offset in anchors:
        renderer.SetWorldPoint(*point, 1)
        renderer.WorldToDisplay()
        x, y, _ = renderer.GetDisplayPoint()
        y = height - y
        label_x, label_y = x + offset[0], y + offset[1]
        draw.line((x, y, label_x, label_y), fill=ACCENT, width=3)
        draw.ellipse((x - 4, y - 4, x + 4, y + 4), fill=ACCENT)
        draw.ellipse((label_x - 24, label_y - 24, label_x + 24, label_y + 24), fill=ACCENT)
        draw.text((label_x, label_y), number, font=font(25, True), fill="white", anchor="mm")
        bounds = (min(bounds[0], int(label_x - 26)), min(bounds[1], int(label_y - 26)),
                  max(bounds[2], int(label_x + 26)), max(bounds[3], int(label_y + 26)))
    window.Finalize()
    x1, y1, x2, y2 = bounds
    return scene.crop((max(0, x1 - 22), max(0, y1 - 22), min(width, x2 + 22), min(height, y2 + 22)))

def put(board, scene, area):
    left, top, width, height = area
    scene = scene.copy()
    scene.thumbnail((width, height), Image.Resampling.LANCZOS)
    board.paste(scene, (left + (width - scene.width) // 2, top + (height - scene.height) // 2))

def start_sheet(kicker, title, subtitle):
    board = Image.new("RGB", (1680, 920), PAPER)
    draw = ImageDraw.Draw(board)
    draw.text((64, 49), kicker, fill=ACCENT, font=font(24, True))
    draw.text((62, 85), title, fill=INK, font=font(57, True))
    draw.text((64, 164), subtitle, fill=MUTED, font=font(27))
    draw.line((64, 220, 1616, 220), fill=(193, 205, 216), width=2)
    return board, draw

base = ("socle", BLACK, ZERO)
leds = [("support_LED_avant", CARRIER, ZERO), ("support_LED_arriere", CARRIER, ZERO)]
diffusers = [("diffuseur_avant", WHITE, ZERO), ("diffuseur_arriere", WHITE, ZERO)]
assembled = render("assemble", [base, ("couvercle", BLACK, ZERO), *diffusers], (-105, 127, 112))
exploded = render("eclate", [
    base, ("couvercle", BLACK, (0, 0, 43)),
    ("support_LED_avant", CARRIER, (0, 5, 17)),
    ("support_LED_arriere", CARRIER, (-15, -4, 18)),
    ("diffuseur_avant", WHITE, (-9, 17, 0)),
    ("diffuseur_arriere", WHITE, (-32, -17, 4)),
], (-98, 134, 93), [
    ("1", (-25, 18, 9), (106, 30)),
    ("2", (11, -4, 75), (112, -32)),
    ("3", (7, 22, 39), (-136, -25)),
    ("4", (-11, 44.6, 17), (-115, 35)),
])
inside = render("interieur", [base, *leds,
    ("carte_nominale", (.37, .29, .62), ZERO),
    ("USB_nominal", (.64, .68, .72), ZERO),
    ("ruban_LED_avant_nominal", (.67, .57, .24), ZERO),
    ("ruban_LED_arriere_nominal", (.67, .57, .24), ZERO),
], (-54, 65, 150))
flat = render("diffuseur_plat", [("diffuseur_a_imprimer", WHITE, ZERO)], (42, -78, 95))

sheet, draw = start_sheet("WIPTALLY   /   BOÎTIER V4", "Tally ATEM · LOLIN S2 Mini", "Le boîtier imprimé et ses pièces de montage")
put(sheet, assembled, (52, 240, 930, 620))
draw.line((1010, 272, 1010, 835), fill=(193, 205, 216), width=2)
for y, heading, description in [
    (288, "62 × 52 × 32 mm", "Corps noir, ESP vissé dans le socle"),
    (442, "4 vis au couvercle", "M2 × 8, une vis à chaque coin"),
    (596, "2 diffuseurs vissés", "Membrane de 0,4 ou 0,6 mm"),
]:
    draw.text((1060, y), heading, fill=INK, font=font(38, True))
    draw.text((1060, y + 62), description, fill=MUTED, font=font(24))
draw.text((1060, 772), "Rendu des modèles de CAO", fill=ACCENT, font=font(23, True))
sheet.save(ROOT / "apercu_geometrie_V4.png")

sheet, draw = start_sheet("WIPTALLY   /   MONTAGE", "Vue éclatée du boîtier", "Six pièces imprimées et leurs fixations")
put(sheet, exploded, (36, 233, 1080, 650))
for i, (heading, description) in enumerate([
    ("Socle", "1 pièce avec les plots ESP"),
    ("Couvercle", "1 pièce, 4 vis M2 × 8"),
    ("Supports LED", "2 pièces, réutilisables depuis la V3"),
    ("Diffuseurs", "2 pièces, 2 vis M2 × 5 chacun"),
], 1):
    y = 280 + 143 * (i - 1)
    draw.ellipse((1120, y, 1164, y + 44), fill=ACCENT)
    draw.text((1142, y + 22), str(i), anchor="mm", font=font(25, True), fill="white")
    draw.text((1182, y - 1), heading, fill=INK, font=font(30, True))
    draw.text((1182, y + 45), description, fill=MUTED, font=font(22))
sheet.save(OUTPUT / "planche_montage.png")

sheet, draw = start_sheet("WIPTALLY   /   IMPRESSION", "Intérieur et diffuseur", "Préparer le câblage et les pièces blanches")
draw.text((64, 256), "ESP et passages de fils", fill=INK, font=font(34, True))
draw.text((884, 256), "Diffuseur à plat", fill=INK, font=font(34, True))
put(sheet, inside, (60, 310, 720, 475))
put(sheet, flat, (860, 310, 720, 475))
draw.text((64, 805), "Baie libre de 16 × 26 × 22,5 mm", fill=MUTED, font=font(25))
draw.text((884, 805), "Bord de 1,6 mm et deux trous de 2,8 mm", fill=MUTED, font=font(25))
draw.text((64, 850), "Carte et rubans représentés par leurs enveloppes", fill=MUTED, font=font(21))
draw.text((884, 850), "Face plane au plateau, sans supports", fill=MUTED, font=font(21))
sheet.save(OUTPUT / "planche_impression.png")
