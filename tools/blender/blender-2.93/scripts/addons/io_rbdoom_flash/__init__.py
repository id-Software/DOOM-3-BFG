# THIS SCRIPT IS EXPERIMENTAL, UNFINISHED AND UNSUPPORTED

# RB: My intention of this script is to provide it as a backup before it gets lost.
# Feel free to experiment with it and to load the id Tech 4.x Flash guis into Blender without animation

import json, sys, bpy
import mathutils
import time
import os
from decimal import *
from math import *

jsonfilename = "C:\\Projects\\RBDOOM-3-BFG\\base\\exported\\swf\\hud.json"
basepath = "C:\\Projects\\RBDOOM-3-BFG\\base\\"

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
        
    
    if entry["type"] == "SHAPE" and buildDict == True:
        
        if "startBounds" in entry:
            startBounds = entry["startBounds"]
            origin = ( startBounds[0], startBounds[1], 0 )
        
        meshName = "shape.{0}".format( characterID )
        
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
        
        # add to SWF dictionary
        bpy.data.collections[ "Dictionary" ].objects.link( meshObj )
        
        # remove from scene default collection
        #bpy.context.scene.collection.objects.unlink( meshObj )
        
        bpy.ops.collection.create( name = characterIDStr )
        group = bpy.data.collections[ characterIDStr ]
        group["characterID"] = characterID
        
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
        
        #bpy.ops.object.editmode_toggle()
        #bpy.ops.mesh.quads_convert_to_tris( quad_method = 'BEAUTY', ngon_method = 'BEAUTY' )
        #bpy.ops.mesh.tris_convert_to_quads()
        #bpy.ops.uv.reset()
        #bpy.ops.uv.unwrap()
        #bpy.ops.object.editmode_toggle()
        
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
        
        spriteObj = None
        
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
            
            #return
        
        spriteObj["characterID"] = characterID
        
        # add to SWF Sprites
        bpy.data.collections[ "Sprites" ].objects.link( spriteObj )
        
        # remove from scene default collection
        bpy.context.scene.collection.objects.unlink( spriteObj )
        
        
        bpy.ops.collection.create( name = characterIDStr )
        spriteGroup = bpy.data.collections[ characterIDStr ]
        spriteGroup["characterID"] = characterID
        
        #sprite.select = True
                
        #scene.objects.active = ob
        
        for command in entry["commands"]:
            
            if command["type"] == "Tag_PlaceObject2":
                
                if "characterID" in command:
                    targetID = command["characterID"]
                    
                    # instantiate target
                    bpy.ops.object.select_all( action = 'DESELECT' )
                    
                    #print( bpy.context.selected_objects )
                    
                    #print( "searching targetID ", targetID )
                    
                    target = None
                    for group in bpy.data.collections:
                        if "characterID" in group and group["characterID"] == targetID:
                            target = group
                            break
                    
                    if target == None:
                        print( "missed target ", targetID )
                    
                    
                    if target != None:
                        
                        #print( "duplicating target = ", target.name )
                        #print( bpy.context.selected_objects )
                        
                        # place instance at startMatrix
                        m = [1.0, 1.0, 0.0, 0.0, 0.0, 0.0 ]
                        
                        if "startMatrix" in command:
                            m = command["startMatrix"]
                            
                        #targetLocation = ( m[4], m[5], -1.0 )
                        targetLocation = spriteObj.location + mathutils.Vector( ( m[4], m[5], -1.0 ) )
                        
                        targetScale = ( m[0], m[1], 1.0 )
                        
                        #bpy.ops.object.duplicate()
                        #bpy.ops.object.collection_instance_add( name = target.name )
                        bpy.ops.object.collection_instance_add( name = target.name, location = targetLocation, scale = targetScale )
                        bpy.ops.object.collection_link( collection = spriteGroup.name )
                        
                        targetClone = bpy.context.selected_objects[0]
                        
                        if "name" in command:
                            targetClone.name = "{0}.{1}".format( spriteObj.name, command["name"] )
                            targetClone.show_name = True
                        else:
                            targetClone.name = "{0}.characterID.{1}".format( spriteObj.name, targetID )
                            
                        #targetClone.parent = spriteObj
                        #targetClone["characterID"] = -1
                                       
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


create_default_collections()

start = time.time() 

for entry in data["dict"]:
    convert_flash_object( entry, True )

i = 0
for entry in data["dict"]:
    convert_flash_object( entry, False )
    
    i += 1
    
    #if i == 4:
    #    break
    
end = time.time()
print( "importing {0} took {1} seconds".format( jsonfilename, ( end - start ) ) )