bl_info = {
    "name": "Export RBDOOM-3-BFG JSON map format (.json)",
    "author": "Robert Beckebans",
    "version": (2, 7),
    "blender": (2, 65, 4),
    "location": "File > Export > Doom 3 Map (.json)",
    "description": "Exports a Doom 3 map that uses the RBDOOM-3-BFG .json format",
    "warning": "",
    "wiki_url": "",
    "category": "Import-Export",
}

import json, sys, bpy, os
import mathutils
import time
from decimal import *
from math import *

from bpy.props import (
        StringProperty,
        BoolProperty,
        FloatProperty,
        EnumProperty,
        )
from bpy_extras.io_utils import (
        ImportHelper,
        ExportHelper,
        orientation_helper_factory,
        path_reference_mode,
        axis_conversion,
        )

def object_children_to_primitives( obj ):
    has_primitives = False
    
    for c in obj.children:
        if c.type == 'MESH':
            has_primitives = True
            break
        
    primitives = []
    if has_primitives:
        for c in obj.children:
            
            if c.type == 'MESH':
                primitive = {}
                verts = []
                
                if len( c.data.uv_layers ) > 0:
                    
                    for i in range( len( c.data.vertices ) ):
                        v = c.data.vertices[i]
                        st = c.data.uv_layers[0].data[i]
                        verts.append( { 'xyz': [ v.co[0], v.co[1], v.co[2] ], 'st': [ st.uv[0], 1.0 - st.uv[1] ], 'normal': [ v.normal[0], v.normal[1], v.normal[2] ] } )
                        
                else:
                    for v in c.data.vertices:
                        verts.append( { 'xyz': [ v.co[0], v.co[1], v.co[2] ], 'normal': [ v.normal[0], v.normal[1], v.normal[2] ] } )    
                    
                primitive['verts'] = verts
                primitives.append( primitive )
                
                polygons = []
                
                for p in c.data.polygons:
                    #print ( p.vertices )
                    indices = []
                    
                    for i in p.vertices:
                        indices.append( i )
                    
                    #for i in range( len( p.vertices ) ):
                    #    indices.append( p.vertices[i] )
                    
                    polygons.append( { 'material': c.data.materials[p.material_index].name, 'indices': indices } )
                    
                primitive['polygons'] = polygons
                
    return primitives

def object_to_entity( obj ):
    #return { 'name': obj.name }
    #return { { 'name': key } for key in obj.keys() }
    #return { "children":[{'name':key,"size":value} for key,value in obj.items()]}
    #return {'name':key, "size":value} for key,value in obj.items()
    #return json.dumps( obj.items(), indent = 4  )
    #return { "items": obj.items() }
    
    #return { "children":[{'name':key,"size":value} for key,value in obj.items()]}
    #return { obj.items() }
    #return sample
    
    #dict = {}
    #for key,value in obj.items():
    #   dict.insert( key, value )
    
    print( "creating entity {0} of type {1}".format( obj.name, obj.type ) )
    
    # make shallow copy without arrays of entries
    #ent = dict( obj )
    #ent = dict()
    
    ignore_keys = [ '_RNA_UI', 'cycles_visibility', 'cycles' ]
    
    ent = {}
    for key,value in obj.items():# if key not in ignore_keys:
        
       if key not in ignore_keys:
            ent[ key] = value
            print( "entity {0} key {1} has value {2}".format( obj.name, key, value ) )
    
    
    # TODO quakeify name if necessary
    ent['name'] = obj.name
    ent['origin'] = "%f %f %f" % ( obj.location[0], obj.location[1], obj.location[2] )
    #ent['blender_object_type'] = obj.type
    
    if obj.type == 'LAMP':
        
        data = obj.data
        color = data.color
        ent['_color'] = "%d %d %d" % ( color[0], color[1], color[2] )
        
        distance = data.distance
        ent['light_radius'] = "%f %f %f" % ( distance, distance, distance )
        
    if obj.type == 'EMPTY':
        primitives = object_children_to_primitives( obj )
        
        if len( primitives ) > 0:
            ent['primitives'] = primitives
   
    return ent
    
    
    
def export_map( filename ):
    ignore_types = ['CAMERA','MESH','ARMATURE']

    entities = [ obj for obj in bpy.context.scene.objects if obj.type not in ignore_types ]

    data = {
        "version": 3,
        "authoring_tool": "Blender " + bpy.app.version_string,
        "entities": [ object_to_entity( obj ) for obj in entities ] }

    j = json.dumps( data, indent = 4 )

    f = open( filename, 'w' )
    #print( j )
    f.write( j )
    f.close()


class ExportMap( bpy.types.Operator, ExportHelper ):
    """Save a JSON file"""
    bl_idname = "export_scene.rbdoom_map_json"
    bl_label = "Export RBDOOM-3-BFG JSON map"
    bl_options = {'UNDO', 'PRESET'}

    directory = StringProperty()

    filename_ext = ".json"
    filter_glob = StringProperty( default="*.json", options = { 'HIDDEN' } )

    def execute(self, context):
        export_map( self.filepath )
        
        return { 'FINISHED' }
        

def menu_func_export(self, context):
    self.layout.operator( ExportMap.bl_idname, text="RBDOOM-3-BFG Map (.json)")


def register():
    bpy.utils.register_module(__name__)

    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_module(__name__)

    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()
    
    """
    ignore_types = ['CAMERA','MESH','ARMATURE']

    entities = [ obj for obj in bpy.context.selected_objects if obj.type not in ignore_types ]

    data = {
        "version": 3,
        "authoring_tool": "Blender " + bpy.app.version_string,
        "entities": [ object_to_entity( obj ) for obj in entities ] }

    j = json.dumps( data, indent = 4 )
    print( j )
    """