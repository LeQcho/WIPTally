"""Tally V4: four lid screws, flat screw-retained diffusers, V3 electronics layout.
CadQuery 2.6.1. mm. No electronics/firmware changes. No mechanical tests implied.
"""
from pathlib import Path
import itertools,json
import cadquery as cq
import trimesh

ROOT=Path(__file__).resolve().parent.parent
P=json.loads((ROOT/'sources/parametres.json').read_text())
for folder in ['STEP','STL','rendus']:(ROOT/folder).mkdir(exist_ok=True)
W,D,H,B=P['width'],P['depth'],P['height'],P['base_height']
T,F,Z=P['wall'],P['floor'],P['led_center_z']
LY=P['led_plane_y']
SCREWS=[(sx*(W/2-5),sy*(D/2-5.3)) for sx in [-1,1] for sy in [-1,1]]
HX=P['pcb_xmin']+P['pcb_length']-P['pcb_hole_end_offset_estimated']
HOLES=[(HX,sy*P['pcb_hole_spacing']/2) for sy in [-1,1]]

def box(x1,x2,y1,y2,z1,z2):
    return cq.Workplane('XY').box(x2-x1,y2-y1,z2-z1).translate(((x1+x2)/2,(y1+y2)/2,(z1+z2)/2))

def rr(w,h,r,z):return cq.Workplane('XY').rect(w,h).extrude(z).edges('|Z').fillet(r)

def mirror_back(s):return s.rotate((0,0,0),(0,0,1),180)

def diffuse_place(s,t=.4,back=False):
    s=s.rotate((0,0,0),(1,0,0),90).translate((0,P['diffuser_front_y'],Z))
    return mirror_back(s) if back else s

def carrier_place(s,back=False):
    s=s.rotate((0,0,0),(1,0,0),90)
    if not back:s=mirror_back(s)
    return s.translate((0,(-1 if back else 1)*LY,Z))

def opening():
    # Closed window restores the continuous upper wall under the lid.
    return rr(P['diffuser_window_width'],P['diffuser_window_height'],2.3,8).rotate((0,0,0),(1,0,0),90).translate((0,D/2+4,Z))

def diffuser_bosses():
    # Horizontal screw bosses behind the front wall, well away from the LED packages.
    s=None
    for x in [-P['diffuser_screw_half_spacing'],P['diffuser_screw_half_spacing']]:
        b=cq.Workplane('XY').circle(2.8).extrude(4).rotate((0,0,0),(1,0,0),90).translate((x,D/2,Z))
        s=b if s is None else s.union(b)
    return s

def diffuser_pilots():
    s=None
    for x in [-P['diffuser_screw_half_spacing'],P['diffuser_screw_half_spacing']]:
        b=cq.Workplane('XY').circle(.85).extrude(3.8).rotate((0,0,0),(1,0,0),90).translate((x,D/2+.2,Z))
        s=b if s is None else s.union(b)
    return s

def make_diffuser(t):
    # All material is supported from the print bed: continuous thin face, raised rim.
    w,h,depth=P['diffuser_frame_width'],P['diffuser_frame_height'],P['diffuser_frame_thickness']
    face=rr(w,h,2.5,t)
    rim=rr(w,h,2.5,depth).cut(rr(P['diffuser_aperture_width'],P['diffuser_aperture_height'],1.5,depth+.2).translate((0,0,-.1)))
    s=face.union(rim)
    for x in [-P['diffuser_screw_half_spacing'],P['diffuser_screw_half_spacing']]:
        s=s.cut(cq.Workplane('XY').center(x,0).circle(P['diffuser_screw_clearance']/2).extrude(depth+.4).translate((0,0,-.2)))
    return s.clean()

def make_led():
    s=rr(40,14,1,2)
    for sign in [-1,1]:
        a,b=sorted([sign*19.9,sign*22]);s=s.union(box(a,b,-4,4,0,2))
        a,b=sorted([sign*12,sign*18]);s=s.cut(box(a,b,2,7.2,-.2,2.2))
    return s.clean()

def guide(sx,sy):
    def b(x1,x2,y1,y2,z1,z2):
        a,b=sorted([sx*x1,sx*x2]);c,d=sorted([sy*y1,sy*y2])
        return box(a,b,c,d,z1,z2)
    s=b(22.3,24.2,LY-1.5,LY+3.5,1.8,27)
    s=s.union(b(19.7,24.2,LY-1.5,LY-.3,1.8,27))
    s=s.union(b(19.7,24.2,LY+2.3,LY+3.5,1.8,27))
    return s.union(b(20.2,24.2,LY-.4,LY+2.4,12.6,13))

