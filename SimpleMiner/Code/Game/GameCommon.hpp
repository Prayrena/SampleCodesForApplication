#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/Vec4.hpp"

// game world settings
constexpr float HUD_SIZE_X = 200.f; // world camera size
constexpr float HUD_SIZE_Y = 100.f;// world camera size
constexpr float COMMON_ENTITY_SCALE = 1.f;

constexpr unsigned char WORLD_COLOR_R = 31; // limit the color under 256
constexpr unsigned char WORLD_COLOR_G = 32;
constexpr unsigned char WORLD_COLOR_B = 62;

//----------------------------------------------------------------------------------------------------------------------------------------------------
// chuck 
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 7;

constexpr int CHUNK_SIZE_X = (1 << CHUNK_BITS_X); // width
constexpr int CHUNK_SIZE_Y = (1 << CHUNK_BITS_Y); // depth
constexpr int CHUNK_SIZE_Z = (1 << CHUNK_BITS_Z); // height

constexpr int CHUNK_BLOCKS_PER_LAYER = (CHUNK_SIZE_X * CHUNK_SIZE_Y);
constexpr int CHUNK_BLOCKS_TOTAL = (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z);

constexpr int CHUNK_MASK_X = (CHUNK_SIZE_X - 1);
// constexpr int CHUNCK_MASK_X = 0b0000000'0000'1111; // for clear bits calculation
constexpr int CHUNK_MASK_Y = (CHUNK_SIZE_Y - 1) << CHUNK_BITS_X;
// constexpr int CHUNCK_MASK_Y = 0b0000000'1111'0000; // for clear bits calculation
constexpr int CHUNK_MASK_Z = ((CHUNK_SIZE_Z - 1) << (CHUNK_BITS_X + CHUNK_BITS_Y));
// constexpr int CHUNCK_MASK_Y = 0b1111111'0000'0000; // for clear bits calculation

constexpr int CHUNK_BITSHIFT_X = 0;
constexpr int CHUNK_BITSHIFT_Y = CHUNK_BITS_X;
constexpr int CHUNK_BITSHIFT_Z = CHUNK_BITS_X + CHUNK_BITS_Y;

constexpr int WATER_LEVEL = int(float(CHUNK_SIZE_Z) * 0.5f);

constexpr int MAX_INT = 999999999;
const IntVec2 BAD_CHUNK_COORDS(MAX_INT, MAX_INT);
const IntVec3 BAD_BLOCK_COORDS(MAX_INT, MAX_INT, MAX_INT);

// The maximum number of active chunks allowed(maxChunks) is computed as follows :
constexpr float CHUNK_ACTIVATE_RANGE = 300.f;
constexpr float CHUNK_DACTIVATE_RANGE = CHUNK_ACTIVATE_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;
constexpr int MAX_CHUNK_RADIUS_X = 1 + int(CHUNK_ACTIVATE_RANGE) / CHUNK_SIZE_X;
constexpr int MAX_CHUNK_RADIUS_Y = 1 + int(CHUNK_ACTIVATE_RANGE) / CHUNK_SIZE_Y;
constexpr int MAX_CHUNKS = (2 * MAX_CHUNK_RADIUS_X)* (2 * MAX_CHUNK_RADIUS_Y); // neighborhood
// constexpr int MAX_CHUNKS = 16; // neighborhood

//----------------------------------------------------------------------------------------------------------------------------------------------------
// block lighting
constexpr int LIGHT_BITS_OUTDOOR = 4;
constexpr int LIGHT_BITS_INDOOR = 4;
constexpr int LIGHT_MASK_INDOOR = 0b00001111; // (1 << LIGHT_BITS_OUTDOOR) - 1;
constexpr int LIGHT_MASK_OUTDOOR = 0b11110000; // LIGHT_MASK_INDOOR << LIGHT_BITS_INDOOR;

