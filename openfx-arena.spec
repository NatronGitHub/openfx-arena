%define debug_package %{nil}
%define git_url https://github.com/olear/openfx-arena
%define timestamp %(date +%Y%m%d)
%define _magick 6.9.1-1

Summary: A set of visual effect plugins for OpenFX compatible applications
Name: openfx-arena

Version: %{timestamp}
Release: 1
License: BSD

Group: Applications/Graphics
URL: %{git_url}

Packager: Ole-André Rodlie, <olear@fxarena.net>
Vendor: FxArena DA, http://fxarena.net
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: freetype-devel fontconfig-devel
Requires: freetype fontconfig
AutoReqProv: no

%description
A set of visual effect plugins for OpenFX compatible applications.

%prep
cd %{_builddir}
rm -rf openfx-arena-%{version}
git clone %{git_url} openfx-arena-%{version}
cd openfx-arena-%{version}
git submodule update -i
git log > CHANGELOG

# Add own magick and opencl
wget https://fxarena.net/~olear/OpenCL/CL.tar.gz
mkdir -p %{_builddir}/OpenCL
tar xvf CL.tar.gz -C %{_builddir}/OpenCL/
wget https://fxarena.net/~olear/OpenCL/CL-lib-Linux-%{_arch}.tar.gz
tar xvf CL-lib-Linux-%{_arch}.tar.gz -C %{_builddir}/
wget ftp://ftp.nluug.nl/pub/ImageMagick/ImageMagick-%{_magick}.tar.xz
tar xvf ImageMagick-%{_magick}.tar.xz
cd ImageMagick-%{_magick}
CFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC" CXXFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC" CPPFLAGS="-I%{_builddir}/OpenCL -L%{_builddir}/CL-lib" ./configure --with-magick-plus-plus=yes --with-quantum-depth=16 --prefix=%{_builddir}/im --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --enable-opencl --with-x
make %{?_smp_mflags} install

%build
cd %{_builddir}/openfx-arena-%{version}
export PKG_CONFIG_PATH=%{_builddir}/im/lib/pkgconfig
export PATH=%{_builddir}/im/bin:$PATH
make DEBUGFLAG=-O3

%install
cd %{_builddir}/openfx-arena-%{version}
mkdir -p %{buildroot}/usr/OFX/Plugins %{buildroot}/%{_docdir}/%{name}-%{version}
mkdir Libraries
cp -a %{_builddir}/im/lib*/libMagick++-6.Q16HDRI.so.6.0.0 Libraries/
cp -a %{_builddir}/im/lib*/libMagickCore-6.Q16HDRI.so.2.0.0 Libraries/
cp -a %{_builddir}/im/lib*/libMagickWand-6.Q16HDRI.so.2.0.0 Libraries/
cp -a %{_builddir}/im/lib*/libMagick++-6.Q16HDRI.so.6 Libraries/
cp -a %{_builddir}/im/lib*/libMagickCore-6.Q16HDRI.so.2 Libraries/
cp -a %{_builddir}/im/lib*/libMagickWand-6.Q16HDRI.so.2 Libraries/
strip -s Libraries/*
mv Libraries Plugin/*-release/*.ofx.bundle/
mv Plugin/*-release/*.ofx.bundle %{buildroot}/usr/OFX/Plugins/
cp CHANGELOG LICENSE README.md %{buildroot}/%{_docdir}/%{name}-%{version}/
cp %{_builddir}/OpenCL/CL/LICENSE %{buildroot}/%{_docdir}/%{name}-%{version}/LICENSE.OpenCL
cp %{_builddir}/im/share/doc/ImageMagick-6/LICENSE %{buildroot}/%{_docdir}/%{name}-%{version}/LICENSE.ImageMagick
cp OpenFX/Support/LICENSE %{buildroot}/%{_docdir}/%{name}-%{version}/LICENSE.OpenFX

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_docdir}/%{name}-%{version}/*
/usr/OFX/Plugins/*.ofx.bundle

%changelog
#
