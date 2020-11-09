bl_info = {
    "name": "Import RBDOOM-3-BFG JSON map format (.json)",
    "author": "Robert Beckebans",
    "version": (0, 8, 1),
    "blender": (2, 83, 1),
    "location": "File > Import > Doom 3 Map (.json)",
    "description": "Imports a Doom 3 map that uses the RBDOOM-3-BFG .json format",
    "warning": "",
    "wiki_url": "",
    "category": "Import-Export",
}

import json, sys, os, bpy, bpy_extras
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
        orientation_helper,
        path_reference_mode,
        axis_conversion,
        )

# path helper
def splitall(path):
    allparts = []
    while 1:
        parts = os.path.split(path)
        if parts[0] == path:  # sentinel for absolute paths
            allparts.insert(0, parts[0])
            break
        elif parts[1] == path: # sentinel for relative paths
            allparts.insert(0, parts[1])
            break
        else:
            path = parts[0]
            allparts.insert(0, parts[1])
    return allparts



def get_vector( data, key ):
    v = list( map( Decimal, data[ key ].split() ) )
    vec = mathutils.Vector( ( v[0], v[1], v[2] ) )
    return vec




def import_map( filename ):
    
    # find basepath which is either base or a mod folder
    filename_split = splitall( filename )
    print( "filename = ", filename_split )
    
    #return
    
    basepath = ''
    for part in filename_split:
        
        if part == "maps":
            break
        
        basepath = os.path.join( basepath, part )
    
    print( "basepath = ", basepath )
    
    deffilename = os.path.join( basepath, 'exported', 'entities.json' )
    
    print( "definitions file = ", deffilename )
    
    defs = json.loads( open( deffilename ).read() )

    #if "gravity" in defs["entities"]["aas48"]: 
    #    print( "cool aas48.gravity = {0}".format( data["entities"]["aas48"]["gravity"] ) )
    
    start = time.time() 
    data = json.loads( open( filename ).read() )
    end = time.time()
    print( "loading {0} took {1} seconds".format( filename, ( end - start ) ) )
    
    numEntities = len( data["entities"] )
    for entNum in range( numEntities ):
        
        ent = data["entities"][entNum]
     
        origin = ( 0, 0, 0 )
        entname = "worldspawn"
        classname = ent["classname"]
        
        if "origin" in ent:
            origin = get_vector( ent, "origin" )
            
        if "name" in ent:
            entname = ent["name"]
            
        print( "creating entity {0} of {1}: {2}".format( entNum, numEntities, entname ) )
            
        #group = bpy.data.groups.new( entname )
        #group = bpy.ops.object.empty_add()
        
        entObj = None
        
        if "light_radius" in ent:
            # create new lamp datablock
            lightData = bpy.data.lights.new( name = entname, type='POINT')
            
            """
            FIXME FIXME
            """
            radius = get_vector( ent, "light_radius" )
            print( radius.magnitude )
            lightData.energy = radius.magnitude * 750.0
            lightData.use_nodes = True

            # create new object with our lamp datablock
            entObj = bpy.data.objects.new( name = entname, object_data=lightData )

            # link light object
            bpy.context.collection.objects.link( entObj )

            # place light to a specified location
            entObj.location = origin
            entObj.show_name = True

            # and finally select it make active
            #entObj.select = True
            #bpy.context.collection.objects.active = entObj
            #bpy.context.view_layer.objects.active = entObj
            
        
        else:
        
            bpy.ops.object.add(
                type='EMPTY', 
                enter_editmode=False,
                location=origin)
        
            entObj = bpy.context.object
            entObj.name = entname
            entObj.show_name = True
            #entObj.select = True
            
        #bpy.context.scene.objects.active = entObj
        
        for key in ent:
            
            if key != "primitives":
                
                value = ent[ key ]
                
                if key in defs["entities"][classname]:
                    #print( "game property {0}.{1} = {2}, default = {3}".format( classname, key, value, defs["entities"][classname][key] ) )
                    #group[ key ] = ent[ key ]
                    
                    #bpy.ops.object.game_property_new( type = 'STRING', name = key )
                    
                    if key != "entity" and key != "name" and key != "origin" and key != "angle":
                        
                        if key == "nodiffuse":
                            entObj[ key ]= int( value )
                        else:
                            entObj[ key ] = value
                else:
                    #if key != "entity" and key != "name" and key != "origin" and key != "angle" and key != "rotation":
                    #    print( "unknown game property {0}.{1} = {2}".format( classname, key, value ) )
                    
                    #bpy.ops.object.game_property_new( type = 'STRING', name = key )
                    #bpy.context.object.value = value
                    
                    if key != "entity" and key != "name" and key != "origin" and key != "angle":
                        entObj[ key ] = value
        
        #print( "origin =", origin )
     
        if "primitives" in ent:
            for i in range( len( ent["primitives"] ) ):
                
                prim = ent["primitives"][i]
                #print( "numverts =", len( prim["verts"] ) )
                
                primName = "{0}.prim{1}".format( entname, i )
                
                mesh = bpy.data.meshes.new( primName )
                meshObj = bpy.data.objects.new( primName, mesh )
                #meshObj.location = origin
                meshObj.show_name = True
     
                # link object to scene and make active
                bpy.context.collection.objects.link( meshObj )
                #scn.objects.active = meshObj
                #meshObj.select = True
        
                #bpy.ops.object.group_link( group = group.name )
                meshObj.parent = entObj
     
                
                # build basic mesh
                verts = []
                for v in prim["verts"]:
                    verts.append( ( v["xyz"][0], v["xyz"][1], v["xyz"][2] ) )
                    
                faces = []
                for f in prim["polygons"]:
                    faces.append( f["indices"] )
                    
                mesh.from_pydata(verts, [], faces)
                
                # assign UVs
                mesh.uv_layers.new( do_init = False, name="UVMap")
                for i in range( len( prim["verts"] ) ):
                    v = prim["verts"][i]
                    mesh.uv_layers[0].data[i].uv = ( v["st"][0], 1.0 - v["st"][1] )
                    
                # collect materials
                materials = []
                for f in prim["polygons"]:
                    mat = f["material"]
                    if not mat in materials:
                        materials.append( mat )
                
                # create blender materials 
                for mat in materials:
                    #print( mat )
                    
                    bmat = None
                    if mat in bpy.data.materials:
                        bmat = bpy.data.materials[ mat ]
                    else:
                        bmat = bpy.data.materials.new( mat )
                    
                    mesh.materials.append( bmat )
                    
                # create blender groups for special materials like textures/common/clip or visportals
                groupDict = [
                    ( "textures/common/caulk", "caulk" ),
                    ( "textures/common/nodraw", "nodraw" ),
                    ( "textures/common/nodrawsolid", "nodrawsolid" ),
                    
                    ( "textures/common/collision", "collision" ),
                    ( "textures/common/clip", "clip" ),
                    ( "textures/common/player_clip", "player_clip" ),
                    ( "textures/common/player_clip", "clip" ),
                    ( "textures/common/full_clip", "full_clip" ),
                    ( "textures/common/full_clip", "clip" ),
                    ( "textures/common/monster_clip", "monster_clip" ),
                    ( "textures/common/monster_clip", "clip" ),
                    ( "textures/common/ik_clip", "ik_clip" ),
                    ( "textures/common/ik_clip", "clip" ),
                    ( "textures/common/movable_clip", "movable_clip" ),
                    ( "textures/common/movable_clip", "clip" ),
                    ( "textures/common/clip_plusmovables", "clip_plusmovables" ),
                    ( "textures/common/clip_plusmovables", "clip" ),
                    
                    ( "textures/common/cushion", "cushion" ),
                    ( "textures/common/slick", "slick" ),
                    ( "textures/common/noimpact", "noimpact" ),
                    ( "textures/common/nodrop", "nodrop" ),
                    ( "textures/common/ladder", "ladder" ),
                    
                    ( "textures/common/mirror", "mirror" ),
                    
                    ( "textures/common/shadow", "shadow" ),
                    ( "textures/common/shadow2", "shadow2" ),
                    
                    ( "textures/common/trigger", "trigger" ),
                    ( "textures/common/trigmulti", "trigger" ),
                    ( "textures/common/trigonce", "trigger" ),
                    ( "textures/common/trigtimer", "trigger" ),
                    ( "textures/common/trigrelay", "trigger" ),
                    ( "textures/common/trighurt", "trigger" ),
                    ( "textures/common/trigfade", "trigger" ),
                    ( "textures/common/triggui", "trigger" ),
                    ( "textures/common/trigentityname", "trigger" ),
                    ( "textures/common/trigentityname_once", "trigger" ),
                    
                    ( "textures/editor/entitygui", "gui" ),
                    ( "textures/editor/entitygui2", "gui" ),
                    ( "textures/editor/entitygui3", "gui" ),
                    ( "textures/editor/pda_gui", "gui" ),
                    
                    ( "textures/editor/aassolid", "aassolid" ),
                    ( "textures/editor/aassolid", "aas" ),
                    ( "textures/editor/aasobstacle", "aasobstacle" ),
                    ( "textures/editor/aassolid", "aas" ),
                    
                    ( "textures/editor/visportal", "visportal" )
                ]
                
                for mat in materials:
                    
                    for ( tex, groupName ) in groupDict:
                        if mat == tex:
                            group = None
                            if groupName in bpy.data.collections:
                                collection = bpy.data.collections[ groupName ]
                                collection.objects.link( meshObj )
                            else:
                                bpy.ops.collection.create( name = groupName )
                            
                
                    
                for i in range( len( prim["polygons"] ) ):
                    f = prim["polygons"][i]
                    mat = f["material"]
                    mesh.polygons[i].material_index = materials.index( mat )
                    #print( "polygon {0} material_index = {1}".format( i, me.polygons[i].material_index ) )
                
                mesh.update()
                
                #meshObj.select = False
            
        # deselect so we don't add objects into the wrong groups
        #entObj.select = False