constexpr int BLOCK_BIT_IS_SKY = 1; 
constexpr int BLOCK_BIT_MASK_IS_SKY = 1 << (BLOCK_BIT_IS_SKY - 1);// 0b0000'0001;
constexpr int BLOCK_BIT_IS_LIGHT_DIRTY = 2; 
constexpr int BLOCK_BIT_MASK_IS_LIGHT_DIRTY = 1 << (BLOCK_BIT_IS_LIGHT_DIRTY - 1);// 0b0000'0010;
constexpr int BLOCK_BIT_IS_FULL_OPAQUE = 3; 
constexpr int BLOCK_BIT_MASK_IS_FULL_OPAQUE = 1 << (BLOCK_BIT_IS_FULL_OPAQUE - 1);// 0b0000'0010;
constexpr int BLOCK_BIT_IS_EMISSIVE = 4; 
constexpr int BLOCK_BIT_MASK_IS_EMISSIVE = 1 << (BLOCK_BIT_IS_EMISSIVE - 1);// 0b0000'0010;
constexpr int BLOCK_BIT_IS_VISIBLE = 5; 
constexpr int BLOCK_BIT_MASK_IS_VISIBLE = 1 << (BLOCK_BIT_IS_VISIBLE - 1);// 0b0000'0010;
constexpr int BLOCK_BIT_IS_SOLID = 6; 
constexpr int BLOCK_BIT_MASK_IS_SOLID = 1 << (BLOCK_BIT_IS_SOLID - 1);// 0b0010'0000;

constexpr int MAX_LIGHT_VALUE = 15;

//----------------------------------------------------------------------------------------------------------------------------------------------------
// time settings
constexpr float HOUR_FRACTION_DAY = 1.f / 24.f;
constexpr int MAX_QUEUEDJOBS_CHUNKGENERATION = 4;

//----------------------------------------------------------------------------------------------------------------------------------------------------
// biome
constexpr int WATER_LEVEL_Z = CHUNK_SIZE_Z / 2; // consider the river and ocean share the same water level
constexpr int RIVER_DEPTH_MAX = 5;
// constexpr int OCEAN_LEVEL_Z = WATER_LEVEL_Z + RIVER_DEPTH_MAX;
constexpr int MOUTAIN_ELEVATION_MAX = 60;
constexpr int RIVER_BED_HEIGHT_Z = WATER_LEVEL_Z - RIVER_DEPTH_MAX;
constexpr int TERRAIN_HEIGHT_ABOVE_WATER_MAX = CHUNK_SIZE_Z - WATER_LEVEL_Z; // the part where moutain stands above the water level
constexpr int OCEAN_LOWING_MAX = 20;
constexpr int TREE_COVER_SIZE_MAX = 3;
constexpr int NEIGHBORHOOD_SIZE_X = CHUNK_SIZE_X + (2 * TREE_COVER_SIZE_MAX);
constexpr int NEIGHBORHOOD_SIZE_Y = CHUNK_SIZE_Y + (2 * TREE_COVER_SIZE_MAX);
constexpr int NEIGHBOOR_ARRAY_SIZE = NEIGHBORHOOD_SIZE_Y * NEIGHBORHOOD_SIZE_X;

constexpr float CAVE_PITCH_MAX = 85.f;
constexpr float CAVE_YAW_MAX = 90.f;

//----------------------------------------------------------------------------------------------------------------------------------------------------
constexpr float CAVE_WORM_MAX_RADIUS = 4.f;
constexpr float CAVE_WORM_MIN_RADIUS = 1.f;
constexpr int	CAVE_COVER_CHUNK_TIMES = 3;
constexpr float CAVE_MAX_RADIUS = float(CHUNK_SIZE_X) * float(CAVE_COVER_CHUNK_TIMES);
constexpr float CAVE_WORM_LENGTH = CAVE_MAX_RADIUS * 2.f;

