#include <iostream>
#include "ShaderMaker.h"
#include <GL/glew.h>
#include <vector>
#include <GL/freeglut.h>

static unsigned int programId;
#define PI 3.14159265358979323846
#define RAGGIO 15
#define GRID_X 8
#define GRID_Y 5
#define PADDING_X 0.0222
#define PADDING_Y 0.0222

unsigned int VAO_ONDA, VAO_PALLA, VAO_MARE, VAO_CIELO, VAO_SOLE, VAO_SABBIA, VAO_PADDLE, VAO_SCIA, VAO_BRICKS;
unsigned int VBO_ONDA, VBO_Pa, MatProj, MatModel, VBO_C, VBO_S, VBO_SABBIA, VBO_MARE, VBO_PADDLE, VBO_SCIA, VBO_BRICKS;

// Effetto scia
typedef struct {
	float r;
	float g;
	float b;
} color;

typedef struct {
	float x;
	float y;
	float alpha;
	float xFactor;
	float yFactor;
	float drag;
	color color;
} PARTICLE;

vector <PARTICLE> particles;

// Include GLM; libreria matematica per le opengl
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

mat4 Projection;  //Matrice di proiezione
mat4 Model; //Matrice per il cambiamento di sistema di riferimento: 

// viewport size
int width = 1280;
int height = 720;

bool bricks_hitted[GRID_X][GRID_Y] = { false };

float angolo = 0.0;
typedef struct { float x, y, z, r, g, b, a; } Point;
int Numero_di_pezzi = 128;
int nVertices_onde = 2 * Numero_di_pezzi;
Point* Punti = new Point[nVertices_onde];
int nTriangles_palla = 30;
int vertices_palla = 3 * 2 * nTriangles_palla;
Point* Palla = new Point[vertices_palla];
int vertices_Cielo = 6;
Point* Cielo = new Point[vertices_Cielo];
int vertices_Paddle = 6;
Point* Paddle = new Point[vertices_Paddle];
int vertices_Mare = 6;
Point* Mare = new Point[vertices_Mare];
int vertices_sabbia = 6;
Point* Sabbia = new Point[vertices_sabbia];
int nTriangles_sole = 30;
int vertices_sole = 3 * 2 * nTriangles_sole;
Point* Sole = new Point[vertices_sole];
auto bricks = new Point[6];

int nPointScia = 5000;
Point* PuntiScia = new Point[nPointScia];

// Distanza del paddle dal centro della scena
int start_paddle = 0;

bool stop_game = false;
// parametri della palla
double	VelocitaOrizzontale = -8; //velocita orizzontale (pixel per frame)
double	VelocitaVerticale = 8; //velocita verticale (pixel per frame)
double	accelerazione = 0.1;	// forza di accelerazione data dalla tastiera
/*
	Il centro della sfera è puntato in (posx, posy)
	che sono anche le coordinate sul piano della posizione iniziale della palla
*/
float	posx = start_paddle + float(width) / 2;
float posy = float(height) * 0.05 + float(height) * 0.05 + RAGGIO;
int score = 0;

// Azione compiuta dall'utente
bool pressing_left = false;
bool pressing_right = false;
bool restart = false;
bool play = false;
bool victory = false;

vec4 col_rosso = { 1.0,0.0,0.0,1.0 };
vec4 col_nero = { 0.0,0.0,0.0,1.0 };

//Movimento onda
int start_onda[] = { 0, 70 };
color computeRainbow();
/// ///////////////////////////////////////////////////////////////////////////////////
///									Gestione eventi
///////////////////////////////////////////////////////////////////////////////////////
void keyboardPressedEvent(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'a':
		pressing_left = true;
		break;
	case 'd':
		pressing_right = true;
		break;
	case 'r':
		restart = true;
		break;
	case 'p':
		play = true;
		break;
	case 27:
		exit(0);
	default:
		break;
	} 
}

void keyboardReleasedEvent(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'a':
		pressing_left = false;
		break;
	case 'd':
		pressing_right = false;
		break;
	case 'r':
		restart = true;
		break;
	case 'p':
		play = true;
		break;
	default:
		break;
	}
}

