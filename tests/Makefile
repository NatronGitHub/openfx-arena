# Plug-in tests, run with the OpenFX demo host hostDemo (OpenFX HostSupport/examples, with PFM output and
# --report: build it with "make -C OpenFX/HostSupport/examples" from an OpenFX that has them).
#   make -C tests check [BUNDLE_DIR=dir containing Arena.ofx.bundle] [HOSTDEMO=path/to/hostDemo]
# Needs Python 3 with NumPy, and ImageMagick for the layered PSD: magick or convert next to the Magick++
# that pkg-config finds, or in PATH; MAGICK=path overrides it.
PYTHON ?= python3
BUNDLE_DIR ?= ../Bundle/$(shell uname -s)-64-release
HOSTDEMO ?= hostDemo

check:
	$(PYTHON) readpsd/test_layers.py --hostdemo "$(HOSTDEMO)" "$(BUNDLE_DIR)"

.PHONY: check
