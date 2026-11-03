"""Procedural test map /Game/Maps/Lvl_ProcGen (run inside UnrealEditor via -ExecutePythonScript, after building the C++ modules).

Only the generator (Plugins/ProcLevelGen) is placed: it assembles the 3 + 2 + 3 module kit from a new seed every run.
The start module always sits at the origin (1000 x 1000 cm, door on +X), so the player starts and the extraction
zone are placed there. No valuables or monsters yet: this map checks the layout, walls, doors and navigation.
"""
import unreal

MAP = "/Game/Maps/Lvl_ProcGen"
PASTEL = "/Game/Proto/Shader/MI_PP_OilPastel"

eal = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

GENERATOR = unreal.load_class(None, "/Script/ProcLevelGen.PLGGenerator")
EXTRACTION = unreal.load_class(None, "/Script/DreamMovers.DMExtractionZone")
V = unreal.Vector

# Dream Movers names for the sample kit (the plugin stays game-agnostic).
MODULE_NAMES = {
    unreal.PLGRole.START: "EntranceHall",
    unreal.PLGRole.HUB: "CarouselHall",
    unreal.PLGRole.END: "AtticBedroom",
}
THEME_NAMES = ["TeacupParlor", "GlassGallery", "ClockStudy", "Chapel"]


def log(msg):
    unreal.log_warning("[DreamMovers] " + str(msg))


def require_map_open():
    """Never save anything but our own map."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or not world.get_path_name().startswith(MAP + "."):
        raise RuntimeError(f"expected {MAP} to be open, got {world.get_path_name() if world else None}; nothing saved")


def open_empty_map():
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


def environment():
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1000), unreal.Rotator(0, -50, 35))
    sun.light_component.set_editor_property("intensity", 6.0)
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sun.set_folder_path("Lighting")
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0)).set_folder_path("Lighting")
    sky = eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500))
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.set_folder_path("Lighting")
    eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0)).set_folder_path("Lighting")

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


def generator():
    gen = eas.spawn_actor_from_class(GENERATOR, V(0, 0, 0))
    gen.set_actor_label("LevelGenerator")
    gen.set_folder_path("Gameplay")

    modules = gen.get_editor_property("modules")
    for m in modules:
        name = MODULE_NAMES.get(m.get_editor_property("role"))
        if name:
            m.set_editor_property("name", name)
    gen.set_editor_property("modules", modules)

    themes = gen.get_editor_property("themes")
    for t, name in zip(themes, THEME_NAMES):
        t.set_editor_property("name", name)
    gen.set_editor_property("themes", themes)

    # A preview in the editor viewport; play builds a fresh one from a new seed.
    gen.preview()
    return gen


def gameplay():
    # Inside the start module (X, Y in [-500, 500]); its only door is on +X.
    zone = eas.spawn_actor_from_class(EXTRACTION, V(-250, 0, 100))
    zone.set_actor_label("ExtractionZone")
    zone.set_folder_path("Gameplay")
    for i, y in enumerate((-180, -60, 60, 180)):
        start = eas.spawn_actor_from_class(unreal.PlayerStart, V(150, y, 60), unreal.Rotator(0, 0, 0))
        start.set_actor_label(f"PlayerStart_{i}")
        start.set_folder_path("Gameplay")

    # Navmesh over the whole generation area (MaxHalfExtent 5000); built at runtime.
    nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, V(0, 0, 150))
    nav.set_actor_scale3d(V(50.0, 50.0, 3.0))  # the default brush is a 200 cm cube
    nav.set_actor_label("NavBounds")
    nav.set_folder_path("Gameplay")
    # Headless runs do not create the navmesh actor on their own; without it nothing is generated at runtime.
    recast = eas.spawn_actor_from_class(unreal.RecastNavMesh, V(0, 0, 0))
    recast.set_folder_path("Gameplay")


def main():
    if GENERATOR is None:
        raise RuntimeError("PLGGenerator class not found: build the ProcLevelGen plugin first")
    open_empty_map()
    environment()
    generator()
    gameplay()
    require_map_open()
    les.save_current_level()
    log("build_proc_level done")


main()