//----------------------------------------------------------------------------------------------------------------------------------------------------
// game camera settings
constexpr float WORLD_CAMERA_ORTHO_X = 200.f;
constexpr float WORLD_CAMERA_ORTHO_Y = 100.f;
constexpr float SCREEN_CAMERA_ORTHO_X = 1600.f;
constexpr float SCREEN_CAMERA_ORTHO_Y = 800.f;
constexpr float CAMERA_SHAKE_AMOUNT_PLAYERDEAD = 4.5f;
constexpr float CAMERA_SHAKE_AMOUNT_EXPLOSION  = 0.6f;
constexpr float CAMERA_SHAKE_REDUCTION = 2.7f;

// UI setting
constexpr float TIME_RETURN_TO_ATTRACTMODE			 = 5.0f;
constexpr float TIME_TRANSITION_GAME = 3.0f;
constexpr float TIME_INTRO_ZOOMOUT = 50.0f;
constexpr float UI_POSITION_PLAYER_LIVES_X			 = 15.f;
constexpr float UI_POSITION_PLAYER_LIVES_Y			 = 94.f;
constexpr float UI_POSITION_PLAYER_LIVES_SPACING	 = 6.f;

constexpr float UI_POSITION_ENERGY_BOOSTER_X = 7.f;
constexpr float UI_POSITION_ENERGY_BOOSTER_Y = 94.f;

// Attract mode settings
constexpr int   UI_POSITION_ATTRACTMODE_ICON_NUM	 = 2;
constexpr float UI_POSITION_ATTRACTMODE_ICONA_X		 = 145.f;
constexpr float UI_POSITION_ATTRACTMODE_ICONA_Y		 = 45.f;
constexpr float UI_ORIENTATION_ATTRACTMODE_ICONB = 120.f;
constexpr float UI_ATTRACTMODE_ICONA_SCALE   = 7.5f;
constexpr float UI_ATTRACTMODE_ICONB_SCALE = 3.f;

constexpr float UI_POSITION_ATTRACTMODE_PLAYBUTTON_X = 100.f;
constexpr float UI_POSITION_ATTRACTMODE_PLAYBUTTON_Y = 50.f;
constexpr float UI_ATTRACTMODE_PLAYBUTTON_SCALE = 3.f;

constexpr unsigned char ATTRACTMODE_PLAYBUTTON_COLOR_R = 24;
constexpr unsigned char ATTRACTMODE_PLAYBUTTON_COLOR_G = 98;
constexpr unsigned char ATTRACTMODE_PLAYBUTTON_COLOR_B = 101;
constexpr unsigned char ATTRACTMODE_PLAYBUTTON_COLOR_A = 150;

// Debris setting
constexpr float DEBRIS_LIFESPAN = 5.f;

// energy settings
constexpr float UI_ENERGYBAR_POS_X = 5.f;
constexpr float UI_ENERGYBAR_POS_Y = 90.f;
constexpr float UI_ENERGYREMAIN_OFFSET = .8f;
constexpr float UI_ENERGYBAR_LENGTH = 30.f;
constexpr float UI_ENERGYBAR_HEIGHT = 2.f;
constexpr float UI_ENERGYBAR_SLOPE = 2.f;

constexpr float ENERGY_GENERATE_RATE = 0.06f;
constexpr float ENERGY_CONSUMING_SHIELD_RATE = 0.01f; // should be 0.18
constexpr float ENERGY_CONSUMING_FIRERATE_RATE = 0.15f;
constexpr float ENERGY_CONSUMING_SPEEDBURST_RATE = 0.09f;

constexpr unsigned char ENERGYBAR_COLOR_R = 24;
constexpr unsigned char ENERGYBAR_COLOR_G = 98;
constexpr unsigned char ENERGYBAR_COLOR_B = 101;
constexpr unsigned char ENERGYBAR_COLOR_A = 150;
constexpr unsigned char ENERGY_REMAIN_COLOR_R = 11;
constexpr unsigned char ENERGY_REMAIN_COLOR_G = 208;
constexpr unsigned char ENERGY_REMAIN_COLOR_B = 224;
constexpr unsigned char ENERGY_REMAIN_COLOR_A = 255;
constexpr unsigned char ENERGY_REMAIN_WARNING_COLOR_R = 211;
constexpr unsigned char ENERGY_REMAIN_WARNING_COLOR_G = 11;
constexpr unsigned char ENERGY_REMAIN_WARNING_COLOR_B = 93;
constexpr unsigned char ENERGY_SELECTION_RING_COLOR_R = 24;
constexpr unsigned char ENERGY_SELECTION_RING_COLOR_G = 98;
constexpr unsigned char ENERGY_SELECTION_RING_COLOR_B = 101;


