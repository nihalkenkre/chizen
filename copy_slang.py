import os
import argparse
import shutil
from pathlib import Path


def copy_slang(src, dst):
    print('Copy slang to ' + str(dst))

    if not dst.exists():
        os.mkdir(dst)

    for src_slang in src.glob("*.slang"):
        shutil.copy(src_slang, dst)


def main(args):
    src_path = Path('.').resolve().parent / 'shaders' / 'slang'
    dst_path = Path(args.output_dir) / 'shaders' / 'slang'

    copy_slang(src_path, dst_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('source_dir')
    parser.add_argument('build_type')
    parser.add_argument('output_dir')

    main(parser.parse_args())
