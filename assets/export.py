#!/usr/bin/env python3

import subprocess
import os

src_path = os.getcwd()
os.chdir('/tmp')

nvdxt_path = os.path.join(os.getenv('HOME'), '.wine/drive_c/Programme/NVIDIA Corporation/DDS Utilities')

for level, size, brightness in [
        (0, 256, .80),
        (1, 128, .80),
        (2,  64, .80**2),
        (3,  32, .80**3),
        (4,  16, .80**4),
        (5,   8, .80**5),
        (6,   4, .80**6),
        (7,   2, .80**7),
        (8,   1, .80**8),
    ]:
    export_filename = f'export_{level:02}.png'
    subprocess.check_call(['inkscape', '--without-gui', '--export-area-page', f'--export-png={export_filename}', f'--export-width={size}', f'--export-height={size}', os.path.join(src_path, 'textur.svg')])
    subprocess.check_call(['wine', os.path.join(nvdxt_path, 'nvdxt.exe'), '-dxt3', '-quality_highest', '-nomipmap',

        # Nur fuer Tunnel-Variante
        # '-brightness', str(brightness-1), str(brightness-1), str(brightness-1), '0',

        '-file', export_filename])

subprocess.check_call(['wine', os.path.join(nvdxt_path, 'stitch.exe'), 'export'])
