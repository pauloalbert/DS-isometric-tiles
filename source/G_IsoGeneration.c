/*
 * G_IsoGeneration.c
 *
 *  Created on: Nov 22, 2023
 *      Author: nds
 */
#include "G_IsoGeneration.h"
typedef enum {
	FACE_FLOOR = 0,
	FACE_WALL_LEFT = 0b01,
	FACE_WALL_RIGHT = 0b10
} BlockFaces;

inline void _setSlice(u16* tiles, int tile, TileSlices slice, u8 color,BlockFaces face){
	tiles[tile] &= ~((SLICE_MASK << slice) | BIT(15));
	tiles[tile] ^= ((face<<3) | color) << slice | BIT(15);
}
inline void _setWhole(u16* tiles, int tile, u8 color_top, BlockFaces face_top, u8 color_middle, BlockFaces face_middle, u8 color_bottom, BlockFaces face_bottom){
	tiles[tile] = BIT(15) | ((face_bottom<<3|color_bottom)<<T_BOTTOM) | ((face_middle<<3|color_middle)<<T_MIDDLE)| ((face_top<<3|color_top)<<T_TOP) ;
}

extern u8 SCALE;
void ISO_GenerateTiles(u16* tiles, s8* world, u8 world_dim_x, u8 world_dim_y, u8 world_dim_z){
	int i,j,k;
	int last_tile;
	for(k = 0; k < (world_dim_z * SCALE)>>SCALE_BITS; k++){
		for(j = 0; j < (world_dim_y * SCALE)>>SCALE_BITS; j++){
			for(i = 0; i < (world_dim_x * SCALE)>>SCALE_BITS; i++){

				int rj = i;
				int ri = j;
				int wx = (ri<<SCALE_BITS)/SCALE;
				int wy = (rj<<SCALE_BITS)/SCALE;
				int wz = (k<<SCALE_BITS)/SCALE;
				//if air, skip
				u8 block_type = world[coords_3d(wx, wy, wz,world_dim_x,world_dim_y)];
				if(! block_type) continue;
				SetTileInWorld(tiles, i,j,k,block_type);
			}
		}
	}
}

void ISO_ReverseGenerateTiles(u16* tiles, s8* world, u8 world_dim_x, u8 world_dim_y, u8 world_dim_z){
	int i,j;
	for(j = TILES_SHAPE_HEIGHT; j >= 0 ; j --){
		for(i = 0; i < TILES_SHAPE_WIDTH - 1; i+=2){
			u16 res = ISO_convertTileToWorld(coords(i,j, TILES_SHAPE_WIDTH),0);

			u8 x = res & 0xff;
			u8 y = res >> 8;
			if(x < 0 || y < 0 || x > 3 + world_dim_x || y > 3 + world_dim_y)
				continue;
			bool top,mid,bot;
			int count = 5;
			int height = 0;
			while(!top || !mid || !bot || --count >= 0){
				int block = world[coords_3d(x,y,height,world_dim_x,world_dim_y)];
				if(block){
					_setSlice(tiles,coords(i,j, TILES_SHAPE_WIDTH),T_TOP,block,FACE_FLOOR);
				}
			}


		}
	}
}

/* Preprocess render */
Block* Preprocess_World(s8* world, u8 world_dim_x, u8 world_dim_y, u8 world_dim_z){

	Block* block_list = malloc(50*sizeof(s8));

	block_list[0].type = 1;
	int i,j,k;
	int block_index = 1;
	for(k = world_dim_z - 1; k >= 0; k--){
		for(j = world_dim_y - 1; j >= 0; j--){
			for(i = world_dim_x - 1 ; i >= 0; i--){
				//increase memory if needed
				if(block_index >= 32 * block_list[0].type){
					printf("max memory! %d %d\n", block_index, 32 * block_list[0].type);
					block_list[0].type *= 2;
					block_list = realloc(block_list, 32 * block_list[0].type);
				}

				s8 block = world[coords_3d(i,j,k,world_dim_x,world_dim_y)];
				if(block){
					block_list[block_index++] = (Block) {i,j,k,block};
				}

			}
		}
	}
	return block_list;
}

void ISO_GenerateTilesFromList(u16* tiles, Block* blocks_list){
	int index = sizeof(blocks_list)/sizeof(Block);
	while(--index >= 0){
		Block block = blocks_list[index];
		if(block.type == 0){
			continue;
		}

		SetTileInWorld(tiles,block.x,block.y,block.z,block.type);

	}
}


/* * * HELPER FUNCTIONS * * */

/*
 * Set the slices in the tile based on the location of the block, and the need for culling
 */
