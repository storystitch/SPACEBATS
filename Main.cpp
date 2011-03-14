
#include "Framework.h"
#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "InputListener.h"
#include "Framebuffer.h"
#include "MotionBlur.h"
#include "Ship.h"
#include "BodyEmitter.h"
#include "HUD.h"
#include "StatusBar.h"
#include "Gate.h"

#include <btBulletDynamicsCommon.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include "MusicManager.h"

#define TIMESTEP (1.0 / 60.0)

using namespace std;

// Note: See the SMFL documentation for info on setting up fullscreen mode
// and using rendering settings
// http://www.sfml-dev.org/tutorials/1.6/window-window.php
sf::WindowSettings settings(24, 8, 2);
sf::RenderWindow window(sf::VideoMode(1000, 650), "sPaCEbaTS", sf::Style::Close, settings);

// This is a clock you can use to control animation.  For more info, see:
// http://www.sfml-dev.org/tutorials/1.6/window-time.php
sf::Clock clck;

GLfloat accum = 0.0;

// This creates an asset importer using the Open Asset Import library.
// It automatically manages resources for you, and frees them when the program
// exits.
Assimp::Importer importer;
Shader *blurShader, *bgShader, *barShader;

sf::Image background;

Model mars;

Camera camera(btVector3(0.0, 0.0, 50.0));

Ship spaceship(btVector3(0.0, 0.0, 0.0), &camera);
BodyEmitter *bodyEmitter;

btDiscreteDynamicsWorld *world;

vector<InputListener*> inputListeners;

Framebuffer *normalsBuffer = NULL;

const int NUM_MOTION_BLUR_FRAMES = 4;
MotionBlur* motionBlur;
bool useMotionBlur = false;

HUD* hud;
StatusBar* boostbar;
StatusBar* healthbar;

MusicManager music;

void initOpenGL();
void loadAssets();
void handleInput();
void renderFrame();
void renderBackground();

int main(int argc, char** argv) {
	
	srand(time(0));
	
	initOpenGL();
	
	btBroadphaseInterface *broadphase = new btDbvtBroadphase();
	btDefaultCollisionConfiguration *collisionConfig = new btDefaultCollisionConfiguration();
	btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collisionConfig);
	btSequentialImpulseConstraintSolver *solver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	world->setGravity(btVector3(0, 0, 0));
	
	spaceship.setWorld(world);
	loadAssets();
	
	hud = new HUD(&spaceship);
	boostbar = new StatusBar(barShader, btVector4(0.90, -0.9, 0.05, 1.0));
	healthbar = new StatusBar(barShader, btVector4(-0.95, -0.9, 0.05, 1.0));
	healthbar->setTopColor(btVector4(0, 1, 0, 0.5));
	healthbar->setBottomColor(btVector4(1, 0, 0, 0.5));
	
	hud->addComponent(boostbar);
	hud->addComponent(healthbar);
	
	//Gate::setScoreboard(healthbar);
	Gate::loadChangeImage();
	
	spaceship.setBoostBar(boostbar);
	spaceship.setHealthBar(healthbar);
	
	motionBlur = new MotionBlur(NUM_MOTION_BLUR_FRAMES, window.GetWidth(), window.GetHeight());
	glClear(GL_ACCUM_BUFFER_BIT);
	
	inputListeners.push_back(&camera);
	inputListeners.push_back(&spaceship);
	
	
	music.playSound(BACKGROUND);
//	sf::Music music;
//	if(!music.OpenFromFile("music/change.wav")){
//		printf("Error with music.\n");
//		return 0;
//	}else{
//		printf("Music loaded.\n");
//	}
//	   
//	music.Play();
	
	// Put your game loop here (i.e., render with OpenGL, update animation)
	while (window.IsOpened()) {	
		handleInput();
		
		accum += clck.GetElapsedTime();
		clck.Reset();
		while (accum > TIMESTEP) {
			spaceship.update(TIMESTEP);
			world->stepSimulation(TIMESTEP);
			spaceship.testCollision();	
			bodyEmitter->emitBodies(TIMESTEP);
			camera.update(TIMESTEP);
			accum -= TIMESTEP;
		}
				
		renderFrame();
		window.Display();
	}
	
	delete blurShader;
	
	delete normalsBuffer;
	
	delete world;
	delete solver;
	delete dispatcher;
	delete collisionConfig;
	delete broadphase;
	
	return 0;
}

void initOpenGL() {
    // Initialize GLEW on Windows, to make sure that OpenGL 2.0 is loaded
#ifdef FRAMEWORK_USE_GLEW
    GLint error = glewInit();
    if (GLEW_OK != error) {
        std::cerr << glewGetErrorString(error) << std::endl;
        exit(-1);
    }
    if (!GLEW_VERSION_2_0 || !GL_EXT_framebuffer_object) {
        std::cerr << "This program requires OpenGL 2.0 and FBOs" << std::endl;
        exit(-1);
    }
#endif
	
    // This initializes OpenGL with some common defaults.  More info here:
    // http://www.sfml-dev.org/tutorials/1.6/window-opengl.php
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
    glViewport(0, 0, window.GetWidth(), window.GetHeight());
}

