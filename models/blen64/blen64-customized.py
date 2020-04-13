# Blen64 (modified)
# forked from https://github.com/GCaldL/Blen64
# this script uses the vertex buffer inefficiently, which can result in poor
# rendering performance. keep that in mind if you use it.
# - fixed invalid output of x,y,z
# - fixed invalid use of vertex buffer indices > 32
# - fixed random value used for flag
# - fixed random value used for alpha
# - removed support for UV coordinates

import bpy
import string

s = 100  # scale constant
loadlim = 30  # amount of verts the sytem will load at a time 32 max limit

bpy.ops.text.new()
o = bpy.data.texts["Text"]
o.name = "meshout"


def clean_name(name):
    # Only take the characters from the name that are valid for C identifiers.
    # Note that this makes it possible to have a duplicate, so we test against all
    # previous names before using.

    name = "".join(
        c
        for c in obj.name
        if c in string.ascii_letters or c in string.digits or c == "_"
    )
    if len(name) == 0 or name[0] in string.digits:
        name = "_" + name
    return name


def export(obj):
    name = clean_name(obj.name)
    vert = obj.data.vertices
    poly = obj.data.polygons

    ###Triangulate
    # Check for polys w/ more than 3 verts
    for p in poly:
        if len(p.vertices) > 3:
            bpy.ops.object.editmode_toggle()
            bpy.ops.mesh.quads_convert_to_tris(
                quad_method="BEAUTY", ngon_method="BEAUTY"
            )
            bpy.ops.object.editmode_toggle()
            break

    # Vertex List
    o.write("Vtx %s_VertList[] = {\n" % name)
    for face in poly:
        for vert, loop in zip(face.vertices, face.loop_indices):
            coord = obj.data.vertices[vert].co
            vcol = (
                obj.data.vertex_colors.active.data[loop].color
                if obj.data.vertex_colors.active
                else (1, 1, 1, 1)
            )
            o.write(
                "   { %i, %i, %i, %i, %i, %i, %i, %i, %i, %i},\n"
                % (
                    coord.x * s,
                    coord.y * s,
                    coord.z * s,
                    0,  # flag
                    0,  # S
                    0,  # T
                    vcol[0] * 255,
                    vcol[1] * 255,
                    vcol[2] * 255,
                    255,  # alpha
                )
            )
    o.write("};\n\n")

    # Face List
    o.write("Gfx %s_PolyList[] = {\n" % name)
    o.write(
        "   gsSPVertex(%s_VertList+%d,%d,0),\n" % (name, 0, loadlim)
    )  # load the first x number of vertices
    i = 0
    offset = 0
    for face in poly:
        # if the face being created contains a vert that has not been loaded into the buffer
        if i * 3 + 2 > offset + loadlim:
            offset = i * 3
            o.write(
                "   gsSPVertex(%s_VertList+%i,%i,%i),\n" % (name, offset, loadlim, 0)
            )

        # build faces:
        o.write(
            "   gsSP1Triangle(%d, %d, %d, %d),\n"
            % (i * 3 - offset, i * 3 + 1 - offset, i * 3 + 2 - offset, 0)
        )
        i += 1
    o.write("   gsSPEndDisplayList(),\n")
    o.write("};\n\n")


for obj in bpy.context.selected_objects:
    export(obj)
