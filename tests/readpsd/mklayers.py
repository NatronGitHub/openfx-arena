#!/usr/bin/env python3
# Generate the layered test files for ReadPSD: GIMP XCF files (file versions 0, 3 and 11, i.e. GIMP 1.x/2.0,
# GIMP 2.8 and GIMP 2.10) and a PSD, all with the same 3 layers on a 200x150 canvas:
#   1 (bottom) "background": an opaque gradient covering the canvas
#   2 "disc": a half-transparent green disc, 120x80 at +30+20
#   3 (top) "square": an opaque blue square with a transparent hole, 40x30 at +140+100
# The XCF files are written directly (layout from GIMP's devel-docs/xcf.txt). The PSD is written with
# ImageMagick (convert or magick), with the composite first, as Photoshop does (skipped if neither tool is
# found). A flat PSD (no layer section: the composite only) is written directly. test_layers.py imports this file for the
# expected images.
# usage: mklayers.py outdir
import os, shutil, struct, subprocess, sys
import numpy as np

W, H = 200, 150

def layers():
    """[(name, uint8 RGBA HxWx4 top-down, x, y)], bottom first"""
    y, x = np.mgrid[0:H, 0:W]
    bg = np.zeros((H, W, 4), np.uint8)
    bg[..., 0] = x * 255 // (W - 1)
    bg[..., 1] = y * 255 // (H - 1)
    bg[..., 2] = 255 - bg[..., 0] // 2
    bg[..., 3] = 255
    w, h = 120, 80
    y, x = np.mgrid[0:h, 0:w]
    disc = np.zeros((h, w, 4), np.uint8)
    disc[..., :3] = (40, 230, 60)
    disc[..., 3] = np.where(np.hypot(x - w / 2 + 0.5, y - h / 2 + 0.5) < 35, 160, 0)
    w, h = 40, 30
    y, x = np.mgrid[0:h, 0:w]
    sq = np.zeros((h, w, 4), np.uint8)
    sq[..., :3] = (20, 40, 250)
    sq[..., 3] = np.where((abs(x - 20) < 6) & (abs(y - 15) < 6), 0, 255)
    return [('background', bg, 0, 0), ('disc', disc, 30, 20), ('square', sq, 140, 100)]

# ---- XCF

def u32(v):
    return struct.pack('>I', v)

def prop(t, payload=b''):
    return u32(t) + u32(len(payload)) + payload

def rle_tile(t):
    """GIMP RLE: each channel in turn, as literal runs (opcode 256-n, then n bytes, n <= 127)"""
    o = bytearray()
    for c in range(t.shape[2]):
        b = t[..., c].tobytes()
        for i in range(0, len(b), 127):
            chunk = b[i:i + 127]
            o.append(256 - len(chunk))
            o += chunk
    return bytes(o)

def xcf(version):
    """version 0 ("gimp xcf file"), 1-3 ("gimp xcf vNNN", 32-bit offsets, no precision), or >= 11
    (64-bit offsets, precision 150 = 8-bit non-linear integer, as GIMP 2.10 writes)"""
    wide = version >= 11
    ptr = (lambda v: struct.pack('>Q', v)) if wide else u32
    psize = 8 if wide else 4
    out = bytearray(b'gimp xcf file\0' if version == 0 else b'gimp xcf v%03d\0' % version)
    out += u32(W) + u32(H) + u32(0)  # RGB
    if version >= 4:
        out += u32(150)
    out += prop(17, bytes([1]))  # PROP_COMPRESSION: RLE
    out += prop(0)
    ls = layers()[::-1]  # XCF lists the top layer first
    lptrs = len(out)
    out += b'\0' * (psize * (len(ls) + 1))  # layer pointers, then 0
    out += ptr(0)                            # no channels
    for i, (name, img, dx, dy) in enumerate(ls):
        h, w = img.shape[:2]
        out[lptrs + psize * i:lptrs + psize * (i + 1)] = ptr(len(out))
        n = name.encode() + b'\0'
        out += u32(w) + u32(h) + u32(1) + u32(len(n)) + n  # type 1: RGBA
        out += prop(6, u32(255)) + prop(8, u32(1)) + prop(15, struct.pack('>ii', dx, dy)) + prop(0)
        hp = len(out)
        out += ptr(0) + ptr(0)  # hierarchy, no mask
        out[hp:hp + psize] = ptr(len(out))
        out += u32(w) + u32(h) + u32(4)
        lp = len(out)
        out += ptr(0) + ptr(0)  # level, then 0
        out[lp:lp + psize] = ptr(len(out))
        out += u32(w) + u32(h)
        tiles = [(ty, tx) for ty in range(0, h, 64) for tx in range(0, w, 64)]
        tp = len(out)
        out += b'\0' * (psize * (len(tiles) + 1))
        for j, (ty, tx) in enumerate(tiles):
            out[tp + psize * j:tp + psize * (j + 1)] = ptr(len(out))
            out += rle_tile(img[ty:ty + 64, tx:tx + 64])
    # ImageMagick reads up to 64*64*6 bytes for the last tile; GIMP files always have more data after it
    out += b'\0' * (64 * 64 * 6)
    return bytes(out)

