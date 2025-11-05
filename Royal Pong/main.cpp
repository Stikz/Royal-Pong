#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <fstream>

using namespace std;

const int WIDTH = 900;
const int HEIGHT = 720;
const int FONT_SIZE = 32;
const int BALL_SPEED = 16;
const int SPEED = 9;
const int SIZE = 16;
const int PI = 3.14159265358979323846;

SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;
SDL_Color color;
bool running;
int frameCount, timerFPS, lastFrame, fps;
Mix_Chunk* paddleHitSound = nullptr;
Mix_Chunk* wallHitSound = nullptr;
Mix_Chunk* moveSound = nullptr;
Mix_Chunk* removeSound = nullptr;

SDL_Texture* backgroundTexture = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
SDL_Texture* ballTexture = nullptr;
SDL_Texture* victoryTexture = nullptr;
SDL_Texture* loseTexture = nullptr;
SDL_Texture* royalPongTexture = nullptr;
vector<SDL_Texture*> personajeTextures;
SDL_Texture* charselectBackgroundTexture = nullptr;

SDL_Rect paddleIzquierdo, paddleDerecho, ball, score_board;
float velX, velY; // velocidad en x y de la pelota
string score; // texto con el puntaje
int l_s, r_s; // puntajes
bool turn; // turno para posicionar la pelota
vector<string> personajes;
SDL_Texture* playerCurrentTexture = nullptr;

// estados del juego
enum GameState {
	MENU,
	CHARSELECT,
	GAME,
	VICTORY,
	LOSE,
	EXIT
};



struct Personaje {
	SDL_Texture* sprite;
	std::string nombre;
};

int personajeSeleccionado = 0;

GameState currentState = MENU; // estado inicial es menu

// funcion que posiciona la pelota y reinicia paletas al anotar
void spawn() {
	// centrar paletas verticalmente
	paddleIzquierdo.y = paddleDerecho.y = (HEIGHT / 2) - (paddleIzquierdo.h / 2);

	if (turn) {
		// si es turno del jugador izq, posiciona la pelota a la derecha de la paleta izq y velocidad positiva
		ball.x = paddleIzquierdo.x + (paddleIzquierdo.w * 4);
		velX = BALL_SPEED / 2;
	}
	else {
		ball.x = paddleDerecho.x - (paddleDerecho.w * 4) - ball.w; // restar ancho para no salir del borde
		velX = -BALL_SPEED / 2;
	}
	velY = 0; // velocidad vertical empieza en cero
	ball.y = HEIGHT / 2 - (ball.h / 2); // centrar pelota verticalmente
	turn = !turn; // cambiar turno
}

// funcion para dibujar texto en pantalla en posicion x,y
void write(string text, int x, int y) {
	SDL_Surface* surface;
	SDL_Texture* texture;
	const char* t = text.c_str(); // convertir string a char*
	surface = TTF_RenderText_Solid(font, t, color);
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	score_board.w = surface->w; // ancho del texto
	score_board.h = surface->h; // alto del texto
	score_board.x = x - score_board.w; // posicion x ajustada para alinear a la derecha
	score_board.y = y - score_board.h; // posicion y ajustada para alinear arriba
	SDL_FreeSurface(surface);
	SDL_RenderCopy(renderer, texture, NULL, &score_board);
	SDL_DestroyTexture(texture);
}

int selectedOption = 0; // indice opcion seleccionada en menu

