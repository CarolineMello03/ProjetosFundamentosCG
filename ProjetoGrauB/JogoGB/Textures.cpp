#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// STB_IMAGE
#include <stb_image.h>

//GLM
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#include <cmath>

// Estrutura de dados das sprites
struct Sprite
{
	GLfloat VAO; // id do buffer de geometria
	GLfloat texID; // id da textura
	vec3 pos, dimensions;
	float angle;

	// Para a animação da spritesheet
	int nAnimations, nFrames;
	int iAnimation, iFrame;
	float ds, dt;

	// Para a movimentação do sprite
	float vel;

	// Para a colisão (AABB)
	vec2 pMax, pMin;
};


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();
Sprite initializeSprite(GLuint texID, vec3 dimensions, vec3 position, float vel = 0.2, int nAnimations=1, int nFrames=1, float angle=0.0);

GLuint loadTexture(string filePath, int &width, int &height);

void drawTriangle(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, vec3 color, vec3 axis = (vec3(0.0, 0.0, 1.0)));
void drawSprite(GLuint shaderID, Sprite &sprite);
void updateSprite(GLuint shaderID, Sprite &sprite);
void moveSprite(GLuint shaderID, Sprite &sprite);

void updateSnowball(GLuint shaderID, Sprite &sprite);
void updateItems(GLuint shaderID, Sprite &sprite);

void spawnItem(Sprite &sprite);

void calculateAABB(Sprite &sprite);
bool checkCollision(Sprite &playerSprite, Sprite &itemSprite);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
uniform mat4 projection;
uniform mat4 model;
out vec2 texCoord;
void main()
{
	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = vec2(texc.s, 1.0-texc.t);
})";

//Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec2 offsetTex;
out vec4 color;
void main()
{
	color = texture(texBuff, texCoord + offsetTex);
})";

// Variáveis globais
float FPS = 8.0f;
float lastTime = 0.0;
bool keys[1024];
GLuint itemsTexIDs[5];
int score = 0;
int missedItems = 0;