def make_mounts():
    s=None
    for x,y in HOLES:
        post=cq.Workplane('XY').center(x,y).circle(2.5).extrude(P['pcb_bottom']-1.8).translate((0,0,1.8))
        s=post if s is None else s.union(post)
    # Small support shelves at USB end prevent the board flexing when plugging in.
    for sy in [-1,1]:
        a,b=sorted([sy*11.4,sy*12.7])
        s=s.union(box(P['pcb_xmin']-.4,P['pcb_xmin']+2,a,b,1.8,P['pcb_bottom']))
    return s

def make_base():
    s=rr(W,D,P['corner'],B).cut(rr(W-2*T,D-2*T,P['corner']-T,B+2).translate((0,0,F)))
    op=opening();s=s.cut(op).cut(mirror_back(op))
    bosses=diffuser_bosses();s=s.union(bosses).union(mirror_back(bosses))
    pilots=diffuser_pilots();s=s.cut(pilots).cut(mirror_back(pilots))
    y=P['usb_aperture_width']/2;lower=P['usb_center_z']-P['usb_aperture_height']/2
    s=s.cut(box(-W/2-3,-W/2+T+1,-y,y,lower,B+2))
    s=s.cut(box(-W/2+1.1,-W/2+T+.2,-13.3,13.3,P['pcb_bottom']-.4,B+1))
    s=s.union(make_mounts())
    for x,y in HOLES:
        s=s.cut(cq.Workplane('XY').center(x,y).circle(P['pcb_pilot_diameter']/2).extrude(P['pcb_bottom']-2.6+.2).translate((0,0,2.6)))
    for sx in [-1,1]:
        for sy in [-1,1]:s=s.union(guide(sx,sy))
    for x,y in SCREWS:
        s=s.union(cq.Workplane('XY').center(x,y).circle(3.5).extrude(B-1.8).translate((0,0,1.8)))
        s=s.cut(cq.Workplane('XY').center(x,y).circle(.85).extrude(10).translate((0,0,B-9)))
    return s.clean()

def make_lid():
    s=rr(W,D,P['corner'],H-B).translate((0,0,B))
    z=P['usb_center_z']+P['usb_aperture_height']/2
    s=s.union(box(-W/2,-W/2+T,-5.85,5.85,z,B+.1))
    # Rigid fingers retain LED carriers, avoiding glue in their guides.
    for sx in [-1,1]:
        a,b=sorted([sx*18.3,sx*20.7])
        for sy in [-1,1]:
            c,d=sorted([sy*(LY+.1),sy*(LY+1.9)])
            s=s.union(box(a,b,c,d,Z+7.2,B+.1))
    for y in [-10,10]:s=s.union(box(W/2-3.2,W/2-2.3,y-2,y+2,B-1,B+.1))
    for x,y in SCREWS:
        s=s.cut(cq.Workplane('XY').center(x,y).circle(1.2).extrude(5).translate((0,0,B-.5)))
        s=s.cut(cq.Solid.makeCone(1.2,2.3,1.1,cq.Vector(x,y,H-1.1),cq.Vector(0,0,1)))
    return s.clean()

print('Construction V4 62 × 52 × 32...',flush=True)
base=make_base();lid=make_lid();led=make_led();d04=make_diffuser(.4);d06=make_diffuser(.6)
assembled={'socle':base,'couvercle':lid,'support_LED_avant':carrier_place(led),
 'support_LED_arriere':carrier_place(led,True),'diffuseur_avant':diffuse_place(d04),'diffuseur_arriere':diffuse_place(d04,back=True)}
parts={'01_socle_noir_4_vis':base,'02_couvercle_noir_4_vis':lid.rotate((0,0,0),(1,0,0),180).translate((0,0,H)),
 '03_support_LED_x2':led,'04_diffuseur_plat_blanc_04mm_x2':d04,'OPTION_diffuseur_plat_blanc_06mm_x2':d06}
# Actual window and screw bosses, exterior wall face down for support-free printing.
sample=base.intersect(box(-22,22,D/2-4,D/2,7,27))
parts['TEST_fenetre_et_vis_diffuseur']=sample.rotate((0,0,0),(1,0,0),-90).translate((0,-17,D/2))
# PCB mounting/USB sample retains both real posts and their actual relative location.
mount_sample=base.intersect(box(-W/2,HX+3,-14,14,0,19))
parts['TEST_fixation_ESP_et_USB']=mount_sample
parts['TEST_glissiere_LED']=guide(1,1).union(box(18.5,25,LY-2.5,LY+4.5,0,2)).translate((-18.5,-LY+2.5,0))

