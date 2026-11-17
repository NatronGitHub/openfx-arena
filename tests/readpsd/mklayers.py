#!/usr/bin/env python3
# Generate the layered test files for ReadPSD, on a 200x150 canvas:
# - layers-v0.xcf, layers-v3.xcf, layers-v11.xcf: GIMP XCF files (file versions 0, 3 and 11, i.e. GIMP 1.x/2.0,
#   GIMP 2.8 and GIMP 2.10), with layers() (bottom first):
#     "background": an opaque gradient covering the canvas
#     "disc": a half-transparent green disc, 120x80 at +30+20
#     "square": an opaque blue square with a transparent hole, 40x30 at +140+100
#     "corner": an orange square, 50x50 at -20,-10 (a negative offset: only part of it is on the canvas)
# - modes-v3.xcf, modes-v11.xcf: mode_layers(): the background, a Multiply layer, and a hidden layer on top
# - single.xcf: one layer (file version 11)
# - flat.psd: a PSD without layers (the composite only), written directly
# - layers.psd: the composite and layers(), written with ImageMagick (skipped if it isn't found): $MAGICK, or
#   magick/convert next to the Magick++ that pkg-config finds, or in PATH
# The XCF files are written directly (layout from GIMP's devel-docs/xcf.txt). test_layers.py imports this
# file for the expected images.
# usage: mklayers.py outdir
import os, shutil, struct, subprocess, sys
import numpy as np

W, H = 200, 150

# GIMP layer modes (PROP_MODE): GIMP 2.10 (file version >= 10) writes the new modes, older ones the legacy ones
MODE_NORMAL = {'legacy': 0, 'new': 28}
MODE_MULTIPLY = {'legacy': 3, 'new': 30}

def layer(name, img, x, y, visible=True, mode='normal'):
    return dict(name=name, img=img, x=x, y=y, visible=visible, mode=mode)

def solid(w, h, rgb, a):
    img = np.zeros((h, w, 4), np.uint8)
    img[..., :3] = rgb
    img[..., 3] = a
    return img

def background():
    y, x = np.mgrid[0:H, 0:W]
    bg = np.zeros((H, W, 4), np.uint8)
    bg[..., 0] = x * 255 // (W - 1)
    bg[..., 1] = y * 255 // (H - 1)
    bg[..., 2] = 255 - bg[..., 0] // 2
    bg[..., 3] = 255
    return bg

def layers():
    """the layers of the layers-*.xcf and layers.psd files, bottom first"""
    w, h = 120, 80
    y, x = np.mgrid[0:h, 0:w]
    disc = solid(w, h, (40, 230, 60), np.where(np.hypot(x - w / 2 + 0.5, y - h / 2 + 0.5) < 35, 160, 0))
    w, h = 40, 30
    y, x = np.mgrid[0:h, 0:w]
    sq = solid(w, h, (20, 40, 250), np.where((abs(x - 20) < 6) & (abs(y - 15) < 6), 0, 255))
    y, x = np.mgrid[0:50, 0:50]
    corner = solid(50, 50, (250, 140, 20), 128 + x * 2)
    return [layer('background', background(), 0, 0), layer('disc', disc, 30, 20), layer('square', sq, 140, 100),
            layer('corner', corner, -20, -10)]

def mode_layers():
    """the layers of the modes-*.xcf files, bottom first"""
    return [layer('background', background(), 0, 0),
            layer('multiply', solid(100, 60, (200, 100, 255), 200), 50, 40, mode='multiply'),
            layer('hidden', solid(80, 80, (255, 0, 0), 255), 10, 10, visible=False)]

def single_layers():
    return [layer('only', solid(120, 90, (90, 180, 30), 220), 40, 30)]

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

def xcf(version, ls):
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
    ls = ls[::-1]  # XCF lists the top layer first
    lptrs = len(out)
    out += b'\0' * (psize * (len(ls) + 1))  # layer pointers, then 0
    out += ptr(0)                            # no channels
    for i, l in enumerate(ls):
        img = l['img']
        h, w = img.shape[:2]
        out[lptrs + psize * i:lptrs + psize * (i + 1)] = ptr(len(out))
        n = l['name'].encode() + b'\0'
        out += u32(w) + u32(h) + u32(1) + u32(len(n)) + n  # type 1: RGBA
        mode = (MODE_MULTIPLY if l['mode'] == 'multiply' else MODE_NORMAL)['new' if version >= 10 else 'legacy']
        out += prop(6, u32(255)) + prop(7, u32(mode)) + prop(8, u32(1 if l['visible'] else 0))
        out += prop(15, struct.pack('>ii', l['x'], l['y'])) + prop(0)
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

# ---- expected images, composite and PSD

