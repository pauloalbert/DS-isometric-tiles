/*
 * G_Isometric.c
 *
 *  In charge of tile graphics
 */
#include "G_Isometric.h"

inline void _setSlice(u16* tiles, int tile, u8 color, TileSlices slice){
	printf("set %d,%d,%d\n",tile,color,slice);
	tiles[tile] &= !(0x7 << slice);
	tiles[tile] ^= color << slice;
}
void GenerateTiles(u16* tiles, s8* world, u8 world_dim_x, u8 world_dim_y, u8 world_dim_z){
	int i,j,k;
	printf("%p\n",world);
	for(k = 0; k < world_dim_z; k++){
		for(j = 0; j < world_dim_y; j++){
			for(i = 0; i < world_dim_x; i++){

				//if air, skip

				if(! world[coords_3d(i,j,k,world_dim_x,world_dim_y)]) continue;
				printf("%d,%d,%d, %d:%d\n",i,j,k, coords_3d(i,j,k,world_dim_x,world_dim_y),world[coords_3d(i,j,k,world_dim_x,world_dim_y)]);
				u8 floor_color = 1;
				u8 wall_color = 2;
				//get topleft tile
				int tile = convertWorldToTile(i,j,k);

				//is this block half shifted down?
				bool is_full = ((i+j)%2 == 0);
				if(is_full){
					//TODO edge case of wrapping, edge case of original tile wrapping
					_setSlice(tiles, tile, floor_color, T_MIDDLE);
					_setSlice(tiles, tile++, wall_color,T_BOTTOM);
					_setSlice(tiles, tile, floor_color, T_MIDDLE);
					_setSlice(tiles, tile, wall_color,T_BOTTOM);

					tile += TILES_SHAPE_WIDTH - 1;

					_setSlice(tiles, tile, wall_color, T_TOP);
					_setSlice(tiles, tile, wall_color, T_MIDDLE);
					tile += 1;
					_setSlice(tiles, tile, wall_color, T_TOP);
					_setSlice(tiles, tile, wall_color, T_MIDDLE);
				}
				else{
					//TODO edge case of wrapping, edge case of original tile wrapping
					_setSlice(tiles, tile++, floor_color, T_BOTTOM);
					_setSlice(tiles, tile, floor_color,T_BOTTOM);

					tile += TILES_SHAPE_WIDTH - 1;
					_setSlice(tiles, tile, floor_color, T_TOP);
					_setSlice(tiles, tile, wall_color, T_MIDDLE);
					_setSlice(tiles, tile, wall_color, T_BOTTOM);
					tile += 1;
					_setSlice(tiles, tile, floor_color, T_TOP);
					_setSlice(tiles, tile, wall_color, T_MIDDLE);
					_setSlice(tiles, tile, wall_color, T_BOTTOM);
					tile += TILES_SHAPE_WIDTH - 1;
					_setSlice(tiles, tile, wall_color, T_TOP);
					tile += 1;
					_setSlice(tiles, tile, wall_color, T_TOP);
				}
			}
		}
	}
}

u16 tiles[TILES_SHAPE_WIDTH * TILES_SHAPE_HEIGHT];
void RenderTiles(s8* world){
	GenerateTiles(tiles, world, WORLD_DIM_X, WORLD_DIM_Y, WORLD_DIM_Z);
	int i,j;
	for(j = 0; j < TILES_SHAPE_WIDTH*TILES_SHAPE_HEIGHT/2; j++){
			int tile = tiles[j];
			u8 bottom = tile & 0xf;
			u8 middle = (tile & 0xf0) >> 4;
			u8 top = (tile & 0xf00) >> 8;
			if(tile){
				if(bottom == middle && middle == top){
					//solid color
					BG_MAP_RAM(1)[j] &= 0 | (middle << 12) | (j%2 == 0 ? BIT(10) : 0);
				}
				else{
					BG_MAP_RAM(1)[j] = 0 | (middle << 12) | ((bottom) << 14) | (j%2 == 0 ? BIT(10) : 0);
				}
			}
	}
}

s16 convertWorldToTile(u8 px, u8 py, u8 pz){
	int tile = TILES_ORIGIN;

	//get the "floor coordinates" equivalent
	s16 x = px - 2*pz;
	s16 y = py - 2*pz;

	//calculate the tile offset from the origin
	int offset_x = (y - x);
	int offset_y = (x+y);
	tile += (offset_x + TILES_SHAPE_WIDTH * offset_y);
	return tile;
}

/*
 * Gives the bottom left solution.
 * return = (px) | (py << 8);  //pz = 0
 * or return = U16_MAX in case of failure
 *
 * returns the lowest tile affecting this one (the bottom triangle)
 *
 */
u16 convertTileToWorld(u16 tile);
