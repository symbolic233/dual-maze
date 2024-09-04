"""
Jude Markabawi (jmarkaba@andrew.cmu.edu)

Take a 256x240 png and make tiles, palettes, etc. out of it

Usage:
    python asset_gen.py image.png

"""

# I have made insignificant edits to the program. ~Yoseph

from argparse import ArgumentParser
import numpy as np
import os
from PIL import Image

# (y, x)
bg_size = (240, 256, 4)
bg_tile_size = (30, 32)


def load_image(path: str):
    with Image.open(path) as im:
        return np.array(im)


def find_tiles_and_palettes(img: np.ndarray):
    assert tuple(img.shape) == bg_size, "Image must be 240x256x4 for this script"
    ## --- Tiles ---
    tiles = {}
    for y in range(0, bg_size[0], 8):
        for x in range(0, bg_size[1], 8):
            tile = img[y : y + 8, x : x + 8, :]
            assert tuple(tile.shape) == (8, 8, 4)

            if tile.tobytes() not in tiles.keys():
                tiles[tile.tobytes()] = tile

    print(f"There are {len(tiles.values())} unique tiles.")

    ## --- Palettes ---
    tiles_palettes = {} # dict: tile bytes ----> palette (set of color tuples)
    for k, tile in tiles.items():
        unique_colors = set()
        colors = tile.reshape(-1, 4)
        for color in colors:
            unique_colors.add(tuple(color))
        assert len(unique_colors) <= 4, "There is a tile with more than 4 colors. This is not supported"
        tiles_palettes[k] = set(unique_colors)
    
    palettes_unreduced_set = set()
    # Make set of unique palettes
    for k, palette in tiles_palettes.items():
        palettes_unreduced_set.add(tuple(sorted(palette)))
    # Remove subset palettes
    palettes_unreduced_list = list(palettes_unreduced_set)
    palettes_set = set()
    for i, palette in enumerate(palettes_unreduced_list):
        is_necessary_palette = True
        for j, other_palette in enumerate(palettes_unreduced_list):
            palette = set(palette)
            if i != j and palette.issubset(other_palette):
                is_necessary_palette = False
        if is_necessary_palette:
            palettes_set.add(tuple(sorted(palette)))
    # Update tile -> palette mapping with reduced palette set
    for k, palette in tiles_palettes.items():
        for other_palette in palettes_set:
            if palette.issubset(other_palette):
                tiles_palettes[k] = other_palette

    print(f"There are {len(palettes_set)} necessary unique palettes.")

    # Map tiles to palette idxs
    palettes = list(palettes_set)
    tiles_palettes_idxs = {} # dict: tile bytes ----> palette idx
    for k, palette in tiles_palettes.items():
        tiles_palettes_idxs[k] = palettes.index(palette)
    
    # Right now, palettes is a N x (1-4) x 4 list of tuples
    # Pad last color to palettes with less than 4
    for i, palette in enumerate(palettes):
        palette = list(palette)
        while len(palette) < 4:
            palette.append(palette[-1])
        palettes[i] = tuple(palette)
        assert len(palettes[i]) == 4

    ## --- More Tile Info ---
    # We now have
    #   palettes - list: N x 4 x 4
    #   tiles_palettes_idxs - dict: tile bytes ----> idx in palettes
    #   tiles - dict: tile bytes ----> tile np array

    # dict: tile bytes ----> (index, tile np array)
    tiles = {k: (i, v) for (i, (k, v)) in enumerate(tiles.items())}

    # Combined dict: tile bytes ----> (tile table idx, palette idx)
    tile_info = {k: (i, tiles_palettes_idxs[k]) for (k, (i, _)) in tiles.items()}

    ### THINGS TO SAVE TO DISK ###
    
    ## --- Background ---
    # - Generate map with tile table idxs and palette idxs
	# The background is stored as a row-major grid of 16-bit values:
	#  the origin of the grid (tile (0,0)) is the bottom left of the grid
	#  each value in the grid gives:
	#    - bits 0-7: tile table index
	#    - bits 8-10: palette table index
	#    - bits 11-15: unused, should be 0
	
	#  bits:  F E D C B A 9 8 7 6 5 4 3 2 1 0
	#        |---------|-----|---------------|
	#            ^        ^        ^-- tile index
	#            |        '----------- palette index
	#            '-------------------- unused (set to zero)
    background = np.zeros(bg_tile_size, dtype=np.uint16)
    for y in range(0, bg_size[0], 8):
        for x in range(0, bg_size[1], 8):
            tile = img[y : y + 8, x : x + 8, :]
            assert tuple(tile.shape) == (8, 8, 4)

            tile_idx, palette_idx = tile_info[tile.tobytes()]
            assert 0 <= tile_idx and tile_idx < (2 ** 8), "Too many tiles or I messed up somewhere"
            assert 0 <= palette_idx and palette_idx < (2 ** 3), "Too many tiles or I messed up somewhere"

            bit_info = np.uint16(0)
            bit_info = (bit_info ^ palette_idx) << 8
            bit_info = (bit_info ^ tile_idx)
            background[y // 8, x // 8] = bit_info
    # Flip y since PPU uses bottom-to-top instead of top-to-bottom
    background = background[::-1, :]

    # Pad to (60, 64)
    background_padded = np.zeros((background.shape[0] * 2, background.shape[1] * 2), dtype=np.uint16)
    background_padded[:background.shape[0],:background.shape[1]] = background
    background = background_padded

    ## --- Palette Table
    palette_table = np.zeros((8, 4, 4), dtype=np.uint8)
    palettes_np = np.array(palettes, dtype=np.uint8) # N x 4 x 4
    assert palettes_np.shape[1] == 4 and palettes_np.shape[2] == 4
    palette_table[:palettes_np.shape[0], :, :] = palettes_np

    ## --- Tile Table
	# The PPU uses 8x8 2-bit indexed-color tiles:
	# each tile is stored as two 8x8 "bit plane" images
	#   each bit-plane image is stored in rows from bottom-to-top
	#   each bit in a row corresponds to a pixel in increasing order:
	#      [ b0 b1 b2 b3 b4 b5 b6 b7 ]
	
	# For example, to read the color index at pixel (2,7):
	#  bit0_at_2_7 = (tile.bit0[7] >> 2) & 1;
	#  bit1_at_2_7 = (tile.bit1[7] >> 2) & 1;
	#  color_index_at_2_7 = (bit1_at_2_7 << 1) | bit0_at_2_7;
    tile_table = np.zeros((16 * 16, 2, 8), dtype=np.uint8)
    for i, (k, (_, palette_idx)) in enumerate(tile_info.items()):
        tile = tiles[k][1]
        assert tile.shape == (8, 8, 4)
        palette = palettes[palette_idx]

        # Axis 0 is bit0 and bit1
        tile_palette_idx_bits = np.zeros((2, 8, 8), dtype=np.uint8)
        for y in range(0, tile.shape[0]):
            for x in range(0, tile.shape[1]):
                color = tile[y, x]
                palette_color_idx = palette.index(tuple(color))
                assert 0 <= palette_color_idx and palette_color_idx < 4
                tile_palette_idx_bits[0, y, x] = palette_color_idx % 2
                tile_palette_idx_bits[1, y, x] = palette_color_idx // 2
        
        # Flip tile table since PPU does bottom-to-top
        tile_palette_idx_bits = tile_palette_idx_bits[:, ::-1, :]

        # Pack rows into uint8
        packed_tile = np.packbits(tile_palette_idx_bits, axis=2, bitorder='little')
        packed_tile = packed_tile.squeeze()
        assert packed_tile.shape == (2, 8)
        tile_table[i, :, :] = packed_tile
    
    ### WRITE EVERYTHING TO DISK ###
    OUTPUT_DIR = "output_of_asset_gen"
    os.makedirs(OUTPUT_DIR)
    background.tofile(f"{OUTPUT_DIR}/background.bin")
    palette_table.tofile(f"{OUTPUT_DIR}/palette_table.bin")
    tile_table.tofile(f"{OUTPUT_DIR}/tile_table.bin")
    os.makedirs(f"{OUTPUT_DIR}/imgs")
    for _, (i, tile) in tiles.items():
        Image.fromarray(tile).save(f"{OUTPUT_DIR}/imgs/tile_{i}.png")
    for i, palette in enumerate(palettes):
        palette_img = np.array(palette).reshape(1, 4, 4)
        Image.fromarray(palette_img).save(f"{OUTPUT_DIR}/imgs/palette_{i}.png")

    print("Wrote everything to disk, delete or rename the asset_gen_output/ folder if you want to re-run")


parser = ArgumentParser()
parser.add_argument("image")
args = parser.parse_args()

img = load_image(args.image)
find_tiles_and_palettes(img)
