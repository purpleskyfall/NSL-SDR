#!/usr/bin/env python3
# coding=UTF-8
"""Combine GPS L1 and BeiDou B1I IQ signals into one composite signal.

The script loads two binary IQ files (interleaved int8), up-converts them to
their respective IF frequencies, sums them, normalizes, and saves the result
as an interleaved int8 file suitable for HackRF transmission.

author: Jon Jiang
email: jiangyingming@live.com
"""

import argparse
import numpy as np


def load_iq(filename: str) -> np.ndarray:
    """Load interleaved int8 IQ data from a binary file.

    Args:
        filename: Path to the binary file.

    Returns:
        Complex64 array normalized to [-1, 1].
    """
    iq_data = np.fromfile(filename, dtype=np.int8)
    iq_data = iq_data[0::2] + 1j * iq_data[1::2]
    iq_data = iq_data.astype(np.complex64) / 128.0
    return iq_data


def main() -> int:
    """Main function: parse arguments, process signals, save result."""
    args = init_args()

    # Unpack arguments
    gps_file = args.gps_file
    bds_file = args.bds_file
    output_file = args.output
    fs = args.sample_rate

    # GPS and BeiDou carrier frequencies (Hz)
    gps_freq = 1575.42e6
    bds_freq = 1561.098e6

    # Center frequency (midpoint between GPS and BDS)
    center_freq = (gps_freq + bds_freq) / 2.0

    # Intermediate frequencies relative to center
    gps_if_freq = gps_freq - center_freq
    bds_if_freq = bds_freq - center_freq

    # Load IQ data
    try:
        gps_signal = load_iq(gps_file)
        bds_signal = load_iq(bds_file)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return 1

    # Align lengths to the shorter signal
    min_len = min(len(gps_signal), len(bds_signal))
    gps_signal = gps_signal[:min_len]
    bds_signal = bds_signal[:min_len]

    # Time axis
    t = np.arange(min_len) / fs

    # Up-convert each signal to its IF
    gps_up = gps_signal * np.exp(1j * 2 * np.pi * gps_if_freq * t)
    bds_up = bds_signal * np.exp(1j * 2 * np.pi * bds_if_freq * t)

    # Combine and normalize
    combined = gps_up + bds_up
    combined /= np.max(np.abs(combined))

    # Convert to interleaved int8 format
    combined_i = np.clip(np.real(combined) * 127, -128, 127).astype(np.int8)
    combined_q = np.clip(np.imag(combined) * 127, -128, 127).astype(np.int8)

    combined_iq = np.empty(2 * len(combined_i), dtype=np.int8)
    combined_iq[0::2] = combined_i
    combined_iq[1::2] = combined_q

    # Save to file
    combined_iq.tofile(output_file)

    print(f'Successfully merged and saved to: {output_file}')
    print(f'HackRF command:\n hackrf_transfer -t {output_file} '
          f'-f {int(center_freq)} -s {int(fs)} -x 40')

    return 0


def init_args() -> argparse.Namespace:
    """Initialize and parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Combine GPS L1C/A and BeiDou B1I IQ signals into one composite signal.",
        epilog="Example: %(prog)s --gps GPSL1.bin --bds B1I.bin -o combined.bin -s 8000000"
    )

    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 1.0.0')
    parser.add_argument('--gps', dest='gps_file', required=True,
                        metavar='<file>', type=str,
                        help='path to GPS L1C/A IQ binary file (interleaved int8)')
    parser.add_argument('--bds', dest='bds_file', required=True,
                        metavar='<file>', type=str,
                        help='path to BeiDou B1I IQ binary file (interleaved int8)')
    parser.add_argument('-o', '--output', default='combined_signal.bin',
                        metavar='<file>', type=str,
                        help='output file path (default: combined_signal.bin)')
    parser.add_argument('-s', '--sample-rate', dest='sample_rate',
                        type=float, default=8e6,
                        help='sampling rate in Hz (default: 8e6)')

    return parser.parse_args()


if __name__ == '__main__':
    main()
