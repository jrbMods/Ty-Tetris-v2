#include "glad/glad.h"

#include "ui.h"
#include "structs.h"
#include "vendor/stb_image.h"

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <unordered_map>

/*
 * Constants.
 */
constexpr auto clear_color = glm::vec4(0.45f, 0.55f, 0.60f, 0.90f);

/*
 * Globals. For convenience.
 */
static std::unordered_map<std::string, int> g_uniform_locations;
static unsigned int g_program = 0;
static glm::mat4 g_model = glm::mat4(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);

static glm::vec3 g_light_pos = glm::vec3(1.0f, 1.0f, 2.0f);
static glm::vec3 g_light_color = glm::vec3(1.0f); /* White light */

/*
 * Forward declarations.
 */
static void set_model(unsigned int);
static void set_view(unsigned int);
static void set_projection(unsigned int);
static void set_light_pos(unsigned int);
static void set_light_color(unsigned int);

/*
 * Checks for OpenGL errors.
 */
static unsigned int gl_print_error(void)
{
    unsigned int error = glGetError();
    if (error == 0)
        std::cout << "No GL errors." << std::endl;
    else
        std::cerr << "GL Error: " << error << std::endl;
    return error;
}

/*
 * Window error event callback.
 */
static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error: " << error << " " << description << std::endl;
}

/*
 * Window keypress event callback.
 */
static void key_callback(GLFWwindow* window,
                         int key,
                         int scancode,
                         int action,
                         int mode)
{
    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            /* Close on escape. */
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_X:
            /* For debugging. */
            gl_print_error();
            break;
        case GLFW_KEY_A:
            g_model = glm::rotate(g_model,
                                  glm::radians(5.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
            set_model(g_program);
            break;
        case GLFW_KEY_D:
            break;
        case GLFW_KEY_W:
            break;
        case GLFW_KEY_S:
            break;
        default:
            break;
    }
}

/*
 * Window resize event callback.
 */
static void size_callback(GLFWwindow* window, int width, int height)
{
    if (width == 0 || height == 0)
        return;

    glViewport(0, 0, width, height);
    cg::perspective.aspect = static_cast<float>(width) / height;
    set_projection(g_program);
};

/*
 * Create window.
 */
GLFWwindow* init_window(void)
{
    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() == GLFW_FALSE)
        return nullptr;

    /*
     * Required for Apple.
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cg::version.gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cg::version.gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    /*
     * Multisampling.
     */
    glfwWindowHint(GLFW_SAMPLES, 4);

    /*
     * Create the graphics context and make it current.
     */
    GLFWwindow* const window = glfwCreateWindow(cg::window.window_width,
                                                cg::window.window_height,
                                                cg::window.window_title,
                                                nullptr,
                                                nullptr);

    if (window == nullptr)
        return nullptr;

    glfwMakeContextCurrent(window);

    /*
     * Enable VSync.
     */
    glfwSwapInterval(1);

    /*
     * Set event callbacks.
     */
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, size_callback);

    /*
     * Load OpenGL with GLAD.
     * Crashes if GL functions are called without this.
     */
    gladLoadGL();

    return window;
}

/*
 * Clear color and depth buffers.
 */