# ---- composite and PSD

def composite(ls):
    """Over of the layers, bottom first, on a transparent canvas: unpremultiplied float RGBA"""
    acc = np.zeros((H, W, 4))  # premultiplied
    for _, img, dx, dy in ls:
        f = img.astype(np.float64) / 255
        h, w = f.shape[:2]
        src = np.concatenate([f[..., :3] * f[..., 3:], f[..., 3:]], -1)
        dst = acc[dy:dy + h, dx:dx + w]
        acc[dy:dy + h, dx:dx + w] = src + dst * (1 - src[..., 3:])
    a = acc[..., 3:]
    rgb = np.divide(acc[..., :3], a, out=np.zeros_like(acc[..., :3]), where=a > 0)
    return np.concatenate([rgb, a], -1)

def pam(path, img):
    h, w = img.shape[:2]
    with open(path, 'wb') as f:
        f.write(b'P7\nWIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n' % (w, h))
        f.write(np.ascontiguousarray(img).tobytes())

def psd_composite(ls):
    """the composite stored in the PSD: composite() in 8 bits"""
    return np.round(composite(ls) * 255) / 255

def flat_psd(ls):
    """a PSD without layers: header, empty colour mode data, resources and layer info, raw planar RGBA"""
    img = (psd_composite(ls) * 255 + 0.5).astype(np.uint8)
    out = b'8BPS' + struct.pack('>H6xHIIHH', 1, 4, H, W, 8, 3) + u32(0) + u32(0) + u32(0)
    out += struct.pack('>H', 0) + b''.join(img[..., c].tobytes() for c in range(4))
    return out

def psd(outdir, ls):
    tool = shutil.which('magick') or shutil.which('convert')
    if not tool:
        return False
    comp = os.path.join(outdir, 'composite.pam')
    pam(comp, (psd_composite(ls) * 255 + 0.5).astype(np.uint8))
    args = [tool, comp]
    tmp = [comp]
    for i, (name, img, dx, dy) in enumerate(ls):
        p = os.path.join(outdir, 'layer%d.pam' % i)
        pam(p, img)
        tmp.append(p)
        args += ['(', p, '-set', 'label', name, '-repage', '+%d+%d' % (dx, dy), ')']
    args += ['-depth', '8', os.path.join(outdir, 'layers.psd')]
    subprocess.check_call(args)
    for p in tmp:
        os.remove(p)
    return True

if __name__ == '__main__':
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    ls = layers()
    for v in (0, 3, 11):
        with open(os.path.join(outdir, 'layers-v%d.xcf' % v), 'wb') as f:
            f.write(xcf(v))
    with open(os.path.join(outdir, 'flat.psd'), 'wb') as f:
        f.write(flat_psd(ls))
    has_psd = psd(outdir, ls)
    print('wrote', outdir, '(PSD too)' if has_psd else '(no PSD: ImageMagick not found)')
