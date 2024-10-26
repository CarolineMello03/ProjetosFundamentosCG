/* Segue o Mouse - código adaptado de https://learnopengl.com/Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Fundamentos de Computação Gráfica - Unisinos
 * Versão inicial: 05/10/2024 (ver gravação da aula)
 * Última atualização em 17/10/2024
 *
 * Este programa desenha um triângulo que segue o cursor do mouse
 * usando OpenGL e GLFW.
 * A posição e a rotação do triângulo são calculadas com base no movimento do mouse.
 */

#include <iostream>
#include <string>
#include <assert.h>

// Bibliotecas GLAD para carregar funções OpenGL
#include <glad/glad.h>

// Biblioteca GLFW para criar janelas e gerenciar entrada de teclado/mouse
#include <GLFW/glfw3.h>

// GLM para operações matemáticas (vetores, matrizes)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

using namespace std;
using namespace glm;

// Constantes
const float Pi = 3.14159265;
const GLuint WIDTH = 800, HEIGHT = 600; // Dimensões da janela

// Estrutura para armazenar informações sobre as geometrias da cena
struct Geometry 
{
    GLuint VAO;        // Vertex Array Geometry
    vec3 position;     // Posição do objeto
    float angle;       // Ângulo de rotação
    vec3 dimensions;   // Escala do objeto (largura, altura)
    vec3 color;        // Cor do objeto
    int nVertices;     // Número de vértices a desenhar
};

// Variáveis globais
bool keys[1024];   // Estados das teclas (pressionadas/soltas)
vec2 mousePos;     // Posição do cursor do mouse
vec3 dir = vec3(0.0, -1.0, 0.0); // Vetor direção (do objeto para o mouse)
float smoothFactor = 0.1;
float maxDistance = 0.1;
float minDistance = 0.05;
bool addNew = false;
vector<Geometry> cobrinha; // Vetor que armazena os segmentos da cobrinha
Geometry eyes; // Objeto que representa os olhos da cobrinha

// Protótipos das funções
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();           // Função para configurar os shaders
void drawGeometry(GLuint shaderID, GLuint VAO, int nVertices, vec3 position, vec3 dimensions, float angle, vec3 color, GLuint drawingMode = GL_TRIANGLES, int offset = 0, vec3 axis = vec3(0.0, 0.0, 1.0));
Geometry createSegment(int i, vec3 dir);
int createEyes(int nPoints, float radius);
int createCircle(int nPoints, float radius, float xc = 0.0, float yc = 0.0);

