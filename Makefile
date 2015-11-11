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
Wave \
PovRay

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

