# -*- coding: utf-8 -*-
#
# Cloud9 using the PovRay node in Natron
# Made by Ole-André Rodlie https://github.com/olear
# Released under the GPLv2
#

import NatronEngine
import sys

#Try to import the extensions file where callbacks and hand-written code should be located.
try:
    from Cloud9Ext import *
except ImportError:
    pass

def getPluginID():
    return "net.fxarena.natron.Cloud9"

def getLabel():
    return "Cloud9"

def getVersion():
    return 1

def getIconPath():
    return "Cloud9.png"

def getGrouping():
    return "Extra/3D"

def getDescription():
    return "Cloud/Sky generator using POV-Ray"

def createInstance(app,group):

    #Create all nodes in the group
    lastNode = app.createNode("fr.inria.built-in.Output", 1, group)
    lastNode.setScriptName("Output1")
    lastNode.setLabel("Output1")
    lastNode.setPosition(536.25, 195)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.699992, 0.699992, 0.699992)
    groupOutput1 = lastNode

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("Info")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("fr.inria.built-in.Input", 1, group)
    lastNode.setScriptName("Input1")
    lastNode.setLabel("Input1")
    lastNode.setPosition(536.25, 38)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupInput1 = lastNode

    param = lastNode.getParam("optional")
    if param is not None:
        param.setValue(True)
        del param

    param = lastNode.getParam("Node")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("Info")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("net.fxarena.openfx.PovRay", 1, group)
    lastNode.setScriptName("PovRay1")
    lastNode.setLabel("PovRay1")
    lastNode.setPosition(530.75, 117.5)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupPovRay1 = lastNode

    param = lastNode.getParam("scene")
    if param is not None:
        param.setValue("#version 3.7;\n#include \"colors.inc\"\n#include \"textures.inc\"\n#include \"shapes.inc\"\n#include \"shapes2.inc\"\n#include \"functions.inc\"\n#include \"math.inc\"\n#include \"transforms.inc\"\n\nglobal_settings{ assumed_gamma 1.0 }\n#default{ finish{ ambient 0.1 diffuse 0.9 }}\n\n#declare Camera_Position = < _CAM_POS_X_, _CAM_POS_Y_,_CAM_POS_Z_>;\n#declare Camera_Look_At = < _CAM_LOOK_X_, _CAM_LOOK_Y_, _CAM_LOOK_Z_>;\n#declare Camera_Angle = _CAM_ANGLE_;\n\n#declare Bumps_Turbulence = _CLOUD_TURB_;\n#declare Bumps_Scale = _CLOUD_SCALE_;\n#declare Bumps_Translate = <_CLOUD_TRANS_X_, _CLOUD_TRANS_Y_,_CLOUD_TRANS_Z_>;\n\ncamera{ location  Camera_Position\n        right     x*image_width/image_height\n        angle     Camera_Angle   \n        look_at   Camera_Look_At\n      }\n\nlight_source{<-1500,2500,-2500> color White}\n\n// sky ---------------------------------------------------------------------\nsky_sphere { pigment { gradient <0,1,0>\n                       color_map { [0.00 rgb <1.0,1.0,1.0>*0.8]\n                                   [0.15 rgb <0.1,0.3,0.7>*0.6]\n                                   [0.95 rgb <0.1,0.3,0.7>*0.6]\n                                   [1.00 rgb <1.0,1.0,1.0>*0.8] \n                                 } \n                       scale 2         \n                     } // end of pigment\n           } //end of skysphere\n\n// the media clouds \nbox { <-1,-1,-1>,<1,1,1>  \n  texture {\n    pigment {\n     rgbf 1  // color Clear\n    }\n  }\n  interior {\n   media {\n    method 3\n    //intervals 2\n    samples 10,10  // increese to 20,20\n    absorption 1\n    emission 0.5\n    scattering { 0.8,<1,1,1>*0.5}\n  \n    density{ bozo //bumps\n             color_map {\n              [0.00 rgb 0]\n              [0.50 rgb 0.01]\n              [0.65 rgb 0.1]\n              [0.75 rgb 0.5]\n              [1.00 rgb 0.2]\n             } // end color_map\n             turbulence Bumps_Turbulence\n             scale Bumps_Scale\n             translate Bumps_Translate   \n           } // end density\n    \n    density{  boxed // or: spherical\n              color_map { \n               [0.0 rgb 0]    // border\n               [0.1 rgb 0.05]   \n               [1.0 rgb 1]    // center\n              } // end color_map\n             scale <1,1,1>*1\n           } // end density\n\n   } // end media\n  } // end interior\n hollow\n scale<1,0.5, 1> \n translate<0,0.5,0>\n //----------------------------------------\nscale 15\ntranslate<0,02,30>\n}//---------------------------------------- ")
        del param

    param = lastNode.getParam("quality")
    if param is not None:
        param.setValue(9, 0)
        del param

    param = lastNode.getParam("Source_channels")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("Source_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("Output_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("Node")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("Info")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode




    #Create the parameters of the group node the same way we did for all internal nodes
    lastNode = group
    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("Info")
    if param is not None:
        param.setVisible(False)
        del param


    #Create the user-parameters
    lastNode.userNatron = lastNode.createPageParam("userNatron", "User")
    param = lastNode.createDouble3DParam("location", "Camera Location")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(0, 1)
    param.setDisplayMaximum(100, 1)
    param.setDefaultValue(1, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(0, 2)
    param.setDisplayMaximum(100, 2)
    param.setDefaultValue(-3, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust camera location")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.location = param
    del param

    param = lastNode.createDouble3DParam("view", "Camera View")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(0, 1)
    param.setDisplayMaximum(100, 1)
    param.setDefaultValue(2.1, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(0, 2)
    param.setDisplayMaximum(100, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust camera view")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.view = param
    del param

    param = lastNode.createIntParam("angle", "Camera Angle")
    param.setMinimum(0, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setDefaultValue(65, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust camera angle")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.angle = param
    del param

    param = lastNode.createDoubleParam("turbulence", "Turbulence")
    param.setMinimum(0, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(10, 0)
    param.setDefaultValue(0.85, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust turbulence")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.turbulence = param
    del param

    param = lastNode.createDoubleParam("scale", "Scale")
    param.setMinimum(0, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(10, 0)
    param.setDefaultValue(0.75, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust scale")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.scale = param
    del param

    param = lastNode.createDouble3DParam("translate", "Translate")
    param.setMinimum(0, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setDefaultValue(1, 0)
    param.setMinimum(0, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(0, 1)
    param.setDisplayMaximum(100, 1)
    param.setDefaultValue(0.75, 1)
    param.setMinimum(0, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(0, 2)
    param.setDisplayMaximum(100, 2)
    param.setDefaultValue(2, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust position")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.translate = param
    del param

    param = lastNode.createIntParam("quality", "Render Quality")
    param.setMinimum(0, 0)
    param.setMaximum(11, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(11, 0)
    param.setDefaultValue(9, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust render quality, 9 is recommended for output, lower for adjusting params.")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.quality = param
    del param

    #Refresh the GUI with the newly created parameters
    lastNode.refreshUserParamsGUI()
    del lastNode

    #Now that all nodes are created we can connect them together, restore expressions
    groupOutput1.connectInput(0, groupPovRay1)

    groupPovRay1.connectInput(0, groupInput1)
    param = groupPovRay1.getParam("scene")
    param.setExpression("str(thisParam.get()).replace(\"_CAM_POS_X_\",str(thisGroup.location.get().x)).replace(\"_CAM_POS_Y_\",str(thisGroup.location.get().y)).replace(\"_CAM_POS_Z_\",str(thisGroup.location.get().z)).replace(\"_CAM_LOOK_X_\",str(thisGroup.view.get().x)).replace(\"_CAM_LOOK_Y_\",str(thisGroup.view.get().y)).replace(\"_CAM_LOOK_Z_\",str(thisGroup.view.get().z)).replace(\"_CAM_ANGLE_\",str(thisGroup.angle.get())).replace(\"_CLOUD_TURB_\",str(thisGroup.turbulence.get())).replace(\"_CLOUD_SCALE_\",str(thisGroup.scale.get())).replace(\"_CLOUD_TRANS_X_\",str(thisGroup.translate.get().x)).replace(\"_CLOUD_TRANS_Y_\",str(thisGroup.translate.get().y)).replace(\"_CLOUD_TRANS_Z_\",str(thisGroup.translate.get().z))", False, 0)
    del param
    param = groupPovRay1.getParam("quality")
    param.setExpression("thisGroup.quality.get()", False, 0)
    del param

    try:
        extModule = sys.modules["Cloud9Ext"]
    except KeyError:
        extModule = None
    if extModule is not None and hasattr(extModule ,"createInstanceExt") and hasattr(extModule.createInstanceExt,"__call__"):
        extModule.createInstanceExt(app,group)