def place(l, x=None, y=None):
    """a layer on the transparent canvas, at its offset (or x,y): unpremultiplied float RGBA"""
    x = l['x'] if x is None else x
    y = l['y'] if y is None else y
    out = np.zeros((H, W, 4))
    f = l['img'].astype(np.float64) / 255
    h, w = f.shape[:2]
    x0, y0, x1, y1 = max(x, 0), max(y, 0), min(x + w, W), min(y + h, H)
    if x1 > x0 and y1 > y0:
        out[y0:y1, x0:x1] = f[y0 - y:y1 - y, x0 - x:x1 - x]
    return out

def composite(ls):
    """the visible layers merged, bottom first, on a transparent canvas, with their modes (Over, or the SVG
    Multiply that ImageMagick implements): unpremultiplied float RGBA"""
    d = np.zeros((H, W, 4))  # premultiplied
    for l in ls:
        if not l['visible']:
            continue
        f = place(l)
        sa, da = f[..., 3:], d[..., 3:]
        s = f[..., :3] * sa
        if l['mode'] == 'multiply':
            rgb = s * d[..., :3] + s * (1 - da) + d[..., :3] * (1 - sa)
        else:
            rgb = s + d[..., :3] * (1 - sa)
        d = np.concatenate([rgb, sa + da - sa * da], -1)
    a = d[..., 3:]
    rgb = np.divide(d[..., :3], a, out=np.zeros_like(d[..., :3]), where=a > 0)
    return np.concatenate([rgb, a], -1)

def psd_composite(ls):
    """the composite stored in the PSD: composite() in 8 bits"""
    return np.round(composite(ls) * 255) / 255

def pam(path, img):
    h, w = img.shape[:2]
    with open(path, 'wb') as f:
        f.write(b'P7\nWIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n' % (w, h))
        f.write(np.ascontiguousarray(img).tobytes())

def flat_psd(ls):
    """a PSD without layers: header, empty colour mode data, resources and layer info, raw planar RGBA"""
    img = (psd_composite(ls) * 255 + 0.5).astype(np.uint8)
    out = b'8BPS' + struct.pack('>H6xHIIHH', 1, 4, H, W, 8, 3) + u32(0) + u32(0) + u32(0)
    out += struct.pack('>H', 0) + b''.join(img[..., c].tobytes() for c in range(4))
    return out

def find_magick():
    """$MAGICK, else magick or convert from the prefix of the Magick++ that pkg-config finds, else from PATH"""
    if os.environ.get('MAGICK'):
        return os.environ['MAGICK']
    try:
        # (stdout/stderr PIPE and universal_newlines: Python 3.6 has no capture_output, text)
        prefix = subprocess.run(['pkg-config', '--variable=prefix', 'Magick++'], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True).stdout.strip()
    except OSError:
        prefix = ''
    for tool in ('magick', 'convert'):
        p = os.path.join(prefix, 'bin', tool) if prefix else ''
        if p and os.access(p, os.X_OK):
            return p
    if shutil.which('magick'):
        return shutil.which('magick')
    # a convert from the PATH only if it is ImageMagick's (Windows has System32\convert.exe)
    conv = shutil.which('convert')
    if conv:
        try:
            out = subprocess.run([conv, '-version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 universal_newlines=True, timeout=30).stdout
        except (OSError, subprocess.SubprocessError):
            out = ''
        if 'ImageMagick' in out:
            return conv
    return None

def psd(outdir, ls):
    tool = find_magick()
    if not tool:
        return None
    comp = os.path.join(outdir, 'composite.pam')
    pam(comp, (psd_composite(ls) * 255 + 0.5).astype(np.uint8))
    args = [tool, comp]
    tmp = [comp]
    for i, l in enumerate(ls):
        p = os.path.join(outdir, 'layer%d.pam' % i)
        pam(p, l['img'])
        tmp.append(p)
        args += ['(', p, '-set', 'label', l['name'], '-repage', '%+d%+d' % (l['x'], l['y']), ')']
    args += ['-depth', '8', os.path.join(outdir, 'layers.psd')]
    subprocess.check_call(args)
    for p in tmp:
        os.remove(p)
    return tool

def write_all(outdir):
    """write the files; returns the ImageMagick tool used for layers.psd, or None"""
    os.makedirs(outdir, exist_ok=True)
    for v in (0, 3, 11):
        with open(os.path.join(outdir, 'layers-v%d.xcf' % v), 'wb') as f:
            f.write(xcf(v, layers()))
    for v in (3, 11):
        with open(os.path.join(outdir, 'modes-v%d.xcf' % v), 'wb') as f:
            f.write(xcf(v, mode_layers()))
    with open(os.path.join(outdir, 'single.xcf'), 'wb') as f:
        f.write(xcf(11, single_layers()))
    with open(os.path.join(outdir, 'flat.psd'), 'wb') as f:
        f.write(flat_psd(layers()))
    return psd(outdir, layers())

if __name__ == '__main__':
    tool = write_all(sys.argv[1])
    print('wrote', sys.argv[1], '(layers.psd with %s)' % tool if tool else '(no layers.psd: ImageMagick not found)')
