"""Phase 0 test map /Game/Maps/Lvl_Proto (run inside UnrealEditor via -ExecutePythonScript, after building the C++ module).

Three open-roofed rooms in a row along +X, joined by 200 cm doorways, to test carrying through doors:
  Hall       X [-1200,    0]  player starts + extraction zone (music box bed stand-in)
  Parlor     X [    0, 1200]  tables with small and medium items
  Back room  X [ 1200, 2400]  heavy two-person items, and a ramp up to a ledge with fragile items

Item stats mirror Docs/Tripo_Asset_List.md (S/M/L). One player lifts up to 20 kg (UDMGrabComponent::MaxLiftKg),
so the 30 kg+ items need two holders.
"""
import unreal

MAP = "/Game/Maps/Lvl_Proto"
MAT = "/Game/Proto/Materials"
PASTEL = "/Game/Proto/Shader/MI_PP_OilPastel"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
CYL = eal.load_asset("/Engine/BasicShapes/Cylinder")
SPHERE = eal.load_asset("/Engine/BasicShapes/Sphere")
BASE_MAT = eal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")
VALUABLE = unreal.load_class(None, "/Script/DreamMovers.DMValuable")
EXTRACTION = unreal.load_class(None, "/Script/DreamMovers.DMExtractionZone")
BUTLER = unreal.load_class(None, "/Script/DreamMovers.DMButler")
BELL_HORSE = unreal.load_class(None, "/Script/DreamMovers.DMBellHorse")
V = unreal.Vector
SHAPE = [None]  # M_Proto_Shape, created in main()

WALL_H = 400.0
WALL_T = 40.0
DOOR_W = 200.0
DOOR_H = 240.0
ROOM_Y = 800.0  # rooms span Y [-800, 800]


def log(msg):
    unreal.log_warning("[DreamMovers] " + str(msg))


def color_material(name, rgb):
    path = f"{MAT}/{name}"
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else         asset_tools.create_asset(name, MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, BASE_MAT)
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(*rgb, 1.0))
    eal.save_loaded_asset(mi)
    return mi


def shape_master():
    """Opaque master with Color / Roughness / Metallic, so glass and brass can be glossy enough for glints."""
    path = f"{MAT}/M_Proto_Shape"
    if eal.does_asset_exist(path):
        # Kept as is: the monster classes load it at startup, and editing a loaded master here crashes the editor.
        return eal.load_asset(path)
    m = asset_tools.create_asset("M_Proto_Shape", MAT, unreal.Material, unreal.MaterialFactoryNew())
    for i, (name, default, prop) in enumerate((("Roughness", 0.6, unreal.MaterialProperty.MP_ROUGHNESS),
                                                ("Metallic", 0.0, unreal.MaterialProperty.MP_METALLIC))):
        p = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200 + i * 120)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", default)
        mel.connect_material_property(p, "", prop)
    c = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -400, 0)
    c.set_editor_property("parameter_name", "Color")
    c.set_editor_property("default_value", unreal.LinearColor(0.8, 0.8, 0.8, 1.0))
    mel.connect_material_property(c, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def block(label, center, size, mat, folder="Architecture"):
    """Static box with its center at `center` and full size `size` in cm."""
    a = eas.spawn_actor_from_object(CUBE, V(*center))
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    a.static_mesh_component.set_material(0, mat)
    a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def wall_with_door(label, x, mat):
    """Wall across the rooms at X = x with a centered doorway."""
    side = (2 * ROOM_Y - DOOR_W) / 2.0
    for sign, suffix in ((-1, "L"), (1, "R")):
        y = sign * (DOOR_W / 2.0 + side / 2.0)
        block(f"{label}_{suffix}", (x, y, WALL_H / 2.0), (WALL_T, side, WALL_H), mat)
    block(f"{label}_Lintel", (x, 0, (DOOR_H + WALL_H) / 2.0), (WALL_T, DOOR_W, WALL_H - DOOR_H), mat)


def valuable(name, mesh, size, location, mass, crack, value, tint, yaw=0.0, rough=0.6, metal=0.0):
    """`size` is the full size in cm (engine shapes are 100 cm); `location` is where the bottom rests."""
    x, y, z = location
    a = eas.spawn_actor_from_class(VALUABLE, V(x, y, z + size[2] / 2.0 + 1.0), unreal.Rotator(0, 0, yaw))
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    comp = a.get_component_by_class(unreal.StaticMeshComponent)
    comp.set_static_mesh(mesh)
    comp.set_material(0, SHAPE[0])
    a.set_editor_property("display_name", unreal.Text(name))
    a.set_editor_property("mass_kg", mass)
    a.set_editor_property("crack_speed", crack)
    a.set_editor_property("base_value", value)
    a.set_editor_property("tint", unreal.LinearColor(*tint, 1.0))
    a.set_editor_property("roughness", rough)
    a.set_editor_property("metallic", metal)
    a.set_actor_label("VAL_" + name.replace(" ", "_"))
    a.set_folder_path("Valuables")
    return a


def environment():
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1000), unreal.Rotator(0, -50, 35))
    sun.light_component.set_editor_property("intensity", 6.0)
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sun.set_folder_path("Lighting")
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0)).set_folder_path("Lighting")
    sky = eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500))
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.set_folder_path("Lighting")
    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0))
    fog.set_folder_path("Lighting")

    # Oil pastel look (run build_shader.py first). ADMHUD swaps it for a dynamic instance to drive "Collapse".
    pastel = eal.load_asset(PASTEL)
    if pastel is None:
        log(f"missing {PASTEL}: run build_shader.py first; the map will have no oil pastel look")
        return
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    settings = ppv.get_editor_property("settings")
    blendables = settings.get_editor_property("weighted_blendables")
    blendables.set_editor_property("array", [unreal.WeightedBlendable(1.0, pastel)])
    settings.set_editor_property("weighted_blendables", blendables)
    ppv.set_editor_property("settings", settings)
    ppv.set_actor_label("PP_OilPastel")
    ppv.set_folder_path("Lighting")


