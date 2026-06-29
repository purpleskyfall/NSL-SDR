#!/usr/bin/env python3
# coding=UTF-8
"""Plot sky map of satellites by matplotlib.

author: Jon Jiang
email: jiangyingming@live.com
"""
import argparse
import os

import numpy as np
import matplotlib.pyplot as plt


COLOR_MAP = {
    'G': 'goldenrod', 'R': 'cornflowerblue', 'C': 'tomato', 'E': 'limegreen'
}


def plot(data: np.recarray, output: str) -> None:
    """Plot sky map using matplotlib."""
    fig = plt.figure(figsize=(7, 7))
    ax = fig.add_subplot(projection="polar", facecolor="azure")
    # Start plot
    for r, theta, sat in zip(data['Elev'], data['Azimu'], data['Sat']):
        ax.scatter(np.radians(theta), 90-r, s=350, c=COLOR_MAP[sat[0]])
        ax.text(np.radians(theta), 90-r, sat, fontsize=9,
                horizontalalignment='center', verticalalignment='center')
    # Figure style setting
    ax.grid(ls=':')
    ax.set_rlabel_position(0)
    ax.set_rticks(np.arange(20, 90, 20), labels=['70', '50', '30', '10'],
                  color='#444')
    ax.set_theta_zero_location('N')
    ax.set_theta_direction('clockwise')
    ax.set_thetagrids(np.arange(0, 360, 45),
                      labels=['N', '', 'E', '', 'S', '', 'W', ''])

    plt.savefig(output, format='jpg', bbox_inches='tight')


def init_args():
    """Initilize function, parse user input."""
    # initilize a argument parser
    parser = argparse.ArgumentParser(
        description="Plot sky map of satellites by matplotlib",
        epilog="Example: %(prog)s sat.txt -out skymap.jpg"
    )
    # add arguments
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 0.1.0')
    parser.add_argument('file', metavar='<file>', type=str,
                        help='satellite information output by siren')
    parser.add_argument('-out', metavar='<file>', default=None,
                        help='output name of figure [default: file_skymap]')

    return parser.parse_args()


def main():
    """Main funciotn."""
    args = init_args()
    data = np.recfromtxt(args.file, encoding='utf8', names=True)
    # The output figure name
    if args.out is None:
        output = os.path.splitext(args.file)[0] + '_skymap.jpg'
    else:
        output = args.out
    plot(data, output)

    return 0


if __name__ == '__main__':
    main()
