#!/usr/bin/env python3
"""
tools/build_fs.py — Build the FAT16 filesystem image (fat16.bin)

Creates a valid FAT16 image containing all scripts from the scripts/ directory.
The volume label is set to "ESP32" so PowerShell can find the drive reliably:
  $d=(gwmi Win32_LogicalDisk|?{$_.VolumeName-eq'ESP32'}|select -First 1 -Exp DeviceID)

Usage:
  python tools/build_fs.py

Output:
  tools/fat16.bin  (10 MB, ready to flash at 0x600000)

Flash after building:
  python tools/flash_fs.py
"""

import struct
import os
import sys

# ── FAT16 geometry ────────────────────────────────────────────────────────────
SECTOR_SIZE         = 512
TOTAL_SECTORS       = 20480       # 10 MB / 512 = 20480
NUM_FATS            = 2
ROOT_ENTRIES        = 512         # max files in root directory
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS    = 4
VOLUME_LABEL        = b'ESP32      '   # exactly 11 bytes — shown as drive name

# Derived layout
root_dir_sectors    = (ROOT_ENTRIES * 32) // SECTOR_SIZE
data_sectors_approx = TOTAL_SECTORS - RESERVED_SECTORS - root_dir_sectors
fat_size            = (data_sectors_approx + SECTOR_SIZE // 2 - 1) // (SECTOR_SIZE // 2)
fat1_start          = RESERVED_SECTORS
fat2_start          = fat1_start + fat_size
rootdir_start       = fat2_start + fat_size
data_start          = rootdir_start + root_dir_sectors   # cluster 2 starts here
cluster_size        = SECTORS_PER_CLUSTER * SECTOR_SIZE

# Validate FAT type
data_sectors  = TOTAL_SECTORS - RESERVED_SECTORS - (NUM_FATS * fat_size) - root_dir_sectors
cluster_count = data_sectors // SECTORS_PER_CLUSTER
assert 4085 < cluster_count < 65525, f"Cluster count {cluster_count} not in FAT16 range"

# ─────────────────────────────────────────────────────────────────────────────
def make_83_name(filename: str) -> bytes:
    """Convert 'foo.ps1' → b'FOO     PS1' (FAT 8.3 format, space-padded)."""
    filename = filename.upper()
    if '.' in filename:
        name, ext = filename.rsplit('.', 1)
    else:
        name, ext = filename, ''
    return (name[:8].ljust(8) + ext[:3].ljust(3)).encode('ascii')


def make_dir_entry(name83: bytes, first_cluster: int, file_size: int,
                   attr: int = 0x20) -> bytes:
    entry = bytearray(32)
    entry[0:11]  = name83
    entry[11]    = attr
    entry[22:24] = struct.pack('<H', (44 << 9) | (1 << 5) | 1)   # date 2024-01-01
    entry[24:26] = struct.pack('<H', 0)                            # time 00:00:00
    entry[26:28] = struct.pack('<H', first_cluster)
    entry[28:32] = struct.pack('<L', file_size)
    return bytes(entry)


def build_image(scripts_dir: str, output_path: str):
    image = bytearray(TOTAL_SECTORS * SECTOR_SIZE)

    # ── Boot Parameter Block ──────────────────────────────────────────────────
    boot = bytearray(SECTOR_SIZE)
    boot[0:3]   = b'\xEB\x58\x90'
    boot[3:11]  = b'MSDOS5.0'
    struct.pack_into('<H', boot, 11, SECTOR_SIZE)
    boot[13]    = SECTORS_PER_CLUSTER
    struct.pack_into('<H', boot, 14, RESERVED_SECTORS)
    boot[16]    = NUM_FATS
    struct.pack_into('<H', boot, 17, ROOT_ENTRIES)
    struct.pack_into('<H', boot, 19, TOTAL_SECTORS)
    boot[21]    = 0xF8          # fixed-disk media descriptor
    struct.pack_into('<H', boot, 22, fat_size)
    struct.pack_into('<H', boot, 24, 63)    # sectors/track (geometry, cosmetic)
    struct.pack_into('<H', boot, 26, 255)   # heads
    struct.pack_into('<L', boot, 28, 0)     # hidden sectors
    struct.pack_into('<L', boot, 32, 0)     # TotSec32 (0 because TotSec16 is set)
    boot[36]    = 0x80          # drive number
    boot[37]    = 0x00
    boot[38]    = 0x29          # extended boot signature
    struct.pack_into('<L', boot, 39, 0xDEADBEEF)   # volume serial number
    boot[43:54] = VOLUME_LABEL  # BPB volume label (also shown by some tools)
    boot[54:62] = b'FAT16   '
    boot[510]   = 0x55
    boot[511]   = 0xAA
    image[0:SECTOR_SIZE] = boot

    # ── FAT tables ────────────────────────────────────────────────────────────
    # Collect all files from scripts_dir
    files = []
    if os.path.isdir(scripts_dir):
        for fname in sorted(os.listdir(scripts_dir)):
            fpath = os.path.join(scripts_dir, fname)
            if os.path.isfile(fpath):
                with open(fpath, 'rb') as f:
                    data = f.read()
                files.append((fname, data))

    # Assign clusters
    file_clusters = []
    next_cluster = 2
    for fname, data in files:
        n = max(1, (len(data) + cluster_size - 1) // cluster_size)
        file_clusters.append((fname, data, next_cluster, n))
        next_cluster += n

    # Write FAT1 and FAT2
    for fat_num in range(NUM_FATS):
        base = (fat1_start + fat_num * fat_size) * SECTOR_SIZE
        # Reserved entries
        image[base]     = 0xF8; image[base + 1] = 0xFF   # cluster 0
        image[base + 2] = 0xFF; image[base + 3] = 0xFF   # cluster 1
        # File chains
        for _, _, start_cluster, n_clusters in file_clusters:
            for i in range(n_clusters - 1):
                off = base + (start_cluster + i) * 2
                struct.pack_into('<H', image, off, start_cluster + i + 1)
            off = base + (start_cluster + n_clusters - 1) * 2
            struct.pack_into('<H', image, off, 0xFFFF)    # end of chain

    # ── Root directory ────────────────────────────────────────────────────────
    rd_base = rootdir_start * SECTOR_SIZE
    entry_idx = 0

    # Entry 0: volume label — required for Windows to show the drive name
    vol_entry = bytearray(32)
    vol_entry[0:11] = VOLUME_LABEL
    vol_entry[11]   = 0x08   # ATTR_VOLUME_ID
    image[rd_base:rd_base + 32] = vol_entry
    entry_idx += 1

    # File entries
    for fname, data, start_cluster, _ in file_clusters:
        if entry_idx >= ROOT_ENTRIES:
            print(f"WARNING: root directory full, skipping {fname}")
            break
        name83 = make_83_name(fname)
        entry  = make_dir_entry(name83, start_cluster, len(data))
        off    = rd_base + entry_idx * 32
        image[off:off + 32] = entry
        entry_idx += 1

    # ── File data ─────────────────────────────────────────────────────────────
    for _, data, start_cluster, n_clusters in file_clusters:
        padded = data + b'\x00' * (n_clusters * cluster_size - len(data))
        off    = (data_start + (start_cluster - 2) * SECTORS_PER_CLUSTER) * SECTOR_SIZE
        image[off:off + len(padded)] = padded

    with open(output_path, 'wb') as f:
        f.write(image)

    return files


# ─────────────────────────────────────────────────────────────────────────────
if __name__ == '__main__':
    root        = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    scripts_dir = os.path.join(root, 'scripts')
    output      = os.path.join(root, 'tools', 'fat16.bin')

    print(f"Building FAT16 image...")
    print(f"  Scripts : {scripts_dir}")
    print(f"  Output  : {output}")
    print(f"  Label   : {VOLUME_LABEL.decode().strip()}")
    print(f"  Layout  : {TOTAL_SECTORS} sectors, FAT16 ({cluster_count} clusters)")
    print()

    files = build_image(scripts_dir, output)

    print(f"Files included in image:")
    if files:
        for fname, data in files:
            print(f"  {fname:<20} {len(data):>6} bytes")
    else:
        print(f"  (none — scripts/ directory is empty)")

    size = os.path.getsize(output)
    print(f"\nImage: {output} ({size:,} bytes)")
    print(f"Flash with: python tools/flash_fs.py")