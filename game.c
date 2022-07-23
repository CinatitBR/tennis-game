#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

#define FPS 60.0
#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 540

#define BALL_SPEED 4
#define BALL_SPAWN_INTERVAL 5
#define MAX_BALL_COUNT 12

#define MAX_POINTS 2
#define PLAYER_SPEED 2

// Print a warning if method failed to initialize
void must_init(bool isInitialized, char* packageName) {
    if (isInitialized) return;

    printf("ERROR: %s could was not initialized.\n");
}

typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    int width;
    int height;
} Entity;

typedef struct {
    int x;
    int y;
} Position;

// The direction the ball will move (to top or to bottom)
typedef enum {
    DIRECTION_TOP = -1,
    DIRECTION_BOTTOM = 1
} Direction;

// Check if objects are colliding
bool collide(Entity shape1, Entity shape2) {
    if (shape1.x > shape2.x+shape2.width) {
        return false;
    }
    if (shape1.x+shape1.width < shape2.x) { 
        return false;
    }    
    if (shape1.y > shape2.y+shape2.height) { 
        return false;
    }
    if (shape1.y+shape1.height < shape2.y) { 
        return false;
    }

    return true;
}

// Update entity position by dx/dy
void move_entity(Entity* ent) {
    ent->x += ent->dx;
    ent->y += ent->dy;
}

Entity* create_ball(Position pos, Direction dir) {
    Entity* ball = (Entity*)malloc(sizeof(Entity));

    ball->x = pos.x;
    ball->y = pos.y;

    ball->dx = dir * BALL_SPEED;
    ball->dy = dir * BALL_SPEED;

    ball->width = 15;
    ball->height = 15;

    return ball;
}

void destroy_ball(Entity* ball) {
    free(ball);
}

// Reflect entity that collides to corners
void corner_collision(Entity* ent) {
    int corner_overflow = 0;

    // Bottom
    if (ent->y + ent->height > DISPLAY_HEIGHT) {
        corner_overflow = (ent->y + ent->height) - DISPLAY_HEIGHT;

        ent->y -= corner_overflow;
        ent->dy *= -1;
    }

    // Right
    if (ent->x + ent->width > DISPLAY_WIDTH) {
        corner_overflow = (ent->x + ent->width) - DISPLAY_WIDTH;

        ent->x -= corner_overflow;
        ent->dx *= -1;
    }

    // Top
    if (ent->y < 0) {
        corner_overflow = -1 * ent->y;

        ent->y += corner_overflow;
        ent->dy *= -1;
    }

    // Left
    if (ent->x < 0) {
        corner_overflow = -1 * ent->x;

        ent->x += corner_overflow;
        ent->dx *= -1;
    }
}

void ball_corner_collision(Entity* ent) {
    int corner_overflow = 0;

    // Right
    if (ent->x + ent->width > DISPLAY_WIDTH) {
        corner_overflow = (ent->x + ent->width) - DISPLAY_WIDTH;

        ent->x -= corner_overflow;
        ent->dx *= -1;
    }

    // Left
    if (ent->x < 0) {
        corner_overflow = -1 * ent->x;

        ent->x += corner_overflow;
        ent->dx *= -1;
    }
}

