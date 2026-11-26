SUBDIRS = Bundle

all: subdirs

.PHONY: subdirs clean install uninstall $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	(cd $@ && $(MAKE))

clean:
	@for i in $(SUBDIRS); do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done

install:
	@for i in $(SUBDIRS) ; do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done

uninstall:
	@for i in $(SUBDIRS) ; do \
	  echo "(cd $$i && $(MAKE) $@)"; \
	  (cd $$i && $(MAKE) $@); \
	done
