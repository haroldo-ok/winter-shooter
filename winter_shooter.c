#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "shot.h"
#include "shots.h"
#include "map.h"
#include "score.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)

#define ENEMY_MAX (3)
#define FOR_EACH_ENEMY(enm) enm = enemies; for (int i = ENEMY_MAX; i; i--, enm++)
#define ENEMY_TYPE_COUNT (3)
#define ENEMY_PATH_COUNT (4)
	
#define POWERUP_BASE_TILE (100)
#define POWERUP_HAT_TILE (POWERUP_BASE_TILE)
#define POWERUP_PRESENT_1_TILE (POWERUP_BASE_TILE + 4)
#define POWERUP_PRESENT_2_TILE (POWERUP_BASE_TILE + 8)
#define POWERUP_PRESENT_3_TILE (POWERUP_BASE_TILE + 12)
#define POWERUP_NONE_TILE (POWERUP_BASE_TILE + 16)
#define POWERUP_HAT (1)
#define POWERUP_PRESENT_1 (2)
#define POWERUP_PRESENT_2 (3)
#define POWERUP_PRESENT_3 (5)

actor player;
actor enemies[ENEMY_MAX];
actor powerup;
actor timer_label;

score_display timer;
score_display score;

typedef struct enemy_type {
	char base_tile, frame_count;
} enemy_type;

struct ply_ctl {
	char shot_delay;
	char shot_type;
	char pressed_shot_selection;
	
	char powerup1, powerup2;
	char powerup1_active, powerup2_active;

	char death_delay;
} ply_ctl;

struct enemy_spawner {
	char type;
	char x;
	char flags;
	char delay;
	char next;
	enemy_type *enm_type;
	path_step *path;
	char all_dead;
} enemy_spawner;

char timer_delay;


const enemy_type enemy_types[ENEMY_TYPE_COUNT] = {
	{66, 2},
	{130, 4},
	{154, 3}
};

path_step *enemy_paths[ENEMY_PATH_COUNT] = {
	(path_step *) path1_path,
	(path_step *) path2_path,
	(path_step *) path3_path,
	(path_step *) path4_path
};

void update_score(actor *enm, actor *sht);

void load_standard_palettes() {
	SMS_loadBGPalette(tileset_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

void handle_player_input() {
	static unsigned char joy;	
	joy = SMS_getKeysStatus();

	if (joy & PORT_A_KEY_LEFT) {
		if (player.x > PLAYER_LEFT) player.x -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_RIGHT) {
		if (player.x < PLAYER_RIGHT) player.x += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) player.y -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) player.y += PLAYER_SPEED;
	}
	
	if (joy & PORT_A_KEY_2) {
		if (!ply_ctl.shot_delay) {
			if (fire_player_shot(&player, ply_ctl.shot_type)) {
				ply_ctl.shot_delay = player_shot_infos[ply_ctl.shot_type].firing_delay;
			}
		}
	}
	
	if (ply_ctl.shot_delay) ply_ctl.shot_delay--;
	if (ply_ctl.death_delay) ply_ctl.death_delay--;

}

void draw_player() {
	if (!(ply_ctl.death_delay & 0x08)) draw_actor(&player);
}

char is_colliding_against_player(actor *_act) {
	static actor *act;
	static int act_x, act_y;
	
	act = _act;
	act_x = act->x;
	act_y = act->y;
	
	if (player.x > act_x - 12 && player.x < act_x + 12 &&
		player.y > act_y - 12 && player.y < act_y + 12) {
		return 1;
	}
	
	return 0;
}

void init_enemies() {
	static actor *enm;

	enemy_spawner.x = 0;	
	enemy_spawner.delay = 0;
	enemy_spawner.next = 0;
	
	FOR_EACH_ENEMY(enm) {
		enm->active = 0;
	}
}