static void clear(void)
{
    glClearColor(clear_color.x * clear_color.w,
                 clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
 * Destroy window.
 */
static void cleanup_window(GLFWwindow* window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

/*
 * Read shader from file.
 */
static std::optional<std::string> read_shader(const std::string& path)
{
    std::string result;

    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (in.good() == false)
        return std::nullopt;

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();

    if (size == -1)
        return std::nullopt;

    result.resize(size);
    in.seekg(0, std::ios::beg);
    in.read(&result[0], size);

    return result;
}

/*
 * Compile shader.
 * Copy the shader source string here just in case.
 * c_str will point to garbage if a string reference goes out of scope.
 */
static unsigned int compile_shader(const std::string shader_source,
                                   unsigned int type)
{
    unsigned int shader = glCreateShader(type);

    const char* c_str = shader_source.c_str();
    glShaderSource(shader, 1, &c_str, nullptr);

    glCompileShader(shader);

    /*
     * Compile error checking.
     */
    int is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == 0)
    {
        std::string log;
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        log.resize(length);
        glGetShaderInfoLog(shader, length, nullptr, &log[0]);

        std::string type_s = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
        std::cerr << "Failed to compile " << type_s << " shader." << std::endl;
        std::cerr << log << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

/*
 * Compile and link shader program.
 */
static unsigned int create_shader(const std::string& vertex_source,
                                  const std::string& fragment_source)
{
    unsigned int program = glCreateProgram();
    unsigned int vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (vertex_shader == 0)
        return 0;
    unsigned int fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (fragment_shader == 0)
        return 0;

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);

    return program;
}

int get_uniform_location(unsigned int program, const std::string& location)
{
    if (g_uniform_locations.find(location) != g_uniform_locations.end())
        return g_uniform_locations[location];

    int uniform = glGetUniformLocation(program, location.c_str());
    if (uniform == -1)
        std::cout << "Warning: Uniform " << location <<
        " does not exist. This uniform will not be set." << std::endl;

    g_uniform_locations[location] = uniform;
    return uniform;
}

static void set_vec3(unsigned int program,
                     const glm::vec3& vector,
                     const std::string& location)
{
    int uniform = get_uniform_location(program, location);
    if (uniform == -1)
        return;
    glUniform3fv(uniform, 1, glm::value_ptr(vector));
}

static void set_matrix(unsigned int program,
                       const glm::mat4& matrix,
                       const std::string& location)
{
    int uniform = get_uniform_location(program, location);
    if (uniform == -1)
        return;
    glUniformMatrix4fv(uniform, 1, false, glm::value_ptr(matrix));
}

/*
 * Set model matrix uniform.
 */
static void set_model(unsigned int program)
{
    set_matrix(program, g_model, "u_model");
}

/*
 * Set view matrix uniform.
 */
static void set_view(unsigned int program)
{
    glm::mat4 view = glm::lookAt(cg::camera.eye,
                                 cg::camera.center,
                                 cg::camera.up);

    set_matrix(program, view, "u_view");
    set_vec3(program, cg::camera.eye, "u_view_pos");
}

/*
 * Set projection matrix uniform.
 */
static void set_projection(unsigned int program)
{
    glm::mat4 projection = glm::perspective(cg::perspective.fov,
                                            cg::perspective.aspect,
                                            cg::perspective.z_near,
                                            cg::perspective.z_far);

    set_matrix(program, projection, "u_projection");
}

/*
 * Set light position.
 */
static void set_light_pos(unsigned int program)
{
    set_vec3(program, g_light_pos, "u_light_pos");
}

/*
 * Set light color.
 */
static void set_light_color(unsigned int program)
{
    set_vec3(program, g_light_color, "u_light_color");
}

/*
 * Vertex buffer object.
 * Uploads draw data to GPU memory.
 */
static unsigned int init_vbo(void)
{
    /*
     * Cube.
     */
    std::array<float, 288> vertices =
    {
    /* Position            Normal                Texture Coords */
    /* Front face */
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,

     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,

    /* Back face */
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,

     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,

    /* Left face */
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

    /* Right face */
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

    /* Top face */
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,

     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,

    /* Bottom face */
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,

     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f
    };

    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

    return vbo;
}

/*
 * Vertex array object.
 * Specifies the format of the draw data.
 */
static unsigned int init_vao(void)
{
    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
     * Position attribute.
     */
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /*
     * Normal attribute.
     */
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    /*
     * Texture coordinate attribute.
     */
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return vao;
}

/*
 * Load and bind texture image.
 */
static unsigned int init_texture(const std::string& path)
{
    int texture_width = 0;
    int texture_height= 0;
    int texture_bpp = 0;

    stbi_set_flip_vertically_on_load(1);

    unsigned char* texture_data = stbi_load(path.c_str(),
                                            &texture_width,
                                            &texture_height,
                                            &texture_bpp,
                                            4);

    if (texture_data == nullptr)
    {
        std::cerr << "Failed to load texture." << std::endl;
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    /*
     * Texture wrapping and filtering.
     * Required.
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 texture_width,
                 texture_height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 texture_data);

    glGenerateMipmap(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    stbi_image_free(texture_data);
    return texture;
}

/*
 * Shader setup.
 */
static unsigned int init_program(const std::string& vertex_path,
                                 const std::string& fragment_path)
{
    const auto vertex_source = read_shader(vertex_path);
    const auto fragment_source = read_shader(fragment_path);
    if (vertex_source.has_value() == false ||
        fragment_source.has_value() == false)
    {
        std::cerr << "Failed to read shaders." << std::endl;
        return 0;
    }

    unsigned int program = create_shader(vertex_source.value(),
                                         fragment_source.value());

    return program;
}

/*
 * Init scene.
 */
static void init(void)
{
    /*
     * Enable z-buffer and multisampling.
     */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    /*
     * Blending. For transparency.
     */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    unsigned int vbo = init_vbo();
    unsigned int vao = init_vao();
    unsigned int texture = init_texture("resources/textures/tu_white.png");

    std::cout << "Data init check:" << std::endl;
    if (gl_print_error() != 0)
        return;

    unsigned int program = init_program("resources/shaders/tex_v.glsl",
                                        "resources/shaders/tex_f.glsl");

    if (program == 0)
    {
        std::cerr << "Failed to compile shaders." << std::endl;
        return;
    }

    glUseProgram(program);
    g_program = program;

    /*
     * Set PVM matrix.
     */
    set_model(program);
    set_view(program);
    set_projection(program);

    /*
     * Set light parameters.
     */
    set_light_pos(program);
    set_light_color(program);
}

/*
 * Draw function.
 */
static void render(void)
{
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

/*
 * Create window and begin drawing.
 */
static void run(void)
{
    GLFWwindow* window = init_window();
    if (window == nullptr)
        std::exit(1);

    cg::init_ImGui(window);
    init();

    /*
     * Main loop.
     * Runs every frame.
     */
    while (glfwWindowShouldClose(window) == 0)
    {
        glfwPollEvents();

        cg::render_ImGui();

        clear();

        render();

        cg::display_ImGui();

        glfwSwapBuffers(window);
    }

    /*
     * Cleanup.
     */
    cg::cleanup_ImGui();
    cleanup_window(window);
}

int main(void)
{
    /*
     * Keep main function brief.
     */
    run();
}