def architecture(floor, wall, table):
    block("Floor", (600, 0, -25), (3700, 2 * ROOM_Y + 200, 50), floor)
    # Outer walls
    block("Wall_North", (600, ROOM_Y + WALL_T / 2, WALL_H / 2), (3640, WALL_T, WALL_H), wall)
    block("Wall_South", (600, -ROOM_Y - WALL_T / 2, WALL_H / 2), (3640, WALL_T, WALL_H), wall)
    block("Wall_West", (-1200 - WALL_T / 2, 0, WALL_H / 2), (WALL_T, 2 * ROOM_Y, WALL_H), wall)
    block("Wall_East", (2400 + WALL_T / 2, 0, WALL_H / 2), (WALL_T, 2 * ROOM_Y, WALL_H), wall)
    wall_with_door("Wall_HallParlor", 0, wall)
    wall_with_door("Wall_ParlorBack", 1200, wall)

    # Parlor tables (70 cm tall) and a narrow gap between two cabinets to squeeze items through.
    for i, (x, y) in enumerate(((350, -450), (350, 450), (850, -450), (850, 450))):
        block(f"Table_{i}", (x, y, 35), (180, 100, 70), table, "Props")
    block("Cabinet_A", (600, -200, 100), (120, 250, 200), table, "Props")
    block("Cabinet_B", (600, 200, 100), (120, 250, 200), table, "Props")

    # Back room ledge with a ramp, for the "bring it down without dropping it" test.
    block("Ledge", (2150, 500, 75), (500, 600, 150), table, "Props")
    ramp = block("Ramp", (1730, 500, 70), (420, 200, 20), table, "Props")
    ramp.set_actor_rotation(unreal.Rotator(0, 20, 0), False)