report={'version':'V4','units':'mm','body_mm':[W,D,H],'physical_fit_validated':False,
 'parts':{},'collisions_mm3':{},'insertion_checks':{},
 'pcb_holes':{'spacing_mm':P['pcb_hole_spacing'],'diameter_mm':P['pcb_hole_diameter'],
 'end_offset_mm':P['pcb_hole_end_offset_estimated'],'end_offset_status':'graphical estimate from official drawing, not dimensioned',
 'centres_mm':HOLES},'printed_assembly_quantity':6,'led_setback_from_v2_mm':2,
 'lid_screw_count':len(SCREWS),'lid_screw_centres_mm':SCREWS,
 'diffuser_screws_per_panel':2,'diffuser_clearance_hole_mm':P['diffuser_screw_clearance'],
 'diffuser_screw_radial_clearance_mm':(P['diffuser_screw_clearance']-2)/2}
for name,s in parts.items():
    print('Export :',name,flush=True)
    assert s.val().isValid() and len(s.solids().vals())==1,name
    cq.exporters.export(s,str(ROOT/'STEP'/f'{name}.step'))
    cq.exporters.export(s,str(ROOT/'STL'/f'{name}.stl'),tolerance=.025,angularTolerance=.08)
    r=cq.importers.importStep(str(ROOT/'STEP'/f'{name}.step'))
    assert r.val().isValid() and len(r.solids().vals())==1,name+' STEP'
    m=trimesh.load_mesh(ROOT/'STL'/f'{name}.stl')
    assert m.is_watertight and m.is_volume,name+' STL'
    assert abs(m.bounds[0,2])<.01,name+' min Z'
    if 'diffuseur_plat' in name:
        unsupported=(m.face_normals[:,2]<-.01)&(m.triangles_center[:,2]>.01)
        assert not unsupported.any(),name+' unsupported underside'
    adj=[[] for _ in range(len(m.faces))]
    for i,j in m.face_adjacency:adj[i].append(j);adj[j].append(i)
    seen={0};todo=[0]
    while todo:
        for j in adj[todo.pop()]:
            if j not in seen:seen.add(j);todo.append(j)
    assert len(seen)==len(m.faces),name+' disconnected mesh'
    bb=s.val().BoundingBox()
    report['parts'][name]={'valid_step':True,'watertight_stl':True,'solids':1,'size_mm':[round(bb.xlen,3),round(bb.ylen,3),round(bb.zlen,3)],'volume_mm3':round(s.val().Volume(),2)}

def collision(a,b):return a.intersect(b).val().Volume()
for (n,a),(m,b) in itertools.combinations(assembled.items(),2):
    v=collision(a,b);report['collisions_mm3'][n+'/'+m]=round(v,6);assert v<.01,(n,m,v)

pcb=box(P['pcb_xmin'],P['pcb_xmin']+P['pcb_length'],-P['pcb_width']/2,P['pcb_width']/2,P['pcb_bottom'],P['pcb_bottom']+P['pcb_thickness'])
for x,y in HOLES:pcb=pcb.cut(cq.Workplane('XY').center(x,y).circle(P['pcb_hole_diameter']/2).extrude(3).translate((0,0,P['pcb_bottom']-.5)))
usb=box(P['pcb_xmin']-P['usb_overhang_assumed'],P['pcb_xmin']+5,-P['usb_width_assumed']/2,P['usb_width_assumed']/2,P['usb_center_z']-P['usb_height_assumed']/2,P['usb_center_z']+P['usb_height_assumed']/2)
# LED strip + package envelope, assumed 34 x 10 x 2.5. Not a measured component.
led_envelope=box(-17,17,LY+2,LY+2+P['led_package_thickness_assumed'],Z-5,Z+5)
references={'carte_nominale':pcb,'USB_nominal':usb,'ruban_LED_avant_nominal':led_envelope,'ruban_LED_arriere_nominal':mirror_back(led_envelope)}
for n,a in references.items():
    for m,b in assembled.items():
        v=collision(a,b);report['collisions_mm3'][n+'/'+m]=round(v,6);assert v<.01,(n,m,v)

# Multi-height checks for top-down assembly with the lid removed.
for n,obj in [('ESP',pcb.union(usb))]+[(n,s) for n,s in assembled.items() if n.startswith('support_LED')]:
    for dz in [.2,.5,1,3,6,12,20,30]:
        v=collision(base,obj.translate((0,0,dz)));assert v<.01,(n,'insertion',dz,v)
    report['insertion_checks'][n]='free at +0.2, 0.5, 1, 3, 6, 12, 20, 30 mm'

# Diffusers approach from outside: no groove to flex into or slide through.
for n in ['diffuseur_avant','diffuseur_arriere']:
    sign=1 if n.endswith('avant') else -1
    for dy in [.2,.5,1,3,6]:assert collision(base,assembled[n].translate((0,sign*dy,0)))<.01,(n,'front insertion',dy)
    report['insertion_checks'][n]='free normal approach from outside at 0.2, 0.5, 1, 3, 6 mm'

