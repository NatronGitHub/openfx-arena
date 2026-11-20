// Writes the sample files of the hostDemo render test (tests/hostdemo/run.cmake) into the
// directory given as argument, with the libraries Arena links: ImageMagick (PNG, PSD for
// ReadPSD, BMP for ReadMisc), libzip (OpenRaster and Krita documents around the PNG), and
// a hand-written one-page PDF for ReadPDF, and an identity Hald CLUT (512x512) for
// HaldCLUT, which would otherwise download one. The sample_xy files are for tests/readerwindow.
#include <Magick++.h>
#include <zip.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const int kWidth = 64;
static const int kHeight = 48;

static std::string readFile(const std::string& path)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// a zip file with the given entries (name, contents); the first entry is stored, the
// others deflated, as OpenRaster requires for "mimetype"
static bool writeZip(const std::string& path, const std::vector<std::pair<std::string, std::string> >& entries)
{
    std::remove(path.c_str());
    int err = 0;
    zip_t* z = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!z) {
        return false;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        zip_source_t* src = zip_source_buffer(z, entries[i].second.data(), entries[i].second.size(), 0);
        if (!src) {
            zip_discard(z);
            return false;
        }
        zip_int64_t idx = zip_file_add(z, entries[i].first.c_str(), src, ZIP_FL_OVERWRITE);
        if (idx < 0) {
            zip_source_free(src);
            zip_discard(z);
            return false;
        }
        if (i == 0) {
            zip_set_file_compression(z, (zip_uint64_t)idx, ZIP_CM_STORE, 0);
        }
    }
    return zip_close(z) == 0;
}

