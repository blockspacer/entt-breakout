#include <SDL.h>
#include <SDL_image.h>
#include "entt/entt.hpp"
#include "components.h"
#include <random>

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 800;

constexpr Vec2i UP{ 0,-1 };
constexpr Vec2i RIGHT{ 1,0 };

constexpr Vec2i coordinate_center{ WINDOW_WIDTH / 2,WINDOW_HEIGHT - 100 };

SDL_Renderer *gRenderer;
SDL_Window *gWindow;
SDL_Texture* gTexture;

SDL_RenderSprite paddle;

Vec2f random_vector() {
	

	// https://en.cppreference.com/w/cpp/header/random
	// seed with a constant (constant seed sequence) to get repeatable results
	static std::mt19937 rng(std::random_device{}());

	std::uniform_real_distribution<float> distribution(-1.0, 1.0);


	return Vec2f{distribution(rng), distribution(rng)}.normalized();

}

//converts game space (centered) to screen space
constexpr Vec2i game_space_to_screen_space(Vec2f location) {	
	auto up = (UP * location.y);
	auto right = (RIGHT * location.x);
	return coordinate_center + up + right;
}

void draw_sprite(SDL_RenderSprite& sprite, SDL_Renderer*render_target) {
	if (render_target && sprite.texture)
	{
		SDL_Rect renderQuad = sprite.to_rect();

		SDL_RenderCopy(render_target, sprite.texture, &sprite.texture_rect, &renderQuad);
	}
}

void draw_sprites_sdl(entt::registry &registry, SDL_Renderer*render_target)
{
	auto spriteview = registry.view<SDL_RenderSprite>();

	for (auto et : spriteview)
	{
		SDL_RenderSprite &sprite = spriteview.get(et);
		draw_sprite(sprite, render_target);
	}
}
void transform_sprites(entt::registry &registry)
{
	auto view = registry.view<SDL_RenderSprite,SpriteLocation>();

	for (auto et : view)
	{
		SDL_RenderSprite &sprite = view.get<SDL_RenderSprite>(et);
		SpriteLocation &location = view.get<SpriteLocation>(et);
		Vec2i screenspace = game_space_to_screen_space(location.location);
		sprite.location = screenspace;
	}
}

void move_objects(entt::registry &registry, float deltaSeconds) {

	auto view = registry.view<MovementComponent, SpriteLocation>();

	for (auto et : view)
	{
		MovementComponent &movement = view.get<MovementComponent>(et);
		SpriteLocation &location = view.get<SpriteLocation>(et);

		movement.velocity += movement.acceleration * deltaSeconds;
		location.location += movement.velocity * deltaSeconds;
	}
}

void process_player_movement(entt::registry &registry) {
	auto view = registry.view<PlayerInputComponent,MovementComponent>();

	for (auto et : view)
	{
		MovementComponent &movement = view.get<MovementComponent>(et);
		PlayerInputComponent &input = view.get<PlayerInputComponent>(et);

		movement.velocity.x = input.movement_input.x * 300;
		//movement.velocity += movement.acceleration * deltaSeconds;
	}
}



void process_border_collisions(entt::registry &registry) {

	//borders
	float min_x = -WINDOW_WIDTH / 2.f;
	float max_x = WINDOW_WIDTH / 2.f;

	//borders
	float min_y = -100;
	float max_y = WINDOW_HEIGHT -100;

	//clamp player to viewport ------------------
	auto playerview = registry.view<PlayerInputComponent, SpriteLocation, SDL_RenderSprite>();
	for (auto et : playerview)
	{
		SpriteLocation &location = playerview.get<SpriteLocation>(et);
		SDL_RenderSprite &sprite = playerview.get<SDL_RenderSprite>(et);
		

		//grab data from location and sprite info
		float xloc = location.location.x;
		float xextent = sprite.width / 2;

		//left edge
		if (xloc - xextent < min_x)
		{
			location.location.x = min_x + xextent;
		}
		else if (xloc + xextent > max_x)
		{
			location.location.x = max_x - xextent;
		}
	}

	//bounce ball ------------------
	auto ballview = registry.view<Ball, SpriteLocation, SDL_RenderSprite, MovementComponent>();
	for (auto et : ballview)
	{
		SpriteLocation &location = ballview.get<SpriteLocation>(et);
		SDL_RenderSprite &sprite = ballview.get<SDL_RenderSprite>(et);
		MovementComponent&movement = ballview.get<MovementComponent>(et);
		//grab data from location and sprite info
		float xloc = location.location.x;
		float xextent = sprite.width / 2;
		float yloc = location.location.y;
		float yextent = sprite.height / 2;
		//left edge
		if (xloc - xextent < min_x)
		{
			location.location.x = min_x + xextent;
			movement.velocity.x *= -1;
		}
		//right edge
		else if (xloc + xextent > max_x)
		{
			location.location.x = max_x - xextent;
			movement.velocity.x *= -1;
		}
		//top edge		
		else if (yloc + yextent > max_y)
		{
			location.location.y = max_y - yextent;
			movement.velocity.y *= -1;
		}
		//down edge
		else if (yloc - yextent < min_y)
		{
			location.location.y = min_y + yextent;
			movement.velocity.y *= -1;
		}
	}
}



