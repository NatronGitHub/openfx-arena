#!/usr/bin/env python3
# ReadPSD layer selection test: render every choice of the "Image layer" menu (Default, Layer 1..) of
# layered XCF files (file versions 0, 3, 11; with layer modes and a hidden layer; with a single layer), of a
# layered PSD and of a flat PSD, with the OpenFX demo host hostDemo, and check each output against the
# expected layer (at its offset, negative offsets included, or at 0,0 with offset=0) or merged image.
# Choices past the last layer must fail cleanly: an error that says "Layer not found in image", not a crash
# (#74, fixed by PR #78, which also fixed the XCF layer mapping that the other cases check).
# Also changes the file, and the layer, during a session (several -s settings in one hostDemo run).
#
# hostDemo isn't Natron, so this tests ReadPSD's fixed menu (Default, Layer 1..9): Default is the
# composite (PSD) or the visible layers merged with their modes (XCF), Layer N the Nth layer from the
# bottom. (Natron gets a menu built from the file.)
#
# usage: test_layers.py [--hostdemo PATH] [--workdir DIR] BUNDLE_DIR
#   BUNDLE_DIR: the directory that contains Arena.ofx.bundle
#   hostDemo: from OpenFX's HostSupport/examples (it needs PFM output and --report), default $HOSTDEMO
#   --workdir: keep the files and renders there (by default, a temporary directory, always removed)
# Needs Python 3 and NumPy, and ImageMagick for the layered PSD (see mklayers.py; $MAGICK overrides it).
# Exit status 0 if all pass.
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
NOT_FOUND = 'Layer not found in image'

def read_pfm(path):
    with open(path, 'rb') as f:
        t = f.readline().strip()
        w, h = map(int, f.readline().split())
        s = float(f.readline())
        d = np.frombuffer(f.read(), dtype='<f4' if s < 0 else '>f4')
    c = 3 if t == b'PF' else 1
    return (d.reshape(h, w, c) if c == 3 else d.reshape(h, w))[::-1]

def premult(img):
    return np.concatenate([img[..., :3] * img[..., 3:], img[..., 3:]], -1)

def render(hostdemo, bundle_dir, workdir, name, settings):
    """run hostDemo with the -s settings, in order; returns (exit status, premultiplied RGBA or None, messages)"""
    d = os.path.join(workdir, 'renders', name)
    if os.path.isdir(d):
        shutil.rmtree(d)
    os.makedirs(d)
    cmd = [hostdemo, '--no-std-paths', '--no-cache', '--quiet', '--format', 'pfm', '-d', 'float', '--report', 'report.json',
           '-p', 'net.fxarena.openfx.ReadPSD', '-c', 'general', '-o', 'out', '--frames', '0', '-i', 'Source=none']
    cmd += sum((['-s', s] for s in settings), [])
    env = dict(os.environ, OFX_PLUGIN_PATH=bundle_dir, OCIO=os.path.join(workdir, 'identity.ocio'))
    # (stdout/stderr PIPE and universal_newlines: Python 3.6 has no capture_output, text)
    p = subprocess.run(cmd, cwd=d, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       universal_newlines=True, timeout=120)
    out = None
    msgs = p.stdout + p.stderr
    try:
        rep = json.load(open(os.path.join(d, 'report.json')))
        msgs += '; '.join(m['text'] for m in rep.get('messages', []) if m.get('text'))
    except (OSError, ValueError):
        rep = None
    if p.returncode == 0:
        # a hostDemo built with the multiview extensions (e.g. by CMake, with OFX_EXTENSIONS_NUKE and VEGAS)
        # writes out.0l.pfm and out.0r.pfm instead of out.0.pfm: use the left view
        base = next((b for b in ('out.0', 'out.0l') if os.path.exists(os.path.join(d, b + '.pfm'))), None)
        if base is None:
            msgs = 'no out.0.pfm or out.0l.pfm; hostDemo wrote: %s. %s' % (
                ', '.join(sorted(os.listdir(d))) or 'nothing', msgs)
        else:
            rgb = read_pfm(os.path.join(d, base + '.pfm'))
            a = read_pfm(os.path.join(d, base + '.alpha.pfm'))
            out = np.concatenate([rgb, a[..., None]], -1).astype(np.float64)
            if rep and rep['clipPreferences']['outputPremultiplication'] != 'OfxImageAlphaPremultiplied':
                out = premult(out)
    return p.returncode, out, ' '.join(msgs.split())

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('bundle_dir')
    ap.add_argument('--hostdemo', default=os.environ.get('HOSTDEMO', 'hostDemo'))
    ap.add_argument('--workdir')
    a = ap.parse_args()
    # hostDemo runs in its own directory per render
    bundle_dir = os.path.abspath(a.bundle_dir)
    hostdemo = os.path.abspath(a.hostdemo) if os.sep in a.hostdemo else a.hostdemo
    if not os.path.isdir(os.path.join(bundle_dir, 'Arena.ofx.bundle')):
        print('FAIL: no Arena.ofx.bundle in %s' % bundle_dir)
        return 1
    workdir = os.path.abspath(a.workdir or tempfile.mkdtemp(prefix='readpsd-layers-'))
    try:
        return run(a, bundle_dir, hostdemo, workdir)
    finally:
        # a temporary work dir goes, whatever happened (--workdir keeps the files and renders)
        if not a.workdir:
            shutil.rmtree(workdir, ignore_errors=True)

