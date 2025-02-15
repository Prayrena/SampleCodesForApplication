#pragma once
#include "Engine/core/Vertex_PCU.hpp"

// game world settings
constexpr float WORLD_SIZE_X = 200.f;
constexpr float WORLD_SIZE_Y = 100.f;
constexpr float COMMON_ENTITY_SCALE = 1.f;

constexpr unsigned char WORLD_COLOR_R = 31; // limit the color under 256
constexpr unsigned char WORLD_COLOR_G = 32;
constexpr unsigned char WORLD_COLOR_B = 62;

// game camera settings
constexpr float WORLD_CAMERA_ORTHO_X = 200.f;
constexpr float WORLD_CAMERA_ORTHO_Y = 100.f;
constexpr float SCREEN_CAMERA_ORTHO_X = 200.f;
constexpr float SCREEN_CAMERA_ORTHO_Y = 100.f;
constexpr float CAMERA_SHAKE_AMOUNT_PLAYERDEAD = 4.5f;
constexpr float CAMERA_SHAKE_AMOUNT_EXPLOSION  = 0.6f;
constexpr float CAMERA_SHAKE_REDUCTION = 2.7f;

// Waves setting
constexpr int ASTEROID_WAVE_1 = 6;
constexpr int ASTEROID_WAVE_2 = 9;
constexpr int ASTEROID_WAVE_3 = 12;
constexpr int ASTEROID_WAVE_4 = 15;
constexpr int ASTEROID_WAVE_5 = 18;

constexpr int WASP_WAVE_1 = 1;
constexpr int WASP_WAVE_2 = 2;
constexpr int WASP_WAVE_3 = 3;
constexpr int WASP_WAVE_4 = 4;
constexpr int WASP_WAVE_5 = 5;

constexpr int BEETLE_WAVE_1 = 1;
constexpr int BEETLE_WAVE_2 = 2;
constexpr int BEETLE_WAVE_3 = 3;
constexpr int BEETLE_WAVE_4 = 4;
constexpr int BEETLE_WAVE_5 = 5;

constexpr int BOID_WAVE_1 = 0;
constexpr int BOID_WAVE_2 = 10;
constexpr int BOID_WAVE_3 = 20;
constexpr int BOID_WAVE_4 = 30;
constexpr int BOID_WAVE_5 = 40;

// debris setting
constexpr float DEBRIS_EXPLOSION_ACCELERATION = 15.f;
constexpr unsigned char DEBRIS_COLOR_A = 127;
constexpr unsigned int MAX_DEBRIS_NUMBERS = 199; // the max of the numbers of the debris that could exist on screen
constexpr float  DEBRIS_LIFESPAN = 2.f;
 
// UI setting
constexpr float TIME_RETURN_TO_ATTRACTMODE			 = 5.0f;
constexpr float TIME_TRANSITION_GAME = .5f;
constexpr float TIME_INTRO_ZOOMOUT = 50.0f;
constexpr float UI_POSITION_PLAYER_LIVES_X			 = 15.f;
constexpr float UI_POSITION_PLAYER_LIVES_Y			 = 94.f;
constexpr float UI_POSITION_PLAYER_LIVES_SPACING	 = 6.f;

constexpr float UI_POSITION_ENERGY_BOOSTER_X		 = 7.f;
constexpr float UI_POSITION_ENERGY_BOOSTER_Y		 = 94.f;

constexpr int   UI_POSITION_ATTRACTMODE_ICON_NUM	 = 2;
constexpr float UI_POSITION_ATTRACTMODE_ICONA_X		 = 145.f;
constexpr float UI_POSITION_ATTRACTMODE_ICONA_Y		 = 45.f;
constexpr float UI_ORIENTATION_ATTRACTMODE_ICONA	 = 30.f;
constexpr float UI_POSITION_ATTRACTMODE_ICONB_X		 = 90.f;
constexpr float UI_POSITION_ATTRACTMODE_ICONB_Y		 = 69.f;
constexpr float UI_ORIENTATION_ATTRACTMODE_ICONB	 = 120.f;
constexpr float UI_ATTRACTMODE_ICONA_SCALE			= 7.5f;
constexpr float UI_ATTRACTMODE_ICONB_SCALE = 3.f;