void process_ball_collisions(entt::registry &registry) {

	//bounce ball ------------------
	auto ballview = registry.view<Ball, SpriteLocation, SDL_RenderSprite, MovementComponent>();
	for (auto et : ballview)
	{
		SpriteLocation &location = ballview.get<SpriteLocation>(et);
		const SDL_RenderSprite &sprite = ballview.get<SDL_RenderSprite>(et);
		MovementComponent&movement = ballview.get<MovementComponent>(et);

		//grab data from location and sprite info
		const float xloc = location.location.x;
		const float xextent = sprite.width / 2;
		const float yloc = location.location.y;
		const float yextent = sprite.height / 2;


		float ball_max_x = xloc + xextent;
		float ball_min_x = xloc - xextent;
		
		float ball_max_y = yloc + yextent;
		float ball_min_y = yloc - yextent;
		
		//collide against bricks
		auto brickview = registry.view<Brick, SpriteLocation, SDL_RenderSprite>();
		for (auto brick : brickview)
		{
			const SpriteLocation &brick_location = brickview.get<SpriteLocation>(brick);
			const SDL_RenderSprite &brick_sprite = brickview.get<SDL_RenderSprite>(brick);

			float brick_min_x = brick_location.location.x - brick_sprite.width / 2;
			float brick_max_x = brick_location.location.x + brick_sprite.width / 2;
			
			float brick_min_y = brick_location.location.y - brick_sprite.height / 2;
			float brick_max_y = brick_location.location.y + brick_sprite.height / 2;
			
			bool collides = true;
			// Collision tests
			if (ball_max_x < brick_min_x || ball_min_x > brick_max_x) collides = false;
			if (ball_max_y < brick_min_y || ball_min_y > brick_max_y) collides = false;

			if (collides) {

				Vec2f diff = brick_location.location - location.location;

				float factor = (float)brick_sprite.height / (float)brick_sprite.width ;
				diff.x *= factor;
				if (abs(diff.x) < abs(diff.y))
				{
					movement.velocity.y *= -1;
				}
				else {
					movement.velocity.x *= -1;
				}

				
				registry.destroy(brick);
			}


		}

		//collide against player paddle
		auto playerview = registry.view<PlayerInputComponent,SpriteLocation, SDL_RenderSprite>();
		for (auto player : playerview)
		{
			
			const SpriteLocation &brick_location =playerview.get<SpriteLocation>(player);
			const SDL_RenderSprite &brick_sprite =playerview.get<SDL_RenderSprite>(player);

			float brick_min_x = brick_location.location.x - brick_sprite.width / 2;
			float brick_max_x = brick_location.location.x + brick_sprite.width / 2;

			float brick_min_y = brick_location.location.y - brick_sprite.height / 2;
			float brick_max_y = brick_location.location.y + brick_sprite.height / 2;

			bool collides = true;
			// Collision tests
			if (ball_max_x < brick_min_x || ball_min_x > brick_max_x) collides = false;
			if (ball_max_y < brick_min_y || ball_min_y > brick_max_y) collides = false;

			if (collides) {
				float deltax = location.location.x - brick_location.location.x;

				deltax /= ((float)brick_sprite.width);

				movement.velocity.x = sin(deltax) * 300;
				movement.velocity.y = cos(deltax) * 300;

				printf("deltax: %f", deltax);
				//registry.destroy(brick);
			}
		}
	}
}