// bucle principal del menu
void menuLoop() {
	SDL_Event e;
	bool inMenu = true;
	const string options[] = { "Jugar", "Salir" };

	while (inMenu) {
		while (SDL_PollEvent(&e)) { // procesar eventos
			if (e.type == SDL_QUIT) {
				running = false; // salir si cierran ventana
				currentState = EXIT;
				return;
			}
			else if (e.type == SDL_KEYDOWN) { // entrada teclado
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					selectedOption = (selectedOption - 1 + 2) % 2;
					Mix_PlayChannel(-1, moveSound, 0); // sonido al mover selección
					break;
				case SDLK_DOWN:
					selectedOption = (selectedOption + 1) % 2;
					Mix_PlayChannel(-1, moveSound, 0); // sonido al mover selección
					break;
				case SDLK_RETURN:
					Mix_PlayChannel(-1, removeSound, 0); // sonido al confirmar selección
					if (selectedOption == 0) {
						currentState = CHARSELECT;
					}
					else {
						currentState = EXIT;
					}
					inMenu = false;
					break;

				}
			}
		}

		// dibujar fondo 
		if (backgroundTexture) {
			SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
		}
		// mostrar logo escalado y centrado
		if (royalPongTexture) {
			int logoW, logoH;
			SDL_QueryTexture(royalPongTexture, NULL, NULL, &logoW, &logoH); // obtiene los tamaños reales del png para no estirarlos
			float logoScale = 3.0f; // escala para tamaño del logo
			SDL_Rect logoRect;
			logoRect.w = static_cast<int>(logoW * logoScale); // convierte el resultado en un entero
			logoRect.h = static_cast<int>(logoH * logoScale);
			logoRect.x = WIDTH / 2 - logoRect.w / 2; // centrar horizontal
			logoRect.y = HEIGHT / 4 - logoRect.h / 2; // posicion vertical relativa
			SDL_RenderCopy(renderer, royalPongTexture, NULL, &logoRect);
		}

		// dibujar opciones del menu, resaltar seleccion con amarillo
		for (int i = 0; i < 2; ++i) {
			SDL_Color c = (i == selectedOption) ? SDL_Color{ 255, 255, 0 } : SDL_Color{ 255, 255, 255 }; // la opcion seleccionada es resaltada en amarillo
			SDL_Surface* surface = TTF_RenderText_Solid(font, options[i].c_str(), c);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect dst;
			dst.w = surface->w;
			dst.h = surface->h;
			dst.x = WIDTH / 2 - dst.w / 2;

			int totalOptions = 2;
			int spacing = 60; // espacio vertical entre opciones
			int menuHeight = totalOptions * spacing; // centra verticalmente las opciones
			dst.y = (HEIGHT / 2 - menuHeight / 2) + i * spacing; // posicion vertical espaciada. i * spacing mueve las opciones para abajo

			SDL_FreeSurface(surface);
			SDL_RenderCopy(renderer, texture, NULL, &dst);
			SDL_DestroyTexture(texture);
		}

		SDL_RenderPresent(renderer); // mostrar todo lo dibujado
		SDL_Delay(16); // delay para limitar fps aprox 60
	}
}

// funcion que carga los nombres de personajes desde un archivo de texto
bool cargarPersonajesDesdeArchivo(const string& path, vector<string>& personajes) {
	ifstream file(path); // abre el archivo en modo lectura
	if (!file.is_open()) {
		cout << "error al abrir " << path << endl;
		return false;
	}
	personajes.clear(); // limpia el vector por si ya tenia datos
	string line;
	while (getline(file, line)) { // lee cada linea del archivo
		if (!line.empty()) // si la linea no esta vacia
			personajes.push_back(line); // la agrega al vector
	}
	file.close(); // cierra el archivo
	return true; // devuelve true si se leyo correctamente
}