// Função MAIN
int main()
{
	srand(time(0)); // Pega o horário do sistema como semente para geração de números aleatórios

	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo Grau B -- Carol", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Inicialização do array de controle das teclas
	for (int i = 0; i < 1024; i++)
	{
		keys[i] = false;
	}

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Criação dos sprites - objetos da cena
	Sprite background, character, snowball, item;
	int imgWidth, imgHeight, texID;

	// Carregando a textura do personagem e armazenando seu id
	texID = loadTexture("../Textures/sprite.png", imgWidth, imgHeight);
	character = initializeSprite(texID, vec3(3*imgWidth, 3*imgHeight, 1.0), vec3(400, 100, 0), 0.3, 3, 4);

	// Carregando a textura do background e armazenando seu id
	texID = loadTexture("../Textures/background.png", imgWidth, imgHeight);
	background = initializeSprite(texID, vec3(2*imgWidth, 2*imgHeight, 1.0), vec3(400, 300, 0));

	// Carregando as texturas dos itens
	itemsTexIDs[1] = loadTexture("../Textures/hat.png", imgWidth, imgHeight);
	itemsTexIDs[2] = loadTexture("../Textures/coat.png", imgWidth, imgHeight);
	itemsTexIDs[3] = loadTexture("../Textures/gloves.png", imgWidth, imgHeight);
	itemsTexIDs[4] = loadTexture("../Textures/pants.png", imgWidth, imgHeight);
	itemsTexIDs[0] = loadTexture("../Textures/boots.png", imgWidth, imgHeight);

	vec3 posItem;
	posItem.x = rand() % 737 + 32; // valor entre 32 e 768
	posItem.y = rand() % 600 + 630; // valor entre 630 e 1229
	posItem.z = 0.0;
	texID = itemsTexIDs[rand() % sizeof(itemsTexIDs)/sizeof(*itemsTexIDs)];
	item = initializeSprite(texID, vec3(3*imgWidth, 3*imgHeight, 1.0), posItem);

	// Carregando a textura da bola de neve
	texID = loadTexture("../Textures/snowball.png", imgWidth, imgHeight);
	vec3 posSnow;
	posSnow.x = rand() % 737 + 32; // valor entre 32 e 768
	posSnow.y = rand() % 600 + 630; // valor entre 630 e 1229
	posSnow.z = 0.0;
	snowball = initializeSprite(texID, vec3(3*imgWidth, 3*imgHeight, 1.0), posSnow);

	// Carregando as texturas das telas de game over
	texID = loadTexture("../Textures/snow_screen.png", imgWidth, imgHeight);
	Sprite gameOverSnow = initializeSprite(texID, vec3(imgWidth * 5, imgHeight * 5, 1.0), vec3(400, 300, 0));
	texID = loadTexture("../Textures/cold_screen.png", imgWidth, imgHeight);
	Sprite gameOverCold = initializeSprite(texID, vec3(imgWidth * 5, imgHeight * 5, 1.0), vec3(400, 300, 0));
	texID = loadTexture("../Textures/win_screen.png", imgWidth, imgHeight);
	Sprite gameWin = initializeSprite(texID, vec3(imgWidth * 5, imgHeight * 5, 1.0), vec3(400, 300, 0));

	glUseProgram(shaderID);

	// Enviar a informação de qual variável armazenará o buffer da textura
	//                                                     id do buffer
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

	// Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);

	//Matriz de projeção paralela ortográfica
	//mat4 projection = ortho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
	mat4 projection = ortho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);  
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	//Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); //matriz identidade
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	// Habilitando o teste de profundidade
	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_ALWAYS);
	
	// Habilitando a transparência
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Checar colisões
		bool gameOver = checkCollision(character, snowball);
		if (checkCollision(character, item))
		{
			score++;
			cout << "Score: " << score << "\n";
			spawnItem(item);
		}
		
		if (!gameOver && missedItems < 3)
		{
			if (score >= 30)
			{
				// Fundo
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				drawSprite(shaderID, gameWin);
			}
			else
			{
				// Fundo
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				drawSprite(shaderID, background);
			
				// Personagem
				moveSprite(shaderID, character);
				updateSprite(shaderID, character);
				drawSprite(shaderID, character);

				// Bola de neve
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				updateSnowball(shaderID, snowball);
				drawSprite(shaderID, snowball);

				// Itens
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				updateItems(shaderID, item);
				drawSprite(shaderID, item);
			}
		}
		else
		{
			if (gameOver)
			{
				// Sufocou em neve
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				drawSprite(shaderID, gameOverSnow);
			}
			else
			{
				// Congelou
				glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0, 0.0);
				drawSprite(shaderID, gameOverCold);
			}
		}

		
		glBindVertexArray(0); //Desconectando o buffer de geometria

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	//glDeleteVertexArrays(1, character.VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
	{
		keys[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		keys[key] = false;
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
		//x     y     z     s     t
		//T0
		-0.5 , -0.5 , 0.0 , 0.0 , 0.0, //v0
		 0.5 , -0.5 , 0.0 , 1.0 , 0.0, //v1
		 0.0 ,  0.5 , 0.0 , 0.5 , 1.0  //v2				  
	};

	GLuint VBO, VAO;
	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 

	// Atributo posição - coord x, y, z - 3 valores
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Atributo coordenada de textura - coord s, t - 2 valores
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0); 

	return VAO;
}

// Inicialização da geometria de uma sprite
Sprite initializeSprite(GLuint texID, vec3 dimensions, vec3 position, float vel, int nAnimations, int nFrames, float angle)
{
	Sprite sprite;

	sprite.texID = texID;
	sprite.dimensions.x = dimensions.x / nFrames;
	sprite.dimensions.y = dimensions.y / nAnimations;
	sprite.pos = position;
	sprite.nAnimations = nAnimations;
	sprite.nFrames = nFrames;
	sprite.vel = vel;
	sprite.angle = angle;
	sprite.iFrame = 0;
	sprite.iAnimation = 0;

	sprite.ds = 1.0 / (float)nFrames;
	sprite.dt = 1.0 / (float)nAnimations;

	GLfloat vertices[] = {
		//x     y     z     s     t
		//T0
		-0.5 ,  0.5 , 0.0 , 0.0 , sprite.dt, //v0
		-0.5 , -0.5 , 0.0 , 0.0 , 0.0, //v1
		 0.5 ,  0.5 , 0.0 , sprite.ds, sprite.dt,  //v3	
		//T1
		-0.5 , -0.5 , 0.0 , 0.0 , 0.0, //v1
		 0.5 , -0.5 , 0.0 , sprite.ds, 0.0, //v2
		 0.5 ,  0.5 , 0.0 , sprite.ds, sprite.dt  //v3	
	};

	GLuint VBO, VAO;
	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo)

	// Atributo posição - coord x, y, z - 3 valores
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Atributo coordenada de textura - coord s, t - 2 valores
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0); 

	sprite.VAO = VAO;

    return sprite;
}