// a one-page PDF with the given content stream, with a correct cross-reference table
static bool writePDF(const std::string& path, const std::string& stream)
{
    std::vector<std::string> objects;
    objects.push_back("<< /Type /Catalog /Pages 2 0 R >>");
    objects.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    {
        std::ostringstream o;
        o << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << kWidth << " " << kHeight
          << "] /Contents 4 0 R /Resources << >> >>";
        objects.push_back(o.str());
    }
    {
        std::ostringstream o;
        o << "<< /Length " << stream.size() << " >>\nstream\n" << stream << "\nendstream";
        objects.push_back(o.str());
    }
    std::ostringstream pdf;
    pdf << "%PDF-1.4\n";
    std::vector<size_t> offsets;
    for (size_t i = 0; i < objects.size(); ++i) {
        offsets.push_back((size_t)pdf.tellp());
        pdf << (i + 1) << " 0 obj\n" << objects[i] << "\nendobj\n";
    }
    const size_t xref = (size_t)pdf.tellp();
    pdf << "xref\n0 " << (objects.size() + 1) << "\n0000000000 65535 f \n";
    for (size_t i = 0; i < offsets.size(); ++i) {
        char line[32];
        std::snprintf(line, sizeof(line), "%010lu 00000 n \n", (unsigned long)offsets[i]);
        pdf << line;
    }
    pdf << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\nstartxref\n" << xref << "\n%%EOF\n";
    std::ofstream f(path.c_str(), std::ios::binary);
    f << pdf.str();
    return (bool)f;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <output directory>" << std::endl;
        return 1;
    }
    const std::string dir = argv[1];
    Magick::InitializeMagick(argv[0]);
    try {
        Magick::Image image;
        image.size(Magick::Geometry(kWidth, kHeight));
        image.read("gradient:#204080-#e0c020");
        image.depth(8);
        image.write(dir + "/sample.png");
        image.write(dir + "/sample.psd");
        image.write(dir + "/sample.bmp");
        Magick::Image hald;
        hald.read("hald:8");
        hald.depth(8);
        hald.write(dir + "/sample_hald.png");
    } catch (const std::exception& e) {
        std::cerr << "ImageMagick: " << e.what() << std::endl;
        return 1;
    }
    // the samples of the render window test (tests/readerwindow): their pixels differ along x
    // and y, unlike the vertical gradient, so that a window copied at the wrong offset shows;
    // the alpha varies too, for the premultiplication in GenericReader
    try {
        std::vector<unsigned char> rgba((size_t)kWidth * kHeight * 4);
        for (int y = 0; y < kHeight; ++y) {
            for (int x = 0; x < kWidth; ++x) {
                unsigned char* p = &rgba[((size_t)y * kWidth + x) * 4];
                p[0] = (unsigned char)(x * 4);
                p[1] = (unsigned char)(y * 5);
                p[2] = (unsigned char)((x * 7 + y * 13) % 256);
                p[3] = (unsigned char)(128 + (x + 2 * y) % 128);
            }
        }
        Magick::Image xy;
        xy.read(kWidth, kHeight, "RGBA", Magick::CharPixel, &rgba[0]);
        xy.depth(8);
        xy.write("PNG32:" + dir + "/sample_xy.png");
    } catch (const std::exception& e) {
        std::cerr << "ImageMagick: " << e.what() << std::endl;
        return 1;
    }
    const std::string png = readFile(dir + "/sample.png");
    const std::string pngXY = readFile(dir + "/sample_xy.png");
    if (png.empty() || pngXY.empty()) {
        std::cerr << "could not read " << dir << "/sample.png or sample_xy.png" << std::endl;
        return 1;
    }
    std::ostringstream size;
    size << "w=\"" << kWidth << "\" h=\"" << kHeight << "\"";
    std::vector<std::pair<std::string, std::string> > ora;
    ora.push_back(std::make_pair(std::string("mimetype"), std::string("image/openraster")));
    ora.push_back(std::make_pair(std::string("stack.xml"),
        "<?xml version='1.0' encoding='UTF-8'?>\n<image version=\"0.0.3\" " + size.str() +
        "><stack><layer name=\"background\" src=\"data/layer0.png\" x=\"0\" y=\"0\" opacity=\"1.0\"/></stack></image>\n"));
    std::vector<std::pair<std::string, std::string> > oraXY = ora;
    // the _big samples (tests/readerwindow, #108): the XML gives a larger image (80x60) than the
    // PNG (sample_xy's 64x48); the PNG is anchored at the top-left corner of the image, and the
    // rest of the RoD is black. sample_offset.ora: an 80x60 canvas with one layer, sample_xy's
    // PNG, at x=10, y=5 from the top-left corner, and no merged image (so that the layer is
    // read), as GIMP and MyPaint save cropped layers
    std::ostringstream bigSize;
    bigSize << "w=\"" << kWidth + 16 << "\" h=\"" << kHeight + 12 << "\"";
    std::vector<std::pair<std::string, std::string> > oraBig;
    oraBig.push_back(ora[0]);
    oraBig.push_back(std::make_pair(std::string("stack.xml"),
        "<?xml version='1.0' encoding='UTF-8'?>\n<image version=\"0.0.3\" " + bigSize.str() +
        "><stack><layer name=\"background\" src=\"data/layer0.png\" x=\"0\" y=\"0\" opacity=\"1.0\"/></stack></image>\n"));
    oraBig.push_back(std::make_pair(std::string("data/layer0.png"), pngXY));
    oraBig.push_back(std::make_pair(std::string("mergedimage.png"), pngXY));
    std::vector<std::pair<std::string, std::string> > oraOffset;
    oraOffset.push_back(ora[0]);
    oraOffset.push_back(std::make_pair(std::string("stack.xml"),
        "<?xml version='1.0' encoding='UTF-8'?>\n<image version=\"0.0.3\" " + bigSize.str() +
        "><stack><layer name=\"background\" src=\"data/layer0.png\" x=\"10\" y=\"5\" opacity=\"1.0\"/></stack></image>\n"));
    oraOffset.push_back(std::make_pair(std::string("data/layer0.png"), pngXY));
    ora.push_back(std::make_pair(std::string("data/layer0.png"), png));
    ora.push_back(std::make_pair(std::string("mergedimage.png"), png));
    oraXY.push_back(std::make_pair(std::string("data/layer0.png"), pngXY));
    oraXY.push_back(std::make_pair(std::string("mergedimage.png"), pngXY));
    std::ostringstream kritaSize;
    kritaSize << "width=\"" << kWidth << "\" height=\"" << kHeight << "\"";
    std::vector<std::pair<std::string, std::string> > kra;
    kra.push_back(std::make_pair(std::string("mimetype"), std::string("application/x-krita")));
    kra.push_back(std::make_pair(std::string("maindoc.xml"),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<DOC><IMAGE name=\"sample\" " + kritaSize.str() +
        " colorspacename=\"RGBA\" mime=\"application/x-kra\"/></DOC>\n"));
    std::vector<std::pair<std::string, std::string> > kraXY = kra;
    kra.push_back(std::make_pair(std::string("mergedimage.png"), png));
    kraXY.push_back(std::make_pair(std::string("mergedimage.png"), pngXY));
    std::ostringstream kritaBigSize;
    kritaBigSize << "width=\"" << kWidth + 16 << "\" height=\"" << kHeight + 12 << "\"";
    std::vector<std::pair<std::string, std::string> > kraBig;
    kraBig.push_back(kra[0]);
    kraBig.push_back(std::make_pair(std::string("maindoc.xml"),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<DOC><IMAGE name=\"sample\" " + kritaBigSize.str() +
        " colorspacename=\"RGBA\" mime=\"application/x-kra\"/></DOC>\n"));
    kraBig.push_back(std::make_pair(std::string("mergedimage.png"), pngXY));
    // a filled rectangle
    std::ostringstream content;
    content << "0.2 0.4 0.8 rg 0 0 " << kWidth << " " << kHeight << " re f "
            << "0.9 0.8 0.1 rg 16 12 32 24 re f";
    // 8 x 6 cells of 8 x 8 points, each of another colour (for tests/readerwindow)
    std::ostringstream contentXY;
    for (int j = 0; j < 6; ++j) {
        for (int i = 0; i < 8; ++i) {
            contentXY << i / 7. << " " << j / 5. << " " << ((i + 2 * j) % 3) / 2. << " rg "
                      << i * 8 << " " << j * 8 << " 8 8 re f ";
        }
    }
    if (!writeZip(dir + "/sample.ora", ora) || !writeZip(dir + "/sample.kra", kra) || !writePDF(dir + "/sample.pdf", content.str()) ||
        !writeZip(dir + "/sample_xy.ora", oraXY) || !writeZip(dir + "/sample_xy.kra", kraXY) || !writePDF(dir + "/sample_xy.pdf", contentXY.str()) ||
        !writeZip(dir + "/sample_big.ora", oraBig) || !writeZip(dir + "/sample_big.kra", kraBig) ||
        !writeZip(dir + "/sample_offset.ora", oraOffset)) {
        std::cerr << "could not write the samples in " << dir << std::endl;
        return 1;
    }
    return 0;
}