bool load_sprite(std::string path, SDL_RenderSprite& sprite)
{
	static std::unordered_map<std::string, SDL_RenderSprite> TextureCache;
	//The final texture
	SDL_Texture* newTexture = NULL;

	if (TextureCache.find(path) == TextureCache.end()) {
		//Load image at specified path
		SDL_Surface* loadedSurface = IMG_Load(path.c_str());
		if (loadedSurface == NULL)
		{
			printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
			return false;
		}
		else
		{
			//Create texture from surface pixels
			newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
			if (newTexture == NULL)
			{
				printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
			}
			else {
				printf("successful load of sprite %s \n", path.c_str());
			}

			sprite.texture = newTexture;
			sprite.texture_rect.x = 0;
			sprite.texture_rect.y = 0;
			sprite.texture_rect.w = loadedSurface->w;
			sprite.texture_rect.h = loadedSurface->h;
			sprite.height = loadedSurface->h;
			sprite.width = loadedSurface->w;
			sprite.location.x = 0;
			sprite.location.y = 0;
			TextureCache[path] = sprite;
			//Get rid of old loaded surface
			SDL_FreeSurface(loadedSurface);
			
		}
	}
	else {
		auto cached_sprite  = TextureCache[path];
		sprite = cached_sprite;
		printf("successful load of sprite from cache %s \n", path.c_str());
	}

	return true;
	
}
uint32_t build_brick(entt::registry &registry, Vec2f location, int type) {
	std::string sprite;
	switch (type)
	{
	case 0:
		sprite = "../assets/sprites/element_yellow_rectangle.png";
		break;
	case 1:
		sprite = "../assets/sprites/element_red_rectangle.png";
		break;
	case 2:
		sprite = "../assets/sprites/element_purple_rectangle.png";
		break;

	case 3:
		sprite = "../assets/sprites/element_grey_rectangle.png";
		break;
	default:

		sprite = "../assets/sprites/element_blue_rectangle.png";
		break;
	}


	//ball
	auto brick = registry.create();
	registry.assign<SDL_RenderSprite>(brick);
	registry.assign<Brick>(brick);
	registry.assign<SpriteLocation>(brick, location);
	load_sprite(sprite, registry.get<SDL_RenderSprite>(brick));
	return brick;
}

int main(int argc, char *argv[])
{
	auto main_registry = entt::registry{};

	
	SDL_Init(SDL_INIT_VIDEO);
	
	gWindow = SDL_CreateWindow(
		"SDL2Test",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		800,
		0
	);

	//Initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
	}

	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	//initialize player
	auto player_entity = main_registry.create();
	main_registry.assign<SDL_RenderSprite>(player_entity);
	main_registry.assign<PlayerInputComponent>(player_entity);
	main_registry.assign<SpriteLocation>(player_entity,0.0f,0.0f);
	main_registry.assign<MovementComponent>(player_entity);
	load_sprite("../assets/sprites/paddleBlu.png", main_registry.get<SDL_RenderSprite>(player_entity));

	//ball
	auto ball_entity = main_registry.create();
	main_registry.assign<SDL_RenderSprite>(ball_entity);
	main_registry.assign<Ball>(ball_entity);
	main_registry.assign<SpriteLocation>(ball_entity, 0.0f, 100.0f);
	main_registry.assign<MovementComponent>(ball_entity);
	load_sprite("../assets/sprites/ballGrey.png", main_registry.get<SDL_RenderSprite>(ball_entity));
	main_registry.get<MovementComponent>(ball_entity).velocity = random_vector() * 400;

	//borders
	const float bricks_min_x = -WINDOW_WIDTH / 2.f +50;
	const float bricks_max_x = WINDOW_WIDTH / 2.f -50;

	//borders
	const float bricks_min_y = 300;
	const float bricks_max_y = WINDOW_HEIGHT - 150;
	 
	const int ny = 10;
	const int nx = 8;

	for (int y = 0; y <= ny; y++)
	{
		for (int x = 0; x <= nx; x++)
		{
			const float fx = x / float(nx);
			const float fy = y /float( ny);
			Vec2f loc;
			loc.x = bricks_min_x + ((bricks_max_x - bricks_min_x) *fx);
			loc.y = bricks_min_y + ((bricks_max_y - bricks_min_y) *fy);

			int type = (x + y) % 4;
			build_brick(main_registry, loc, type);
		}
	}
	//loadMedia();
	SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	bool quit = false;
	SDL_Event e;
	//While application is running
	while (!quit)
	{

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			else
			{
				//If a key was pressed
				if (e.type == SDL_KEYDOWN)
				{
					//Adjust the velocity
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.y = 1;
						break;
					case SDLK_DOWN:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.y = -1;
						break;
					case SDLK_LEFT: 
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.x = -1;
						break;
					case SDLK_RIGHT: 
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.x = 1;
						break;
					}
				}
				else if (e.type == SDL_KEYUP)
				{
					//Adjust the velocity
					switch (e.key.keysym.sym)
					{
					case SDLK_UP:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.y = 0;
						break;
					case SDLK_DOWN:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.y = 0;
						break;
					case SDLK_LEFT:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.x = 0;
						break;
					case SDLK_RIGHT:
						main_registry.get<PlayerInputComponent>(player_entity).movement_input.x = 0;
						break;
					}
				}
			}
		}

		//Clear screen
		SDL_RenderClear(gRenderer);

		process_player_movement(main_registry);
		move_objects(main_registry, 1.0f / 60.0f);

		process_ball_collisions(main_registry);
		process_border_collisions(main_registry);

		transform_sprites(main_registry);
		draw_sprites_sdl(main_registry, gRenderer);
		

		//Update screen
		SDL_RenderPresent(gRenderer);
	}


	SDL_Delay(3000);

	SDL_DestroyWindow(gWindow);
	SDL_Quit();

	return 0;
}