# THIS SCRIPT IS EXPERIMENTAL, UNFINISHED AND UNSUPPORTED

# RB: My intention of this script is to provide it as a backup before it gets lost.
# Feel free to experiment with it and to load the id Tech 4.x Flash guis into Blender without animation

import json, sys, bpy
import mathutils
import time
import os
from decimal import *
from math import *

#jsonfilename = "C:\\Projects\\RBDOOM-3-BFG\\base\\exported\\swf\\shell.json"
jsonfilename = "C:\\Projects\\RBDOOM-3-BFG\\mod_ragetoolkit\\exported\\swf\\hud.json"

basepath = "C:\\Projects\\RBDOOM-3-BFG\\mod_ragetoolkit\\"

start = time.time() 
data = json.loads( open( jsonfilename ).read() )
end = time.time()
print( "loading {0} took {1} seconds".format( jsonfilename, ( end - start ) ) )


scene = bpy.context.scene

#[ %f, %f, %f, %f, %f, %f ]", m.xx, m.yy, m.xy, m.yx, m.tx, m.ty
def transform_by_stylematrix( m, uv ):
    return ( ( uv[0] * m[0] ) + ( uv[1] * m[2] ) + m[4], ( uv[1] * m[1] ) + ( uv[0] * m[3] ) + m[5] )

def inverse_stylematrix( m ):
    inverse = [ 0, 0, 0, 0, 0, 0 ];
      
    det = ( ( m[0] * m[1] ) - ( m[2] * m[3] ) )
	#if( idMath::Fabs( det ) < idMath::FLT_SMALLEST_NON_DENORMAL )
	#{
	#	return *this;
	#}
    
    invDet = 1.0 / det
    
    inverse[0] = invDet * m[1]
    inverse[3] = invDet * -m[3]
    inverse[2] = invDet * -m[2]
    inverse[1] = invDet * m[0]
    #inverse.tx = invDet * ( xy * ty ) - ( yy * tx );
    #inverse.ty = invDet * ( yx * tx ) - ( xx * ty );
    return inverse


