#ifndef TEMPLATES_REPLAY_HH_
#define TEMPLATES_REPLAY_HH_

#include <cstdint>
#include <string>
#include <vector>

struct waffle_display;
struct waffle_window;
struct waffle_config;

void draw(waffle_display* display, waffle_window* window, waffle_config* config);

std::vector<uint8_t> read_all(const std::string& path);

void check_gl_error();

void check_shader_compile(GLuint shader);
void check_program_link(GLuint program);

void probe(waffle_window* window);
void probe2(waffle_window* window);

void capture_fb(const std::string& path);

#endif  // TEMPLATES_REPLAY_HH_