void loadAssets() {
	blurShader = new Shader("shaders/blur");
	bgShader = new Shader("shaders/background");
	barShader = new Shader("shaders/bar");
	
	bodyEmitter = new BodyEmitter(world);
	bodyEmitter->loadModels();
	
	Model::loadShaders();
	normalsBuffer = new Framebuffer(window.GetWidth(), window.GetHeight());
	Model::setNormalsBuffer(normalsBuffer);
	
	background.LoadFromFile("models/space2.jpg");
	
	spaceship.model.loadFromFile("models/ship", "space_frigate_0.3DS", importer);
	spaceship.model.setScaleFactor(0.5);
}


void handleInput() {
	//////////////////////////////////////////////////////////////////////////
	// TODO: ADD YOUR INPUT HANDLING HERE. 
	//////////////////////////////////////////////////////////////////////////

	// Event loop, for processing user input, etc.  For more info, see:
	// http://www.sfml-dev.org/tutorials/1.6/window-events.php
	sf::Event evt;
	while (window.GetEvent(evt)) {
		switch (evt.Type) {
			case sf::Event::Closed: 
				// Close the window.  This will cause the game loop to exit,
				// because the IsOpened() function will no longer return true.
				window.Close(); 
				break;
			case sf::Event::Resized: 
				// If the window is resized, then we need to change the perspective
				// transformation and viewport
				glViewport(0, 0, evt.Size.Width, evt.Size.Height);
				break;
			case sf::Event::KeyPressed:
				switch (evt.Key.Code) {
					case sf::Key::Escape:
						window.Close();
						break;
					case sf::Key::Space:
						if(boostbar->getValue() <= 0) return;
						
						music.playSound(POWERUP);
						useMotionBlur = true;
						bodyEmitter->setBoostMode(true);
						bodyEmitter->boostSpeed();
						break;
					default:
						break;
				}
				break;
			case sf::Event::KeyReleased:
				switch (evt.Key.Code) {
					case sf::Key::Space:
						
						music.stopSound(POWERUP);
						useMotionBlur = false;
						bodyEmitter->setBoostMode(false);
						bodyEmitter->resetSpeed();
						break;
					default:
						break;
				}
				break;
			default: 
				break;
		}
		for (unsigned i = 0; i < inputListeners.size(); i++)
			inputListeners[i]->handleEvent(evt, window.GetInput());
	}
}


void setupLights()
{
	GLfloat pos[] = { 0.0, 1.0, 0.0, 0.0 };
	GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat ambient[] = { 0.3, 0.3, 0.3, 1.0 };
	
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
}

void renderBackground()
{
	glUseProgram(bgShader->programID());
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLint tex = glGetUniformLocation(bgShader->programID(), "texture");
	glUniform1i(tex, 0);
	glActiveTexture(GL_TEXTURE0);
	background.Bind();
	
	GLint pos = glGetAttribLocation(bgShader->programID(), "positionIn");
	glBegin(GL_QUADS);
	glVertexAttrib2f(pos, -1.0, -1.0);
	glVertexAttrib2f(pos, 1.0, -1.0);
	glVertexAttrib2f(pos, 1.0, 1.0);
	glVertexAttrib2f(pos, -1.0, 1.0);
	glEnd();
}

void clearNormalsBuffer()
{
	normalsBuffer->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	normalsBuffer->unbind();
}


void renderFrame() {
	glViewport(0, 0, window.GetWidth(), window.GetHeight());

	clearNormalsBuffer();
	camera.setProjectionAndView((float)window.GetWidth()/window.GetHeight());
	spaceship.model.render(NORMALS_PASS);
	bodyEmitter->drawBodies(NORMALS_PASS);
	
	if (motionBlur->shouldRenderFrame()) {	
		motionBlur->bind();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderBackground();	

		glClear(GL_DEPTH_BUFFER_BIT);

		camera.setProjectionAndView((float)window.GetWidth() / window.GetHeight());
		setupLights();

		bodyEmitter->drawBodies(FINAL_PASS);
		motionBlur->unbind();
	}	

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if(useMotionBlur && boostbar->getValue() <= 0){
		useMotionBlur = false;
		bodyEmitter->setBoostMode(false);
		bodyEmitter->resetSpeed();
	}
	
	if(useMotionBlur){
		motionBlur->render(blurShader);
	} else {
		renderBackground();	
	}
	motionBlur->update();

	glClear(GL_DEPTH_BUFFER_BIT);	

	camera.setProjectionAndView((float)window.GetWidth()/window.GetHeight());
	setupLights();
	bodyEmitter->drawBodies(FINAL_PASS);
	spaceship.model.render(FINAL_PASS);


	hud->render();	
	
}