constexpr float UI_POSITION_ATTRACTMODE_PLAYBUTTON_X = 30.f;
constexpr float UI_POSITION_ATTRACTMODE_PLAYBUTTON_Y = 50.f;
constexpr float UI_ATTRACTMODE_PLAYBUTTON_SCALE = 3.f;

constexpr int   UI_STARFIELD_FAR_NUM = 39;
constexpr int   UI_STARFIELD_NEAR_NUM = 9;
constexpr float UI_STARFIELD_FAR_SCALE = 0.21f;
constexpr float UI_STARFIELD_NEAR_SCALE = 0.5f;
constexpr float UI_STARFIELD_FAR_MOVING_SCALE = 0.09f;
constexpr float UI_STARFIELD_NEAR_MOVING_SCALE = 0.15f;

// energy settings
constexpr float UI_ENERGYBAR_POS_X = 5.f;
constexpr float UI_ENERGYBAR_POS_Y = 90.f;
constexpr float UI_ENERGYREMAIN_OFFSET = .8f;
constexpr float UI_ENERGYBAR_LENGTH = 30.f;
constexpr float UI_ENERGYBAR_HEIGHT = 2.f;
constexpr float UI_ENERGYBAR_SLOPE = 2.f;

constexpr float ENERGY_GENERATE_RATE = 0.06f;
constexpr float ENERGY_CONSUMING_SHIELD_RATE = 0.1f; // should be 0.18
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
constexpr int   ENTITY_DEBRIS_MIN_NUM = 3;
constexpr int   ENTITY_DEBRIS_MAX_NUM = 12;
constexpr float DEBRIS_MIN_SCALE = 0.1f; // shrink the size of the cosmetic radius
constexpr float DEBRIS_MAX_SCALE = 0.8f; // shrink the size of the cosmetic radius
constexpr float DEBRIS_DEFLECT_RANGE_DEGREES = 45.f;
constexpr float DEBRIS_ANGULAR_SPEED_DEGREES = 90.f;
constexpr float DEBRIS_LINEAR_SPEED_SCALE_MAX = 1.6f;
constexpr float DEBRIS_LINEAR_SPEED_SCALE_MIN = .3f;

// Beetle Settings
constexpr int	BEETLE_HEALTH = 3;
constexpr float BEETLE_SPEED = 12.f;
//constexpr float BEETLE_TURNRATE = 100.f;
//constexpr float BEETLE_ACCERATION = 30.f;
constexpr float BEETLE_PHYSICS_RADIUS = 2.f;
constexpr float BEETLE_COSMETIC_RADIUS = 2.25f;
constexpr int	MAX_BEETLES = 10;
constexpr unsigned char BEETLE_COLOR_R = 139;
constexpr unsigned char BEETLE_COLOR_G = 191;
constexpr unsigned char BEETLE_COLOR_B = 159;
constexpr unsigned char BEETLE_COLOR_A = 255;


// Wasp Settings
constexpr int	WASP_HEALTH = 1;
constexpr float WASP_MAXSPEED = 30.f;
//constexpr float WASP_TURNRATE = 100.f;
constexpr float WASP_ACCERATION = 30.f;
constexpr float WASP_PHYSICS_RADIUS = 1.75f;
constexpr float WASP_COSMETIC_RADIUS = 2.f;
constexpr int	MAX_WASPS = 10;
constexpr unsigned char WASP_COLOR_R = 242;
constexpr unsigned char WASP_COLOR_G = 220;
constexpr unsigned char WASP_COLOR_B = 93;
constexpr unsigned char WASP_COLOR_A = 255;

// Boid Settings
constexpr int	MAX_BOIDS = 90;

constexpr float BOID_PHYSICS_RADIUS = 1.75f;
constexpr float BOID_COSMETIC_RADIUS = 2.f;

constexpr float BOID_MAXSPEED = 30.f;
constexpr float BOID_ACCERATION = 45.f;
constexpr float BOID_TRUST_DURATION = 3.f;
constexpr float BOID_LAUNCHING_INTERVAL = 1.f;