// Creo le particelle
void update_fireworks(int x) {
	color rgb = computeRainbow();
	for (int i = 0; i < 10; i++) {
		PARTICLE p;
		p.x = width / 2;
		p.y = height / 2;
		p.alpha = 1.0;
		p.drag = 1.05;
		p.xFactor = (rand() % 1000 + 1) / 300 * (rand() % 2 == 0 ? -1 : 1);
		p.yFactor = (rand() % 1000 + 1) / 300 * (rand() % 2 == 0 ? -1 : 1);
		p.color.r = rgb.r;
		p.color.g = rgb.g;
		p.color.b = rgb.b;
		particles.push_back(p);
	}
	if (victory) {
		glutTimerFunc(24, update_fireworks, 0);
	}
}

/// ///////////////////////////////////////////////////////////////////////////////////
///									Funzioni di supporto
///////////////////////////////////////////////////////////////////////////////////////

color computeRainbow() {
	static float rgb[3] = { 1.0, 0.0, 0.0 };
	static int fase = 0, counter = 0;
	const float step = 0.1;
	color paint;

	switch (fase) {
	case 0: rgb[1] += step;
		break;
	case 1: rgb[0] -= step;
		break;
	case 2: rgb[2] += step;
		break;
	case 3: rgb[1] -= step;
		break;
	case 4: rgb[0] += step;
		break;
	case 5: rgb[2] -= step;
		break;
	default:
		break;
	}
	//fprintf(stdout, "Rosso e verde e blu: %f,%f,%f, counter= %i\n", rgb[0], rgb[1], rgb[2], counter);

	counter++;
	if (counter > 1.0 / step) {
		counter = 0;
		fase < 5 ? fase++ : fase = 0;
	}

	paint.r = rgb[0];
	paint.g = rgb[1];
	paint.b = rgb[2];
	return paint;
}

// Ottengo l'equazione della retta a partire da due punti
void getLine(double x1, double y1, double x2, double y2, double& a, double& b, double& c)
{
	// (x- p1X) / (p2X - p1X) = (y - p1Y) / (p2Y - p1Y) 
	a = y1 - y2; 
	b = x2 - x1;
	c = x1 * y2 - x2 * y1;
}
// Calcolo la distanza tra il punto (px_1, py_1) e la retta passante per (px_2, py_2) e (px_3, py_3) 
double dist(double pct1X, double pct1Y, double pct2X, double pct2Y, double pct3X, double pct3Y)
{
	double a, b, c;
	getLine(pct2X, pct2Y, pct3X, pct3Y, a, b, c);
	return abs(a * pct1X + b * pct1Y + c) / sqrt(a * a + b * b);
}

double lerp(double a, double b, double amount) {
	//Interpolazione lineare tra a e b secondo amount
	return (1 - amount) * a + amount * b;
}

// Se la distanza punto retta è minore del raggio allora controllo se la pallina stia colpendo 
// la sinistra, destra o centro del paddle e compio un'azione
bool check_intersection(double distance, float pos, double coord_1, double coord_2, int i, int j) {
	if (distance < RAGGIO && pos > coord_1 && pos < coord_2) {
		bricks_hitted[i][j] = true;
		double dim_block = abs(coord_2 - coord_1);
		if (posx < coord_1 + dim_block / 3) {
			VelocitaVerticale *= -1;
			VelocitaOrizzontale += accelerazione;
			if (abs(VelocitaOrizzontale) > RAGGIO) {
				VelocitaOrizzontale = 10;
			}
		}
		else if (posx > coord_1 + dim_block / 3 && posx < (coord_1 + dim_block) * (2 / 3)) {
			VelocitaVerticale *= -1;
		}
		else {
			VelocitaVerticale *= -1;
			VelocitaVerticale += accelerazione;
			if (abs(VelocitaVerticale) > RAGGIO) {
				VelocitaVerticale = 10;
			}
		}
		score++;
		printf("Punteggio attuale: %d \n", score);
		return true;
	}
	return false;
}

