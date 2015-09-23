# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
#

SUBDIRS = Bundle

SUBDIRS_NOMULTI = \
Arc \
Charcoal \
DaveHill \
Dragan \
Edges \
Emboss \
Glow \
Implode \
Modulate \
Oilpaint \
Polar \
Polaroid \
ReadPSD \
ReadSVG \
Reflection \
Roll \
Sketch \
Swirl \
Text \
TextPango \
Texture \
Tile \
Wave

all: subdirs

.PHONY: nomulti subdirs clean install install-nomulti uninstall uninstall-nomulti $(SUBDIRS)

nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)"

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for i in $(SUBDIRS) ; do \
	  $(MAKE) -C $$i $@; \
	done

install:
	for i in $(SUBDIRS) ; do \
	  $(MAKE) -C $$i $@; \
	done

install-nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)" install

uninstall:
	for i in $(SUBDIRS) ; do \
	  $(MAKE) -C $$i $@; \
	done

uninstall-nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)" uninstall

