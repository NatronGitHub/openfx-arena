# -*- coding: utf-8 -*-
#
# Text3D using the PovRay node in Natron
# Made by Ole-André Rodlie https://github.com/olear
# Released under the GPLv2
#

import NatronEngine
import sys

#Try to import the extensions file where callbacks and hand-written code should be located.
try:
    from Text3DExt import *
except ImportError:
    pass

def getPluginID():
    return "net.fxarena.natron.Text3D"

def getLabel():
    return "Text3D"

def getVersion():
    return 1

def getIconPath():
    return "Text3D.png"

def getGrouping():
    return "Draw"

def getDescription():
    return "Generate 3D text using POV-Ray"

def createInstance(app,group):

    #Create all nodes in the group
    lastNode = app.createNode("fr.inria.built-in.Output", 1, group)
    lastNode.setScriptName("Output1")
    lastNode.setLabel("Output1")
    lastNode.setPosition(344.375, 195)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.699992, 0.699992, 0.699992)
    groupOutput1 = lastNode

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("fr.inria.built-in.Input", 1, group)
    lastNode.setScriptName("Input1")
    lastNode.setLabel("Input1")
    lastNode.setPosition(344.375, 40)
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
    lastNode.setPosition(344.375, 107.75)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupPovRay1 = lastNode

    param = lastNode.getParam("scene")
    if param is not None:
        param.setValue("background {\nrgb <1,1,1>\n}\n\ncamera {\nright   x*image_width/image_height\nlocation <_LOCATION_X_, _LOCATION_Y_, _LOCATION_Z_>\nlook_at <_VIEW_X_,_VIEW_Y_,_VIEW_Z_>\nangle _CAM_ANGLE_\n}\n\nlight_source {\n<1, 4, 3>, <1,1,1>\nfade_distance 4 fade_power 2\narea_light x*8, y*8, 12,12 circular orient adaptive 1\n}\n\nlight_source {\n<-3, 3, -3>, <1,0,0>\nfade_distance 4 fade_power 2\narea_light x*8, y*8, 12,12 circular orient adaptive 1\n}\n\nlight_source {\n<0, 3, -3>, <0,1,0>\nfade_distance 4 fade_power 2\narea_light x*8, y*8, 12,12 circular orient adaptive 1\n}\n\nlight_source {\n<3, 3, -3>, <0,0,1>\nfade_distance 4 fade_power 2\narea_light x*8, y*8, 12,12 circular orient adaptive 1\n}\n\nplane {\ny, -0.125\npigment { rgb <.5059, .8667, 1> }\nfinish { specular 1 reflection .3 }\n}\n\ntext {\nttf \"_USE_FONT_\" \"_ENTER_TEXT_\" _LETTER_DEPTH_, _LETTER_SPACE_*x\npigment { rgb <_COLOR_R_,_COLOR_G_,_COLOR_B_> }\nfinish {\n\tambient .1\n\tdiffuse .1\n\tspecular 1\n\troughness .001\n\tmetallic\n\treflection { .75 metallic }\n}\nrotate <_ROTATE_X_,_ROTATE_Y_,_ROTATE_Z_>\ntranslate -2.9*x\n}")
        del param

    param = lastNode.getParam("quality")
    if param is not None:
        param.setValue(0, 0)
        del param

    param = lastNode.getParam("antialiasing")
    if param is not None:
        param.setValue(0, 0)
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
    param.setDisplayMinimum(-100, 0)
    param.setDisplayMaximum(100, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(-100, 1)
    param.setDisplayMaximum(100, 1)
    param.setDefaultValue(3, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(-100, 2)
    param.setDisplayMaximum(100, 2)
    param.setDefaultValue(-12, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Camera location")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.location = param
    del param

    param = lastNode.createDouble3DParam("view", "Camera View")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(-100, 0)
    param.setDisplayMaximum(100, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(-100, 1)
    param.setDisplayMaximum(100, 1)
    param.setDefaultValue(0.3, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(-100, 2)
    param.setDisplayMaximum(100, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust camera viewpoint")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.view = param
    del param

    param = lastNode.createDouble3DParam("rotate", "Text Rotate")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(-100, 0)
    param.setDisplayMaximum(100, 0)
    param.setDefaultValue(30, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(-100, 1)
    param.setDisplayMaximum(100, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(-100, 2)
    param.setDisplayMaximum(100, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust text rotation")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.rotate = param
    del param

    param = lastNode.createColorParam("textColor", "Text Color", False)
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(1, 0)
    param.setDefaultValue(1, 0)
    param.setMinimum(-2.14748e+09, 1)
    param.setMaximum(2.14748e+09, 1)
    param.setDisplayMinimum(0, 1)
    param.setDisplayMaximum(1, 1)
    param.setDefaultValue(0.9, 1)
    param.setMinimum(-2.14748e+09, 2)
    param.setMaximum(2.14748e+09, 2)
    param.setDisplayMinimum(0, 2)
    param.setDisplayMaximum(1, 2)
    param.setDefaultValue(0.2, 2)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust text color")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.textColor = param
    del param

    param = lastNode.createIntParam("angle", "Camera Angle")
    param.setMinimum(0, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setDefaultValue(35, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust camera angle")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.angle = param
    del param

    param = lastNode.createDoubleParam("letterDepth", "Letter Depth")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(10, 0)
    param.setDefaultValue(0.9, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust letter depth")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.letterDepth = param
    del param

    param = lastNode.createDoubleParam("letterSpace", "Letter Spacing")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(10, 0)
    param.setDefaultValue(0.1, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust letter spacing")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.letterSpace = param
    del param

    param = lastNode.createIntParam("renderQuality", "Render Quality")
    param.setMinimum(0, 0)
    param.setMaximum(11, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(11, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust render quality, use low settings when adjusting params for best performance, use value 9(+) for output/renders, AA 3+ is recommended on output/renders.")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.renderQuality = param
    del param

    param = lastNode.createIntParam("renderAA", "Render Antialiasing")
    param.setMinimum(0, 0)
    param.setMaximum(9, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(9, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust AA")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.renderAA = param
    del param

    param = lastNode.createStringParam("text", "Text")
    param.setType(NatronEngine.StringParam.TypeEnum.eStringTypeDefault)
    param.setDefaultValue("Enter text")

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Adjust text")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.text = param
    del param

    param = lastNode.createStringParam("font", "Font")
    param.setType(NatronEngine.StringParam.TypeEnum.eStringTypeDefault)
    param.setDefaultValue("/usr/share/fonts/dejavu/DejaVuSans.ttf")

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Font used")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.font = param
    del param

    #Refresh the GUI with the newly created parameters
    lastNode.refreshUserParamsGUI()
    del lastNode

    #Now that all nodes are created we can connect them together, restore expressions
    groupOutput1.connectInput(0, groupPovRay1)

    groupPovRay1.connectInput(0, groupInput1)
    param = groupPovRay1.getParam("scene")
    param.setExpression("str(thisParam.get()).replace(\"_LOCATION_X_\",str(thisGroup.location.get().x)).replace(\"_LOCATION_Y_\",str(thisGroup.location.get().y)).replace(\"_LOCATION_Z_\",str(thisGroup.location.get().z)).replace(\"_ENTER_TEXT_\",thisGroup.text.get()).replace(\"_USE_FONT_\",thisGroup.font.get()).replace(\"_LETTER_DEPTH_\",str(thisGroup.letterDepth.get())).replace(\"_LETTER_SPACE_\",str(thisGroup.letterSpace.get())).replace(\"_VIEW_X_\",str(thisGroup.view.get().x)).replace(\"_VIEW_Y_\",str(thisGroup.view.get().y)).replace(\"_VIEW_Z_\",str(thisGroup.view.get().z)).replace(\"_CAM_ANGLE_\",str(thisGroup.angle.get())).replace(\"_ROTATE_X_\",str(thisGroup.rotate.get().x)).replace(\"_ROTATE_Y_\",str(thisGroup.rotate.get().y)).replace(\"_ROTATE_Z_\",str(thisGroup.rotate.get().z)).replace(\"_COLOR_R_\",str(thisGroup.textColor.get().r)).replace(\"_COLOR_G_\",str(thisGroup.textColor.get().g)).replace(\"_COLOR_B_\",str(thisGroup.textColor.get().b))", False, 0)
    del param
    param = groupPovRay1.getParam("quality")
    param.setExpression("thisGroup.renderQuality.get()", False, 0)
    del param
    param = groupPovRay1.getParam("antialiasing")
    param.setExpression("thisGroup.renderAA.get()", False, 0)
    del param

    try:
        extModule = sys.modules["Text3DExt"]
    except KeyError:
        extModule = None
    if extModule is not None and hasattr(extModule ,"createInstanceExt") and hasattr(extModule.createInstanceExt,"__call__"):
        extModule.createInstanceExt(app,group)