// Ottengo il numero di particellari in scena
int getParticle() {

	int P_size = 0; // particles.size();

	for (int i = 0; i < particles.size(); i++) {
		particles.at(i).xFactor /= particles.at(i).drag;
		particles.at(i).yFactor /= particles.at(i).drag;

		particles.at(i).x += particles.at(i).xFactor;
		particles.at(i).y += particles.at(i).yFactor;

		particles.at(i).alpha -= 0.05;

		float xPos = -1.0f + ((float)particles.at(i).x) * 2 / ((float)(width));
		float yPos = -1.0f + ((float)(particles.at(i).y)) * 2 / ((float)(height));

		if (particles.at(i).alpha <= 0.0) {
			particles.erase(particles.begin() + i);
		}
		else {
			PuntiScia[i].x = xPos;
			PuntiScia[i].y = yPos;
			PuntiScia[i].z = 0.0;
			PuntiScia[i].r = particles.at(i).color.r;
			PuntiScia[i].g = particles.at(i).color.g;
			PuntiScia[i].b = particles.at(i).color.b;
			PuntiScia[i].a = particles.at(i).alpha;
			P_size += 1;
		}
	}
	return P_size;
}
/// ///////////////////////////////////////////////////////////////////////////////////
///									Animazione
///////////////////////////////////////////////////////////////////////////////////////

void update(int a)
{
	// Se l'utente ha chiesto di ricominciare la partita
	if (restart) {
		printf("Punteggio partita precedente %d \n", score);
		start_paddle = 0;
		posx = float(width) / 2;
		posy = float(height) * 0.05 + float(height) * 0.05 + RAGGIO;
		restart = false;
		stop_game = false;
		score = 0;
		VelocitaOrizzontale = -8;
		play = false;
		VelocitaVerticale = -8;
		victory = false;
		for (int i = 0; i < GRID_X; i++) {
			for (int j = 0; j < GRID_Y; j++) {
				bricks_hitted[i][j] = false;
			}
		}
		printf("Nuova partita iniziata \n");
	}
	else {
		// Se la sessione di gioco è attiva
		if (!stop_game) {
			// Se l'utente ha premuto il tasto sinistro
			if (pressing_left) {
				start_paddle -= 20;
				if (start_paddle + float(width) / 2 * 0.83 < 0) {
					start_paddle = -1 * float(width) / 2 * 0.83;
				}
			}
			// Se l'utente ha premuto il tasto destro
			if (pressing_right) {
				start_paddle += 20;
				if (start_paddle + float(width) / 2 * 0.83 > float(width) - float(width) * 0.17) {
					start_paddle = float(width) / 2 * 0.83;
				}
			}
			// Se la pallina è ormai uscita fuori dalla zona giocabile
			if (posy < float(height) * 0.05) {
				printf("Partita finita con punteggio %d \n", score);
				stop_game = true;
			}

			//Aggioramento della posizione in x della pallina, che regola il movimento orizzontale
			if (play) {
				posx += VelocitaOrizzontale;
				posy += VelocitaVerticale;
			}
			// Controllo se c'è intersezione tra la sfera e il paddle
			double p1_x = start_paddle + float(width) / 2 * 0.83;
			double p1_y = float(height) * 0.05 + float(height) * 0.05;
			double p2_x = start_paddle + float(width) / 2 * 0.83 + float(width) * 0.17;
			double p2_y = p1_y;
			double distance = dist(posx, posy, p1_x, p1_y, p2_x, p2_y) + 0.1;

			if (distance <= double(RAGGIO) && posx > p1_x && posx < p2_x) 
			{
				double dim_block = abs(p2_x - p1_x);
				if (posx < p1_x + dim_block / 3) {
					VelocitaVerticale *= -1;
					VelocitaOrizzontale += accelerazione;
					if (abs(VelocitaOrizzontale) > RAGGIO) {
						VelocitaOrizzontale = RAGGIO;
					}
				}
				else if (posx > p1_x + dim_block / 3 && posx < (p1_x + dim_block) * (2 / 3)) {
					VelocitaVerticale *= -1;
				}
				else {
					VelocitaVerticale *= -1;
					VelocitaVerticale += accelerazione;
					if (abs(VelocitaVerticale) > RAGGIO) {
						VelocitaVerticale = RAGGIO;
					}
				}
			}
			//Se la palla urta i bordi dello schermo 
			// ovvero assume una posizione x<0 o x> width (bordi viewport)
			if (posx < 30) {
				posx = 30;
				VelocitaOrizzontale *= -1;
			}
			if (posx > width - RAGGIO) {
				posx = width - RAGGIO;
				VelocitaOrizzontale *= -1;
			}

			if (posy < 0) {
				posy = 0;
				VelocitaVerticale *= -1;
			}

			if (posy > height - 30) {
				posy = height - 30;
				VelocitaVerticale *= -1;
			}
			// Controllo se la palla sta intersecando un blocco
			float dim_x = float(width) * 0.1;
			float dim_y = float(height) * 0.07;
			for (int i = 0; i < GRID_X; i++) {
				for (int j = 0; j < GRID_Y; j++) {
					if (!bricks_hitted[i][j]) {
						// Distanza dalla linea frontale
						double p1_x = float(i) * dim_x + float(i + 1) * float(width) * PADDING_X;
						double p1_y = float(height) - float(j) * dim_y - dim_y - PADDING_Y * float(height) - j * PADDING_Y * float(height) - RAGGIO;
						double p2_x = p1_x + dim_x;
						double p2_y = p1_y;
						double distance = dist(posx, posy, p1_x, p1_y, p2_x, p2_y);
						// Se la distanza tra la linea frontale e il centro della palla non è minore del raggio
						if (!check_intersection(distance, posx, p1_x, p2_x, i, j)) {
							p1_x = float(i) * dim_x + float(i + 1) * float(width) * PADDING_X;
							p1_y = float(height) - float(j) * dim_y - dim_y - PADDING_Y * float(height) - j * PADDING_Y * float(height) + dim_y - RAGGIO;
							p2_x = p1_x + dim_x;
							p2_y = p1_y;
							distance = dist(posx, posy, p1_x, p1_y, p2_x, p2_y);
							// Se la distanza tra la linea parallela alla frontale e il centro della palla non è minore del raggio
							if (!check_intersection(distance, posx, p1_x, p2_x, i, j)) {
								// Se la distanza tra la linea laterale sinistra e il centro della palla è minore del raggio
								p1_x = float(i) * dim_x + float(i + 1) * float(width) * PADDING_X;
								p1_y = float(height) - float(j) * dim_y - dim_y - PADDING_Y * float(height) - j * PADDING_Y * float(height) - RAGGIO;
								p2_x = p1_x;
								p2_y = p1_y + dim_y;
								distance = dist(posx, posy, p1_x, p1_y, p2_x, p2_y);
								if (!check_intersection(distance, posy, p1_y, p2_y, i, j)) {
									// Se la distanza tra la linea laterale destra e il centro della palla non è minore del raggio
									p1_x = float(i) * dim_x + float(i + 1) * float(width) * PADDING_X + dim_x;
									p1_y = float(height) - float(j) * dim_y - dim_y - PADDING_Y * float(height) - j * PADDING_Y * float(height) - RAGGIO;
									p2_x = p1_x;
									p2_y = p1_y + dim_y;
									distance = dist(posx, posy, p1_x, p1_y, p2_x, p2_y);
									check_intersection(distance, posy, p1_y, p2_y, i, j);
								}
							}
						}
					}
				}
			}
			// Se ho colpito tutti i brick
			if (score == GRID_X * GRID_Y) {
				victory = true;
				stop_game = true;
				printf("Complimenti! Hai vinto. Premi 'R' per ricominciare a giocare");
				glutTimerFunc(24, update_fireworks, 0);
			}
		}
	}

	// Se la partita non è iniziata la pallina si muove con il paddle
	if (!play) {
		posx = start_paddle + float(width) / 2;
	}

	glutPostRedisplay();
	glutTimerFunc(24, update, 0);
}

