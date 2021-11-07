import bpy
import io
import os
import subprocess
import json
import struct
import mathutils
from mathutils import Vector,Matrix
from math import floor,sqrt
import bmesh
import sys

def clamp(i, small, big):
    return max(min(i, big), small)

class ByteArrayWriter():
    def __init__(self):
        self.data = io.BytesIO()

    def writeByte(self, i, signed=False):
        self.data.write(i.to_bytes(1, byteorder='little', signed=signed))
    
    def writeWord(self, i, signed=False):
        self.data.write(i.to_bytes(2, byteorder='little', signed=signed))
    
    def writeDWord(self, i, signed=False):
        self.data.write(i.to_bytes(4, byteorder='little', signed=signed))
        
    def writeFloat(self, f):
        self.data.write(bytearray(struct.pack("f", f)))

    def pad(self, padding):
        sz = self.size()
        if (sz % padding) != 0:
            bytes_needed = padding - (sz % padding)
            zero = 0
            self.data.write(zero.to_bytes(bytes_needed, byteorder='little', signed=False))

    def write(self, s):
        self.file.write(s)

    def size(self):
        return self.data.tell()

    def get(self):
        self.data.seek(0)
        return self.data.read()


def write_files(asset_text, data, filepath, use_zstd, zstd_path_override):
    data_file = open(filepath.replace('.asset', '.data'), 'wb')

    subr_count = 0
    for data_to_write in data:
        if len(data_to_write) > 0:
            subr_count += 1

    asset_text += 'SUBRESOURCE-COUNT ' + str(subr_count) + '\n'
    asset_text += 'SUBRESOURCES'

    for data_to_write in data:
        using_zstd = False
        uncompressed_length = len(data_to_write)

        if uncompressed_length == 0:
            continue

        if use_zstd and uncompressed_length > 512:
            zstd_cmd = 'zstd' if zstd_path_override=='' else zstd_path_override

            try:
                result = subprocess.run([zstd_cmd, '-6'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, input=data_to_write)
                if result.returncode != 0:
                    raise RuntimeError('zstd error ' + result.stderr.decode())
                compressed = result.stdout

                compression_ratio = len(compressed) / uncompressed_length

                if compression_ratio <= 0.8:
                    asset_text += ' ZSTD ' + str(uncompressed_length) + '>' + str(len(compressed))
                    data_to_write = compressed
                    using_zstd = True
            except FileNotFoundError:
                print('ZSTD NOT FOUND')


        if not using_zstd:
            asset_text += ' NONE ' + str(uncompressed_length)
        
        data_file.write(data_to_write)
    
    asset_text += '\n'

    with open(filepath, 'w') as f:
        f.write(asset_text)

    data_file.close()

    

class Object():
    def __init__(self, blender_object, vertex_offset, vertex_count):
        self.blender_object = blender_object
        self.vertex_offset = vertex_offset
        self.vertex_count = vertex_count

class Vertex():
    def __init__(self, position, normal):
        self.position, self.normal = position, normal
        self.uv = None
        self.tangent = None
        self.bitangent_sign = None
        self.bone_indices = [-1,-1]
        self.bone_weight = 0.0

        # If a vertex has multiple UV coords / tangents, it is split
        # The indices into the vertices list are stored here
        self.extra_vertex_indices = []

    def split(self, uv, tangent, bitangent_sign):
        v = Vertex(self.position, self.normal)
        v.bone_indices, v.bone_weight = self.bone_indices, self.bone_weight
        v.uv, v.tangent, v.bitangent_sign = uv, tangent, bitangent_sign
        return v


def switch_coord_system(coords):
    # Objects face +Z in Vulkan coordinate system
    return [coords[0], coords[2], -coords[1]]


def get_objects_and_vertices(flat_shading, export_tangents, bones, bone_name_to_index):
    objects = []

    max_position = Vector ([-9999.0,-9999.0,-9999.0])
    min_position = Vector ([9999.0,9999.0,9999.0])

    vertices = []

    # Number of vertices before splitting vertexes with multiple texture coordinates
    originalVertexArraySize = 0


    for original_blender_object in bpy.data.objects:
        if not hasattr(original_blender_object.data, 'polygons') or \
            len(original_blender_object.data.vertices) < 3 or \
            len(original_blender_object.data.polygons) < 1:
            continue


        blender_object = original_blender_object.copy()


        # Triangulate
        m = bmesh.new()
        m.from_mesh(original_blender_object.data)
        bmesh.ops.triangulate(m, faces=m.faces[:])
        m.to_mesh(blender_object.data)
        m.free()

        obj = Object(blender_object, len(vertices), len(blender_object.data.vertices))
        objects.append(obj)

        if flat_shading:
            blender_object.data.calc_normals()
        if export_tangents:
            try:
                blender_object.data.calc_tangents()
            except:
                raise RuntimeError('calc_tangents error. Problematic object: ' + original_blender_object.name+
                    '. Is the object UV mapped?')
    


        obj.normal_matrix = blender_object.matrix_world.to_3x3()
        obj.normal_matrix.invert()
        obj.normal_matrix = obj.normal_matrix.transposed().to_4x4()


        for blender_vertex in blender_object.data.vertices:
            if flat_shading:
                normal = None
            else:
                normal = obj.normal_matrix @ blender_vertex.normal
                normal.normalize()
                normal = switch_coord_system(normal)
            vertex = Vertex(switch_coord_system(blender_object.matrix_world @ blender_vertex.co), normal)

            if len(bones) > 0:
                # bone weights
                bone_weights = [0.0,0.0]
                bone_indices = [-1,-1]
                for vgroup in blender_vertex.groups:
                    w = vgroup.weight
                    bone_index = bone_name_to_index[blender_object.vertex_groups[vgroup.group].name]

                    if w >= bone_weights[0]:
                        bone_weights[1] = bone_weights[0]
                        bone_indices[1] = bone_indices[0]
                        bone_weights[0] = w
                        bone_indices[0] = bone_index

                    elif w > bone_weights[1]:
                        bone_weights[1] = w
                        bone_indices[1] = bone_index


                weight_sum = bone_weights[0] + bone_weights[1]
                weight_mul = 1.0 / weight_sum
                vertex.bone_weight = bone_weights[0]*weight_mul
                vertex.bone_indices = bone_indices

            vertices.append(vertex)

            if vertex.position[0] > max_position[0]:
                max_position[0] = vertex.position[0]
            if vertex.position[1] > max_position[1]:
                max_position[1] = vertex.position[1]
            if vertex.position[2] > max_position[2]:
                max_position[2] = vertex.position[2]
            if vertex.position[0] < min_position[0]:
                min_position[0] = vertex.position[0]
            if vertex.position[1] < min_position[1]:
                min_position[1] = vertex.position[1]
            if vertex.position[2] < min_position[2]:
                min_position[2] = vertex.position[2]

    return objects,vertices,min_position,max_position

class Bone:
    pass

def get_bones():
    bones = []
    bone_name_to_index = {}
    blender_bone_to_index = {}

    for obj in bpy.data.objects:
        if not hasattr(obj.data, 'bones'):
            continue
        for blender_bone in obj.data.bones:
            bone = Bone()
            bone.name = blender_bone.name
            bone.head = switch_coord_system(obj.matrix_world @ blender_bone.head_local)
            bone.tail = switch_coord_system(obj.matrix_world @ blender_bone.tail_local)
            bone.blender_bone = blender_bone
            bone.blender_object = obj

            bone_name_to_index[bone.name] = len(bones)
            blender_bone_to_index[bone.blender_bone] = len(bones)
            bones.append(bone)

    for b in bones:
        b.m = None
        b.parent_index = None
        if b.blender_bone.parent != None and b.blender_bone != b.blender_bone.parent:
            b.parent_index = blender_bone_to_index[b.blender_bone.parent]
        if b.parent_index == None:
            b.parent_index = -1

    return bones, bone_name_to_index

def get_mesh_data(flat_shading, export_tangents):
    bones, bone_name_to_index = get_bones()
    objects,vertices,min_position,max_position = get_objects_and_vertices(flat_shading, export_tangents, bones, bone_name_to_index)

    
    class Material:
        def __init__(self, name, colour, flat_colour):
            self.indices = []
            self.name = name
            self.colour = colour
            self.flat_colour = flat_colour
            self.texture = ''
            self.normal_texture = ''
            self.specular = 0

        def is_equiv(self, mat2):
            return self.colour[0] == mat2.colour[0] and self.colour[1] == mat2.colour[1]\
                and self.colour[2] == mat2.colour[2] and self.texture == mat2.texture\
                and self.flat_colour[0] == mat2.flat_colour[0] and self.flat_colour[1] == mat2.flat_colour[1]\
                and self.flat_colour[2] == mat2.flat_colour[2]\
                and self.normal_texture == mat2.normal_texture\
                and self.specular == mat2.specular

    materials = []
    blender_material_ref_to_materials_index = {}
    
    for mat in bpy.data.materials:
        if mat.name == 'Dots Stroke':
            continue

        m = Material(mat.name, [0.8, 0.8, 0.8], [0.8, 0.8, 0.8])
        m.name = mat.name

        nodes = mat.node_tree.nodes
        bsdfnodes = [n for n in nodes if isinstance(n, bpy.types.ShaderNodeBsdfPrincipled)]
        if len(bsdfnodes) > 0:
            bsdf = bsdfnodes[0]
            colour_input = bsdf.inputs['Base Color']
            m.flat_colour[0] = colour_input.default_value[0]
            m.flat_colour[1] = colour_input.default_value[1]
            m.flat_colour[2] = colour_input.default_value[2]
            m.colour = m.flat_colour

            m.specular = bsdf.inputs['Specular'].default_value

            if len(colour_input.links) > 0:
                texture_node = colour_input.links[0].from_node
                if isinstance(texture_node, bpy.types.ShaderNodeMixRGB):
                    mix_node = texture_node.inputs['Color1']
                    if len(mix_node.links) > 0:
                        texture_node = mix_node.links[0].from_node
                if isinstance(texture_node, bpy.types.ShaderNodeTexImage):
                    m.texture = texture_node.image.filepath.split('/')[-1]
                    m.colour = [0.8, 0.8, 0.8]

            normal_map_input = bsdf.inputs['Normal']
            if len(normal_map_input.links) > 0:
                normal_map_node = normal_map_input.links[0].from_node
                if normal_map_node.space == 'TANGENT':
                    normal_texture_input = normal_map_node.inputs['Color']
                    if len(normal_texture_input.links) > 0:
                        normal_texture_node = normal_texture_input.links[0].from_node
                        if isinstance(normal_texture_node, bpy.types.ShaderNodeTexImage):
                            m.normal_texture = normal_texture_node.image.filepath.split('/')[-1]
                else:
                    print('Normals must be in tangent space')



        materials.append(m)
        blender_material_ref_to_materials_index[mat] = materials[-1]

    
    default_mat = Material('Default Material', [0.8, 0.8, 0.8], [0.8, 0.8, 0.8])
    materials.append(default_mat)

    if flat_shading:
        new_vertices = []


    for obj in objects:

        for polygon in obj.blender_object.data.polygons:
            face_indices = polygon.vertices
            loop_indices = polygon.loop_indices

            if len(face_indices) != len(loop_indices):
                raise RuntimeError('Invalid face data')

            if len(face_indices) > 3:
                raise RuntimeError('Triangulation error')
            elif len(polygon.vertices) < 3:
                continue
             


            if polygon.material_index >= 0 and len(obj.blender_object.data.materials) > 0 \
                    and len(bpy.data.materials) > 0:
                mat = blender_material_ref_to_materials_index[obj.blender_object.data.materials[polygon.material_index]]
            else:
                mat = materials[-1]
            
            if flat_shading:
                # Always create new vertices for flat shading
                for j in range(len(face_indices)):
                    index = face_indices[j]
                    loop_index = loop_indices[j]

                    uv = (0.0, 0.0)
                    if obj.blender_object.data.uv_layers.active is not None:
                        uv_ = obj.blender_object.data.uv_layers.active.data[loop_index].uv 
                        uv = (uv_.x, uv_.y)

                    vertex_index = obj.vertex_offset + index
                    vertex = vertices[vertex_index]

                    normal = obj.normal_matrix @ polygon.normal
                    normal.normalize()

                    vertex2 = Vertex(vertex.position, switch_coord_system(normal))
                    vertex2.uv = uv
                    vertex2.tangent = obj.normal_matrix @ obj.blender_object.data.loops[loop_index].tangent
                    vertex2.tangent.normalize()
                    vertex2.tangent = switch_coord_system(vertex2.tangent)
                    vertex2.bitangent_sign = obj.blender_object.data.loops[loop_index].bitangent_sign

                    mat.indices.append(len(new_vertices))
                    new_vertices.append(vertex2)
            else:
                for j in range(len(face_indices)):
                    index = face_indices[j]
                    loop_index = loop_indices[j]

                    uv = obj.blender_object.data.uv_layers.active.data[loop_index].uv 
                    uv = [uv.x, uv.y]

                    vertex_index = obj.vertex_offset + index
                    vertex = vertices[vertex_index]

                    tangent = obj.normal_matrix @ obj.blender_object.data.loops[loop_index].tangent
                    tangent.normalize()
                    tangent = switch_coord_system(tangent)
                    bitangent_sign = obj.blender_object.data.loops[loop_index].bitangent_sign

                    if vertex.uv is None:
                        vertex.uv = uv
                        vertex.tangent = tangent
                        vertex.bitangent_sign = bitangent_sign
                        mat.indices.append(vertex_index)
                    elif vertex.uv == uv and vertex.tangent == tangent and vertex.bitangent_sign == bitangent_sign:
                        mat.indices.append(vertex_index)
                    else:
                        # Look through already generated extra vertices.
                        vertex2_index = -1
                        for i in vertex.extra_vertex_indices:
                            if vertices[i].uv == uv and vertices[i].tangent == tangent and vertices[i].bitangent_sign == bitangent_sign:
                                vertex2_index = i
                                break

                        if vertex2_index == -1:
                            # Create a new vertex
                            vertex2 = vertex.split(uv, tangent, bitangent_sign)
                            index2 = len(vertices)
                            mat.indices.append(index2)
                            vertex.extra_vertex_indices.append(index2)
                            vertices.append(vertex2)
                        else:
                            # Use existing split vertex
                            mat.indices.append(vertex2_index)

    
    if flat_shading:
       vertices = new_vertices 

    optimised_materials = []

    i = 0
    while i < len(materials):
        mat = materials[i]

        j = i+1
        while j < len(materials):
            mat2 = materials[j]
            if mat.is_equiv(mat2):
                mat.indices += mat2.indices
                materials.remove(mat2)
            else:
                j += 1

        if len(mat.indices) >= 3:
            optimised_materials.append(mat)
        
        i += 1

    materials = optimised_materials

    if flat_shading:
        # Group vertices by material

        new_vertices = []
        for mat in materials:
            for i in mat.indices:
                new_vertices.append(vertices[i])
        vertices = new_vertices

    return objects,vertices,min_position,max_position,materials,bones

class Animation:
    pass

class BoneData:
    pass

def get_animation_data(loop_animations, bones):
    animations = []

    # Matrices (bone matrices assume blender coord system but vertices will be in vulkan coord system)
    to_vulkan_coords = Matrix()
    to_vulkan_coords[1][1] = 0.0
    to_vulkan_coords[1][2] = 1.0
    to_vulkan_coords[2][1] = -1.0
    to_vulkan_coords[2][2] = 0.0

    to_blender_coords = to_vulkan_coords.inverted()


    for action in bpy.data.actions:
        anim = Animation()
        anim.name = action.name

        anim.loops = False
        if anim.name in loop_animations:
            anim.loops = True
            loop_animations.remove(anim.name)

        # Make action current on all armatures
        for obj in bpy.data.objects:
            if hasattr(obj.data, 'bones') and hasattr(obj.data, 'animation_data'):
                obj.animation_data.action = action

        first_frame = int(action.frame_range[0])
        total_frames = int(action.frame_range[1]) - first_frame
        if anim.loops or total_frames == 2:
            # If looped, last frame should be same as first (so interpolation is correct)
            # If there are only 2 frames then this is a single pose (not animated)
            # ^ first_frames is (1,2) for both 1 and 2 frames of animations for some reason
            total_frames -= 1

        anim.frames = []

        for frame in range(total_frames):
            frame_data = []
            bpy.context.scene.frame_set(first_frame + frame)

            for bone_index,b in enumerate(bones):
                                
                obj = b.blender_object
                pose_bone = obj.pose.bones[b.name]
                
                edit_mode_transform = b.blender_bone.matrix_local
                pose_mode_transform = pose_bone.matrix
                
                m = to_vulkan_coords @ b.blender_object.matrix_world @\
                 pose_mode_transform @ edit_mode_transform.inverted() @\
                  b.blender_object.matrix_world.inverted() @ to_blender_coords

                
                bone_data = BoneData()
                bone_data.translation, bone_data.rotation, scale = m.decompose()
                bone_data.scale = (scale.x + scale.y + scale.z) / 3.0
               

                frame_data.append(bone_data)
                

            anim.frames.append(frame_data)
        animations.append(anim)

    if len(loop_animations) > 0:
        print('The following animations were referenced in the import file but were not found:', loop_animations)

    
    return bpy.context.scene.render.fps, animations

def create_model_file(context, asset_name, filepath, use_zstd, zstd_path_override, flat_shading, \
export_uv, export_tangents, loop_animations):
    if filepath[-6:] != '.asset':
        raise ValueError('Output file should be a .asset file')


    objects,vertices,min_position,max_position,materials,bones = get_mesh_data(flat_shading, export_tangents)
    position_value_range = max_position - min_position


    asset_text_file = '#' + asset_name + '\n'
    asset_text_file += 'TYPE MODEL\n'
    asset_text_file += 'VERTEX-COUNT ' + str(len(vertices)) + '\n'
    
    indices_count = 0

    if len(vertices) > 0:
        asset_text_file += 'BOUNDS-MINIMUM ' + str(min_position[0]) + ' ' + str(min_position[1]) + \
            ' ' + str(min_position[2]) + '\n'
        asset_text_file += 'BOUNDS-RANGE ' + str(position_value_range[0]) + ' ' + str(position_value_range[1]) + \
            ' ' + str(position_value_range[2]) + '\n'


        asset_text_file += 'VERTEX-ATTRIBUTES POSITION-NORMALISED'

        if len(bones) > 0:
            asset_text_file += ' BONE'

        if export_uv:
            asset_text_file += ' UV-FLOAT'

        asset_text_file += ' NORMAL'

        if export_tangents:
            asset_text_file += ' TANGENT'

        asset_text_file += '\n'



        if not flat_shading:
            for m in materials:
                indices_count += len(m.indices)

        if indices_count == 0:
            asset_text_file += 'INDICES-COUNT 0\n'
        else:
            asset_text_file += 'INDICES-TYPE ' + ("U32" if len(vertices) > 65536 else "U16") + '\n'
            asset_text_file += 'INDICES-COUNT ' + str(indices_count) + '\n'

        asset_text_file += 'MATERIALS-COUNT ' + str(len(materials)) + '\n'

        indices_or_vertices_offset = 0
        for m in materials:
            asset_text_file += 'MATERIAL ' + str(m.name) + '\n'
            asset_text_file += "COLOUR {:.3f} {:.3f} {:.3f}\n".format(m.colour[0], m.colour[1], m.colour[2])
            asset_text_file += "FLAT-COLOUR {:.3f} {:.3f} {:.3f}\n".format(m.flat_colour[0], m.flat_colour[1], m.flat_colour[2])
            asset_text_file += 'FIRST ' + str(indices_or_vertices_offset) + '\n'
            material_index_count = len(m.indices)
            indices_or_vertices_offset += material_index_count
            asset_text_file += 'COUNT ' + str(material_index_count) + '\n'
            if m.texture != '':
                asset_text_file += 'TEXTURE ' + m.texture + '\n'
            if m.normal_texture != '':
                asset_text_file += 'NORMAL-MAP ' + m.normal_texture + '\n'
            asset_text_file += 'SPECULAR ' + str(m.specular) + '\n'
            

    if len(bones) > 0:
        asset_text_file += 'BONES-COUNT ' + str(len(bones)) + '\n'
        for bone in bones:
            asset_text_file += 'BONE ' + bone.name + '\n'
            asset_text_file += 'HEAD ' + str(bone.head[0]) + ' ' + str(bone.head[1]) + ' ' + str(bone.head[2]) + '\n'
            asset_text_file += 'TAIL ' + str(bone.tail[0]) + ' ' + str(bone.tail[1]) + ' ' + str(bone.tail[2]) + '\n'
            if bone.parent_index >= 0:
                asset_text_file += 'PARENT ' + str(bone.parent_index) + '\n'

        fps,animations = get_animation_data(loop_animations, bones)
        asset_text_file += 'FRAME-RATE ' + str(fps) + '\n'
        asset_text_file += 'ANIMATIONS-COUNT ' + str(len(animations)) + '\n'

        for anim in animations:
            asset_text_file += 'ANIMATION ' + anim.name + '\n'
            asset_text_file += 'LOOPS ' + ('YES' if anim.loops else 'NO') + '\n'
            asset_text_file += 'FRAMES ' + str(len(anim.frames)) + '\n'

    
    writers = [ByteArrayWriter()]

    for v in vertices:
        def pos_get_float(i):
            return (v.position[i] - min_position[i]) / position_value_range[i]

        x = int(pos_get_float(0) * 2047.0)
        y = int(pos_get_float(1) * 2047.0)
        z = int(pos_get_float(2) * 1023.0)

        datum_rgb = ((x>>1) & 1023) | (((y>>1) & 1023) << 10) | ((z & 1023) << 20)
        datum_a = (x & 1) | ((y & 1) << 1)
        datum = (datum_a << 30) | datum_rgb

        writers[0].writeDWord(datum)

    if len(bones) > 0:
        w = ByteArrayWriter()
        writers.append(w)
        for v in vertices:
            if v.bone_indices[0] < 0 or v.bone_indices[0] > 255:
                v.bone_indices[0] = 0
            if v.bone_indices[1] < 0 or v.bone_indices[1] > 255:
                v.bone_indices[1] = 0
                v.bone_weight = 1.0

            w.writeWord((v.bone_indices[0] << 8) | v.bone_indices[1])
            
            w.writeWord(int(v.bone_weight * 65535.0))

    if export_uv:
        w = ByteArrayWriter()
        writers.append(w)
        for v in vertices:
            if v.uv is None:
                # w.writeWord(0)
                # w.writeWord(0)
                w.writeFloat(0)
                w.writeFloat(0)
            else:
                # 16-bit unorm coordinates half the size in bytes of UV coords but don't work with repeating textures
                # x = int((v.uv[0] % 1) * 65535.0)
                # y = int((1 - (v.uv[1] % 1)) * 65535.0)                
                # w.writeWord(x)
                # w.writeWord(y)

                w.writeFloat(v.uv[0])
                w.writeFloat(1-v.uv[1])

    w = ByteArrayWriter()
    writers.append(w)
    for v in vertices:

        nx = int(v.normal[0] * 511.0)
        ny = int(v.normal[1] * 511.0)
        nz = int(v.normal[2] * 511.0)
        
        datum = (nx & 1023) | ((ny & 1023) << 10) | ((nz & 1023) << 20)
        w.writeDWord(datum)

    if export_tangents:
        w = ByteArrayWriter()
        writers.append(w)
        for v in vertices:
            if v.tangent is None:
                v.tangent = [0.0, 0.0, 0.0]
            if v.bitangent_sign is None:
                v.bitangent_sign = 0

            x = int((v.tangent[0]) * 511.0)
            y = int((v.tangent[1]) * 511.0)
            z = int((v.tangent[2]) * 511.0)
            bi = int((v.bitangent_sign) * 1.0)
            
            datum = (x & 1023) | ((y & 1023) << 10) | ((z & 1023) << 20) | ((bi & 3) << 30)
            w.writeDWord(datum)
    


    if indices_count > 0:
        w = ByteArrayWriter()
        writers.append(w)
        if len(vertices) <= 65536:
            for m in materials:
                for i in m.indices:
                    w.writeWord(i)
        else:
            for m in materials:
                for i in m.indices:
                    w.writeDWord(i)

    if len(bones) > 0:

        for anim in animations:
            w = ByteArrayWriter()
            writers.append(w)
            for f in anim.frames:
                for i,b in enumerate(f):
                    w.writeFloat(b.rotation[0])
                    w.writeFloat(b.rotation[1])
                    w.writeFloat(b.rotation[2])
                    w.writeFloat(b.rotation[3])
                    w.writeFloat(b.translation[0])
                    w.writeFloat(b.translation[1])
                    w.writeFloat(b.translation[2])
                    w.writeFloat(b.scale)
            
        

    data = [x.get() for x in writers]

    write_files(asset_text_file, data, filepath, use_zstd, zstd_path_override)


    return {'FINISHED'}


if __name__ == "__main__":
    # https://blender.stackexchange.com/a/8405/74215
    output_file = sys.argv[sys.argv.index("--") + 1]  # get all args after "--"

    use_flat_shading = False

    loop_animations = []

    with open(sys.argv[1] + '.import', 'r') as f:
        lines = f.readlines()
        for l in lines:
            x = l.split()
            if len(x) >= 1: 
                if x[0].upper() == 'FLAT-SHADING':
                    use_flat_shading = True
                elif x[0].upper() == 'LOOP':
                    loop_animations.append(x[1])
                else:
                    print('Unrecognised import option: ' + x)

    print ('Outputting to ' + output_file)
    create_model_file(None, sys.argv[1], output_file, use_zstd=True, zstd_path_override='', \
    flat_shading=use_flat_shading, export_uv=True, export_tangents=True, loop_animations=loop_animations)