// bucle de seleccion de personaje
int characterSelectLoop(vector<string>& personajes, SDL_Texture* indicatorTex, Mix_Chunk* moveSound, Mix_Chunk* removeSound) {
	int selected = 0; 
	SDL_Event e;

	while (running) { 
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
				return -1;
			}
			else if (e.type == SDL_KEYDOWN) { 
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
					running = false;
					return -1;

				case SDLK_UP: 
					if (selected > 0) {
						selected--;
						Mix_PlayChannel(-1, moveSound, 0);
					}
					break;

				case SDLK_DOWN:
					if (selected < (int)personajes.size() - 1) {
						selected++;
						Mix_PlayChannel(-1, moveSound, 0);
					}
					break;

				case SDLK_RETURN: 
					Mix_PlayChannel(-1, removeSound, 0); 
					return selected;
				}
			}
		}

		SDL_RenderCopy(renderer, charselectBackgroundTexture, NULL, NULL);

		// muestra imagen del personaje seleccionado a la derecha
		if (selected >= 0 && selected < (int)personajeTextures.size() && personajeTextures[selected]) {
			float escala = 3.5f; // escala de la imagen

			int texW, texH;
			SDL_QueryTexture(personajeTextures[selected], NULL, NULL, &texW, &texH); // obtiene tamaño original

			int newW = static_cast<int>(texW * escala); // calcula nuevo ancho
			int newH = static_cast<int>(texH * escala); // calcula nuevo alto

			int posX = WIDTH - newW - 50; // posicion horizontal
			int posY = HEIGHT / 2 - newH / 2 + 30; // posicion vertical

			SDL_Rect destRect = { posX, posY, newW, newH }; // rectangulo de destino

			SDL_RenderCopy(renderer, personajeTextures[selected], NULL, &destRect);
		}

		// muestra la lista de nombres de personajes a la izquierda
		int y = 100;
		for (int i = 0; i < (int)personajes.size(); i++) {
			SDL_Color color = { 255, 255, 255, 255 };
			SDL_Surface* surface = TTF_RenderText_Solid(font, personajes[i].c_str(), color); 
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, surface);
			int w, h;
			SDL_QueryTexture(textTexture, NULL, NULL, &w, &h); // obtiene tamaño del texto
			SDL_Rect dst = { 120, y, w, h }; // posicion del texto
			SDL_RenderCopy(renderer, textTexture, NULL, &dst); // dibuja el nombre del personaje
			SDL_FreeSurface(surface);
			SDL_DestroyTexture(textTexture);

			if (i == selected && indicatorTex) { // si es el personaje seleccionado, dibuja el indicador
				SDL_Rect indicatorRect = { 80, y + 10, 32, 32 };
				SDL_RenderCopy(renderer, indicatorTex, NULL, &indicatorRect);
			}

			y += 50; // espacio entre personajes
		}

		string leyendaText = "UP/DOWN: seleccionar | ENTER: elegir | ESC: salir";
		SDL_Surface* leyendaSurface = TTF_RenderText_Solid(font, leyendaText.c_str(), { 200, 200, 200, 255 });
		SDL_Texture* leyendaTexture = SDL_CreateTextureFromSurface(renderer, leyendaSurface);
		int lw, lh;
		SDL_QueryTexture(leyendaTexture, NULL, NULL, &lw, &lh);
		SDL_Rect leyendaRect = { 20, HEIGHT - lh - 20, lw, lh };
		SDL_RenderCopy(renderer, leyendaTexture, NULL, &leyendaRect);
		SDL_FreeSurface(leyendaSurface);
		SDL_DestroyTexture(leyendaTexture);

		SDL_RenderPresent(renderer); // muestra todo lo dibujado
		SDL_Delay(16); // delay para limitar fps
	}

	return -1; // si se sale del bucle sin elegir
}


