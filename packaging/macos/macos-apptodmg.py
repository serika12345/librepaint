#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import os
import plistlib
import shutil
import subprocess
import argparse

# TODO: port all shutil move to Path.move after python 3.14

class DmgFile:
    def __init__(self, src_location: pathlib.Path, dst_path: [str|pathlib.Path],dst_name: str=None):
        self.src_path = src_location
        self.dst_path = pathlib.Path(dst_path)
        self.dst_name = src_location.name if not dst_name else dst_name

    def install(self, dmg_root: pathlib.Path):
        final_dst = dmg_root.joinpath(self.dst_path)
        if not final_dst.exists():
            final_dst.mkdir()

        shutil.copy2(self.src_path, final_dst.joinpath(self.dst_name))



def main():
    parser = argparse.ArgumentParser(prog='macos_apptodmg',
                                     description='Utility to generate a dmg from a LibrePaint.app')
    parser.add_argument('librepaint_app', metavar='LibrePaint.app', help="LibrePaint.app path location")
    parser.add_argument('--buildroot', help="Directory where the source tree and _install are located",
                        default=os.getenv("BUILDROOT", False))
    parser.add_argument('--media-path', dest="media_path", metavar='<path>', help="path location of background, icons, and ToS")

    parser.add_argument('--style', help="Style defined from dmgstyle.sh's output",metavar='<file>')
    parser.add_argument('--bg', help="Set a background image for dmg window",metavar='<file>')
    parser.add_argument('-n','--dmg_name', help="Set DMG output dmg_name",metavar='<string>')
    parser.add_argument('--suffix', dest="suffix", help="Set DMG output name suffix", metavar='<string>')
    args = parser.parse_args()

    librepaint_app = pathlib.Path(args.librepaint_app).resolve()
    print(f'Creating dmg from: {librepaint_app}')

    if librepaint_app.name != 'LibrePaint.app':
        raise SystemExit(f"Expected a LibrePaint.app bundle, got {librepaint_app.name}")

    # --- Style options DMG
    if args.buildroot:
        print(f"root {args.buildroot}")
        if args.media_path:
            print("WARNING: --media-path ignored as --buildroot or env BUILDROOT is present")
        krita_root: pathlib.Path = pathlib.Path(args.buildroot).resolve()
        krita_source_dir = pathlib.Path(os.path.join(krita_root, "krita"))
        krita_media_path = krita_source_dir.joinpath('packaging','macos')

    else:
        if not args.media_path:
            print("ERROR: if --builroot or env BUILDROOT is missing --media-path must be present")
            exit(1)
        krita_media_path = pathlib.Path(args.media_path).resolve()


    krita_dmg_background = krita_media_path.joinpath('librepaint-dmg-background.png') if not args.bg or not os.path.exists(args.bg) else pathlib.Path(args.bg)
    dmg_files: list[DmgFile] = [
        DmgFile(
            krita_dmg_background
            , ".background"
        ),
        DmgFile(
            krita_media_path.joinpath('Terms_of_use.rtf')
            , 'Terms of Use'
            , 'Terms_of_use.rtf'
        ),
        DmgFile(
            krita_media_path.joinpath('LibrePaint.icns')
            , "."
            , '.VolumeIcon.icns'
        )
    ]

    # preparing style to parsed string.
    krita_dmg_style = krita_media_path.joinpath('default.style') if not args.style or not os.path.exists(args.style) else pathlib.Path(args.style)
    # we still don't know the mounting point, so we pass through the first substitution
    kritadmg_style_formatted = krita_dmg_style.read_text() % ("%s", krita_dmg_background.name)


    # --- LibrePaint version adjustments
    with librepaint_app.joinpath('Contents', 'Info.plist').open('rb') as handle:
        bundle_info = plistlib.load(handle)
    expected_identity = {
        'CFBundleDisplayName': 'LibrePaint',
        'CFBundleExecutable': 'LibrePaint',
        'CFBundleIdentifier': 'local.librepaint.macos',
        'CFBundleName': 'LibrePaint',
    }
    for key, expected in expected_identity.items():
        actual = bundle_info.get(key)
        if actual != expected:
            raise SystemExit(f"{key} is {actual!r}; expected {expected!r}")
    bundle_version = bundle_info['CFBundleShortVersionString']

    kis_name = f"LibrePaint-{bundle_version}"
    if args.dmg_name:
        kis_name = args.dmg_name
    if args.suffix:
        kis_name += args.suffix

    krita_dmg = kritaCreateDMG(librepaint_app, dmg_files, kis_name, kritadmg_style_formatted)


    if args.buildroot:
        krita_packaging = krita_root.joinpath("_packaging")
    else:
        krita_packaging = pathlib.Path("_packaging").resolve()

    krita_packaging.mkdir(exist_ok=True)
    shutil.move(krita_dmg, krita_packaging)

    print(f'LibrePaint.app to dmg finished!')
    print(f'output file {krita_dmg} saved to {krita_packaging}')



