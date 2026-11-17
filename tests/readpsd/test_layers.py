#!/usr/bin/env python3
# ReadPSD layer selection test: render every choice of the "Image layer" menu (Default, Layer 1..) of
# layered XCF (file versions 0, 3, 11) and PSD files, and of a flat PSD, with the OpenFX demo host hostDemo,
# and check each output against the expected layer (at its offset, or at 0,0 with offset=0) or composite.
# Choices past the last layer must fail cleanly (an error, not a crash).
#
# hostDemo isn't Natron, so this tests ReadPSD's fixed menu (Default, Layer 1..9): Default is the
# composite, Layer N the Nth layer from the bottom. (Natron gets a menu built from the file.)
#
# usage: test_layers.py [--hostdemo PATH] [--workdir DIR] BUNDLE_DIR
#   BUNDLE_DIR: the directory that contains Arena.ofx.bundle
#   hostDemo: from OpenFX's HostSupport/examples (it needs PFM output and --report), default $HOSTDEMO
#   --workdir: keep the files and renders there (by default, a temporary directory, removed if all pass)
# Needs Python 3 and NumPy, and ImageMagick (magick or convert) for the layered PSD. Exit status 0 if all pass.
import argparse, json, os, shutil, subprocess, sys, tempfile
import numpy as np

sys.dont_write_bytecode = True  # no __pycache__ in the source tree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mklayers  # noqa: E402

# an OCIO config whose colourspaces are all the identity, so that the output is the file's values
OCIO_CONFIG = """ocio_profile_version: 1
roles:
  default: linear
  reference: linear
  scene_linear: linear
  color_picking: linear
  compositing_log: linear
  data: linear
displays:
  default:
    - !<View> {name: Raw, colorspace: linear}
colorspaces:
  - !<ColorSpace> {name: linear, family: "", bitdepth: 32f, isdata: false}
  - !<ColorSpace> {name: sRGB, family: "", bitdepth: 32f, isdata: false}
"""
TOL = 1e-4

def read_pfm(path):
    with open(path, 'rb') as f:
        t = f.readline().strip()
        w, h = map(int, f.readline().split())
        s = float(f.readline())
        d = np.frombuffer(f.read(), dtype='<f4' if s < 0 else '>f4')
    c = 3 if t == b'PF' else 1
    return (d.reshape(h, w, c) if c == 3 else d.reshape(h, w))[::-1]

def place(img, dx, dy):
    """an unpremultiplied RGBA layer on the transparent canvas"""
    out = np.zeros((mklayers.H, mklayers.W, 4))
    h, w = img.shape[:2]
    out[dy:dy + h, dx:dx + w] = img
    return out

def premult(img):
    return np.concatenate([img[..., :3] * img[..., 3:], img[..., 3:]], -1)

def render(hostdemo, bundle_dir, workdir, name, filename, params):
    d = os.path.join(workdir, name)
    os.makedirs(d, exist_ok=True)
    cmd = [hostdemo, '--no-std-paths', '--no-cache', '--quiet', '--format', 'pfm', '-d', 'float', '--report', 'report.json',
           '-p', 'net.fxarena.openfx.ReadPSD', '-c', 'general', '-o', 'out', '--frames', '0', '-i', 'Source=none',
           '-s', 'filename=' + filename] + sum((['-s', p] for p in params), [])
    env = dict(os.environ, OFX_PLUGIN_PATH=bundle_dir, OCIO=os.path.join(workdir, 'identity.ocio'))
    p = subprocess.run(cmd, cwd=d, env=env, capture_output=True, text=True, timeout=120)
    out = None
    if p.returncode == 0:
        rgb = read_pfm(os.path.join(d, 'out.0.pfm'))
        a = read_pfm(os.path.join(d, 'out.0.alpha.pfm'))
        out = np.concatenate([rgb, a[..., None]], -1).astype(np.float64)
        rep = json.load(open(os.path.join(d, 'report.json')))
        if rep['clipPreferences']['outputPremultiplication'] != 'OfxImageAlphaPremultiplied':
            out = premult(out)
    msgs = ''
    try:
        rep = json.load(open(os.path.join(d, 'report.json')))
        msgs = '; '.join(m['text'] for m in rep.get('messages', []) if m.get('text'))
    except (OSError, ValueError):
        pass
    return p.returncode, out, msgs

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('bundle_dir')
    ap.add_argument('--hostdemo', default=os.environ.get('HOSTDEMO', 'hostDemo'))
    ap.add_argument('--workdir')
    a = ap.parse_args()
    workdir = a.workdir or tempfile.mkdtemp(prefix='readpsd-layers-')
    files = os.path.join(workdir, 'files')
    subprocess.check_call([sys.executable, os.path.join(HERE, 'mklayers.py'), files], stdout=subprocess.DEVNULL)
    with open(os.path.join(workdir, 'identity.ocio'), 'w') as f:
        f.write(OCIO_CONFIG)

    ls = mklayers.layers()
    layer_imgs = [place(img.astype(np.float64) / 255, dx, dy) for _, img, dx, dy in ls]
    disc_at_0 = place(ls[1][1].astype(np.float64) / 255, 0, 0)
    # (file, choice, params, expected unpremultiplied RGBA or None for a clean failure)
    cases = []
    for f in ['layers-v0.xcf', 'layers-v3.xcf', 'layers-v11.xcf'] + (['layers.psd'] if os.path.exists(os.path.join(files, 'layers.psd')) else []):
        comp = mklayers.composite(ls) if f.endswith('.xcf') else mklayers.psd_composite(ls)
        cases.append((f, 0, [], comp))
        for i, img in enumerate(layer_imgs):
            cases.append((f, i + 1, [], img))
        cases.append((f, 2, ['offset=0'], disc_at_0))
        cases.append((f, len(ls) + 1, [], None))
        cases.append((f, 9, [], None))
    cases.append(('flat.psd', 0, [], mklayers.psd_composite(ls)))
    cases.append(('flat.psd', 1, [], None))

    nfail = 0
    for f, choice, params, exp in cases:
        name = '%s_layer%d%s' % (f.replace('.', '_'), choice, ''.join('_' + p.replace('=', '') for p in params))
        rc, out, msgs = render(a.hostdemo, a.bundle_dir, workdir, name, os.path.join(files, f), ['layer=%d' % choice] + params)
        if exp is None:
            ok = rc > 0  # a clean failure; a negative rc is a signal (crash)
            detail = 'rc=%d %s' % (rc, msgs)
        elif rc != 0:
            ok, detail = False, 'rc=%d %s' % (rc, msgs)
        elif out.shape != exp.shape:
            ok, detail = False, 'size %s, expected %s' % (out.shape[:2], exp.shape[:2])
        else:
            d = np.abs(out - premult(exp))
            ok = d.max() <= TOL
            detail = 'max diff %.3g' % d.max()
        nfail += not ok
        print('%-4s %-34s %s' % ('ok' if ok else 'FAIL', name, detail))
    print('%d/%d passed (work dir %s)' % (len(cases) - nfail, len(cases), workdir))
    if not nfail and not a.workdir:
        shutil.rmtree(workdir)
    return 1 if nfail else 0

if __name__ == '__main__':
    sys.exit(main())