int main() {
    // Initialize core methods
    must_init(al_init(), "allegro");
    must_init(al_install_keyboard(), "keyboard");
    must_init(al_init_primitives_addon(), "primitives");
    must_init(al_init_font_addon(), "font");

    // Primitives antialising
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST); 
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

    // Will fire an event 30 times per second (30 FPS)
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    ALLEGRO_DISPLAY* display = al_create_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Register events to queue
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));

    bool redraw = true;
    bool done = false;

    // ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_EVENT event;
    ALLEGRO_KEYBOARD_STATE keyboardState;

    // Ball spawn positions
    Position spawn1, spawn2, spawn3, spawn4;
    spawn1.x = 0; // Top left
    spawn1.y = 0; // Top left
    spawn2.x = DISPLAY_WIDTH; // Top right 
    spawn2.y = 0; // Top right
    spawn3.x = 0; // Bottom left
    spawn3.y = DISPLAY_HEIGHT; // Bottom left 
    spawn4.x = DISPLAY_WIDTH; // Bottom right
    spawn4.y = DISPLAY_HEIGHT; // Bottom right

    Position ball_spawns[] = {spawn1, spawn2, spawn3, spawn4};
    int ball_spawn_index = 1;

    // Create balls vector
    Entity* balls[MAX_BALL_COUNT];

    balls[0] = create_ball(ball_spawns[ball_spawn_index], DIRECTION_BOTTOM);
    int ball_count = 1;
    ball_spawn_index++;

    // Create player1
    Entity player1;
    player1.width = player1.height = 50;
    player1.x = DISPLAY_WIDTH / 2;
    player1.y = DISPLAY_HEIGHT - player1.height - 20;
    player1.dx = player1.dy = 0;
    int player1_points = 0;

    // Create player2
    Entity player2;
    player2.width = player2.height = 50;
    player2.x = DISPLAY_WIDTH / 2;
    player2.y = 20;
    player2.dx = player2.dy = 0;
    int player2_points = 0;

    Entity* players[2];
    players[0] = &player1;
    players[1] = &player2;

    int field_limit_y = DISPLAY_HEIGHT / 2;
    int ball_spawn_interval = FPS * BALL_SPAWN_INTERVAL;

    void draw_balls() {
        for (int i = 0; i < ball_count; i++) {
            al_draw_filled_circle(balls[i]->x, balls[i]->y, balls[i]->width, al_map_rgb(137, 71, 187));
        }
    }

    void draw_players() {
        // Renders players
        for (int i = 0; i < 2; i++) {
            al_draw_filled_rectangle(
                players[i]->x, 
                players[i]->y, 
                players[i]->x + players[i]->width, 
                players[i]->y + players[i]->height, 
                al_map_rgb(240, 84, 84)
            );
        }
    }

    void draw_points() {
        // Renders player2 points
        al_draw_textf(font, al_map_rgb(255, 255, 255), 30, 30, 0, "Pontuação: %d", player2_points);

        // Renders player1 points
        al_draw_textf(font, al_map_rgb(255, 255, 255), 30, DISPLAY_HEIGHT - 30, 0, "Pontuação: %d", player1_points);
    }

    bool is_game_finished() {
        return player1_points >= MAX_POINTS || player2_points >= MAX_POINTS;
    }

    al_start_timer(timer);
    while(1) {
        // Wait ultil an event, fill event variable with information.
        al_wait_for_event(event_queue, &event);

        // Event pool
        switch (event.type) {
            // Game logic goes here. Represents a frame.
            case ALLEGRO_EVENT_TIMER:
                ball_spawn_interval--;

                // Destroy out of corner ball
                for (int i = 0; i < ball_count; i++) {

                    // Top    
                    if (balls[i]->y + balls[i]->height < 0) {
                        player1_points++;
                    }

                    // Bottom
                    else if (balls[i]->y > DISPLAY_HEIGHT) {
                        player2_points++;
                    }

                    else
                        continue;

                    destroy_ball(balls[i]);
                    ball_count--;
                }
                
                // Spawn new ball
                if (ball_spawn_interval == 0 && ball_count <= 12) {
                    Direction ball_direction;

                    if (ball_spawn_index < 2)
                        ball_direction = DIRECTION_BOTTOM;
                    else 
                        ball_direction = DIRECTION_TOP;

                    Position current_spawn = ball_spawns[ball_spawn_index];
                    balls[ball_count] = create_ball(current_spawn, ball_direction);

                    ball_spawn_interval = FPS * BALL_SPAWN_INTERVAL;
                    ball_count++;

                    if (ball_spawn_index < 3)
                        ball_spawn_index++;
                    else 
                        ball_spawn_index = 0;
                }

                // Update balls
                for (int i = 0; i < ball_count; i++) {
                    // Update balls position
                    move_entity(balls[i]);

                    // Reflect ball on window corner collision
                    ball_corner_collision(balls[i]);
                }

                // Update players
                for (int i = 0; i < 2; i++) {

                    // Update player position
                    move_entity(players[i]);

                    // Reflect player on window corner collision
                    corner_collision(players[i]);
                }


                // Collisions to oponent's field
                // Player 1
                if (player1.y < field_limit_y) {
                    int field_overflow = field_limit_y - player1.y;

                    player1.y += field_overflow; 
                    player1.dy *= -1;
                }
                // Player 2
                if (player2.y + player2.height > field_limit_y) {
                    int field_overflow = (player2.y + player2.height) - field_limit_y;

                    player2.y -= field_overflow; 
                    player2.dy *= -1;
                }

                player1.dx *= 0.9;
                player1.dy *= 0.9;
                player2.dx *= 0.9;
                player2.dy *= 0.9;

                // Get keyboard state
                al_get_keyboard_state(&keyboardState);

                // Increase players size
                if (al_key_down(&keyboardState, ALLEGRO_KEY_P)) {
                    player1.width += 2;
                    player1.height += 2;

                    player2.width += 2;
                    player2.height += 2;
                }

                // player1 movements
                if (al_key_down(&keyboardState, ALLEGRO_KEY_W)) { // Up
                    player1.dy += -1 * PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_A)) { // Left
                    player1.dx += -1 * PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_S)) { // Down
                    player1.dy += PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_D)) { // Right
                    player1.dx += PLAYER_SPEED;
                }

                // player2 movements
                if (al_key_down(&keyboardState, ALLEGRO_KEY_I)) { // Up
                    player2.dy += -1 * PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_J)) { // Left
                    player2.dx += -1 * PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_K)) { // Down
                    player2.dy += PLAYER_SPEED;
                }

                if (al_key_down(&keyboardState, ALLEGRO_KEY_L)) { // Right
                    player2.dx += PLAYER_SPEED;
                } 

                redraw = true;
                break;
            
            case ALLEGRO_EVENT_KEY_DOWN:
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                    done = true;

                // Keys to hit the ball: "E" and "O"
                int should_player1_hit = event.keyboard.keycode == ALLEGRO_KEY_E;
                int should_player2_hit = event.keyboard.keycode == ALLEGRO_KEY_O;

                if (should_player1_hit || should_player2_hit)
                {
                    for (int i = 0; i < ball_count; i++) {
                        // player1 or player2 hit the ball
                        if (collide(player1, *balls[i]) && should_player1_hit
                            || collide(player2, *balls[i]) && should_player2_hit)
                        {
                            balls[i]->dx *= -1;
                            balls[i]->dy *= -1;
                        }
                    }
                }

                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                done = true;
                break;
        }

        // Finishes event query (break while)
        if (done) 
            break;
        
        bool is_queue_empty = al_is_event_queue_empty(event_queue);

        // Rendering
        if (redraw && is_queue_empty) {
            al_clear_to_color(al_map_rgb(58, 63, 71));

            if (is_game_finished()) {
                char text[50];

                if (player1_points >= 10) {
                    sprintf(text, "P L A Y E R  1  W O N");
                } else {
                    sprintf(text, "P L A Y E R  1  W O N");
                }

                al_draw_text(font, al_map_rgb(255, 255, 255), 400, DISPLAY_HEIGHT / 2, 0, text);
            }

            else {
                draw_balls();
                draw_players();
                draw_points();
            }


            al_flip_display();
            redraw = false;
        }
    }

    // Destroy pointers
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_font(font);

    return 0;
}

// gcc game.c -o game.exe -lallegro -lallegro_font -lallegro_image -lallegro_primitives -lallegro_acodec