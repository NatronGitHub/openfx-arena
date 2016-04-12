SUBDIRS = Bundle

SUBDIRS_NOMULTI = \
Arc \
Charcoal \
Edges \
Emboss \
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
PovRay \
OpenRaster \
ReadKrita \
ReadMisc \
TextFX

all: subdirs

.PHONY: nomulti subdirs clean install install-nomulti uninstall uninstall-nomulti $(SUBDIRS)

nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)"

subdirs: $(SUBDIRS)

$(SUBDIRS):
	(cd $@ && $(MAKE))

clean:
	@for i in $(SUBDIRS) $(SUBDIRS_NOMULTI); do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done

install:
	@for i in $(SUBDIRS) ; do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done

install-nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)" install

uninstall:
	@for i in $(SUBDIRS) ; do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done

uninstall-nomulti:
	$(MAKE) SUBDIRS="$(SUBDIRS_NOMULTI)" uninstall

