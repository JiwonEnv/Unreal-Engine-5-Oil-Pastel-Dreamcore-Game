"""Checks the C++ level generator over many seeds inside the editor (run after build_proc_level.py):

  UnrealEditor-Cmd.exe DreamMovers.uproject -ExecutePythonScript="Tools/ue/test_proc_level.py"

For every seed: one start / hub / end, RoomCount rooms, no overlapping footprints, every door link is two-way and
meets at the same point, everything is reachable from the start, no dead-end corridors, the first rooms do not repeat
a theme, and graybox instances were built. Prints [PLGTest] PASS/FAIL lines; nothing is saved.
"""
import unreal

MAP = "/Game/Maps/Lvl_ProcGen"
SEEDS = range(1, 201)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
R = unreal.PLGRole
EDGE_QUARTER = {unreal.PLGEdge.POS_X: 0, unreal.PLGEdge.POS_Y: 1, unreal.PLGEdge.NEG_X: 2, unreal.PLGEdge.NEG_Y: 3}


def log(msg):
    unreal.log_warning("[PLGTest] " + msg)


def rot(x, y, q):
    return [(x, y), (-y, x), (-x, -y), (y, -x)][q % 4]


def socket_world(defn, placed, sock):
    sx, sy = defn.size.x / 2, defn.size.y / 2
    q = EDGE_QUARTER[sock.edge]
    off = sock.offset
    lx, ly = [(sx, off), (off, sy), (-sx, off), (off, -sy)][q]
    wx, wy = rot(lx, ly, placed.quarter)
    return placed.location.x + wx, placed.location.y + wy


def footprint(defn, placed):
    sx, sy = (defn.size.x, defn.size.y) if placed.quarter % 2 == 0 else (defn.size.y, defn.size.x)
    x, y = placed.location.x, placed.location.y
    return x - sx / 2 + 1, y - sy / 2 + 1, x + sx / 2 - 1, y + sy / 2 - 1


def overlaps(a, b):
    return not (a[0] > b[2] or b[0] > a[2] or a[1] > b[3] or b[1] > a[3])


def check(gen, defs, room_count, num_themes):
    mods = list(gen.get_editor_property("layout").modules)
    live = [i for i, m in enumerate(mods) if not m.removed]
    if not live:
        return "empty layout"
    roles = [defs[mods[i].def_].role for i in live]
    for role, want in ((R.START, 1), (R.HUB, 1), (R.END, 1), (R.ROOM, room_count)):
        if roles.count(role) != want:
            return f"{roles.count(role)} modules with role {role}, expected {want}"
    boxes = {i: footprint(defs[mods[i].def_], mods[i]) for i in live}
    for a in live:
        for b in live:
            if a < b and overlaps(boxes[a], boxes[b]):
                return f"modules {a} and {b} overlap"
    for i in live:
        m = mods[i]
        d = defs[m.def_]
        links = list(m.links)
        for s, other in enumerate(links):
            if other < 0:
                continue
            if mods[other].removed:
                return f"module {i} links to removed module {other}"
            back = [k for k, b in enumerate(mods[other].links) if b == i]
            p = socket_world(d, m, d.sockets[s])
            od = defs[mods[other].def_]
            if not any(abs(socket_world(od, mods[other], od.sockets[k])[0] - p[0]) < 1 and
                       abs(socket_world(od, mods[other], od.sockets[k])[1] - p[1]) < 1 for k in back):
                return f"door {i}:{s} -> {other} does not meet a door back"
        if d.role == R.CORRIDOR and sum(1 for o in links if o >= 0) < 2:
            return f"dead-end corridor {i}"
    seen, stack = {live[0]}, [live[0]]
    while stack:
        for o in mods[stack.pop()].links:
            if o >= 0 and o not in seen:
                seen.add(o)
                stack.append(o)
    if seen != set(live):
        return f"{len(set(live) - seen)} module(s) unreachable from the start"
    themes = [mods[i].theme for i in live if defs[mods[i].def_].role == R.ROOM]
    first = themes[:num_themes]
    if len(set(first)) != len(first):
        return f"repeated theme among the first rooms: {themes}"
    instances = sum(c.get_instance_count() for c in gen.get_components_by_class(unreal.InstancedStaticMeshComponent))
    if instances == 0:
        return "no graybox instances"
    return None


def main():
    les.load_level(MAP)
    gens = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "PLGGenerator"]
    if not gens:
        log("FAIL no PLGGenerator in " + MAP)
        return
    gen = gens[0]
    defs = list(gen.get_editor_property("modules"))
    room_count = gen.get_editor_property("room_count")
    num_themes = len(gen.get_editor_property("themes"))
    failures, sizes = 0, []
    for seed in SEEDS:
        gen.set_editor_property("seed", seed)
        gen.preview()
        problem = check(gen, defs, room_count, num_themes)
        if problem:
            failures += 1
            log(f"FAIL seed {seed}: {problem}")
        else:
            sizes.append(sum(1 for m in gen.get_editor_property("layout").modules if not m.removed))
    if sizes:
        log(f"modules per map: min {min(sizes)}, avg {sum(sizes) / len(sizes):.1f}, max {max(sizes)}")
    log(("PASS" if failures == 0 else "FAIL") + f" {len(SEEDS) - failures}/{len(SEEDS)} seeds")


main()
