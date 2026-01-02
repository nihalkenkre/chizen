import os
import argparse
import shutil
from pathlib import Path


def copy_images(src, dst):
    print('Copy images to ' + str(dst))

    if not dst.exists():
        os.mkdir(dst)

    for src_img in src.glob('*'):
        shutil.copy(src_img, dst)


def main(args):
    src_path = Path('.').resolve().parent / 'images'
    dst_path = Path(args.output_dir) / 'images'

    copy_images(src_path, dst_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('source_dir')
    parser.add_argument('build_type')
    parser.add_argument('output_dir')

    main(parser.parse_args())