void drawTriangle(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, vec3 color, vec3 axis)
{
	//Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); //matriz identidade
	//Translação
	model = translate(model,position);
	//Rotação 
	model = rotate(model,radians(angle),axis);
	//Escala
	model = scale(model,dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	glUniform4f(glGetUniformLocation(shaderID, "inputColor"), color.r, color.g, color.b , 1.0f); //enviando cor para variável uniform inputColor
		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLES, 0, 3);

}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; //id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data) 
	{ 
		if (nrChannels== 3) //jpg, bmp    
		{ 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);    
		}    
		else // assume que é 4 canais png    
		{ 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    	}    
		glGenerateMipmap(GL_TEXTURE_2D); 
	} 
	else 
	{ 
		std::cout<< "Failed to load texture" << filePath << std::endl; 
	}

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

void drawSprite(GLuint shaderID, Sprite &sprite)
{
	glBindVertexArray(sprite.VAO); //Conectando ao buffer de geometria
	glBindTexture(GL_TEXTURE_2D, sprite.texID); // conectando com o buffer de textura que será usado no draw call 

	//Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); //matriz identidade
	//Translação
	model = translate(model,sprite.pos);
	//Rotação 
	model = rotate(model,radians(sprite.angle), vec3(0.0, 0.0, 1.0));
	//Escala
	model = scale(model,sprite.dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	// Chamada de desenho - drawcall
	// Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0); // Desconectando ao buffer de geometria
	glBindTexture(GL_TEXTURE_2D, 0); // Desconectando com o buffer de textura
}

void updateSprite(GLuint shaderID, Sprite &sprite)
{
	// Incrementa o índice do frame apenas quando fecha a taxa de FPS desejada
	float now = glfwGetTime();
	float dt = now - lastTime;
	if (dt >= 1 / FPS)
	{
		sprite.iFrame = (sprite.iFrame + 1) % sprite.nFrames;
		lastTime = now;
	}
	
	vec2 offsetTex;
	offsetTex.s = sprite.iFrame * sprite.ds;
	offsetTex.t = sprite.iAnimation * sprite.dt;

	glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t); // Enviando cor para a variável uniform offsetTex
}

void moveSprite(GLuint shaderID, Sprite &sprite)
{
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		if (sprite.pos.x - sprite.vel > 32)
		{
			sprite.pos.x -= sprite.vel;
		}
		sprite.iAnimation = 1;
	}
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		if (sprite.pos.x + sprite.vel < 768)
		{
			sprite.pos.x += sprite.vel;
		}
		sprite.iAnimation = 2;
	}
	if (!keys[GLFW_KEY_A] && !keys[GLFW_KEY_D] && !keys[GLFW_KEY_LEFT] && !keys[GLFW_KEY_RIGHT])
	{
		sprite.iAnimation = 3;
	}
}

void updateSnowball(GLuint shaderID, Sprite &sprite)
{
	sprite.vel += 0.000001;
	if (sprite.pos.y > 50)
	{
		sprite.pos.y -= sprite.vel;
	}
	else
	{
		sprite.pos.x = rand() % 737 + 32; // valor entre 32 e 768
		sprite.pos.y = rand() % 600 + 630;
    }
}

void updateItems(GLuint shaderID, Sprite & sprite)
{
	sprite.vel += 0.000001;
	if (sprite.pos.y > 50)
	{
		sprite.pos.y -= sprite.vel;
	}
	else
	{
		missedItems++;
		spawnItem(sprite);
    }
}

void spawnItem(Sprite &sprite)
{
	sprite.pos.x = rand() % 737 + 32; // valor entre 32 e 768
	sprite.pos.y = rand() % 600 + 630;
	sprite.texID = itemsTexIDs[rand() % sizeof(itemsTexIDs)/sizeof(*itemsTexIDs)];
}

void calculateAABB(Sprite &sprite)
{
	sprite.pMin.x = sprite.pos.x - sprite.dimensions.x / 2.0;
	sprite.pMin.y = sprite.pos.y - sprite.dimensions.y / 2.0;

	sprite.pMax.x = sprite.pos.x + sprite.dimensions.x / 2.0;
	sprite.pMax.y = sprite.pos.y + sprite.dimensions.y / 2.0;
}

bool checkCollision(Sprite &playerSprite, Sprite &itemSprite)
{
	calculateAABB(playerSprite);
	calculateAABB(itemSprite);
	// Colisão no eixo x
	bool collisionX = playerSprite.pMax.x >= itemSprite.pMin.x && itemSprite.pMax.x >= playerSprite.pMin.x;
	bool collisionY = playerSprite.pMax.y >= itemSprite.pMin.y && itemSprite.pMax.y >= playerSprite.pMin.y;

    return collisionX && collisionY;
}