def kritaCreateDMG(librepaint_app: pathlib.Path, krita_dmg_media: list[DmgFile], dmg_name: str,
                   kritadmg_style: str
                   ) -> pathlib.Path:

    # Create dmg_root location must only contain LibrePaint.app
    krita_dmg_root = pathlib.Path('_dmg_wd').resolve()
    krita_dmg_root.mkdir()

    # Qt 6 plugins may carry extended attributes, so use rsync to preserve them.
    print(f'Cloning source LibrePaint.app to working dir {krita_dmg_root}')
    cmd = ['rsync', '-aE', librepaint_app, krita_dmg_root]
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as err:
        print(f"## ERROR: Cloning LibrePaint.app failed!\n{err.stderr}")
        exit(1);

    kritadmg_title = dmg_name + '.dmg'
    kritadmg_output = pathlib.Path(kritadmg_title).resolve()

    # hardcoded size to 2.0G
    dmg_size = 2000

    krita_dmgtmp = pathlib.Path("LibrePaint.temp.dmg")

    cmd = f'hdiutil create -srcfolder {krita_dmg_root} -volname {dmg_name} \
            -fs APFS -format UDIF -verbose -size {dmg_size}m'.split()
    cmd.append(str(krita_dmgtmp))
    print(f'## RUNNING: {cmd}')
    subprocess.run(cmd)

    kritadmg_mountpoint = pathlib.Path("_kritadmg")
    cmd = f"hdiutil attach -mountpoint {kritadmg_mountpoint}".split()
    cmd.append('-readwrite')
    cmd.append('-noverify')
    cmd.append('-noautoopen')
    cmd.append(f'{krita_dmgtmp}')
    print(f'## RUNNING: {" ".join(cmd)}')
    subprocess.run(cmd)


    kritadmg_applink = pathlib.Path(kritadmg_mountpoint,'Applications')
    kritadmg_applink.symlink_to(pathlib.Path('/','Applications'))

    # copy media files for style to dmg
    for entry in krita_dmg_media:
        entry.install(kritadmg_mountpoint)

    # set icon and style for dmg
    cmd = f'SetFile -a C {kritadmg_mountpoint}'.split()
    subprocess.run(cmd)
    print("## RUNNING: osascript")
    subprocess.run('osascript', input=kritadmg_style % kritadmg_mountpoint, text=True)

    cmd = f'chmod -Rf go-w {kritadmg_mountpoint}'.split()
    subprocess.run(cmd)

    # Make sure all writting operations to dmg are finished
    subprocess.run(['sync'])

    cmd = f'hdiutil detach {kritadmg_mountpoint}'.split()
    subprocess.run(cmd)

    cmd = f'hdiutil convert {krita_dmgtmp} -format UDZO -imagekey -zlib-level=9 \
        -o {kritadmg_title}'.split()
    print(f'## RUNNING: {" ".join(cmd)}')
    subprocess.run(cmd)

    krita_dmgtmp.unlink()

    return kritadmg_output



if __name__ == '__main__':
    main()