// Aggiorna la posizione delle onde
void update_wave(int a) {
	for (int i = 0; i < 2; i++) {
		start_onda[i] += 30;
		if (start_onda[i] > ((float)0.3 * height)) {
			start_onda[i] = 0;
		}
	}
	glutTimerFunc(500, update_wave, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////// 

void disegna_piano(float x, float y, float width, float height, vec4 color_top, vec4 color_bot, Point* piano)
{
	piano[0].x = x;	piano[0].y = y;piano[0].z = 0;
	piano[0].r = color_bot.r; piano[0].g = color_bot.g; piano[0].b = color_bot.b; piano[0].a = color_bot.a;
	piano[1].x = x + width;	piano[1].y = y;	piano[1].z = 0;
	piano[1].r = color_top.r; piano[1].g = color_top.g; piano[1].b = color_top.b; piano[1].a = color_top.a;
	piano[2].x = x + width;	piano[2].y = y + height; piano[2].z = 0;
	piano[2].r = color_bot.r; piano[2].g = color_bot.g; piano[2].b = color_bot.b; piano[2].a = color_bot.a;

	piano[3].x = x + width;	piano[3].y = y + height; piano[3].z = 0;
	piano[3].r = color_bot.r; piano[3].g = color_bot.g; piano[3].b = color_bot.b; piano[3].a = color_bot.a;
	piano[4].x = x;	piano[4].y = y + height; piano[4].z = 0;
	piano[4].r = color_top.r; piano[4].g = color_top.g; piano[4].b = color_top.b; piano[4].a = color_top.a;
	piano[5].x = x;	piano[5].y = y; piano[5].z = 0;
	piano[5].r = color_bot.r; piano[5].g = color_bot.g; piano[5].b = color_bot.b; piano[5].a = color_bot.a;
}

void disegna_cerchio(int nTriangles, int step, vec4 color_top, vec4 color_bot, Point* Cerchio) {
	int i;
	float stepA = (2 * PI) / nTriangles;

	int comp = 0;
	// step = 1 -> triangoli adiacenti, step = n -> triangoli distanti step l'uno dall'altro
	for (i = 0; i < nTriangles; i += step)
	{
		Cerchio[comp].x = cos((double)i * stepA);
		Cerchio[comp].y = sin((double)i * stepA);
		Cerchio[comp].z = 0.0;
		Cerchio[comp].r = color_top.r; Cerchio[comp].g = color_top.g; Cerchio[comp].b = color_top.b; Cerchio[comp].a = color_top.a;

		Cerchio[comp + 1].x = cos((double)(i + 1) * stepA);
		Cerchio[comp + 1].y = sin((double)(i + 1) * stepA);
		Cerchio[comp + 1].z = 0.0;
		Cerchio[comp + 1].r = color_top.r; Cerchio[comp + 1].g = color_top.g; Cerchio[comp + 1].b = color_top.b; Cerchio[comp + 1].a = color_top.a;

		Cerchio[comp + 2].x = 0.0;
		Cerchio[comp + 2].y = 0.0;
		Cerchio[comp + 2].z = 0.0;
		Cerchio[comp + 2].r = color_bot.r; Cerchio[comp + 2].g = color_bot.g; Cerchio[comp + 2].b = color_bot.b; Cerchio[comp + 2].a = color_bot.a;

		comp += 3;
	}
}

void disegna_onda(float x0, float y0, int altezza_onda, int larghezza_onda, int numero_di_creste, vec4 color_top, vec4 color_bot, Point* v_montagna)
{
	int i = 0;

	float dimensione_pezzi = larghezza_onda / (float)Numero_di_pezzi;
	float frequenza = PI * numero_di_creste;

	int n_vertici = 0;
	for (i = 0; i < Numero_di_pezzi; i += 1)
	{

		v_montagna[n_vertici].x = x0 + i * dimensione_pezzi;
		v_montagna[n_vertici].y = -1 * y0;
		v_montagna[n_vertici].z = 0;
		v_montagna[n_vertici].r = color_bot.r; v_montagna[n_vertici].g = color_bot.g; v_montagna[n_vertici].b = color_bot.b; v_montagna[n_vertici].a = color_bot.a;
		v_montagna[n_vertici + 1].x = x0 + i * dimensione_pezzi;
		v_montagna[n_vertici + 1].y = -1 * (y0 + altezza_onda * abs(sin(i / (float)Numero_di_pezzi * frequenza)));
		v_montagna[n_vertici + 1].z = 0;
		v_montagna[n_vertici + 1].r = color_top.r; v_montagna[n_vertici + 1].g = color_top.g; v_montagna[n_vertici + 1].b = color_top.b; v_montagna[n_vertici + 1].a = color_top.a;

		n_vertici += 2;
	}
}

void disegna_sole(int nTriangles, Point* Sole) {
	int i, cont;
	Point* OutSide;
	int vertici = 3 * nTriangles;
	OutSide = new Point[vertici];

	vec4 col_top_sole = { 1.0, 1.0, 1.0, 1.0 };
	vec4 col_bottom_sole = { 1.0, 0.8627, 0.0, 1.0 };
	disegna_cerchio(nTriangles, 1, col_top_sole, col_bottom_sole, Sole);

	col_top_sole = { 1.0, 1.0, 1.0, 0.0 };
	col_bottom_sole = { 1.0, 0.8627, 0.0, 1.0 };
	disegna_cerchio(nTriangles, 1, col_top_sole, col_bottom_sole, OutSide);

	cont = 3 * nTriangles;
	for (i = 0; i < 3 * nTriangles; i++)
	{
		Sole[cont + i].x = OutSide[i].x;
		Sole[cont + i].y = OutSide[i].y;
		Sole[cont + i].z = OutSide[i].z;
		Sole[cont + i].r = OutSide[i].r;Sole[cont + i].g = OutSide[i].g;Sole[cont + i].b = OutSide[i].b;Sole[cont + i].a = OutSide[i].a;
	}
}

void disegna_sabbia(void) {
	vec4 col_top = { 0.92,0.77, 0.36,1.0 };
	// rgb(237,200,85)
	vec4 col_bottom = { 0.92 , 0.78, 0.33, 1.0 };
	disegna_piano(0, 0 , 1, 1, col_bottom, col_top, Sabbia);
}

void disegna_palla(int nTriangles, Point* Palla) {

	int i, cont;
	int vertici = 3 * nTriangles;

	//Costruisco la geometria della palla ed i suoi colori
	vec4 col_bottom = { 1.0, 0.8, 0.0, 1.0 };
	disegna_cerchio(nTriangles, 1, col_rosso, col_bottom, Palla);
}

void initShader(void)
{
	GLenum ErrorCheckValue = glGetError();

	char* vertexShader = (char*)"vertexShader_C_M.glsl";
	char* fragmentShader = (char*)"fragmentShader_C_M.glsl";

	programId = ShaderMaker::createProgram(vertexShader, fragmentShader);
	glUseProgram(programId);

}

void init(void)
{
	Projection = ortho(0.0f, float(width), 0.0f, float(height));
	MatProj = glGetUniformLocation(programId, "Projection");
	MatModel = glGetUniformLocation(programId, "Model");

	//Costruzione geometria e colori del CIELO
	vec4 col_top = { 0.3,0.6,1.0,1.0 };
	vec4 col_bottom = { 0.0 , 0.1, 1.0, 1.0 };
	disegna_piano(0, 0, 1, 1, col_bottom, col_top, Cielo);
	//Generazione del VAO del Cielo
	glGenVertexArrays(1, &VAO_CIELO);
	glBindVertexArray(VAO_CIELO);
	glGenBuffers(1, &VBO_C);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_C);
	glBufferData(GL_ARRAY_BUFFER, vertices_Cielo * sizeof(Point), &Cielo[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);


	//Costruzione geometria e colori della sabbia
	disegna_sabbia();
	//Generazione del VAO della Sabbia
	glGenVertexArrays(1, &VAO_SABBIA);
	glBindVertexArray(VAO_SABBIA);
	glGenBuffers(1, &VBO_SABBIA);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_SABBIA);
	glBufferData(GL_ARRAY_BUFFER, vertices_sabbia * sizeof(Point), &Sabbia[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);


	//Costruzione geometria e colori del SOLE
	disegna_sole(nTriangles_sole, Sole);
	//Genero il VAO del SOLE
	glGenVertexArrays(1, &VAO_SOLE);
	glBindVertexArray(VAO_SOLE);
	glGenBuffers(1, &VBO_S);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_S);
	glBufferData(GL_ARRAY_BUFFER, vertices_sole * sizeof(Point), &Sole[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);

	//Costruzione geometria e colore Mare
	col_top = { 0.36, 0.7, 0.9, 1.0000 };
	disegna_piano(0, 0, 1, 1, col_top, col_top, Mare);
	//Generazione del VAO del Mare
	glGenVertexArrays(1, &VAO_MARE);
	glBindVertexArray(VAO_MARE);
	glGenBuffers(1, &VBO_MARE);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_MARE);
	glBufferData(GL_ARRAY_BUFFER, vertices_Mare * sizeof(Point), &Mare[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);

	//Costruzione geometria e colori delle onde
	col_bottom = { 0.69, 0.83, 0.94, 1.0000 };
	col_top = { 0.36, 0.7, 0.9, 1.0000 };
	disegna_onda(0, 0, 100, width, 5, col_bottom, col_top, Punti);
	//Genero un VAO per le onde
	glGenVertexArrays(1, &VAO_ONDA);
	glBindVertexArray(VAO_ONDA);
	glGenBuffers(1, &VBO_ONDA);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_ONDA);
	for (int i = 0; i < 2; i++) {
		glBufferData(GL_ARRAY_BUFFER, nVertices_onde * sizeof(Point), &Punti[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}
	//Scollego il VAO
	glBindVertexArray(0);


	//Generazione del VAO del paddle
	col_bottom = { 0.5, 0.5, 0.5, 1.0000 };
	col_top = { 0.35, 0.35, 0.35, 1.0000 };
	disegna_piano(0, 0, 1, 1, col_top, col_bottom, Paddle);
	glGenVertexArrays(1, &VAO_PADDLE);
	glBindVertexArray(VAO_PADDLE);
	glGenBuffers(1, &VBO_PADDLE);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_PADDLE);
	glBufferData(GL_ARRAY_BUFFER, vertices_Paddle * sizeof(Point), &Paddle[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);

	//Costruzione geometria e colori della PALLA
	disegna_palla(nTriangles_palla, Palla);
	glGenVertexArrays(1, &VAO_PALLA);
	glBindVertexArray(VAO_PALLA);
	glGenBuffers(1, &VBO_Pa);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_Pa);
	glBufferData(GL_ARRAY_BUFFER, vertices_palla * sizeof(Point), &Palla[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);


	//VAO dei fireworks
	glGenVertexArrays(1, &VAO_SCIA);
	glBindVertexArray(VAO_SCIA);
	glGenBuffers(1, &VBO_SCIA);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_SCIA);
	glBindVertexArray(0);

	// Costruisco un blocchetto che poi verrà ripetuto in fase di drawing in modo da costruire la griglia
	col_bottom = { 0.0, 0.6, 0.0, 1.0000 };
	col_top = { 0.5, 0.8, 0.5, 1.0000 };
	glGenVertexArrays(1, &VAO_BRICKS);
	glBindVertexArray(VAO_BRICKS);
	glGenBuffers(1, &VBO_BRICKS);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_BRICKS);
	disegna_piano(0, 0, 1, 1, col_bottom, col_top, bricks);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(Point), &bricks[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	//Scollego il VAO
	glBindVertexArray(0);

	//Definisco il colore che verrà assegnato allo schermo
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glutSwapBuffers();

}

void drawScene(void)
{
		glUniformMatrix4fv(MatProj, 1, GL_FALSE, value_ptr(Projection));
		glClear(GL_COLOR_BUFFER_BIT);

		//Disegna cielo
		//Matrice per il cambiamento del sistema di riferimento del cielo
		Model = mat4(1.0);
		Model = translate(Model, vec3(0.0, float(height) / 2, 0.0));
		Model = scale(Model, vec3(float(width), float(height) / 2, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glBindVertexArray(VAO_CIELO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, vertices_Cielo);
		glBindVertexArray(0);

		//Disegna spiaggia
		//Matrice per il cambiamento del sistema di riferimento della spiaggia
		Model = mat4(1.0);
		Model = translate(Model, vec3(0.0, 0.0, 0.0));
		Model = scale(Model, vec3(float(width), float(height) * 0.2, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		//Matrice per il cambiamento del sistema di riferimento della spiaggia
		glBindVertexArray(VAO_SABBIA);
		glDrawArrays(GL_TRIANGLES, 0, vertices_sabbia);
		glBindVertexArray(0);

		// Disegna sole
		if (!victory) {
			Model = mat4(1.0);
			Model = translate(Model, vec3(float(width) * 0.5, float(height) * 0.8, 0.0));
			Model = scale(Model, vec3(30.0, 30.0, 1.0));
			glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
			glBindVertexArray(VAO_SOLE);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_TRIANGLES, 0, vertices_sole / 2);
			glBindVertexArray(0);
			//Disegna Alone del sole
			Model = mat4(1.0);
			Model = translate(Model, vec3(float(width) * 0.5, float(height) * 0.8, 0.0));
			Model = scale(Model, vec3(80.0, 80.0, 1.0));
			glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
			glBindVertexArray(VAO_SOLE);
			glDrawArrays(GL_TRIANGLES, vertices_sole / 2, vertices_sole / 2);
			glBindVertexArray(0);
		}


		auto ax = vec3(0.0, float(height) / 2, 0.0);

		//Disegna Mare
		//Matrice per il cambiamento del sistema di riferimento del mare
		vec4 col_bottom = { 0.69, 0.83, 0.94, 1.0000 };
		vec4 col_top = { 0.36, 0.7, 0.9, 1.0000 };
		disegna_onda(0, 0, 100, width + (width / 5 * 2), 5, col_bottom, col_top, Punti);
		Model = mat4(1.0);
		Model = translate(Model, vec3(0.0, float(height) * 0.2, 0.0));
		Model = scale(Model, vec3(float(width), float(height) * 0.3, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glBindVertexArray(VAO_MARE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, vertices_Mare);
		glBindVertexArray(0);

		//Disegna onde
		for (int i = 0; i < 2; i++) {
			Model = mat4(1.0);
			Model = translate(Model, vec3(0.0, (float(height) / 2) - start_onda[i], 0.0));
			Model = glm::scale(Model, glm::vec3(1, .5, 1));
			glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
			glBindVertexArray(0);
			glBindVertexArray(VAO_ONDA);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, nVertices_onde);
			//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(0);
		}

		// Cambio di sistema di riferimento per Paddle
		Model = mat4(1.0);
		Model = translate(Model, vec3(start_paddle + float(width) / 2 * 0.83, float(height) * 0.05, 0.0));
		Model = scale(Model, vec3(float(width) * 0.17, float(height) * 0.05, 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glBindVertexArray(VAO_PADDLE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, vertices_Paddle);
		glBindVertexArray(0);


		// Disegna il muro di blocchi
		float dim_x = float(width) * 0.1;
		float dim_y = float(height) * 0.07;
		glBindVertexArray(VAO_BRICKS);
		for (int i = 0; i < GRID_X; i++) {
			for (int j = 0; j < GRID_Y; j++) {
				if (!bricks_hitted[i][j]) {
					Model = mat4(1.0);
					Model = translate(Model, vec3(float(i) * dim_x + float(i + 1) * float(width) * PADDING_X, float(height) - float(j) * dim_y - dim_y - PADDING_Y * float(height) - j * PADDING_Y * float(height), 0.0));
					Model = scale(Model, vec3(dim_x, dim_y, 1.0));
					glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
					//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}
			}
		}
		glBindVertexArray(0);

		// Disegna palla
		Model = mat4(1.0);
		Model = scale(Model, vec3(float(RAGGIO), (50.0), 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glBindVertexArray(VAO_PALLA);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, vertices_palla / 2, vertices_palla / 2);
		glBindVertexArray(0);
		//Matrice per il cambiamento del sistema di riferimento per la  palla
		Model = mat4(1.0);
		Model = translate(Model, vec3(posx, posy, 0.0f));
		Model = scale(Model, vec3(float(RAGGIO), float(RAGGIO), 1.0));
		glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
		glBindVertexArray(VAO_PALLA);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, vertices_palla / 2);
		glBindVertexArray(0);

		/**************************************************
		* Sezione con fireworks
		***************************************************/

		int P_size = getParticle();

		if (victory) {
			Model = mat4(1.0);
			Model = translate(Model, vec3(double(width) / 2, 3 * double(height) / 4, 0.0f));
			Model = scale(Model, vec3(double(width) * 2, double(height) * 2, 1.0));
			glUniformMatrix4fv(MatModel, 1, GL_FALSE, value_ptr(Model));
			glBindVertexArray(VAO_SCIA);
			glBindBuffer(GL_ARRAY_BUFFER, VBO_SCIA);
			glBufferData(GL_ARRAY_BUFFER, P_size * sizeof(Point), &PuntiScia[0], GL_STATIC_DRAW);
			// Configura l'attributo posizione
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			// Configura l'attributo colore
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glPointSize(3.0);
			glDrawArrays(GL_POINTS, 0, P_size);
			glBindVertexArray(0);
		}

		glutSwapBuffers();
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);

	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Breakout");
	glutDisplayFunc(drawScene);
	//Evento tastiera tasto premuto
	glutKeyboardFunc(keyboardPressedEvent);
	//Evento tastiera tasto rilasciato (per fermare lo spostamento a dx e sx mantenendo il rimbalzo sul posto)
	glutKeyboardUpFunc(keyboardReleasedEvent);

	//gestione animazione
	glutTimerFunc(66, update, 0);
	glutTimerFunc(500, update_wave, 0);
	glewExperimental = GL_TRUE;
	glewInit();

	initShader();
	init();

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glutMainLoop();
}