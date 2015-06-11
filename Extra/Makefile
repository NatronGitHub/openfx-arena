PLUGINOBJECTS = \
        Mirror.o \
        Texture.o \
        Tile.o \
	PluginRegistrationCombined.o \

PLUGINNAME = Extra

RESOURCES = \
../Mirror/net.fxarena.openfx.Mirror.png \
../Mirror/net.fxarena.openfx.Mirror.svg \
../Texture/net.fxarena.openfx.Texture.png \
../Texture/net.fxarena.openfx.Texture.svg \
../Tile/net.fxarena.openfx.Tile.png \
../Tile/net.fxarena.openfx.Tile.svg \

VPATH += \
../Mirror \
../Texture \
../Tile \

include ../Makefile.master

CXXFLAGS += -I../Mirror -I../Texture -I../Tile

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