int main() {
    // Inicializa GLFW e configurações de versão do OpenGL
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Cobrinha", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Inicializa GLAD para carregar todas as funções OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    // Informações sobre o Renderer e a versão OpenGL
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "Versão OpenGL suportada: " << version << endl;

    // Configurações de viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    // Criação da cabeça
    Geometry head = createSegment(0, dir);
    cobrinha.push_back(head);

    // Criação dos olhos
    eyes.VAO = createEyes(32, 0.25);
    eyes.nVertices = 34;
    eyes.position = vec3(400, 300, 0.0);
    eyes.dimensions = vec3(50, 50, 1.0);
    eyes.angle = 0.0;
    eyes.color = vec3(1.0, 1.0, 1.0);

    // Ativa o teste de profundidade
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS); // Sempre passa no teste de profundidade (desnecessário se não houver profundidade)

    glUseProgram(shaderID);

    // Matriz de projeção ortográfica (usada para desenhar em 2D)
    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Loop da aplicação
    while (!glfwWindowShouldClose(window)) {
        // Processa entradas (teclado e mouse)
        glfwPollEvents();

        // Limpa a tela
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pega a posição do mouse e calcula a direção
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);
        mousePos = vec2(xPos, height - yPos);  // Inverte o eixo Y para se alinhar à tela
        float lookangle = atan2(dir.y, dir.x);

        dir = normalize(vec3(mousePos, 0.0) - vec3(mousePos, 0.0));
        vec3 position = vec3(mousePos, 0.0) + 0.2f * dir;

        cobrinha[0].position = position;
        cobrinha[0].angle = lookangle;
        eyes.position = position;
        eyes.angle = lookangle;

        for (int i = 1; i < cobrinha.size(); i++)
        {
            vec3 dir = normalize(cobrinha[i - 1].position - cobrinha[i].position);
            float distance = length(cobrinha[i - 1].position - cobrinha[i].position);

            vec3 targetPosition = cobrinha[i].position;
            float dynamicSmoothFactor = smoothFactor * (distance / maxDistance);

            if (distance < minDistance)
            {
                targetPosition = cobrinha[i].position + (distance - minDistance) * dir;
            }
            else if (distance > maxDistance)
            {
                targetPosition = cobrinha[i].position + (distance - maxDistance) * dir;
            }

            cobrinha[i].position = mix(cobrinha[i].position, targetPosition, dynamicSmoothFactor);
        }

        if (addNew)
        {
            cobrinha.push_back(createSegment(cobrinha.size(), -dir));
            addNew = false;
        }

        for (int i = cobrinha.size() - 1; i >= 0; i--)
        {
            drawGeometry(shaderID, cobrinha[i].VAO, cobrinha[i].nVertices, 
                cobrinha[i].position, cobrinha[i].dimensions, cobrinha[i].angle,
                cobrinha[i].color, GL_TRIANGLE_FAN);

            if (i == 0)
            {
                drawGeometry(shaderID, eyes.VAO, eyes.nVertices, eyes.position,
                eyes.dimensions, eyes.angle, eyes.color, GL_TRIANGLE_FAN, 0);
                drawGeometry(shaderID, eyes.VAO, eyes.nVertices, eyes.position,
                eyes.dimensions, eyes.angle, eyes.color, GL_TRIANGLE_FAN, 34);

                drawGeometry(shaderID, eyes.VAO, eyes.nVertices, eyes.position,
                eyes.dimensions, eyes.angle, vec3(0.0, 0.0, 0.0), GL_TRIANGLE_FAN, 2 *34);
                drawGeometry(shaderID, eyes.VAO, eyes.nVertices, eyes.position,
                eyes.dimensions, eyes.angle, vec3(0.0, 0.0, 0.0), GL_TRIANGLE_FAN, 3 * 34);
            }

        }

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Limpa a memória alocada pelos buffers
    glfwTerminate();
    return 0;
}

// Callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        addNew = true;
    }

    if (action == GLFW_PRESS)
        keys[key] = true;
    if (action == GLFW_RELEASE)
        keys[key] = false;
}


// Configura e compila os shaders
int setupShader() {
    // Código do vertex shader
    const GLchar *vertexShaderSource = R"(
    #version 400
    layout (location = 0) in vec3 position;
    uniform mat4 projection;
    uniform mat4 model;
    void main() {
        gl_Position = projection * model * vec4(position, 1.0);
    })";

    // Código do fragment shader
    const GLchar *fragmentShaderSource = R"(
    #version 400
    uniform vec4 inputColor;
    out vec4 color;
    void main() {
        color = inputColor;
    })";

    // Compilação do vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Verificando erros de compilação do vertex shader
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Compilação do fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Verificando erros de compilação do fragment shader
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Linkando os shaders no programa
    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    // Verificando erros de linkagem do programa de shaders
    glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Limpando os shaders compilados após o link
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderID;
}

// Função para desenhar o objeto
void drawGeometry(GLuint shaderID, GLuint VAO, int nVertices, vec3 position, vec3 dimensions, float angle, vec3 color, GLuint drawingMode, int offset, vec3 axis) {
    glBindVertexArray(VAO); // Vincula o VAO

    // Aplica as transformações de translação, rotação e escala
    mat4 model = mat4(1.0f);
    model = translate(model, position);
    model = rotate(model, angle, axis);
    model = scale(model, dimensions);

    // Envia a matriz de modelo ao shader
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
    
    // Envia a cor do objeto ao shader
    glUniform4f(glGetUniformLocation(shaderID, "inputColor"), color.r, color.g, color.b, 1.0f);

    // Desenha o objeto
    glDrawArrays(drawingMode, offset, nVertices);

    // Desvincula o VAO
    glBindVertexArray(0);
}

