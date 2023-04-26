import bpy
import numpy as np
import functools
import bpy
from pathlib import Path
from mathutils import Matrix
import math

C = bpy.context
data = bpy.data

########################################
# arguments #
########################################
gltfExtraIdentifier = 'CameraLensFrames'
LensFcurveTarget = 'CameraAction'
actionTarget = 'intro_anim'
cameraTarget = 'Camera'
########################################

print("///////////////////////////////////////////////////////////////////////////////")
print("sample Camera Lens fcurve and set as extra float array prop on target animation")

lensFcurveAction = None
lensFcurve = None
action = None
camera = None

#find camera object
for obj in data.cameras:
    if (obj.name == cameraTarget):
        camera = obj
        break

# iterate over all actions and find target camera with focal length fcurve and target action to add it to
for obj in data.actions:
    if (not lensFcurve and obj.fcurves and obj.id_root == 'CAMERA' and obj.name == LensFcurveTarget):
        for fc in obj.fcurves:
            if "lens" in fc.data_path:
                lensFcurve = fc
                lensFcurveAction = obj
    
    if (not action and obj.name == actionTarget):
        action = obj
        
    if (action and lensFcurve):
        break
    
print ("LensCurve: {0} in action {1}".format(lensFcurve,lensFcurveAction))
print ('action: %s' % action)
print ('camera: %s' % camera) 

if action and lensFcurve and camera:
    if gltfExtraIdentifier in action:
        print("Property reset")
        del action[gltfExtraIdentifier]
    
    action[gltfExtraIdentifier] = np.array([], dtype = np.float32)
                                    
    print ("setting {0} values from action {1} ".format(action.frame_range[1],lensFcurveAction.name) )
    for val in range(0,int(action.frame_range[1] + 1)):
        hFov = math.degrees(2 * math.atan(camera.sensor_width /(2 *  lensFcurve.evaluate(val))))
        action[gltfExtraIdentifier] = np.append(action[gltfExtraIdentifier], hFov)
else:
    print("Failed! Property NOT SET")

print ("Done!") 