constexpr float BOID_TOWARDSCENTER_RANGE = 6.f;
constexpr float BOID_SAMEDIRECTION_RANGE = 12.f;
constexpr float BOID_SOCIAL_DISTANCE = 3.f;
constexpr float BOID_COHESION_MULTIPLIER   = 2.f;
constexpr float BOID_SEPERATION_MULTIPLIER = 1000.f;
constexpr float BOID_ALIGNMENT_MULTIPLIER  = 20.f;
constexpr float BOID_PLAYERATTRACTION_MULTIPLER = 4.5f;
constexpr float BOID_AWAY_MULTIPLIER = 10.f;



// PlayerShip Settings
constexpr int	PLAYERSHIP_HEALTH = 1;
constexpr float PLAYERSHIP_TURNRATE = 100.f;
constexpr float PLAYERSHIP_ACCERATION_LOW = 10.f;
constexpr float PLAYERSHIP_ACCERATION_HIGH = 30.f;
constexpr float PLAYERSHIP_PHYSICS_RADIUS = 1.75f;
constexpr float PLAYERSHIP_SHIELD_RADIUS = 2.5f;
constexpr float PLAYERSHIP_COSMETIC_RADIUS = 2.25f;
constexpr float PLAYERSHIP_FLAME_LENGTH_LONG = 9.0f;
constexpr float PLAYERSHIP_FLAME_LENGTH_SHORT = 3.0f;

constexpr float PLAYERSHIP_FIRE_RATE_LONG = 0.4f;
constexpr float PLAYERSHIP_FIRE_RATE_SHORT = 0.1f;

constexpr int   PLAYERSHIP_DEBRIS_MIN_NUM = 5;
constexpr int   PLAYERSHIP_DEBRIS_MAX_NUM = 30;
constexpr unsigned char PLAYERSHIP_COLOR_R = 102;
constexpr unsigned char PLAYERSHIP_COLOR_G = 153;
constexpr unsigned char PLAYERSHIP_COLOR_B = 204;
constexpr unsigned char PLAYERSHIP_COLOR_A = 255;
constexpr unsigned char PLAYERSHIP_FLAME_COLOR_R = 255;
constexpr unsigned char PLAYERSHIP_FLAME_COLOR_G = 255;
constexpr unsigned char PLAYERSHIP_FLAME_COLOR_B = 0;;
constexpr unsigned char PLAYERSHIP_TRAIL_COLOR_A = 255;


// Bullets Settings
constexpr int	MAX_BULLETS = 99;
constexpr int	BULLET_HEALTH = 1;
constexpr float BULLET_SPEED = 50.f;
constexpr float BULLET_PHYSICS_RADIUS = .5f;
constexpr float BULLET_COSMETIC_RADIUS = 2.f;
constexpr unsigned char BULLET_DEBRIS_COLOR_R = 255;
constexpr unsigned char BULLET_DEBRIS_COLOR_G = 255;
constexpr unsigned char BULLET_DEBRIS_COLOR_B = 0;
constexpr unsigned char BULLET_DEBRIS_COLOR_A = 255;
constexpr int   BULLET_DEBRIS_MIN_NUM = 1;
constexpr int   BULLET_DEBRIS_MAX_NUM = 3;


// Asteroid Settings
constexpr int	MAX_ASTEROIDS = 24;
constexpr int	ASTEROID_HEALTH = 3;
constexpr float ASTEROID_SPEED = 10.f;
constexpr float ASTEROID_ANGULAR_SPEED_RANGE = 200.f;
constexpr int	NUM_STARTING_ASTEROIDS = 6;
constexpr float ASTEROID_PHYSICS_RADIUS = 1.6f;
constexpr float ASTEROID_COSMETIC_RADIUS = 2.f;

constexpr unsigned char ASTEROID_COLOR_R = 100; 
constexpr unsigned char ASTEROID_COLOR_G = 100;
constexpr unsigned char ASTEROID_COLOR_B = 100;
constexpr unsigned char ASTEROID_COLOR_A = 255;

// Debug drawing settings
constexpr int DEBUG_NUM_SIDES = 12;
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
constexpr int SHIELD_RING_NUM_SIDES = 24;
constexpr int SHIELD_RING_NUM_TRIS = SHIELD_RING_NUM_SIDES * 2;
constexpr int SHIELD_RING_NUM_VERTS = SHIELD_RING_NUM_TRIS * 3;
constexpr float SHIELD_DEGREES_PERSIDE = 360.f / static_cast<float>(SHIELD_RING_NUM_SIDES);


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