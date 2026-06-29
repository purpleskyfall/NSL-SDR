#!/usr/bin/env python3
# coding=UTF-8
"""Check difference of two binary files.

Author: Jon Jiang
Email: jiangyingming@live.com
"""
import argparse


def main() -> int:
    """Main funciton."""
    args = init_args()
    file1, file2 = args.file
    # Start checking
    with open(file1, 'rb') as fp1, open(file2, 'rb') as fp2:
        buf1, buf2, idx = fp1.read(1024), fp2.read(1024), 0
        while buf1 == buf2 and len(buf1) > 0:
            buf1, buf2 = fp1.read(1024), fp2.read(1024)
            idx += len(buf1)

        if buf1 != buf2:
            if args.index:
                idx += next(i for i in range(len(buf1)) if buf1[i] != buf2[i])
                print(f'NO, index: {idx}')
            else:
                print('NO')
        else:
            print('OK')

    return 0


def init_args():
    """Initilize function, parse user input."""
    # initilize a argument parser
    parser = argparse.ArgumentParser(
        description="Check difference of two binary files.",
        epilog="Example: %(prog)s gpssim.bin gpssim.iq"
    )
    # add arguments
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 0.1.1')
    parser.add_argument('file', metavar='<file>', nargs=2, type=str,
                        help='binary files')
    parser.add_argument('--index', action='store_true',
                        help='Show index of difference bit')

    return parser.parse_args()


if __name__ == '__main__':
    main()