// bucle para pantalla de victoria o derrota con opciones
void victoryLoop() {
	SDL_Event e;
	int selected = 0;
	const string options[] = { "Volver a jugar", "Salir" };
	bool inVictory = true;

	while (inVictory) {
		while (SDL_PollEvent(&e)) { // procesar eventos
			if (e.type == SDL_QUIT) {
				running = false;
				currentState = EXIT;
				return;
			}
			else if (e.type == SDL_KEYDOWN) { // entrada teclado
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					selected = (selected - 1 + 2) % 2;
					Mix_PlayChannel(-1, moveSound, 0); // sonido al mover selección
					break;
				case SDLK_DOWN:
					selected = (selected + 1) % 2;
					Mix_PlayChannel(-1, moveSound, 0); // sonido al mover selección
					break;
				case SDLK_RETURN:
					Mix_PlayChannel(-1, removeSound, 0); // sonido al confirmar selección
					if (selected == 0) {
						l_s = r_s = 0;
						currentState = CHARSELECT;
					}
					else {
						currentState = EXIT;
					}
					inVictory = false;
					break;
				}
			}
		}

		// mostrar imagen de fondo segun estado victoria o derrota
		if (currentState == VICTORY && victoryTexture) {
			SDL_RenderCopy(renderer, victoryTexture, NULL, NULL);
		}
		else if (currentState == LOSE && loseTexture) {
			SDL_RenderCopy(renderer, loseTexture, NULL, NULL);
		}

		if (currentState == VICTORY) {
			SDL_Surface* surface = TTF_RenderText_Solid(font, "Ganaste", color);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect dst;
			dst.w = surface->w;
			dst.h = surface->h;
			dst.x = WIDTH / 2 - dst.w / 2;
			dst.y = HEIGHT / 3 - dst.h / 2;
			SDL_FreeSurface(surface);
			SDL_RenderCopy(renderer, texture, NULL, &dst);
			SDL_DestroyTexture(texture);
		}

		// mostrar texto "perdiste" si derrota
		if (currentState == LOSE) {
			SDL_Surface* surface = TTF_RenderText_Solid(font, "Perdiste", color);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect dst;
			dst.w = surface->w;
			dst.h = surface->h;
			dst.x = WIDTH / 2 - dst.w / 2;
			dst.y = HEIGHT / 3 - dst.h / 2;
			SDL_FreeSurface(surface);
			SDL_RenderCopy(renderer, texture, NULL, &dst);
			SDL_DestroyTexture(texture);
		}

		// dibujar opciones para volver a jugar o salir
		for (int i = 0; i < 2; ++i) {
			SDL_Color c = (i == selected) ? SDL_Color{ 255, 255, 0 } : SDL_Color{ 255, 255, 255 };
			SDL_Surface* surface = TTF_RenderText_Solid(font, options[i].c_str(), c);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect dst;
			dst.w = surface->w;
			dst.h = surface->h;
			dst.x = WIDTH / 2 - dst.w / 2;
			dst.y = HEIGHT / 2 + i * 60;
			SDL_FreeSurface(surface);
			SDL_RenderCopy(renderer, texture, NULL, &dst);
			SDL_DestroyTexture(texture);
		}

		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}
}

// movimiento pelota, colisiones, puntajes, IA
void update() {

	// colision pelota con paleta derecha
	if (SDL_HasIntersection(&ball, &paddleDerecho)) {
		Mix_PlayChannel(-1, paddleHitSound, 0);
		double rel = (paddleDerecho.y + (paddleDerecho.h / 2)) - (ball.y + (SIZE / 2)); // calcula el angulo en donde pega la pelota con la paleta
		double norm = rel / (paddleDerecho.h / 2); // divide rel por la mitad de la altura de la paleta. -1 = borde superior, 0 = centro, 1 = borde inferior
		double bounce = norm * (5 * PI / 12); // calcular angulo de rebote max 75 grados
		velX = -BALL_SPEED * cos(bounce); // cambia direccion a la izquierda y da proporcion horizontal de velocidad
		velY = BALL_SPEED * -sin(bounce); // da la proporcion vertical de velocidad
	}
	// colision pelota con paleta izquierda
	if (SDL_HasIntersection(&ball, &paddleIzquierdo)) {
		Mix_PlayChannel(-1, paddleHitSound, 0);
		double rel = (paddleIzquierdo.y + (paddleIzquierdo.h / 2)) - (ball.y + (SIZE / 2));
		double norm = rel / (paddleIzquierdo.h / 2);
		double bounce = norm * (5 * PI / 12);
		velX = BALL_SPEED * cos(bounce);
		velY = BALL_SPEED * -sin(bounce);
	}
	// IA para mover paleta derecha hacia posicion de la pelota
	if (ball.y > paddleDerecho.y + (paddleDerecho.h / 2)) // si la pelota esta por encima del paddle derecho, el paddle derecho se mueve para arriba
		paddleDerecho.y += SPEED;
	if (ball.y < paddleDerecho.y + (paddleDerecho.h / 2))
		paddleDerecho.y -= SPEED;

	// colisiones con bordes de pantalla 
	if (ball.x <= 0) {
		r_s++; // sumar punto jugador derecho
		Mix_PlayChannel(-1, wallHitSound, 0); // sonido pared
		spawn(); // reiniciar posicion pelota y paletas
	}
	if (ball.x + SIZE >= WIDTH) { // si el borde derecho de la "pelota" esta fuera de pantalla, punto para el jugador
		l_s++; // sumar punto jugador izquierdo
		Mix_PlayChannel(-1, wallHitSound, 0);
		spawn();
	}

	// rebote pelota en borde superior e inferior
	if (ball.y <= 0 || ball.y + SIZE >= HEIGHT) velY = -velY; // si la pelota esta arriba del todo, rebota para abajo y viceversa

	// mover pelota con velocidad 
	ball.x += velX;
	ball.y += velY;

	// actualizar texto de puntaje
	score = to_string(l_s) + "   " + to_string(r_s); // convierte los puntajes en texto

	// evitar que paletas salgan de la pantalla verticalmente
	if (paddleIzquierdo.y < 0)paddleIzquierdo.y = 0;
	if (paddleIzquierdo.y + paddleIzquierdo.h > HEIGHT)paddleIzquierdo.y = HEIGHT - paddleIzquierdo.h; // limita el borde inferior del paddle con el alto de la ventana
	if (paddleDerecho.y < 0)paddleDerecho.y = 0;
	if (paddleDerecho.y + paddleDerecho.h > HEIGHT)paddleDerecho.y = HEIGHT - paddleDerecho.h;

	// si algun jugador llega a 7 puntos, cambiar estado a victoria o derrota
	if (l_s >= 7) { currentState = VICTORY; }
	if (r_s >= 7) { currentState = LOSE; }
}