def convert_flash_object( entry, buildDict ):
    #print( entry["type"] )
    
    origin = ( 0, 0, 0 )
    characterID = entry["characterID"]
    characterIDStr = "characterID.{0}".format( entry["characterID"] )
    
    #print( "processing characterID {0} = {1}".format( characterID, entry["type"] ) )
    
    
    if entry["type"] == "IMAGE" and buildDict == True:
        
        imgfile = entry["imageFile"]
        imgfilename = os.path.normpath( basepath + imgfile )
        
        width = entry["width"]
        height = entry["height"]
        
        if os.path.exists( imgfilename ):
            print( "found " + imgfilename )
            
        # create material
        mat = None
        if characterIDStr in bpy.data.materials:
            mat = bpy.data.materials[ characterIDStr ]
        else:
            mat = bpy.data.materials.new( characterIDStr )
            mat.preview_render_type = 'FLAT'
            mat.use_nodes = True
            
            matNodes = mat.node_tree.nodes
            matLinks = mat.node_tree.links
            
            matDisney = matNodes.get( "Principled BSDF" )
            
            #bmattex.use_map_alpha = True
        
            # create image and assign to material
            image = None
            if characterIDStr in bpy.data.images:
                image = bpy.data.images[ characterIDStr ]
            else:
                image = bpy.data.images.load( imgfilename )
        
            #btex = None
            #if characterIDStr in bpy.data.textures:
            #    btex = bpy.data.textures[ characterIDStr ]
            #else:
            #    btex = bpy.data.textures.new( characterIDStr, type = 'IMAGE' )
            #btex.image = image
            
            # create image node
            nodeTex = matNodes.new( "ShaderNodeTexImage" )
            nodeTex.image = image
            
            # create the texture coordinate
            nodeUV = matNodes.new( "ShaderNodeTexCoord" )
            
            matLinks.new( nodeTex.inputs["Vector"], nodeUV.outputs["UV"] )
            matLinks.new( matDisney.inputs["Base Color"], nodeTex.outputs["Color"] )
            matLinks.new( matDisney.inputs["Alpha"], nodeTex.outputs["Alpha"] )
            
            mat.blend_method = 'BLEND'
        
    
    if entry["type"] == "SHAPE" and buildDict == True:
        
        if "startBounds" in entry:
            startBounds = entry["startBounds"]
            origin = ( startBounds[0], startBounds[1], 0 )
        
        meshName = "shape.{0}.mesh".format( characterID )
        
        mesh = bpy.data.meshes.new( meshName )
        meshObj = bpy.data.objects.new( meshName, mesh )
        meshObj.location = origin
        #ob.show_name = True
        
        # give object unique characterID so any sprite can reference it
        meshObj["characterID"] = characterID
 
        # link object to scene and make active
        #bpy.context.collection.objects.link( meshObj )
        #bpy.context.scene.objects.active = ob
        #ob.select = True
        
        #shapeName = "shape.{0}".format( characterID )
        shapeCollection = bpy.data.collections.new( characterIDStr )
        shapeCollection.color_tag = 'COLOR_04'
        bpy.context.scene.collection.children.link( shapeCollection )
        shapeCollection["characterID"] = characterID
        
        shapeCollection.objects.link( meshObj )
        bpy.data.collections[ "Dictionary" ].children.link( shapeCollection )
        
        # add to SWF dictionary
        #bpy.data.collections[ "Dictionary" ].objects.link( meshObj )
        
        # remove from scene default collection
        bpy.context.scene.collection.children.unlink( shapeCollection )
        #bpy.context.scene.collection.objects.unlink( meshObj )
        
        #bpy.ops.collection.create( name = characterIDStr )
        #group = bpy.data.collections[ characterIDStr ]
        
        
        #for i in range( len( entry["fillDraws"] ) ):   
        #    fillDraw = entry["fillDraws"][i]
        
        #if len( entry["fillDraws"] ) > 1:
        #    error
        
        # build basic mesh from all fillDraws and assign different materials to the faces
        verts = []
        faces = []
        numverts = 0
        
        if "fillDraws" in entry:
            for fillDraw in entry["fillDraws"]:
            
                if "startVerts" not in fillDraw:
                    continue
            
                # convert triangles to faces
                indices = fillDraw["indices"]
                for i in range( 0, len( indices ), 3 ):
                    faces.append( ( numverts + indices[i], numverts + indices[i+1], numverts + indices[i+2] ) )
                
                fillDraw["firstVert"] = numverts
                
                for v in fillDraw["startVerts"]:
                    verts.append( ( v["v"][0], v["v"][1], 0.0 ) )
                    numverts += 1
                    
        if "lineDraws" in entry:
            
            print( "characterID {0} = {1} has lineDraws".format( characterID, entry["type"] ) )
            
            for lineDraw in entry["lineDraws"]:
            
                if "startVerts" not in lineDraw:
                    continue
            
                # convert triangles to faces
                indices = lineDraw["indices"]
                for i in range( 0, len( indices ), 3 ):
                    faces.append( ( numverts + indices[i], numverts + indices[i+1], numverts + indices[i+2] ) )
                
                lineDraw["firstVert"] = numverts
                
                for v in lineDraw["startVerts"]:
                    verts.append( ( v["v"][0], v["v"][1], 0.0 ) )
                    numverts += 1
                
        mesh.from_pydata( verts, [], faces )
        
        # set mesh object to active
        bpy.context.view_layer.objects.active = meshObj
        meshObj.select_set( True )
        
        # convert tris to quads
        bpy.ops.object.mode_set(mode = 'EDIT')
        bpy.ops.mesh.tris_convert_to_quads()
        bpy.ops.object.editmode_toggle()
        
        # create uv map
        mesh.uv_layers.new( do_init = False, name="UVMap")
        #mesh.update()
        
        if "fillDraws" in entry:
            #for fillDraw in entry["fillDraws"]:
            for i in range( len( entry["fillDraws"] ) ):
                
                fillDraw = entry["fillDraws"][i]
                
                if fillDraw["style"]["type"] == "bitmap":
                    
                    bitmapID = fillDraw["style"]["bitmapID"]
                    
                    if bitmapID == 65535:
                        continue   
                    
                    stylematrix = inverse_stylematrix( fillDraw["style"]["startMatrix"] )
                   
                    # build uv coords
                    #firstVert = fillDraw["firstVert"]
                    
                    # FIXME only calculate UVs vertices of this bitmap
                    for i in range( len( mesh.vertices ) ):
                        v = mesh.vertices[i]
                        
                        width = startBounds[2] - startBounds[0]
                        height = startBounds[3] - startBounds[1]
                        
                        uv = ( ( ( v.co[0] - startBounds[0] ) * ( 1.0 / width ) * 1.0 ) , ( v.co[1] - startBounds[1] ) * ( 1.0 / height ) * 1.0 )
                        uv = ( 1.0 - uv[0], uv[1] )
                        mesh.uv_layers[0].data[i].uv = uv
                        
                        #uv = ( v.co[0] * ( 1.0 / startBounds["width"] ) * 20.0 , v.co[1] * ( 1.0 / startBounds["height"] ) * 20.0 )
                        #uv = ( 1.0 - uv[0], uv[1] )
                        #me.uv_layers[0].data[i].uv = transform_by_stylematrix( stylematrix, uv )
                        
                        if characterID == 135:
                            print( "v = ({0},{1}) uv = ({2},{3})".format( v.co[0], v.co[1], uv[0], uv[1] ) )
                    
                    
                    # assign bitmap
                    
                    bitmap = "characterID.{0}".format( bitmapID )
                    
                    if bitmap not in mesh.materials:
                        bitmapmat = bpy.data.materials[ "characterID.{0}".format( bitmapID ) ]
                        mesh.materials.append( bitmapmat )
                    
                    #mesh.update()
                    
                    
                if fillDraw["style"]["type"] == "solid":
                    
                    startColor = [ 1.0, 1.0, 1.0, 1.0 ]
                    
                    if "startColor" in fillDraw["style"]:
                        startColor = fillDraw["style"]["startColor"]
                      
                    solidmat = bpy.data.materials.new( "shape.{0}.fillDraw.{1}".format( characterID, i ) )
                    solidmat.preview_render_type = 'FLAT'
                    #solidmat.use_shadeless = True
                    #solidmat.use_transparency = True
                    solidmat.diffuse_color = ( startColor[0], startColor[1], startColor[2], startColor[3] )
                    mesh.materials.append( solidmat )
    
       
    
    if entry["type"] == "SPRITE" and buildDict == False:
        
        #spriteObj = None
        
        bpy.ops.object.add(
            type='EMPTY', 
            enter_editmode=False,
            location=origin)
            
        spriteObj = bpy.context.object
        
        if "mainsprite" in entry:
            
            spriteObj.name = "mainsprite"
            spriteObj.show_name = True
        else:
            spriteObj.name = "sprite.{0}".format( characterID )
        
        spriteObj["characterID"] = characterID
        
        
        spriteCollection = bpy.data.collections.new( characterIDStr )
        spriteCollection.color_tag = 'COLOR_03'
        bpy.context.scene.collection.children.link( spriteCollection )
        
        #spriteCollection = bpy.data.collections[ characterIDStr ]
        spriteCollection["characterID"] = characterID
        spriteCollection.objects.link( spriteObj )
        
        # add to SWF Sprites
        bpy.data.collections[ "Sprites" ].children.link( spriteCollection )
                        
        # remove from scene default collection
        bpy.context.scene.collection.children.unlink( spriteCollection )
        #bpy.context.scene.collection.objects.unlink( placeObj )
        
        for command in entry["commands"]:
            
            if command["type"] == "Tag_PlaceObject2" or command["type"] == "Tag_PlaceObject3":
                
                if "characterID" in command:
                    sourceID = command["characterID"]
                    
                    bpy.ops.object.select_all( action = 'DESELECT' )
                    
                    #print( bpy.context.selected_objects )
                    
                    #print( "searching sourceID ", sourceID )
                    
                    sourceCollection = None
                    for source in bpy.data.collections:
                        if "characterID" in source and source["characterID"] == sourceID:
                            sourceCollection = source
                            break
                    
                    if sourceCollection == None:
                        print( "missed clone source ", sourceID )
                    
                    else:
                        
                        #print( "duplicating target = ", target.name )
                        #print( bpy.context.selected_objects )
                        
                        # place instance at startMatrix
                        m = [1.0, 1.0, 0.0, 0.0, 0.0, 0.0 ]
                        
                        if "startMatrix" in command:
                            m = command["startMatrix"]
                            
                        #targetLocation = ( m[4], m[5], command["depth"] )
                        #targetLocation = spriteObj.location + mathutils.Vector( ( m[4], m[5], -1.0 ) )
                        
                        #targetScale = ( m[0], m[1], 1.0 )
                        
                        # https://blender.stackexchange.com/questions/156473/how-to-avoid-operator-collection-instance-add
                        
                        #bpy.ops.object.duplicate()
                        placeObj = bpy.data.objects.new( name = sourceCollection.name, object_data = None )
                        placeObj.instance_collection = sourceCollection
                        placeObj.instance_type = 'COLLECTION'
                        
                        parentCollection = spriteCollection
                        parentCollection.objects.link( placeObj )
                        
                        #parentCollection = bpy.context.view_layer.active_layer_collection
                        #parentCollection.collection.objects.link( placeObj )
                        
                        
                        
                        #if "mainsprite" in entry:
                        #    placeObj.name = "mainsprite"
                        #else:
                        #    placeObj.name = "sprite.{0}".format( characterID )
                            
                        if "name" in command:
                            placeObj.name = "{0}.{1}.{2}".format( spriteObj.name, command["name"], sourceID )
                            placeObj.show_name = True
                        else:
                            placeObj.name = "{0}.characterID.{1}".format( spriteObj.name, sourceID )
                            
                        placeObj.parent = spriteObj
                        #targetClone["characterID"] = -1
                        
                        placeObj.location = ( m[4], m[5], command["depth"] )
                        placeObj.scale = ( m[0], m[1], 1.0 )
                                       
                        bpy.ops.object.select_all( action = 'DESELECT' )
                        
                           

def create_default_collections():
    
    # remove all groups
    for group in bpy.data.collections:
        bpy.data.collections.remove( group )
        
    #bpy.ops.collection.create( name = "Dictionary" )
    #bpy.ops.collection.create( name = "Sprites" )
    collection = bpy.data.collections.new( "Dictionary" )
    bpy.context.scene.collection.children.link( collection )
    
    collection = bpy.data.collections.new( "Sprites" )
    bpy.context.scene.collection.children.link( collection )
    
    print( bpy.data.collections )
    

# clear old materials
for mat in bpy.data.materials:
    mat.user_clear()
    bpy.data.materials.remove( mat )

create_default_collections()

start = time.time() 

for entry in data["dict"]:
    convert_flash_object( entry, True )

i = 0
for entry in data["dict"]:
    convert_flash_object( entry, False )
    
    i += 1
    
    #if i == 4:
    #   break
    
end = time.time()
print( "importing {0} took {1} seconds".format( jsonfilename, ( end - start ) ) )