#!/usr/bin/env python3
# coding=UTF-8
"""Generate path by a point coordinates file.

author: Jon Jiang
email: jiangyingming@live.com
"""
import argparse

import numpy as np


def init_args():
    """Initilize function, parse user input."""
    # initilize a argument parser
    parser = argparse.ArgumentParser(
        description="Generate path by a point coordinates file.",
        epilog="Example: %(prog)s points.csv -out path.csv"
    )
    # add arguments
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 0.1.0')
    parser.add_argument('file', metavar='<file>', type=str,
                        help='point coordinates in (time,lat,lon,hgt)')
    parser.add_argument('-out', metavar='<file>', required=True,
                        help='output file name of path [required]')
    parser.add_argument('-len', metavar='<length>', type=int, default=300,
                        help='length of output path (seconds) [default 300]')

    return parser.parse_args()


def main():
    """Main function."""
    args = init_args()
    # Parse file as an array
    points = np.genfromtxt(args.file, delimiter=',')
    times = np.arange(0, args.len, 0.1)
    # Start interpolate
    lats = np.interp(times, points[:, 0], points[:, 1])
    lons = np.interp(times, points[:, 0], points[:, 2])
    hgts = np.interp(times, points[:, 0], points[:, 3])
    # Output
    with open(args.out, 'w') as fp:
        for time, lat, lon, hgt in zip(times, lats, lons, hgts):
            fp.write(f'{time:.1f},{lat:.1f},{lon:.1f},{hgt:.1f}\n')

    return 0


if __name__ == '__main__':
    main()