Geometry createSegment(int i, vec3 dir)
{
    cout << "criando segmento " << i << endl;

    // inicializa objeto Geometry para armazenar as informações do segmento
    Geometry segment;
    segment.VAO = createCircle(32, 0.5);
    segment.nVertices = 34;

    // posição inicial do segmento
    if (i == 0) // cabeça
    {
        segment.position = vec3(400, 300, 0.0); // posição inicial no centro da tela
    }
    else
    {
        // Ajusta a direção com base na posição dos segmentos anteriores para evitar sobreposição
        if (i <= 2)
        {
            dir = normalize(cobrinha[i - 1].position - cobrinha[i - 2].position);
        }
        // Posiciona o novo segmento a uma distância mínima do anterior
        segment.position = cobrinha[i - 1].position + minDistance * dir;
    }

    // Define as dimensões do segmento (tamanho do círculo)
    segment.dimensions = vec3(50, 50, 1.0);
    segment.angle = 0.0;

    // Alterna a cor do segmento entre azul e amarelo, dependendo do índice
    if (i % 2 == 0)
    {
        segment.color = vec3(0.0, 0.0, 1.0);
    }
    else
    {
        segment.color = vec3(1.0, 1.0, 0.0);
    }

    return segment;
}

int createEyes(int nPoints, float radius)
{
    vector<GLfloat> vertices;

    // ângulo inicial e incremento para cada ponto do círculo
    float angle = 0.0;
    float slice = 2 * Pi / static_cast<float>(nPoints);

    // Posições iniciais para os círculos dos olhos
    float xi = 0.125f; // Posição inicial X das escleras
    float yi = 0.3f;   // Posição inicial Y das escleras
    radius = 0.225f;   // Raio das escleras

    // Olho esquerdo (esclera)
    vertices.push_back(xi);
    vertices.push_back(yi);
    vertices.push_back(0.0f);

    for (int i = 0; i < nPoints + 1; i++)
    {
        float x = xi + radius * cos(angle);
        float y = yi + radius * sin(angle);
        float z = 0.0f;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        angle += slice; // Incrementa o ângulo para o próximo ponto
    }

    // Olho direito (esclera)
    vertices.push_back(xi);
    vertices.push_back(-yi);
    vertices.push_back(0.0f);

    for (int i = 0; i < nPoints + 1; i++)
    {
        float x = xi + radius * cos(angle);
        float y = -yi + radius * sin(angle);
        float z = 0.0f;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        angle += slice; // Incrementa o ângulo para o próximo ponto
    }

    // Olho esquerdo (pupila)
    radius = 0.18f;
    xi += 0.09f;
    angle = 0.0;

    vertices.push_back(xi);
    vertices.push_back(yi);
    vertices.push_back(0.0f);

    for (int i = 0; i < nPoints + 1; i++) 
    {
        float x = xi + radius * cos(angle);
        float y = yi + radius * sin(angle);
        float z = 0.0f;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        angle += slice;
    }

    // Olho direito (pupila)
    radius = 0.18f;
    xi += 0.09f;
    angle = 0.0;

    vertices.push_back(xi);
    vertices.push_back(-yi);
    vertices.push_back(0.0f);

    for (int i = 0; i < nPoints + 1; i++) 
    {
        float x = xi + radius * cos(angle);
        float y = -yi + radius * sin(angle);
        float z = 0.0f;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        angle += slice;
    }

    // Identificadores para o VBO e VAO
    GLuint VBO, VAO;

    // Geração do identificador do VBO e vinculação
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Envia os dados do vetor de vértices para a GPU
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    // Geração do identificador do VAO e vinculação
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Configuração do ponteiro de atributos para os vértices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Desvincula o VBO e o VAO para evitar modificações acidentais
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Retorna o identificador do VAO, que será utilizado para desenhar os olhos
    return VAO;
}

int createCircle(int nPoints, float radius, float xc, float yc)
{
    vector<GLfloat> vertices;

    float angle = 0.0;
    float slice = 2 * Pi / static_cast<float>(nPoints);

    vertices.push_back(xc);
    vertices.push_back(yc);
    vertices.push_back(0.0);

    for (int i = 0; i < nPoints + 1; i++)
    {
        float x = xc + radius * cos(angle);
        float y = yc + radius * sin(angle);
        float z = 0.0f;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);

        angle += slice;
    }

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Desvincula o VAO e o VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
