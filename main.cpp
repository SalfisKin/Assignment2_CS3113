/**
* Author: Chenyue Shen
* Assignment: Pong Clone
* Date due: 2023-10-21, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float PADDLE_HALF_HEIGHT = 0.25f;
const float TOP_BOUNDARY = 3.75f - PADDLE_HALF_HEIGHT;
const float BOTTOM_BOUNDARY = -3.75f + PADDLE_HALF_HEIGHT;
const float RIGHT_BOUNDARY = 5.0f - PADDLE_HALF_HEIGHT;
const float LEFT_BOUNDARY = -5.0f + PADDLE_HALF_HEIGHT;
const float PADDLE_WIDTH =1.0f;
const float PADDLE_HEIGHT =1.0f;
const float BALL_SIZE = 0.5f;
const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH_A[] = "arctic tern.png";
const char PLAYER_SPRITE_FILEPATH_B[] = "urchin.png";
const char PLAYER_SPRITE_FILEPATH_C[] = "background.png";
const char PLAYER_SPRITE_FILEPATH_D[] = "ball.png";
const char PLAYER_SPRITE_FILEPATH_E[] = "winnermessageA.png";
const char PLAYER_SPRITE_FILEPATH_F[] = "winnermessageB.png";

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_is_growing = true;

ShaderProgram g_shader_program;
glm::mat4 view_matrix, m_model_matrix_A, m_projection_matrix, m_trans_matrix_A, m_model_matrix_B, 
m_trans_matrix_B, m_model_matrix_C,m_model_matrix_D, m_trans_matrix_D,m_model_matrix_E, m_model_matrix_F ;

float m_previous_ticks = 0.0f;

float speedX = 1.0f;
float speedY = 1.0f;
float speedA = 2.0f;
float m_triangle_x = 0.0f;
float m_triangle_y = 0.0f;
float m_triangle_rotate = 0.0f;

GLuint g_player_texture_id_A;
SDL_Joystick* g_player_A_controller;
GLuint g_player_texture_id_B;
SDL_Joystick* g_player_B_controller;
GLuint g_background_texture_id;
GLuint g_ball_id;
GLuint g_wmA_id;
GLuint g_wmB_id;

// overall position
glm::vec3 g_player_position_A = glm::vec3(3.25f, 0.0f, 0.0f);
glm::vec3 g_player_position_B = glm::vec3(-3.25f, 0.0f, 0.0f);
glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
// movement tracker
glm::vec3 g_player_movement_A = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_movement_B = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);

void draw_background();
void draw_ball();
void draw_winner_A();
void draw_winner_B();

enum GameState {
    Playing,
    PlayerAWins,
    PlayerBWins
};

enum GameMode {
    One_Player_Mode,
    Multiple_Player_Mode
};

GameMode currentGameMode = Multiple_Player_Mode;

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
    case x_coordinate:
        return ((coordinate / WINDOW_WIDTH) * 10.0f) - (10.0f / 2.0f);
    case y_coordinate:
        return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
    default:
        return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    // Open the first controller found. Returns null on error
    g_player_A_controller = SDL_JoystickOpen(0);
    g_player_B_controller = SDL_JoystickOpen(0);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    m_model_matrix_A = glm::mat4(1.0f);
    m_model_matrix_B = glm::mat4(1.0f);
    m_model_matrix_C = glm::mat4(1.0f);
    m_model_matrix_D = glm::mat4(1.0f);
    m_model_matrix_E = glm::mat4(1.0f);
    m_model_matrix_F = glm::mat4(1.0f);
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    m_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.


    g_shader_program.set_projection_matrix(m_projection_matrix);
    g_shader_program.set_view_matrix(view_matrix);
    // Notice we haven't set our model matrix yet!

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_player_texture_id_A = load_texture(PLAYER_SPRITE_FILEPATH_A);
    g_player_texture_id_B = load_texture(PLAYER_SPRITE_FILEPATH_B);
    g_background_texture_id = load_texture(PLAYER_SPRITE_FILEPATH_C);
    g_ball_id = load_texture(PLAYER_SPRITE_FILEPATH_D); 
    g_wmA_id = load_texture(PLAYER_SPRITE_FILEPATH_E);
    g_wmB_id = load_texture(PLAYER_SPRITE_FILEPATH_F);
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
}

void process_input()
{
    g_player_movement_A = glm::vec3(0.0f);
    g_player_movement_B = glm::vec3(0.0f);

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_WINDOWEVENT_CLOSE:
        case SDL_QUIT:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT:
                g_player_movement_A.y = 1.0f;
                break;
            case SDLK_LEFT:
                g_player_movement_A.y = -1.0f;
                break;
            case SDLK_w:
                g_player_movement_B.y = 1.0f;
                break;
            case SDLK_s:
                g_player_movement_B.y = -1.0f;
                break;
            case SDLK_q:
                g_game_is_running = false;
                break;
            case SDLK_t:
                if (currentGameMode == One_Player_Mode) {
                    currentGameMode = Multiple_Player_Mode;
                }
                else {
                    currentGameMode = One_Player_Mode;
                }
            default:
                break;
            }
        default:
            break;
        }
    }

    const Uint8* key_states = SDL_GetKeyboardState(NULL); // array of key states [0, 0, 1, 0, 0, ...]


    if (key_states[SDL_SCANCODE_UP])
    {
        g_player_movement_A.y = 1.0f;
    }
    else if (key_states[SDL_SCANCODE_DOWN])
    {
        g_player_movement_A.y = -1.0f;
    }

    if (glm::length(g_player_movement_A) > 1.0f)
    {
        g_player_movement_A = glm::normalize(g_player_movement_A);
    }

    if (key_states[SDL_SCANCODE_W])
    {
        g_player_movement_B.y = 1.0f;
    }
    else if (key_states[SDL_SCANCODE_S])
    {
        g_player_movement_B.y = -1.0f;
    }

    if (glm::length(g_player_movement_B) > 1.0f)
    {
        g_player_movement_B = glm::normalize(g_player_movement_B);
    }
}


GameState currentGameState = Playing;

void update()
{
    if (currentGameState != Playing) {
        return; // stops updating game mechanics
    }
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - m_previous_ticks; // the delta time is the difference from the last frame
    m_previous_ticks = ticks;

    bool collisionA =
        m_triangle_x < g_player_position_A.x + PADDLE_WIDTH &&
        m_triangle_x + BALL_SIZE > g_player_position_A.x &&
        m_triangle_y < g_player_position_A.y + PADDLE_HEIGHT &&
        m_triangle_y + BALL_SIZE > g_player_position_A.y;

    // Box-to-box collision detection for ball and paddle B
    bool collisionB =
        m_triangle_x < g_player_position_B.x + PADDLE_WIDTH/2 &&
        m_triangle_x + BALL_SIZE > g_player_position_B.x &&
        m_triangle_y < g_player_position_B.y + PADDLE_HEIGHT/2 &&
        m_triangle_y + BALL_SIZE > g_player_position_B.y;

    if (collisionA || collisionB)
    {
        speedX = -speedX;
    }

    m_triangle_x += speedX * delta_time;
    m_triangle_y += speedY * delta_time;

    switch (currentGameMode) {
    case One_Player_Mode:
        // Handle 1-player logic.
        // Example: Move paddle A up and down automatically.
        g_player_position_A.y += speedA * delta_time;

        if (g_player_position_A.y > TOP_BOUNDARY || g_player_position_A.y < BOTTOM_BOUNDARY) {
            speedA = -speedA; // reverse direction
        }

        // Handle movement for player B using keyboard input.
        g_player_position_B += g_player_movement_B * delta_time * 4.0f;
        break;
    case Multiple_Player_Mode:
        // Handle 2-player logic.
        // Move both paddles A and B using keyboard input.
        g_player_position_A += g_player_movement_A * delta_time * 4.0f;
        g_player_position_B += g_player_movement_B * delta_time * 4.0f;
        break;
    }

    m_model_matrix_A = glm::mat4(1.0f);
    m_model_matrix_B = glm::mat4(1.0f);
    m_model_matrix_D = glm::mat4(1.0f);

    m_model_matrix_A = glm::translate(m_model_matrix_A, g_player_position_A);
    m_model_matrix_B = glm::translate(m_model_matrix_B, g_player_position_B);
    m_model_matrix_D = glm::translate(m_model_matrix_D, glm::vec3(m_triangle_x, m_triangle_y, 0.0f));
    //Check if reached top or bottom of the screen
    if (g_player_position_A.y > TOP_BOUNDARY) {
        g_player_position_A.y = TOP_BOUNDARY;
    }
    else if (g_player_position_A.y < BOTTOM_BOUNDARY) {
        g_player_position_A.y = BOTTOM_BOUNDARY;
    }

    if (g_player_position_B.y > TOP_BOUNDARY) {
        g_player_position_B.y = TOP_BOUNDARY;
    }
    else if (g_player_position_B.y < BOTTOM_BOUNDARY) {
        g_player_position_B.y = BOTTOM_BOUNDARY;
    }
    if (m_triangle_y > TOP_BOUNDARY) {
        speedY = -speedY;
    }
    else if (m_triangle_y < BOTTOM_BOUNDARY) {
        speedY = -speedY;
    }

    if (m_triangle_x < LEFT_BOUNDARY) {
        currentGameState = PlayerAWins;
    }
    else if (m_triangle_x > RIGHT_BOUNDARY) {
        currentGameState = PlayerBWins;
    }
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    draw_background();
    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(m_model_matrix_A, g_player_texture_id_A);
    draw_object(m_model_matrix_B, g_player_texture_id_B);
    draw_ball();
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    if (currentGameState == PlayerAWins) {
        draw_winner_A();
    }
    else if (currentGameState == PlayerBWins) {
        draw_winner_B();
    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_JoystickClose(g_player_A_controller);
    SDL_JoystickClose(g_player_B_controller);
    SDL_Quit();
}

/**
 Start here¡ªwe can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
void draw_background()
{
    // Define fullscreen quad
    float vertices[] = {
        -5.0f, -3.75f, // bottom-left
         5.0f, -3.75f, // bottom-right
         5.0f,  3.75f, // top-right

        -5.0f, -3.75f, // bottom-left
         5.0f,  3.75f, // top-right
        -5.0f,  3.75f, // top-left
    };

    float texture_coordinates[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(m_model_matrix_C, g_background_texture_id);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void draw_ball()
{
    float vertices[] = {
        -0.25f, -0.25f, 0.25f, -0.25f, 0.25f, 0.25f,  // triangle 1
        -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(m_model_matrix_D, g_ball_id);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void draw_winner_A() {
    float vertices[] = {
    -2.0f, -1.0f, 2.0f, -1.0f, 2.0f, 3.0f,  // triangle 1
    -2.0f, -1.0f, 2.0f, 3.0f, -2.0f, 3.0f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(m_model_matrix_E, g_wmA_id);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_winner_B() {
    float vertices[] = {
    -2.0f, -1.0f, 2.0f, -1.0f, 2.0f, 3.0f,  // triangle 1
    -2.0f, -1.0f, 2.0f, 3.0f, -2.0f, 3.0f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(m_model_matrix_F, g_wmB_id);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