// Entity universal setting
constexpr int	PLAYER_LIVES_NUM = 3;


// Debug drawing settings
constexpr int DEBUG_NUM_SIDES = 48;
constexpr int DEBUG_NUM_TRIS = DEBUG_NUM_SIDES * 2;
constexpr int DEBUG_NUM_VERTS = DEBUG_NUM_TRIS * 3;
constexpr float DEBUG_RING_DEGREES_PERSIDE = 360.f / static_cast<float>(DEBUG_NUM_SIDES);
constexpr float DEBUGRING_THICKNESS = 0.2f;

// Debug drawing settings
constexpr int DISK_NUM_SIDES = 24;
constexpr int DISK_NUM_TRIS	 = DISK_NUM_SIDES * 1;
constexpr int DISK_NUM_VERTS = DISK_NUM_TRIS * 3;
constexpr float DISK_DEGREES_PERSIDE = 360.f / static_cast<float>(DISK_NUM_SIDES);

// Debug drawing settings
constexpr int DEBUGRING_PHYSICS_COLOR_R = 0;
constexpr int DEBUGRING_PHYSICS_COLOR_G = 255;
constexpr int DEBUGRING_PHYSICS_COLOR_B = 255;
constexpr int DEBUGRING_PHYSICS_COLOR_A = 255;

constexpr int DEBUGRING_COSMETIC_COLOR_R = 255;
constexpr int DEBUGRING_COSMETIC_COLOR_G = 0;
constexpr int DEBUGRING_COSMETIC_COLOR_B = 255;
constexpr int DEBUGRING_COSMETIC_COLOR_A = 255;

constexpr float DEBUGLINE_THICKNESS = 0.2f;

constexpr unsigned char DEBUGLINE_VELOCITY_COLOR_R = 255;
constexpr unsigned char DEBUGLINE_VELOCITY_COLOR_G = 255;
constexpr unsigned char DEBUGLINE_VELOCITY_COLOR_B = 0;
constexpr unsigned char DEBUGLINE_VELOCITY_COLOR_A = 255;

constexpr unsigned char DEBUGLINE_AXISY_COLOR_R = 255;
constexpr unsigned char DEBUGLINE_AXISY_COLOR_G = 0;
constexpr unsigned char DEBUGLINE_AXISY_COLOR_B = 0;
constexpr unsigned char DEBUGLINE_AXISY_COLOR_A = 255;

constexpr unsigned char DEBUGLINE_AXISX_COLOR_R = 0;
constexpr unsigned char DEBUGLINE_AXISX_COLOR_G = 255;
constexpr unsigned char DEBUGLINE_AXISX_COLOR_B = 0;
constexpr unsigned char DEBUGLINE_AXISX_COLOR_A = 255;

constexpr unsigned char DEBUGLINE_TARGET_COLOR_R = 50;
constexpr unsigned char DEBUGLINE_TARGET_COLOR_G = 50;
constexpr unsigned char DEBUGLINE_TARGET_COLOR_B = 50;
constexpr unsigned char DEBUGLINE_TARGET_COLOR_A = 255;

void DebugDrawLine(Vec2 StartPos, Vec2 EndPos, float thickness, Rgba8 const& color);
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DrawDisk(Vec2 const& center, float radius, Rgba8 const& colorA, Rgba8 const& colorB);
Rgba8 GetColorFromVec4(Vec4 const& float4);