// procesa entrada del jugador para mover paleta izquierda y salir con ESC
void input() {
	SDL_Event e;
	const Uint8* keystates = SDL_GetKeyboardState(NULL);
	while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = false; // salir si cierran ventana
	if (keystates[SDL_SCANCODE_ESCAPE]) running = false; // salir con ESC
	if (keystates[SDL_SCANCODE_UP]) paddleIzquierdo.y -= SPEED; // mover paleta arriba
	if (keystates[SDL_SCANCODE_DOWN]) paddleIzquierdo.y += SPEED; // mover paleta abajo
}

// renderiza todos los elementos en pantalla: fondo, paletas, pelota, texto
void render() {
	if (backgroundTexture) { SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL); }
	if (playerCurrentTexture) {SDL_RenderCopy(renderer, playerCurrentTexture, NULL, &paddleIzquierdo);
	}
	if (enemyTexture) { SDL_RenderCopy(renderer, enemyTexture, NULL, &paddleDerecho); }

	// controlar fps para limitar a 60 cuadros por segundo
	frameCount++;
	timerFPS = SDL_GetTicks() - lastFrame;
	if (timerFPS < (1000 / 60)) {
		SDL_Delay((1000 / 60) - timerFPS); // hace delay para que el frame dure exactamente 1/60 segundos (60fps)
	}

	// dibujar pelota con textura y volteo horizontal si va hacia la izquierda
	if (ballTexture) {
		SDL_RendererFlip flip = SDL_FLIP_NONE;
		if (velX < 0) {
			flip = SDL_FLIP_HORIZONTAL;  // da vuelta el sprite si va a la izquierda
		}
		SDL_RenderCopyEx(renderer, ballTexture, NULL, &ball, 0, NULL, flip);
	}
	// dibujar puntaje en pantalla
	write(score, WIDTH / 2 + FONT_SIZE, FONT_SIZE * 2);
	SDL_RenderPresent(renderer);
}