def gameplay():
    zone = eas.spawn_actor_from_class(EXTRACTION, V(-700, 0, 100))
    zone.set_actor_label("ExtractionZone")
    zone.set_folder_path("Gameplay")
    for i, y in enumerate((-180, -60, 60, 180)):
        start = eas.spawn_actor_from_class(unreal.PlayerStart, V(-1000, y, 60), unreal.Rotator(0, 0, 0))
        start.set_actor_label(f"PlayerStart_{i}")
        start.set_folder_path("Gameplay")

    T = 70.0  # table top
    gold, glass, red, wood, brass = (0.95, 0.75, 0.3), (0.7, 0.85, 1.0), (0.8, 0.1, 0.12), (0.45, 0.28, 0.15), (0.75, 0.55, 0.25)
    # Small (S): one hand, light, mostly fragile
    valuable("크리스탈 쿠페 잔", SPHERE, (25, 25, 25), (330, -470, T), 0.5, 220, 1200, glass, rough=0.05)
    valuable("도자기 찻잔", CUBE, (22, 22, 15), (380, -420, T), 0.6, 260, 600, (0.95, 0.85, 0.9))
    valuable("토끼 머리 열쇠", CUBE, (10, 35, 8), (330, 450, T), 0.4, 600, 800, (0.8, 0.8, 0.85))
    valuable("회중시계", CYL, (18, 18, 6), (380, 420, T), 0.5, 380, 700, brass, rough=0.25, metal=1.0)
    valuable("향수병", CYL, (14, 14, 25), (2100, 400, 150), 0.4, 200, 900, (1.0, 0.6, 0.75), rough=0.05)
    valuable("금박 달걀", SPHERE, (20, 20, 26), (2200, 600, 150), 0.8, 350, 1500, gold, rough=0.2, metal=1.0)
    # Medium (M): one player, heavy-ish
    valuable("황동 오일 램프", CYL, (35, 35, 70), (850, -470, T), 6.0, 330, 1400, brass, rough=0.25, metal=1.0)
    valuable("붉은 하트 성물함", SPHERE, (50, 45, 55), (850, 450, T), 8.0, 300, 2500, red, rough=0.08)
    valuable("유리 랜턴", CUBE, (40, 40, 70), (950, 0, 0), 7.0, 300, 1800, glass, rough=0.05)
    valuable("도자기 광대 인형", CUBE, (35, 30, 50), (200, 0, 0), 5.0, 280, 1100, (0.95, 0.9, 0.7))
    valuable("별자리 지구본", SPHERE, (55, 55, 55), (2300, 350, 150), 9.0, 450, 1000, (0.25, 0.35, 0.6))
    # Large (L): two players
    valuable("회전목마 말", CUBE, (120, 40, 90), (1550, -450, 0), 32.0, 400, 3500, (0.95, 0.9, 0.95), yaw=20)
    valuable("대형 오르골", CUBE, (100, 70, 60), (1900, -300, 0), 35.0, 300, 4000, wood)
    valuable("축음기", CYL, (80, 80, 90), (1600, 100, 0), 30.0, 400, 3000, (0.5, 0.35, 0.2))
    valuable("금박 액자 그림", CUBE, (10, 140, 110), (2300, -500, 0), 25.0, 380, 2800, gold, rough=0.3, metal=1.0)


def require_map_open():
    """Never save anything but our own map (an earlier version of this script saved over an engine template)."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or not world.get_path_name().startswith(MAP + "."):
        raise RuntimeError(f"expected {MAP} to be open, got {world.get_path_name() if world else None}; nothing saved")


def open_empty_map():
    """Reuse the existing map (it is usually the open startup map, so it cannot be deleted) and clear it."""
    if eal.does_asset_exist(MAP):
        les.load_level(MAP)
        require_map_open()
        for a in eas.get_all_level_actors():
            if not isinstance(a, (unreal.WorldSettings, unreal.Brush)):
                eas.destroy_actor(a)
    else:
        if not les.new_level(MAP):
            raise RuntimeError(f"could not create {MAP}")
        require_map_open()


def monsters():
    # Navmesh area over the whole house; generated at runtime (RuntimeGeneration=Dynamic in DefaultEngine.ini).
    nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(600, 0, 150))
    nav.set_actor_scale3d(V(19.5, 9.0, 3.0))  # the default brush is a 200 cm cube
    nav.set_actor_label("NavBounds")
    nav.set_folder_path("Gameplay")
    origin, extent = nav.get_actor_bounds(False)
    log(f"nav bounds extent {extent}")

    # Butler: loops parlor <-> back room through the doorway, past the tables and the heavy items.
    butler = eas.spawn_actor_from_class(BUTLER, V(700, 400, 100))  # unrotated: patrol points are relative to its transform
    butler.set_editor_property("patrol_points", [V(0, 0, 0), V(0, -800, 0), V(800, -550, 0), V(650, 250, 0)])
    butler.set_actor_label("Butler")
    butler.set_folder_path("Monsters")

    # Bell horse: waits in the back room, where the heavy (loud) two-person items are.
    horse = eas.spawn_actor_from_class(BELL_HORSE, V(2050, 0, 90), unreal.Rotator(0, 0, 180))
    horse.set_actor_label("BellHorse")
    horse.set_folder_path("Monsters")


def main():
    open_empty_map()
    SHAPE[0] = shape_master()
    floor = color_material("MI_Proto_Floor", (0.55, 0.42, 0.32))
    wall = color_material("MI_Proto_Wall", (0.9, 0.85, 0.75))
    table = color_material("MI_Proto_Furniture", (0.35, 0.22, 0.15))
    environment()
    architecture(floor, wall, table)
    gameplay()
    monsters()
    require_map_open()
    les.save_current_level()
    log("build_level done")


main()
