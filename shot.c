#include "shot.h"

const path_step single_path[] = {
	{0, -4},
	{-128, -128}
};

const path_step spread_path0[] = {
	{-2, -3},
	{-128, -128}
};

const path_step spread_path1[] = {
	{0, -4},
	{-128, -128}
};

const path_step spread_path2[] = {
	{2, -3},
	{-128, -128}
};

const path_step spread_path3[] = {
	{-1, -4},
	{0, -4},
	{-128, -128}
};

const path_step spread_path4[] = {
	{1, -4},
	{0, -4},
	{-128, -128}
};

const path single_paths[] = {
	{4, -8, 0, single_path}
};

const path spread_paths[] = {
	{2, -6, 0, spread_path0},
	{4, -8, 0, spread_path1},
	{6, -6, 0, spread_path2}
};

const path surround_paths[] = {
	{2, -6, 0, spread_path0},
	{4, -8, 0, spread_path1},
	{6, -6, 0, spread_path2},
	{4, 12, PATH_FLIP_Y, spread_path0},
	{6, 12, PATH_FLIP_Y, spread_path2}
};

const shot_info player_shot_infos[PLAYER_SHOT_TYPE_COUNT] = {
	{26, 3, 45, 12, 1, single_paths}, // 0
	{26, 2, 45, 12, 3, spread_paths}, // 1
	{26, 2, 45, 12, 5, surround_paths}, // 2
};
