"""Oil pastel post-process material (run inside UnrealEditor via -ExecutePythonScript).

Creates
  /Game/Proto/Shader/M_PP_OilPastel   post-process material, after tonemapping, one Custom node (Tools/ue/oil_pastel.hlsl)
  /Game/Proto/Shader/MI_PP_OilPastel  instance to tweak in the editor; build_level.py puts it in the map's post-process volume

"Collapse" is driven at runtime by ADMHUD from the dream timer; leave it at 0 in the instance.
"""
import os

import unreal

FOLDER = "/Game/Proto/Shader"
MATERIAL = FOLDER + "/M_PP_OilPastel"
INSTANCE = FOLDER + "/MI_PP_OilPastel"
HLSL = os.path.join(os.path.dirname(__file__), "oil_pastel.hlsl")

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# name, default, group
SCALARS = [
    ("BrushSize", 7.0, "1 Paint"),
    ("Saturation", 1.15, "1 Paint"),
    ("PaperScale", 2.2, "2 Paper"),
    ("Grain", 0.75, "2 Paper"),
    ("StrokeScale", 2.5, "2 Paper"),
    ("Stroke", 0.55, "2 Paper"),
    ("LineWidth", 1.6, "3 Lines"),
    ("LineStrength", 0.85, "3 Lines"),
    ("BoilFPS", 6.0, "3 Lines"),
    ("GlintSize", 22.0, "4 Glints"),
    ("Glint", 0.9, "4 Glints"),
    ("GlintThreshold", 0.72, "4 Glints"),
    ("Collapse", 0.0, "5 Dream"),
]
VECTORS = [
    ("PaperColor", (0.95, 0.92, 0.84), "2 Paper"),
    ("InkColor", (0.2, 0.14, 0.13), "3 Lines"),
    ("GlintColor", (1.0, 0.96, 0.82), "4 Glints"),
]
# Custom node pin name -> scene texture it must make available to SceneTextureLookup
SCENE_TEXTURES = [
    ("SceneColor", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),
    ("SceneDepthIn", unreal.SceneTextureId.PPI_SCENE_DEPTH),
    ("WorldNormalIn", unreal.SceneTextureId.PPI_WORLD_NORMAL),
    ("RoughnessIn", unreal.SceneTextureId.PPI_ROUGHNESS),
]


def log(msg):
    unreal.log_warning("[DreamMovers] " + str(msg))


def load_or_create(path, cls, factory):
    """Update in place: the map's post-process volume references these, so deleting them would fail."""
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    return asset_tools.create_asset(name, folder, cls, factory)


def link(a, a_out, b, b_in):
    if not mel.connect_material_expressions(a, a_out, b, b_in):
        log(f"  link failed {a.get_name()}.{a_out} -> {b.get_name()}.{b_in}")


def build_material():
    m = load_or_create(MATERIAL, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    # After tonemapping: the filter works on display colors, like pigment on paper.
    m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

    with open(HLSL, encoding="utf-8") as f:
        code = f.read()

    pins = [n for n, _ in SCENE_TEXTURES] + [n for n, _, _ in SCALARS] + ["Time"] + [n for n, _, _ in VECTORS]
    custom = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", code)
    custom.set_editor_property("description", "OilPastel")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for name in pins:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)

    y = -600
    for pin, tex_id in SCENE_TEXTURES:
        st = mel.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -900, y)
        st.set_editor_property("scene_texture_id", tex_id)
        link(st, "Color", custom, pin)
        y += 120
    for name, default, group in SCALARS:
        p = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", default)
        p.set_editor_property("group", group)
        link(p, "", custom, name)
        y += 90
    time = mel.create_material_expression(m, unreal.MaterialExpressionTime, -900, y)
    link(time, "", custom, "Time")
    y += 90
    for name, rgb, group in VECTORS:
        p = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, y)
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", unreal.LinearColor(*rgb, 1.0))
        p.set_editor_property("group", group)
        link(p, "RGB", custom, name)
        y += 140

    mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def build_instance(parent):
    mi = load_or_create(INSTANCE, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    mel.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    return mi


def main():
    m = build_material()
    build_instance(m)
    log("build_shader done")


main()