void SetTileInWorld(u16* tiles, int x, int y, int z,u8 block_type){
	s8 cull = 0;
	int tile = ISO_convertWorldToTile(x,y,z, &cull);
	//this block goes off screen
	if(tile == -1) return;
	//is this block half shifted down?
	bool is_full = ((x+y)%2 == 0);
	if(is_full){
		//cull tells me if the tile is drawn on the edge of the map, and only one side needs to be done
		if(cull >= 0 && tile >= 0){
			_setSlice(tiles, tile, T_MIDDLE, block_type, FACE_FLOOR);
			_setSlice(tiles, tile,T_BOTTOM, block_type,FACE_WALL_LEFT);
		}
		tile ++;
		if(cull <=0 && tile >= 0){
			_setSlice(tiles, tile, T_MIDDLE, block_type, FACE_FLOOR);
			_setSlice(tiles, tile, T_BOTTOM, block_type, FACE_WALL_RIGHT);
		}
		tile += TILES_SHAPE_WIDTH - 1;
		if(tile >= TILES_SHAPE_WIDTH*TILES_SHAPE_HEIGHT) return;
		if(cull >= 0 && tile >= 0){
			_setSlice(tiles, tile,T_TOP, block_type, FACE_WALL_LEFT);
			_setSlice(tiles, tile,T_MIDDLE, block_type, FACE_WALL_LEFT);
		}
		tile += 1;
		if(cull <= 0 && tile >= 0){
			_setSlice(tiles, tile,T_TOP, block_type, FACE_WALL_RIGHT);
			_setSlice(tiles, tile,T_MIDDLE, block_type, FACE_WALL_RIGHT);
		}
	}
	else{

		if(cull >=0 && tile >= 0) _setSlice(tiles, tile, T_BOTTOM, block_type, FACE_FLOOR);
		tile += 1;
		if(cull <= 0 && tile >= 0)_setSlice(tiles, tile, T_BOTTOM, block_type, FACE_FLOOR);
		tile += TILES_SHAPE_WIDTH - 1;
		if(tile >= TILES_SHAPE_WIDTH*TILES_SHAPE_HEIGHT) return;
		if(cull >= 0 && tile >= 0)_setWhole(tiles,tile,block_type, FACE_FLOOR, block_type, FACE_WALL_LEFT, block_type, FACE_WALL_LEFT);
		tile += 1;
		if(cull <=0 && tile >= 0)_setWhole(tiles,tile,block_type, FACE_FLOOR, block_type, FACE_WALL_RIGHT, block_type, FACE_WALL_RIGHT);
		tile += TILES_SHAPE_WIDTH - 1;
		if(tile >= TILES_SHAPE_WIDTH*TILES_SHAPE_HEIGHT) return;
		if(cull >= 0 && tile >= 0)_setSlice(tiles, tile, T_TOP, block_type, FACE_WALL_LEFT);
		tile += 1;
		if(cull <= 0 && tile >= 0)_setSlice(tiles, tile, T_TOP, block_type, FACE_WALL_RIGHT);
	}
}

extern int TILES_ORIGIN;
s16 ISO_convertWorldToTile(u8 px, u8 py, u8 pz,s8* cull_lr){
	int tile = TILES_ORIGIN;

	//get the "floor coordinates" equivalent

	s16 x = (s8)px;
	s16 y = (s8)py;


	//calculate the tile offset from the origin
	int offset_x = (y - x);
	int offset_y = (x+y)/2 - pz;
	if(loop_mod(tile,TILES_SHAPE_WIDTH) + offset_x < -1 || loop_mod(tile,TILES_SHAPE_WIDTH) + offset_x >= TILES_SHAPE_WIDTH) return -1;
	int old_tile = tile;
	tile += (offset_x + TILES_SHAPE_WIDTH * offset_y);
	if(tile < -2*TILES_SHAPE_WIDTH || tile/TILES_SHAPE_WIDTH >= TILES_SHAPE_HEIGHT + 2*TILES_SHAPE_WIDTH) return -1;
	if(cull_lr){
		*(cull_lr) = (s8)((loop_mod(old_tile,TILES_SHAPE_WIDTH) + offset_x >= TILES_SHAPE_WIDTH-1) - (loop_mod(old_tile,TILES_SHAPE_WIDTH) + offset_x < 0));
	}
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
u16 ISO_convertTileToWorld(u16 tile,s8 height){
	tile += !ISO_isTileFlipped(tile);

	int t_y = (tile / TILES_SHAPE_WIDTH) - (TILES_ORIGIN / TILES_SHAPE_WIDTH);
	int t_x = (tile % TILES_SHAPE_WIDTH) - (TILES_ORIGIN % TILES_SHAPE_WIDTH);

	return ((t_y + t_x)<<8) | (t_y - t_x + 1);
}