def run(a, bundle_dir, hostdemo, workdir):
    files = os.path.join(workdir, 'files')
    has_psd = mklayers.write_all(files) is not None
    with open(os.path.join(workdir, 'identity.ocio'), 'w') as f:
        f.write(OCIO_CONFIG)

    def fn(f):
        return 'filename=' + os.path.join(files, f)

    ls, ms, ss = mklayers.layers(), mklayers.mode_layers(), mklayers.single_layers()
    # (name, -s settings in order, expected unpremultiplied RGBA, or None for a "Layer not found" failure)
    cases = []

    def menu(prefix, f, layers, default):
        cases.append(('%s_default' % prefix, [fn(f), 'layer=0'], default))
        for i, l in enumerate(layers):
            cases.append(('%s_layer%d' % (prefix, i + 1), [fn(f), 'layer=%d' % (i + 1)], mklayers.place(l)))
        cases.append(('%s_layer%d' % (prefix, len(layers) + 1), [fn(f), 'layer=%d' % (len(layers) + 1)], None))
        cases.append(('%s_layer9' % prefix, [fn(f), 'layer=9'], None))

    for v in (0, 3, 11):
        p = 'xcf%d' % v
        menu(p, 'layers-v%d.xcf' % v, ls, mklayers.composite(ls))
        cases.append((p + '_layer2_offset0', [fn('layers-v%d.xcf' % v), 'layer=2', 'offset=0'], mklayers.place(ls[1], 0, 0)))
        cases.append((p + '_layer4_offset0', [fn('layers-v%d.xcf' % v), 'layer=4', 'offset=0'], mklayers.place(ls[3], 0, 0)))
    for v in (3, 11):
        menu('modes%d' % v, 'modes-v%d.xcf' % v, ms, mklayers.composite(ms))
    menu('single', 'single.xcf', ss, mklayers.composite(ss))
    if has_psd:
        menu('psd', 'layers.psd', ls, mklayers.psd_composite(ls))
        cases.append(('psd_layer4_offset0', [fn('layers.psd'), 'layer=4', 'offset=0'], mklayers.place(ls[3], 0, 0)))
    cases.append(('flat_default', [fn('flat.psd'), 'layer=0'], mklayers.psd_composite(ls)))
    cases.append(('flat_layer1', [fn('flat.psd'), 'layer=1'], None))
    # during a session: change the file (the layer choice stays), then the layer
    if has_psd:
        cases.append(('session_psd_to_xcf', [fn('layers.psd'), 'layer=4', fn('layers-v11.xcf')], mklayers.place(ls[3])))
    cases.append(('session_xcf_to_single', [fn('layers-v11.xcf'), 'layer=4', fn('single.xcf')], None))
    cases.append(('session_single_to_modes', [fn('single.xcf'), 'layer=1', fn('modes-v11.xcf')], mklayers.place(ms[0])))
    cases.append(('session_past_end_then_back', [fn('layers-v11.xcf'), 'layer=9', 'layer=2'], mklayers.place(ls[1])))

    nfail = 0
    for name, settings, exp in cases:
        rc, out, msgs = render(hostdemo, bundle_dir, workdir, name, settings)
        if exp is None:
            # a clean failure: an error status (a negative one is a signal, i.e. a crash), and the message,
            # so that a plug-in that failed to load doesn't pass
            ok = rc > 0 and NOT_FOUND in msgs
            detail = 'rc=%d %s' % (rc, msgs[:120])
        elif rc != 0:
            ok, detail = False, 'rc=%d %s' % (rc, msgs[:120])
        elif out is None:
            ok, detail = False, msgs[:200]
        elif out.shape != exp.shape:
            ok, detail = False, 'size %s, expected %s' % (out.shape[:2], exp.shape[:2])
        else:
            d = np.abs(out - premult(exp))
            ok = d.max() <= TOL
            detail = 'max diff %.3g' % d.max()
        nfail += not ok
        print('%-4s %-30s %s' % ('ok' if ok else 'FAIL', name, detail))
    if not has_psd:
        print('note: ImageMagick not found, the layered PSD cases were skipped')
    print('%d/%d passed%s' % (len(cases) - nfail, len(cases),
                              ' (work dir %s)' % workdir if a.workdir else
                              ('' if not nfail else ' (rerun with --workdir DIR to keep the renders)')))
    return 1 if nfail else 0

if __name__ == '__main__':
    sys.exit(main())