int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {cout << "SDL_image no pudo inicializarse: " << IMG_GetError() << endl;
		return -1;
	}
	TTF_Init();
	font = TTF_OpenFont("Peepo.ttf", FONT_SIZE);
	if (!cargarPersonajesDesdeArchivo("characters.txt", personajes)) {cout << "Error cargando personajes" << endl;
		running = false;
	}
	running = true;
	SDL_Surface* indicatorSurface = IMG_Load("indicator.png");
	SDL_Texture* indicatorTex = SDL_CreateTextureFromSurface(renderer, indicatorSurface);
	SDL_FreeSurface(indicatorSurface);

	// cargar imagen de fondo
	SDL_Surface* bgSurface = IMG_Load("Background.png");
	backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
	SDL_FreeSurface(bgSurface);

	SDL_Surface* defaultPlayerSurface = IMG_Load("Player.png");
	SDL_Texture* defaultPlayerTexture = SDL_CreateTextureFromSurface(renderer, defaultPlayerSurface);
	SDL_FreeSurface(defaultPlayerSurface);
	playerCurrentTexture = defaultPlayerTexture;

	SDL_Surface* azulPlayerSurface = IMG_Load("Azulplayer.png");
	SDL_Texture* azulPlayerTexture = SDL_CreateTextureFromSurface(renderer, azulPlayerSurface);
	SDL_FreeSurface(azulPlayerSurface);

	SDL_Surface* undeadPlayerSurface = IMG_Load("UndeadPlayer.png");
	SDL_Texture* undeadPlayerTexture = SDL_CreateTextureFromSurface(renderer, undeadPlayerSurface);
	SDL_FreeSurface(undeadPlayerSurface);


	// cargar imagen del enemigo
	SDL_Surface* enemySurface = IMG_Load("Enemy.png");
	enemyTexture = SDL_CreateTextureFromSurface(renderer, enemySurface);
	SDL_FreeSurface(enemySurface);

	// cargar imagen de la pelota
	SDL_Surface* ballSurface = IMG_Load("Ball.png");
	ballTexture = SDL_CreateTextureFromSurface(renderer, ballSurface);
	SDL_FreeSurface(ballSurface);

	// cargar imagen de victoria
	SDL_Surface* victorySurface = IMG_Load("Victory.png");
	victoryTexture = SDL_CreateTextureFromSurface(renderer, victorySurface);
	SDL_FreeSurface(victorySurface);

	// cargar imagen de derrota
	SDL_Surface* loseSurface = IMG_Load("Lose.png");
	loseTexture = SDL_CreateTextureFromSurface(renderer, loseSurface);
	SDL_FreeSurface(loseSurface);

	// cargar imagen del logo
	SDL_Surface* royalSurface = IMG_Load("Royalpong.png");
	royalPongTexture = SDL_CreateTextureFromSurface(renderer, royalSurface);
	SDL_FreeSurface(royalSurface);

	SDL_Surface* charselectSurface = IMG_Load("Charselect.png");
	charselectBackgroundTexture = SDL_CreateTextureFromSurface(renderer, charselectSurface);
	SDL_FreeSurface(charselectSurface);

	// Cargar texturas de personajes (mismo orden que characters.txt)
	for (const auto& nombre : personajes) {
		string filename = nombre + ".png"; // ej: Default.png
		SDL_Surface* surf = IMG_Load(filename.c_str()); // intenta cargar la imagen del personaje
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf); 
		personajeTextures.push_back(tex); // agrega al vector
		SDL_FreeSurface(surf); // libera la superficie temporal	
	}

	playerCurrentTexture = defaultPlayerTexture; // establece el personaje por defecto como textura inicial del jugador

	// definir tamaño de la pelota escalada
	int ballW, ballH;
	SDL_QueryTexture(ballTexture, NULL, NULL, &ballW, &ballH); // obtiene ancho y alto original
	float ballScale = 3.0f; // escala deseada
	ball.w = (int)(ballW * ballScale); // aplica escala al ancho
	ball.h = (int)(ballH * ballScale); // aplica escala al alto

	// establecer tamaño de paddle izquierdo (jugador) usando textura seleccionada
	int w, h;
	SDL_QueryTexture(playerCurrentTexture, NULL, NULL, &w, &h); // obtiene ancho y alto de textura actual del jugador
	float scale = 3.0f; // escala deseada para el sprite del jugador

	paddleIzquierdo = {
		32, // posición X fija a la izquierda
		HEIGHT / 2 - (int)((h * scale) / 2), // centrado verticalmente
		(int)(w * scale), // ancho escalado
		(int)(h * scale)  // alto escalado
	};

	// establecer tamaño de paddle derecho (enemigo)
	int enemyW, enemyH;
	SDL_QueryTexture(enemyTexture, NULL, NULL, &enemyW, &enemyH); // obtiene tamaño de textura del enemigo
	paddleDerecho = {
		WIDTH - (int)(enemyW * scale) - 32, // posición a la derecha con margen
		HEIGHT / 2 - (int)((enemyH * scale) / 2), // centrado vertical
		(int)(enemyW * scale), // ancho escalado
		(int)(enemyH * scale)  // alto escalado
	};

	// blanco para los textos
	color = { 255, 255, 255 }; 

	// iniciar puntajes en 0
	l_s = r_s = 0;

	// cargar sonidos
	paddleHitSound = Mix_LoadWAV("PaddleHit.WAV");
	wallHitSound = Mix_LoadWAV("WallHit.wav");
	moveSound = Mix_LoadWAV("move.wav");
	removeSound = Mix_LoadWAV("remove.wav");

	menuLoop();

