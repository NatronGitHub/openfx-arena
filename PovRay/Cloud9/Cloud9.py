# -*- coding: utf-8 -*-
#
# Cloud/Sky generator using the PovRay node in Natron
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
    return "Clouds"

def getVersion():
    return 2

def getIconPath():
    return "Cloud9.png"

def getGrouping():
    return "Extra/Draw"

def getDescription():
    return "Cloud/Sky generator"

def createInstance(app,group):

    #Create all nodes in the group
    lastNode = app.createNode("fr.inria.built-in.Output", 1, group)
    lastNode.setScriptName("Output1")
    lastNode.setLabel("Output1")
    lastNode.setPosition(536.25, 262)
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
    lastNode.setPosition(536.25, 67)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupInput1 = lastNode

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
    lastNode.setPosition(538.5, 176.625)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupPovRay1 = lastNode

    param = lastNode.getParam("scene")
    if param is not None:
        param.setValue("// Cloud9 - Sky generator for Natron\n  //\n  // POV-Ray template scene\n  // based on FASTSKY Copyright March 2002 Rune S. Johansen - http://rsj.mobilixnet.dk\n  // Modified by Ole-André Rodlie - https://github.com/olear/openfx-arena\n  // Licenced under the GPLv2.\n\n  // Compat\n  #version 3.6\n\n  // Params\n  #declare camX = 0;\n  #declare camY = 0;\n  #declare camZ = 0;\n  // 50\n  #declare camAngle = _ANGLE_;\n  #declare camLX = 0;\n  #declare camLY = 10;\n  #declare camLZ = 20;\n  #declare camTrans = 10;\n  #declare turbulence1 = 1.0;\n  // 0.35\n  #declare turbulence2 = _TURBULENCE_;\n  #declare turbulence3 = 0.7;\n  #declare cloudScale = 300;\n  #declare horizonR = 0.25;\n  #declare horizonG = 0.35;\n  #declare horizonB = 0.80;\n  // 0.70\n  #declare skyR = _SKYR_;\n  // 0.80\n  #declare skyG = _SKYG_;\n  // 1.00\n  #declare skyB = _SKYB_;\n  #declare moveCloud=transform { translate x*clock }\n\n  camera {\n    //spherical\n    location <camX,camY,camZ> // 0,0,0\n    angle camAngle // 50 // if spherical:value value*3/4 (defvalue=120)\n    look_at  <camLX,camLY,camLZ> // 0,10,20\n    translate camTrans*y // value*y (defvalue=10)\n  }\n\n  light_source {<2,2,0>*1000000, color 3}\n\n  //plane {y, 0 pigment {checker color rgb 0.9, color rgb 1.0}}\n\n  #declare Clouds =\n  union {\n    plane {\n\ty, 260\n\ttexture {\n\t  finish {ambient 0 diffuse 0.7}\n\t  pigment {\n\t      bozo turbulence turbulence1 translate 200*y // turbulence 1.00\n\t      color_map {\n\t\t[0.5, rgb 1 transmit 1.0]\n\t\t[1.0, rgb 1 transmit 0.6]\n\t      }\n\t  transform moveCloud\n\t  warp { turbulence .3 }\n\t  transform { moveCloud inverse }\n\t      scale 0.125\n\t  }\n\t  scale 1500\n\t}\n    }\n    plane {y, 200}\n    plane {y, 220}\n    plane {y, 240}\n    texture {\n\tfinish {ambient 0 diffuse 0.7}\n\tpigment {\n\t  bozo turbulence turbulence2 translate 300*y // turbulence 0.35\n\t  color_map {\n\t      [0.5, rgb 1 transmit 1.0]\n\t      [0.8, rgb 1 transmit 0.5]\n\t  }\n\t  //scale 10 warp {turbulence turbulence3} scale 1/10 // turbulence 0.7\n\t  transform moveCloud\n\t  warp { turbulence turbulence2 }\n\t  transform { moveCloud inverse }\n\t  scale 0.125\n\t}\n\tscale 2500\n    }\n    texture {\n\tfinish {ambient 0 diffuse 0.5}\n\tpigment {\n\t  bozo turbulence turbulence2 translate 400*y // turbulence 0.35\n\t  color_map {\n\t      [0.5, rgb 1 transmit 1.0]\n\t      [0.8, rgb 1 transmit 0.4]\n\t  }\n\t  //scale 10 warp {turbulence turbulence3} scale 1/10 // turbulence 0.7\n\t  transform moveCloud\n\t  warp { turbulence turbulence2 }\n\t  transform { moveCloud inverse }\n\t  scale 0.125\n\t}\n\tscale 2500\n    }\n    hollow double_illuminate\n    scale 1/200\n  }\n\n  object {Clouds scale cloudScale} // distance above ground (y=0) to lowest parts of clouds (300)\n  background {color <horizonR,horizonG,horizonB>} // color at horizon (0.25,0.35,0.80)\n  fog {\n    color <skyR,skyG,skyB> // color at top of sky (0.70,0.80,1.00)\n    fog_type 2\n    distance 300\n    fog_offset 0\n    fog_alt 60\n}\n")
        del param

    param = lastNode.getParam("quality")
    if param is not None:
        param.setValue(9, 0)
        del param

    param = lastNode.getParam("startFrame")
    if param is not None:
        param.setValue(1, 0)
        del param

    param = lastNode.getParam("endFrame")
    if param is not None:
        param.setValue(250, 0)
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
    param = lastNode.createIntParam("start", "Start Frame")
    param.setMinimum(0, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(1000, 0)
    param.setDefaultValue(1, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.start = param
    del param

    param = lastNode.createIntParam("end", "End Frame")
    param.setMinimum(0, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(10000, 0)
    param.setDefaultValue(250, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.end = param
    del param

    param = lastNode.createIntParam("quality", "Quality")
    param.setMinimum(0, 0)
    param.setMaximum(11, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(9, 0)
    param.setDefaultValue(9, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.quality = param
    del param

    param = lastNode.createDoubleParam("angle", "Angle")
    param.setMinimum(0, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(70, 0)
    param.setDefaultValue(50, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.angle = param
    del param

    param = lastNode.createDoubleParam("turbulence", "Turbulence")
    param.setMinimum(0, 0)
    param.setMaximum(2, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(2, 0)
    param.setDefaultValue(0.35, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.turbulence = param
    del param

    param = lastNode.createColorParam("sky", "Sky", False)
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(1, 0)
    param.setDefaultValue(0.7, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(0, 1)
    param.setDisplayMaximum(1, 1)
    param.setDefaultValue(0.8, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(0, 2)
    param.setDisplayMaximum(1, 2)
    param.setDefaultValue(1, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.sky = param
    del param

    #Refresh the GUI with the newly created parameters
    lastNode.refreshUserParamsGUI()
    del lastNode

    #Now that all nodes are created we can connect them together, restore expressions
    groupOutput1.connectInput(0, groupPovRay1)

    groupPovRay1.connectInput(0, groupInput1)
    param = groupPovRay1.getParam("scene")
    param.setExpression("str(thisParam.get()).replace(\"_ANGLE_\",str(thisGroup.angle.get())).replace(\"_TURBULENCE_\",str(thisGroup.turbulence.get())).replace(\"_SKYR_\",str(thisGroup.sky.get().r)).replace(\"_SKYG_\",str(thisGroup.sky.get().g)).replace(\"_SKYB_\",str(thisGroup.sky.get().b))", False, 0)
    del param
    param = groupPovRay1.getParam("quality")
    param.setExpression("thisGroup.quality.get()", False, 0)
    del param
    param = groupPovRay1.getParam("startFrame")
    param.setExpression("thisGroup.start.get()", False, 0)
    del param
    param = groupPovRay1.getParam("endFrame")
    param.setExpression("thisGroup.end.get()", False, 0)
    del param

    try:
        extModule = sys.modules["CloudNineExt"]
    except KeyError:
        extModule = None
    if extModule is not None and hasattr(extModule ,"createInstanceExt") and hasattr(extModule.createInstanceExt,"__call__"):
        extModule.createInstanceExt(app,group)
