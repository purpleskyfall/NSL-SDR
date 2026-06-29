#!/usr/bin/env python3
# coding=UTF-8
"""Map projection, reducing the surface of the Earth to a flat map.

In cartography, a map projection is a way to flatten a globe's surface into a
plane in order to make a map. This requires a systematic transformation of the
latitudes and longitudes of locations from the surface of the globe into
locations on a plane.

The direct method convert coordinate from geodetic into cartographic, however
the inverse method convert the coordinate from cartographic into geodetic.

"""
from collections import namedtuple
import argparse
import math
import sys

Cartographic = namedtuple('Cartographic', ['x', 'y'])
Ellipsoidal = namedtuple('Ellipsoidal', ['latitude', 'longitude'])
Ellipsoid = namedtuple('Ellipsoid', ['a', 'f', 'GM', 'omega'])

CGCS2000 = Ellipsoid(6378137.0, 1/298.257222101, 3.986004418e14, 7.292115e-5)
ellipsoids = {'CGCS2000': CGCS2000}


def gauss_inverse(point: Cartographic, central_meridian: float, false_east=0,
                  false_north=0) -> Ellipsoidal:
    """Inverse method for Gauss–Krüger projection.

    Algorithm is published on:

        ZHAO Chang-sheng, Iterative Algorithm of Gauss Projection Plane
        Rectangular Coordinates, Bulletin of Surveying and Mapping, 16-17
        2004(vol 3).

    Example, from WANG Jian's teaching materials in CUMT::

        >>> point = Cartographic(3548910.811, 179854.617)
        >>> geodetic = gauss_inverse(point, 117, 'WGS84')
        >>> round(geodetic.latitude, 9), round(geodetic.longitude, 9)
        (32.049347831, 118.904227942)

    """
    datum = ellipsoids['CGCS2000']
    # The semi-major axis a, eccentricity e and e'
    a = datum.a
    e = math.sqrt(2 * datum.f - datum.f ** 2)
    e_ = math.sqrt(1 / (1 - e ** 2) - 1)
    # The A0, B0, C0, D0, E0
    e2, e4, e6, e8 = e ** 2, e ** 4, e ** 6, e ** 8
    A0 = a * (1 - e2) * (
        1 + 3 / 4 * e2 + 45 / 64 * e4 + 175 / 256 * e6 + 11025 / 16384 * e8
    )
    B0 = a * (1 - e2) * (
        3 / 4 * e2 + 45 / 64 * e4 + 175 / 256 * e6 + 11025 / 16384 * e8
    )
    C0 = a * (1 - e2) * (15 / 32 * e4 + 175 / 384 * e6 + 3675 / 8192 * e8)
    D0 = a * (1 - e2) * (35 / 96 * e6 + 735 / 2048 * e8)
    E0 = a * (1 - e2) * (315 / 1024 * e8)
    # Remove the offset of x, y
    x, y = point.x - false_north, point.y - false_east
    # Initialize the B, a1 and l
    B = x / A0
    a1 = a * math.cos(B) / math.sqrt(1 - e2 * math.sin(B) ** 2)
    l = y / a1
    B_0, l_0 = 0.0, 0.0
    while abs(B - B_0) > 4.8e-10 and abs(l - l_0) > 4.8e-10:
        B_0, l_0 = B, l
        sinB, cosB, tanB = math.sin(B), math.cos(B), math.tan(B)
        # The correction of the central meridian arc length
        dX = (
            B0 + (C0 + (D0 + E0 * sinB ** 2) * sinB ** 2) * sinB ** 2
        ) * sinB * cosB
        # The N and a2, a3, a4, a5, a6
        N = a / math.sqrt(1 - e2 * sinB ** 2)
        a2 = N * sinB * cosB / 2
        a3 = N * cosB ** 3 * (1 - tanB ** 2 + (e_ * cosB) ** 2) / 6
        a4 = N * sinB * cosB ** 3 * (
            5 - tanB ** 2 + 9 * (e_ * cosB) ** 2 + 4 * (e_ * cosB) ** 4
        ) / 24
        a5 = N * cosB ** 5 * (
            5 - 18 * tanB ** 2 + tanB ** 4 + 14 * (e_ * cosB) ** 2
            - 58 * (e_ * sinB) ** 2
        ) / 120
        a6 = N * sinB * cosB ** 5 * (61 - 58 * tanB ** 2 + tanB ** 4) / 720
        # The new latitude
        B = (x + dX - a2 * l_0 ** 2 - a4 * l_0 ** 4 - a6 * l_0 ** 6) / A0
        # The new a1
        a1 = a * math.cos(B) / math.sqrt(1 - e2 * math.sin(B) ** 2)
        # The new difference of the longitude
        l = (y - a3 * l_0 ** 3 - a5 * l_0 ** 5) / a1

    return Ellipsoidal(math.degrees(B), central_meridian + math.degrees(l))


def zone2merid(index: int, zone: int) -> float:
    """Convert Gauss–Krüger projection zone index to central-meridian."""
    if zone not in (3, 6):
        raise ValueError('The type of zone should be 3 or 6.')

    if zone == 3:
        return index * 3
    else:
        return index * 6 - 3


def main():
    """Main function."""
    args = init_args()
    x = float(args.x)
    zone_index = int(args.y[:2])
    central_merid = zone2merid(zone_index, args.zone)
    y = float(args.y[2:]) - 500_000_000

    point = Cartographic(x, y)
    geodetic = gauss_inverse(point, central_merid)

    print(
        f'latitude: {geodetic.latitude:.9f},'
        f' longitude: {geodetic.longitude:.9f}',
        file=args.out
    )

    return 0


def init_args():
    """Initialize user input"""
    # Create a arguments parser
    parser = argparse.ArgumentParser(
        description="Convert cartographic coordinate to geodetic system.")
    # Add arguments
    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 0.0.1')
    parser.add_argument('-x', metavar='<val>', dest='x',
                        required=True, help='the X coordinate')
    parser.add_argument('-y', metavar='<val>', dest='y',
                        required=True, help='the Y coordinate')
    parser.add_argument('-zone', metavar='<val>', dest='zone', choices=[3, 6],
                        type=int, required=True, help='The type of zone')
    parser.add_argument('-out', metavar='<path>', default=sys.stdout,
                        type=argparse.FileType('w'),
                        help='the output path of result')

    # Parse arguments
    return parser.parse_args()


if __name__ == '__main__':
    main()
