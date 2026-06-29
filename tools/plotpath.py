#!/usr/bin/env python3
# coding=UTF-8
"""Plot path by Matplotlib and Cartopy.

author: Jon Jiang
email: jiangyingming@live.com
"""
import argparse
import os

import numpy as np
import matplotlib.pyplot as plt
import cartopy.crs as ccrs
from cartopy.io.img_tiles import GoogleTiles


def plot(lats: np.ndarray, lons: np.ndarray, style: str, output: str):
    """Plot using matplotlib and cartopy."""
    # Determine extent
    lat_max, lat_min = max(lats), min(lats)
    lon_max, lon_min = max(lons), min(lons)
    extent = [
        lon_min - (lon_max - lon_min) * 0.3,
        lon_max + (lon_max - lon_min) * 0.3,
        lat_min - (lat_max - lat_min) * 0.3,
        lat_max + (lat_max - lat_min) * 0.3
    ]
    # Determine figure size
    size = [12, round(12 * (lat_max - lat_min) / (lon_max - lon_min))]
    # Start plot
    imagery = GoogleTiles(style=style)
    fig = plt.figure(figsize=size)
    ax = fig.add_subplot(1, 1, 1, projection=imagery.crs)
    ax.set_extent(extent, ccrs.PlateCarree())
    # Add imagery to the map
    ax.add_image(imagery, 18)
    # Plot trace
    ax.plot(lons, lats, '-', color='m', transform=ccrs.PlateCarree())
    ax.gridlines(ls='-.')
    plt.savefig(output, format='jpg', bbox_inches='tight')


def init_args():
    """Initilize function, parse user input."""
    # initilize a argument parser
    parser = argparse.ArgumentParser(
        description="Plot path by Matplotlib and Cartopy.",
        epilog="Example: %(prog)s circle.csv -out circle.jpg"
    )
    # add arguments
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 0.1.1')
    parser.add_argument('file', metavar='<file>', type=str,
                        help='path file in CSV, GGA or geojson')
    parser.add_argument('-out', metavar='<file>', default=None,
                        help='output name of figure [default: path file name]')
    parser.add_argument('-style', metavar='<style>', default='satellite',
                        help='in [street, satellite, terrain, only_streets]')

    return parser.parse_args()


def main():
    """Main funciotn."""
    args = init_args()
    # Parse records in CSV or NMEA GPGGA file
    if args.file.lower().endswith('.csv'):
        data = np.genfromtxt(args.file, delimiter=',')
        lats, lons = data[:, 1], data[:, 2]
    elif args.file.lower().endswith('.gga'):
        import pynmea2
        with pynmea2.NMEAFile(args.file) as nmeafile:
            records = [rec for rec in nmeafile]
        lats = np.array([rec.latitude for rec in records])
        lons = np.array([rec.longitude for rec in records])
    elif args.file.lower().endswith('.geojson'):
        import json
        with open(args.file) as fp:
            data = json.load(fp)
        path = np.array(data['geometry']['coordinates'])
        lats, lons = path[:, 1], path[:, 0]
    else:
        raise ValueError('Trace format not support.')
    # The output figure name
    if args.out is None:
        output = os.path.splitext(args.file)[0] + '.jpg'
    else:
        output = args.out
    plot(lats, lons, args.style, output)

    return 0


if __name__ == '__main__':
    main()