# Retention of panels comes from M2 screw heads. Verify shaft clearance and full bearing.
report['retention_interference_checks']={}
for n in ['diffuseur_avant','diffuseur_arriere']:
    sign=1 if n.endswith('avant') else -1
    for x in [-P['diffuser_screw_half_spacing'],P['diffuser_screw_half_spacing']]:
        # Nominal round M2 head: diameter 4 mm, 1.6 mm height; M2x5 shaft behind it.
        head=cq.Workplane('XY').center(x,0).circle(2).extrude(1.6).rotate((0,0,0),(1,0,0),-90).translate((0,P['diffuser_front_y'],Z))
        shaft=cq.Workplane('XY').center(x,0).circle(1).extrude(5).rotate((0,0,0),(1,0,0),90).translate((0,P['diffuser_front_y'],Z))
        if sign<0:head=mirror_back(head);shaft=mirror_back(shaft)
        assert collision(assembled[n],shaft)<.01,(n,'shaft does not clear diffuser')
        assert collision(head,assembled[n].translate((0,sign*.4,0)))>.01,(n,'head does not retain diffuser')
        for m,obj in references.items():assert collision(shaft,obj)<.01,(n,'screw touches electronics',m)
    report['retention_interference_checks'][n]={'M2_shaft_clearance':True,'M2_head_retains_panel':True,'screw_clear_of_nominal_electronics':True}
for n in ['support_LED_avant','support_LED_arriere']:
    assert collision(lid,assembled[n].translate((0,0,.6)))>.01,(n,'lid does not retain carrier')
    report['retention_interference_checks'][n]={'lift_0_6mm':True}

# Wire space must remain empty in the complete assembled model, including nominal electronics.
wire_spaces={'baie_laterale':(8,24,-13,13,2.5,25),'sous_ESP':(-25,3,-6,6,2.5,8.5),'corridor_6mm':(12,18,-18,18,2.5,9)}
report['wire_spaces_mm']={}
for n,bounds in wire_spaces.items():
    region=box(*bounds)
    for m,obj in dict(assembled,**references).items():assert collision(region,obj)<.01,(n,'wire bay obstructed',m)
    x1,x2,y1,y2,z1,z2=bounds
    report['wire_spaces_mm'][n]={'width':x2-x1,'length':y2-y1,'height':z2-z1,'obstructions':False}
for s,t in [(d04,.4),(d06,.6)]:
    assert s.val().isInside(cq.Vector(0,0,t/2)) and not s.val().isInside(cq.Vector(0,0,t+.02))
report['membranes_checked_mm']=[.4,.6]
report['diffuser_to_LED_carrier_face_mm']=P['diffuser_front_y']-.4-(LY+2)
report['diffuser_to_assumed_LED_package_mm']=report['diffuser_to_LED_carrier_face_mm']-P['led_package_thickness_assumed']
report['diffuser_frame_thickness_mm']=P['diffuser_frame_thickness']
report['diffuser_print_supports_required_by_geometry']=False
report['usb_protrusion_assumed_mm']=round(-W/2-(P['pcb_xmin']-P['usb_overhang_assumed']),3)

assy=cq.Assembly(name='Tally_V4_62x52x32')
for n,s in assembled.items():assy.add(s,name=n,color=cq.Color(.92,.91,.85) if n.startswith('diffuseur') else cq.Color(.19,.21,.22))
assy.save(str(ROOT/'STEP/00_ASSEMBLAGE_reference_6_pieces.step'))
ref=cq.Assembly(name='Enveloppes_nominales_non_imprimables')
for n,s in dict(assembled,**references).items():ref.add(s,name=n)
ref.save(str(ROOT/'STEP/REFERENCE_electronique_non_imprimable.step'))
for filename,expected in [('00_ASSEMBLAGE_reference_6_pieces.step',6),('REFERENCE_electronique_non_imprimable.step',10)]:
    solids=cq.importers.importStep(str(ROOT/'STEP'/filename)).solids().vals()
    assert len(solids)==expected and all(s.isValid() for s in solids),filename
report['assembly_step_reimport_valid']=True
for n,s in dict(assembled,**references).items():cq.exporters.export(s,str(ROOT/'rendus'/f'{n}.stl'),tolerance=.03,angularTolerance=.1)
cq.exporters.export(d04,str(ROOT/'rendus/diffuseur_a_imprimer.stl'),tolerance=.03,angularTolerance=.1)
(ROOT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('V4 exportée et vérifiée.',flush=True)