void handle_enemies() {
	static actor *enm, *sht;	
	
	if (enemy_spawner.delay) {
		enemy_spawner.delay--;
	} else if (enemy_spawner.next != ENEMY_MAX) {
		if (!enemy_spawner.x) {
			enemy_spawner.type = rand() & 1;
			enemy_spawner.x = 8 + rand() % 124;
			enemy_spawner.flags = 0;
			enemy_spawner.enm_type = enemy_types + rand() % ENEMY_TYPE_COUNT;
			enemy_spawner.path = enemy_paths[rand() % ENEMY_PATH_COUNT];
			if (rand() & 1) {
				enemy_spawner.x += 124;
				enemy_spawner.flags |= PATH_FLIP_X;
			}
		}
		
		enm = enemies + enemy_spawner.next;
		
		init_actor(enm, enemy_spawner.x, 0, 2, 1, 
			enemy_spawner.enm_type->base_tile, 
			enemy_spawner.enm_type->frame_count);
			
		enm->path_flags = enemy_spawner.flags;
		enm->path = enemy_spawner.path;
		enm->state = enemy_spawner.type;

		enemy_spawner.delay = 10;
		enemy_spawner.next++;
	}
	
	enemy_spawner.all_dead = 1;
	FOR_EACH_ENEMY(enm) {
		move_actor(enm);
		
		if (enm->x < -32 || enm->x > 287 || enm->y < -16 || enm->y > 192) {
			enm->active = 0;
		}

		if (enm->active) {
			sht = check_collision_against_shots(enm);
			if (sht) {
				update_score(enm, sht);
				sht->active = 0;
				enm->active = 0;
			}
			
			if (!ply_ctl.death_delay && is_colliding_against_player(enm)) {
				if (ply_ctl.shot_type) ply_ctl.shot_type--;
				enm->active = 0;
				ply_ctl.death_delay = 60;
			}
		}
		
		if (enm->active) enemy_spawner.all_dead = 0;
	}	
	
	if (enemy_spawner.all_dead) {
		enemy_spawner.x = 0;
		enemy_spawner.next = 0;
	}
}

void draw_enemies() {
	static actor *enm;
	
	FOR_EACH_ENEMY(enm) {
		draw_actor(enm);
	}
}

void init_powerups() {
	init_actor(&powerup, 0, 0, 2, 1, POWERUP_HAT_TILE, 1);
	powerup.active = 0;
}

char powerup_base_tile(char type) {
	switch (type) {
	case POWERUP_HAT: return POWERUP_HAT_TILE;
	case POWERUP_PRESENT_1: return POWERUP_PRESENT_1_TILE;
	case POWERUP_PRESENT_2: return POWERUP_PRESENT_2_TILE;
	case POWERUP_PRESENT_3: return POWERUP_PRESENT_3_TILE;
	}
	
	return POWERUP_NONE_TILE;
}

void handle_powerups() {
	powerup.y++;
	if (powerup.y > SCREEN_H) powerup.active = 0;

	if (powerup.active) {
		// Check collision with player
		if (powerup.x > player.x - 16 && powerup.x < player.x + 24 &&
			powerup.y > player.y - 16 && powerup.y < player.y + 16) {
			update_score(&powerup, 0);
			if (powerup.state == 1 && ply_ctl.shot_type < PLAYER_SHOT_TYPE_COUNT - 1) ply_ctl.shot_type++;
			powerup.active = 0;			
		}
	} else {
		powerup.x = 8 + rand() % (256 - 24);
		powerup.y = -16;
		powerup.active = 1;
		powerup.state = 1 + rand() % 0x03;
		powerup.base_tile = powerup_base_tile(powerup.state);
	}	
}

void draw_powerups() {
	draw_actor(&powerup);
}

void update_score(actor *enm, actor *sht) {
	increment_score_display(&score, enm == &powerup ? 5 : 1);
}

void init_score() {
	init_actor(&timer_label, 16, 8, 1, 1, 178, 1);
	init_score_display(&timer, 24, 8, 236);
	update_score_display(&timer, 120);
	timer_delay = 60;
	
	init_score_display(&score, 16, 24, 236);
}

void handle_score() {
	if (timer_delay) {
		timer_delay--;
	} else {
		if (timer.value) increment_score_display(&timer, -1);
		timer_delay = 60;
	}
}

void draw_score() {
	draw_actor(&timer_label);
	draw_score_display(&timer);

	draw_score_display(&score);
}

void main() {	
	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	SMS_loadPSGaidencompressedTiles(tileset_tiles_psgcompr, 256);
	load_standard_palettes();
	
	init_map(level1_bin);
	draw_map_screen();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 2, 1, 2, 1);
	player.animation_delay = 20;
	ply_ctl.shot_delay = 0;
	ply_ctl.shot_type = 0;
	ply_ctl.powerup1 = 1;
	ply_ctl.powerup2 = 0;
	ply_ctl.powerup1_active = 1;
	ply_ctl.powerup2_active = 0;
	ply_ctl.death_delay = 0;

	init_enemies();
	init_player_shots();
	init_powerups();
	init_score();
	
	while (1) {	
		handle_player_input();
		handle_enemies();
		handle_powerups();
		handle_player_shots();
		handle_score();
	
		SMS_initSprites();

		draw_player();
		draw_enemies();
		draw_powerups();
		draw_player_shots();		
		draw_score();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
		
		// Scroll two lines per frame
		draw_map();		
		draw_map();		
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,3, 2021,12,24, "Haroldo-OK\\2021", "Winter Shooter",
  "A Christmas-Themed SHMUP.\n"
  "Originally made for the Jame Gam Christmas Edition - https://itch.io/jam/jame-gam-christmas-edition\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