class ImportMap( bpy.types.Operator, ImportHelper ):
    """Load a JSON file"""
    bl_idname = "import_scene.rbdoom_map_json"
    bl_label = "Import RBDOOM-3-BFG JSON map"
    bl_options = {'UNDO', 'PRESET'}

    directory = StringProperty()

    filename_ext = ".json"
    filter_glob = StringProperty( default="*.json", options = { 'HIDDEN' } )

    def execute(self, context):
        import_map( self.filepath )
        
        return { 'FINISHED' }
        

def menu_func_import(self, context):
    self.layout.operator( ImportMap.bl_idname, text="RBDOOM-3-BFG Map (.json)")


classes = (
    ImportMap,
)

def make_annotations(cls):
    """Converts class fields to annotations if running with Blender 2.8"""
    if bpy.app.version < (2, 80):
        return cls
    bl_props = {k: v for k, v in cls.__dict__.items() if isinstance(v, tuple)}
    if bl_props:
        if '__annotations__' not in cls.__dict__:
            setattr(cls, '__annotations__', {})
        annotations = cls.__dict__['__annotations__']
        for k, v in bl_props.items():
            annotations[k] = v
            delattr(cls, k)
    return cls





def register():
    for cls in classes:
        make_annotations( cls ) 
        bpy.utils.register_class( cls )

    bpy.types.TOPBAR_MT_file_import.append( menu_func_import )


def unregister():
    for cls in classes:
        bpy.utils.unregister_class( cls )

    bpy.types.INFO_MT_file_import.remove( menu_func_import )

if __name__ == "__main__":
    register()
    
    # test for fast iteration
    #filename = 'C:/Projects/RBDOOM-3-BFG/base/maps/testmaps/test_box.json'
    #filename = 'C:/Projects/RBDOOM-3-BFG/base/maps/game/pdas.json'
    #import_map( filename )