while (running) {
	if (currentState == MENU) {
		menuLoop();
	}
	else if (currentState == CHARSELECT) {
		int seleccionado = characterSelectLoop(personajes, indicatorTex, moveSound, removeSound);
		if (seleccionado == -1) {
			currentState = MENU;
		}
		else {
			personajeSeleccionado = seleccionado;

			// Elegir la textura según el índice
			if (personajeSeleccionado == 0) {
				playerCurrentTexture = defaultPlayerTexture;
			}
			else if (personajeSeleccionado == 1) {
				playerCurrentTexture = azulPlayerTexture;
			}
			else if (personajeSeleccionado == 2) {
				playerCurrentTexture = undeadPlayerTexture;
			}
			else {
				playerCurrentTexture = defaultPlayerTexture;
			}

			// Recalcular tamaño de paddle izquierdo según nuevo personaje
			int w, h;
			SDL_QueryTexture(playerCurrentTexture, NULL, NULL, &w, &h);
			float scale = 3.0f;
			paddleIzquierdo = {
				32,
				HEIGHT / 2 - (int)((h * scale) / 2),
				(int)(w * scale),
				(int)(h * scale)
			};

			currentState = GAME;
		}
	}
	else if (currentState == GAME) {
		spawn();
		static int lastTime = 0;
		while (running && currentState == GAME) {
			lastFrame = SDL_GetTicks();
			if (lastFrame >= (lastTime + 1000)) {
				lastTime = lastFrame;
				fps = frameCount;
				frameCount = 0;
			}
			update();
			input();
			render();

			if (currentState == VICTORY || currentState == LOSE) {
				victoryLoop(); // cambia el estado internamente
			}
		}
	}
	else if (currentState == EXIT) {
		break; // salir del bucle principal
	}
}

	// limpieza de recursos al salir
	TTF_CloseFont(font);
	if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
	if (playerTexture) SDL_DestroyTexture(playerTexture);
	if (enemyTexture) SDL_DestroyTexture(enemyTexture);
	if (ballTexture) SDL_DestroyTexture(ballTexture);
	if (victoryTexture) SDL_DestroyTexture(victoryTexture);
	if (loseTexture) SDL_DestroyTexture(loseTexture);
	if (royalPongTexture) SDL_DestroyTexture(royalPongTexture);
	if (charselectBackgroundTexture) SDL_DestroyTexture(charselectBackgroundTexture);

	for (auto tex : personajeTextures) {
		if (tex) SDL_DestroyTexture(tex);
	}
	SDL_DestroyTexture(defaultPlayerTexture);
	SDL_DestroyTexture(azulPlayerTexture);
	SDL_DestroyTexture(undeadPlayerTexture);
	SDL_DestroyTexture(indicatorTex);
	Mix_FreeChunk(moveSound);
	Mix_FreeChunk(removeSound);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	Mix_FreeChunk(paddleHitSound);
	Mix_FreeChunk(wallHitSound);
	Mix_CloseAudio();
	SDL_Quit();
	return 0;
}
