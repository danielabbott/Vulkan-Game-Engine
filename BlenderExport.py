import bpy
import io
import os
import subprocess
import json
import struct
import mathutils
from mathutils import Vector
import math
from math import floor
import bmesh

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


def write_files(json_data, data_to_write, filepath, use_zstd, zstd_path_override):
    if use_zstd:
        zstd_cmd = 'zstd' if zstd_path_override=='' else zstd_path_override
        result = subprocess.run([zstd_cmd, '-19'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, input=data_to_write)
        if result.returncode != 0:
            print(result.stderr.decode())
            raise RuntimeError('zstd error')
        compressed = result.stdout

        uncompressed_length = len(data_to_write)
        uncompressed_length_in_sectors = floor(uncompressed_length/512)
        compressed_length_in_sectors = floor(len(compressed)/512)
        compression_ratio = len(compressed) / uncompressed_length

        if uncompressed_length_in_sectors != compressed_length_in_sectors and compression_ratio <= 0.9:
            json_data['compression_ratio'] = compression_ratio
            json_data['compression'] = 'zstd'
            data_to_write = compressed
    
    with open(filepath, 'w') as f:
        f.write(json.dumps(json_data, indent=4))

    with open(filepath.replace('.json', '.pigeon-data'), 'wb') as f:
        f.write(data_to_write)

    

class Object():
    def __init__(self, blender_object, vertex_offset, vertex_count):
        self.blender_object = blender_object
        self.vertex_offset = vertex_offset
        self.vertex_count = vertex_count

class Vertex():
    def __init__(self, position, normal):
        self.position, self.normal = position, normal
        self.uv = None

        # Tangent is average of face tangents
        self.tangent_sum = Vector((0.0,0.0,0.0),)
        self.tangent_divisor = 0.0

        # If a vertex has multiple UV coords, it is split
        # The indices into the vertices list are stored here
        self.extra_vertex_indices = []

    def split(self, tangent, uv):
        v = Vertex(self.position, self.normal)
        v.tangent_sum, v.uv = tangent, uv
        v.tangent_divisor = 1.0
        return v


def switch_coord_system(coords):
    # Objects face +Z in OpenGL coordinate system
    return Vector([coords[0], coords[2], -coords[1]])


def get_objects_and_vertices(flat_shading, export_tangents):
    objects = []

    max_position = Vector ([-9999.0,-9999.0,-9999.0])
    min_position = Vector ([9999.0,9999.0,9999.0])

    vertices = []

    # Number of vertices before splitting vertexes with multiple texture coordinates
    originalVertexArraySize = 0


    for blender_object in bpy.data.objects:
        if not hasattr(blender_object.data, 'polygons'):
            continue

        obj = Object(blender_object, len(vertices), len(blender_object.data.vertices))
        objects.append(obj)

        if flat_shading:
            blender_object.data.calc_normals()
        if export_tangents:
            try:
                blender_object.data.calc_tangents()
            except:
                raise RuntimeError('calc_tangents error. Problematic object: ' + obj.blender_object.name)
    


        obj.normal_matrix = blender_object.matrix_world.to_3x3()
        obj.normal_matrix.invert()
        obj.normal_matrix = obj.normal_matrix.transposed().to_4x4()
        obj.normal_matrix = blender_object.matrix_world


        for blender_vertex in blender_object.data.vertices:
            normal = None if flat_shading else switch_coord_system(obj.normal_matrix @ blender_vertex.normal)
            vertex = Vertex(switch_coord_system(blender_object.matrix_world @ blender_vertex.co), normal)
            vertices.append(vertex)

            if vertex.position.x > max_position.x:
                max_position.x = vertex.position.x
            if vertex.position.y > max_position.y:
                max_position.y = vertex.position.y
            if vertex.position.z > max_position.z:
                max_position.z = vertex.position.z
            if vertex.position.x < min_position.x:
                min_position.x = vertex.position.x
            if vertex.position.y < min_position.y:
                min_position.y = vertex.position.y
            if vertex.position.z < min_position.z:
                min_position.z = vertex.position.z

    return objects,vertices,min_position,max_position

def get_mesh_data(flat_shading, export_tangents):
    objects,vertices,min_position,max_position = get_objects_and_vertices(flat_shading, export_tangents)
    

    if flat_shading:
        new_vertices = []
        indices = None
    else:
        indices = []

    for obj in objects:

        for polygon in obj.blender_object.data.polygons:
            face_indices = polygon.vertices
            loop_indices = polygon.loop_indices

            if len(face_indices) != len(face_indices):
                raise RuntimeError('Invalid face data')

            if len(face_indices) > 4:
                raise RuntimeError('Only triangles and quadrilateral faces are supported. Problematic object: ' + obj.blender_object.name)
            elif len(polygon.vertices) < 3:
                continue

            if len(face_indices) == 4:
                face_indices = [face_indices[0], face_indices[1], face_indices[2], face_indices[2], face_indices[3], face_indices[0]]
                loop_indices = (loop_indices[0], loop_indices[1], loop_indices[2], loop_indices[2], loop_indices[3], loop_indices[0])
            
            face_tangent_local = obj.blender_object.data.loops[loop_indices[0]].tangent
            face_tangent_world = Vector([face_tangent_local[0], face_tangent_local[1], face_tangent_local[2], 0.0])
            face_tangent = switch_coord_system(obj.blender_object.matrix_world @ Vector([face_tangent_world[0], face_tangent_world[1], face_tangent_world[2]]))
            

                

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

                    vertex2 = Vertex(vertex.position, switch_coord_system(obj.normal_matrix @ polygon.normal))
                    vertex2.uv = uv
                    vertex2.tangent_sum = face_tangent
                    vertex2.tangent_divisor = 1.0

                    new_vertices.append(vertex2)
            else:
                for j in range(len(face_indices)):
                    index = face_indices[j]
                    loop_index = loop_indices[j]

                    uv = (0.0, 0.0)
                    if obj.blender_object.data.uv_layers.active is not None:
                        uv_ = obj.blender_object.data.uv_layers.active.data[loop_index].uv 
                        uv = (uv_.x, uv_.y)

                    vertex_index = obj.vertex_offset + index
                    vertex = vertices[vertex_index]

                    if vertex.uv is None:
                        vertex.uv = uv
                        vertex.tangent_sum = face_tangent
                        vertex.tangent_divisor = 1.0
                        indices.append(vertex_index)
                    elif vertex.uv == uv:
                        vertex.tangent_sum += face_tangent
                        vertex.tangent_divisor += 1.0
                        indices.append(vertex_index)
                    else:
                        # Look through already generated extra vertices.
                        vertex2_index = -1
                        for i in vertex.extra_vertex_indices:
                            if vertices[i].uv == uv:
                                vertex2_index = i
                                break

                        if vertex2_index == -1:
                            # Create a new vertex
                            vertex2 = vertex.split(face_tangent, uv)
                            index2 = len(vertices)
                            indices.append(index2)
                            vertex.extra_vertex_indices.append(index2)
                            vertices.append(vertex2)
                        else:
                            # Use existing split vertex
                            vertex = vertices[vertex2_index]
                            vertex.tangent_sum += face_tangent
                            vertex.tangent_divisor += 1.0
                            indices.append(vertex2_index)

    if flat_shading:
       vertices = new_vertices 

    return objects,vertices,min_position,max_position,indices

def create_model_file(context, filepath, use_zstd, zstd_path_override, flat_shading, export_uv, export_tangents):
    if filepath[-5:] != '.json':
        raise ValueError('Output file should be a .json file')

    objects,vertices,min_position,max_position,indices = get_mesh_data(flat_shading, export_tangents)
    position_value_range = max_position - min_position


    json_data = {
        "asset_type": "model",
        "compression": "none",
        "decompressed_size": 0,
        "vertex_count": len(vertices),
        "bounds_minimum": [min_position[0], min_position[1], min_position[2]],
        "bounds_range": [position_value_range[0], position_value_range[1], position_value_range[2]],
    }
    writer = ByteArrayWriter()

    data_offset = 0
    attribute_position = {
        "attribute_type": "PositionNormalised",
        "data_offset": data_offset,
        "data_size": len(vertices)*4
    }
    data_offset += len(vertices)*4

    attributes = [attribute_position]

    attribute_normal = {
        "attribute_type": "Normal",
        "data_offset": data_offset,
        "data_size": len(vertices)*4
    }
    data_offset += len(vertices)*4
    attributes.append(attribute_normal)

    if export_tangents:
        attribute_tangent = {
            "attribute_type": "Tangent",
            "data_offset": data_offset,
            "data_size": len(vertices)*4
        }
        data_offset += len(vertices)*4
        attributes.append(attribute_tangent)
    

    if export_uv:
        attribute_uv = {
            "attribute_type": "Texture",
            "data_offset": data_offset,
            "data_size": len(vertices)*4
        }
        data_offset += len(vertices)*4
        attributes.append(attribute_uv)
    


    large_indices = len(vertices) > 65536
    json_data["indices_type"] = "big" if large_indices else "small"
    json_data["indices_data_offset"] = data_offset
    json_data["indices_data_size"] = 0 if indices == None else (len(indices) * (4 if large_indices else 2))
    json_data["indices_count"] = 0 if indices == None else len(indices)

    for v in vertices:
        def pos_get_float(i):
            return (v.position[i] - min_position[i]) / position_value_range[i]

        x = int(pos_get_float(0) * 2047.0)
        y = int(pos_get_float(1) * 2047.0)
        z = int(pos_get_float(2) * 1023.0)

        datum_rgb = ((x>>1) & 1023) | (((y>>1) & 1023) << 10) | ((z & 1023) << 20)
        datum_a = (x & 1) | ((y & 1) << 1)
        datum = (datum_a << 30) | datum_rgb

        writer.writeDWord(datum)


    for v in vertices:
        nx = int(v.normal[0] * 511.0)
        ny = int(v.normal[1] * 511.0)
        nz = int(v.normal[2] * 511.0)
        
        datum = (nx & 1023) | ((ny & 1023) << 10) | ((nz & 1023) << 20)
        writer.writeDWord(datum)

    if export_tangents:
        for v in vertices:
            x = int((v.tangent_sum[0] / v.tangent_divisor) * 511.0)
            y = int((v.tangent_sum[1] / v.tangent_divisor) * 511.0)
            z = int((v.tangent_sum[2] / v.tangent_divisor) * 511.0)
            
            datum = (x & 1023) | ((y & 1023) << 10) | ((z & 1023) << 20)
            writer.writeDWord(datum)
    

    if export_uv:
        for v in vertices:
            if v.uv is None:
                writer.writeDWord(0)
            else:
                x = int(clamp(v.uv[0], 0.0, 1.0) * 65535.0)
                y = int(clamp(v.uv[1], 0.0, 1.0) * 65535.0)
                
                datum = (x & 1023) | ((y & 1023) << 16)
                writer.writeDWord(datum)

    if indices != None:
        if len(vertices) <= 65536:
            for i in indices:
                writer.writeWord(i)
        else:
            for i in indices:
                writer.writeDWord(i)

    json_data['vertex_attributes'] = attributes
    json_data['decompressed_size'] = writer.size()
    write_files(json_data, writer.get(), filepath, use_zstd, zstd_path_override)


    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "pigeon_model_export.export"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export"

    # ExportHelper mixin class uses this
    filename_ext = ".json"

    filter_glob: StringProperty(
        default="*.json",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    use_zstd: BoolProperty(
        name="ZStd",
        description="Compress data with ZStd",
        default=False,
    )

    zstd_path_override: StringProperty(
        name="ZStd path override",
        description="Specify location of zstd",
        default="",
        maxlen=260,
    )

    flat_shading: BoolProperty(
        name="Flat Shading",
        description="Per-face normals",
        default=False,
    )

    # type: EnumProperty(
    #     name="Example Enum",
    #     description="Choose between two items",
    #     items=(
    #         ('OPT_A', "First Option", "Description one"),
    #         ('OPT_B', "Second Option", "Description two"),
    #     ),
    #     default='OPT_A',
    # )

    def execute(self, context):
        return create_model_file(context, self.filepath, self.use_zstd, self.zstd_path_override, flat_shading)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="Text Export Operator")


def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    #register()

    # test call
    #bpy.ops.pigeon_model_export.export('INVOKE_DEFAULT')

    
    create_model_file(None, '/home/daniel/Pigeon/ExampleAssets/Models/alien.json', use_zstd=True, zstd_path_override='', flat_shading=False, export_uv=True, export_tangents=True)
