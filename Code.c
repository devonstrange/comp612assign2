/* This template provides a basic FPS - limited render loop for an animated scene,
* plus keyboard handling for smooth game - like control of an object such as a
* character or vehicle.
*
*A simple static lighting setup is provided via initLights(), which is not
*included in the animationalcontrol.c template.There are no other changes.
*
******************************************************************************/
#define _CRT_SECURE_NO_WARNINGS 
#include <Windows.h>
#include <freeglut.h>
#include <math.h>
#include <stdio.h>
#include <GL/gl.h>  // OpenGL headers
#include <stdio.h>  // For file operations
#include <stdlib.h> // For memory management (malloc, free)
/******************************************************************************
* Animation & Timing Setup
******************************************************************************/
// Target frame rate (number of Frames Per Second).
#define TARGET_FPS 60
// Ideal time each frame should be displayed for (in milliseconds).
const unsigned int FRAME_TIME = 1000 / TARGET_FPS;
// Frame time in fractional seconds.
// Note: This is calculated to accurately reflect the truncated integer value of
// FRAME_TIME, which is used for timing, rather than the more accurate fractional
// value we'd get if we simply calculated "FRAME_TIME_SEC = 1.0f / TARGET_FPS".
const float FRAME_TIME_SEC = (1000 / TARGET_FPS) / 1000.0f;
// Time we started preparing the current frame (in milliseconds since GLUT was initialized).
unsigned int frameStartTime = 0;
/******************************************************************************
* Some Simple Definitions of Motion
******************************************************************************/
#define MOTION_NONE 0 // No motion.
#define MOTION_CLOCKWISE -1 // Clockwise rotation.
#define MOTION_ANTICLOCKWISE 1 // Anticlockwise rotation.
#define MOTION_BACKWARD -1 // Backward motion.
#define MOTION_FORWARD 1 // Forward motion.
#define MOTION_LEFT -1 // Leftward motion.
#define MOTION_RIGHT 1 // Rightward motion.
#define MOTION_DOWN -1 // Downward motion.
#define MOTION_UP 1 // Upward motion.
// Represents the motion of an object on four axes (Yaw, Surge, Sway, and Heave).
//
// You can use any numeric values, as specified in the comments for each axis. However,
// the MOTION_ definitions offer an easy way to define a "unit" movement without using
// magic numbers (e.g. instead of setting Surge = 1, you can set Surge = MOTION_FORWARD).
//
typedef struct {
	int Yaw; // Turn about the Z axis [<0 = Clockwise, 0 = Stop, >0 = Anticlockwise]
	int Surge; // Move forward or back [<0 = Backward, 0 = Stop, >0 = Forward]
	int Sway; // Move sideways (strafe) [<0 = Left, 0 = Stop, >0 = Right]
	int Heave; // Move vertically [<0 = Down, 0 = Stop, >0 = Up]
} motionstate4_t;
/******************************************************************************
* Keyboard Input Handling Setup
******************************************************************************/
// Represents the state of a single keyboard key.Represents the state of a single keyboard key.
typedef enum {
	KEYSTATE_UP = 0, // Key is not pressed.
	KEYSTATE_DOWN // Key is pressed down.
} keystate_t;
// Represents the states of a set of keys used to control an object's motion.
typedef struct {
	keystate_t MoveForward;
	keystate_t MoveBackward;
	keystate_t MoveLeft;
	keystate_t MoveRight;
	keystate_t MoveUp;
	keystate_t MoveDown;
	keystate_t TurnLeft;
	keystate_t TurnRight;
} motionkeys_t;
// Current state of all keys used to control our "player-controlled" object's motion.
motionkeys_t motionKeyStates = {
KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP,
KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP, KEYSTATE_UP };
// How our "player-controlled" object should currently be moving, solely based on keyboard input.
//
// Note: this may not represent the actual motion of our object, which could be subject to
// other controls (e.g. mouse input) or other simulated forces (e.g. gravity).
motionstate4_t keyboardMotion = { MOTION_NONE, MOTION_NONE, MOTION_NONE, MOTION_NONE };
// Define all character keys used for input (add any new key definitions here).
// Note: USE ONLY LOWERCASE CHARACTERS HERE. The keyboard handler provided converts all
// characters typed by the user to lowercase, so the SHIFT key is ignored.
#define KEY_MOVE_FORWARD 'w'
#define KEY_MOVE_BACKWARD 's'
#define KEY_MOVE_LEFT 'a'
#define KEY_MOVE_RIGHT 'd'
#define KEY_RENDER_FILL 'l'
#define KEY_CHANGE_VIEW 'v'
#define KEY_TOGGLE_ORIGIN 'o'
#define KEY_EXIT 27 // Escape key.
// Define all GLUT special keys used for input (add any new key definitions here).
#define SP_KEY_MOVE_UP GLUT_KEY_UP
#define SP_KEY_MOVE_DOWN GLUT_KEY_DOWN
#define SP_KEY_TURN_LEFT GLUT_KEY_LEFT
#define SP_KEY_TURN_RIGHT GLUT_KEY_RIGHT
/******************************************************************************
* GLUT Callback Prototypes
******************************************************************************/
void display(void);
void reshape(int width, int h);
void keyPressed(unsigned char key, int x, int y);
void specialKeyPressed(int key, int x, int y);
void keyReleased(unsigned char key, int x, int y);
void specialKeyReleased(int key, int x, int y);
void idle(void);
void loadTextures(void);
/******************************************************************************
* Animation-Specific Function Prototypes (add your own here)
******************************************************************************/
void main(int argc, char** argv);
void init(void);
void think(void);
void initLights(void);
void drawOriginMarker();
/******************************************************************************
* Animation-Specific Setup (Add your own definitions, constants, and globals here)
******************************************************************************/
// Render objects as filled polygons (1) or wireframes (0). Default filled.
int renderFillEnabled = 1;

GLint windowWidth = 1000;
GLint windowHeight = 800;

GLfloat cameraLookAt[3] = { 10.0f, 10.0f, 10.0f };
GLfloat camerastate = 0;

GLfloat originActive = 1;

GLUquadricObj* sphereQuadric;
#define BodyRadius 1.0

GLfloat BirdPos[3] = { 0.0f, 0.0f, 0.0f };
GLfloat BirdRot = 0.0f;

GLfloat WingRot = 0;
GLfloat UpDown = 0;

const GLfloat PALE_GREEN[3] = { 0.0f, 1.0, 1.0 };

int texID[3]; // Texture ID's for the three textures
char* textureFileNames[3] = {   // file names for the files from which texture images are loaded
			"assets//feathers_64_debug.ppm",
			"assets//grass_debug.ppm",
			"assets//marble.ppm"
};

/******************************************************************************
* Entry Point (don't put anything except the main function here)
******************************************************************************/
void main(int argc, char** argv)
{
	// Initialize the OpenGL window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Animation");
	// Set up the scene.
	init();
	// Disable key repeat (keyPressed or specialKeyPressed will only be called once when a key is first pressed).
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	// Register GLUT callbacks.
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(specialKeyPressed);
	glutKeyboardUpFunc(keyReleased);
	glutSpecialUpFunc(specialKeyReleased);
	glutIdleFunc(idle);
	// Record when we started rendering the very first frame (which should happen after we call glutMainLoop).
	frameStartTime = (unsigned int)glutGet(GLUT_ELAPSED_TIME);
	// Enter the main drawing loop (this will never return).
	glutMainLoop();
}
/******************************************************************************
* GLUT Callbacks (don't add any other functions here)
******************************************************************************/
/*
Called when GLUT wants us to (re)draw the current animation frame.
Note: This function must not do anything to update the state of our
simulated
world. Animation (moving or rotating things, responding to keyboard input,
etc.) should only be performed within the think() function provided below.
*/
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	gluLookAt(BirdPos[0] + cameraLookAt[0], BirdPos[1] + cameraLookAt[1], BirdPos[2] + cameraLookAt[2],  // eye
		BirdPos[0], BirdPos[1], BirdPos[2],  // looking at
		0.0, 1.0, 0.0); // up

	if (originActive == 1)
	{
		drawOriginMarker();
	}

	glPushMatrix();
	glTranslatef(BirdPos[0], BirdPos[1], BirdPos[2]);
	glRotatef(BirdRot, 0.0f, 1.0f, 0.0f);
	// Body
	glColor3fv(PALE_GREEN);
	//gluQuadricDrawStyle(sphereQuadric, GLU_LINE);
	gluSphere(sphereQuadric, BodyRadius, 50.0, 50.0);

	// Beak
	glPushMatrix();
	glColor3f(1.0f, 0.0f, 0.0f);
	glTranslatef(BodyRadius - 0.1f, 0.0f, 0.0f);
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f); //angle, along the x, along the y, along the z
	glutSolidCone(BodyRadius / 2, 1.0, 50.0, 50.0);
	glPopMatrix();
	// Eye right      
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(0.5f, BodyRadius / 2, 0.5f);
	gluSphere(sphereQuadric, BodyRadius / 3, 10.0, 10.0);
	glPushMatrix();
	glColor3f(0.0f, 0.0f, 0.0f);
	glTranslatef(0.25f, 0.05f, 0.0f);
	gluSphere(sphereQuadric, BodyRadius / 6, 10.0, 10.0);
	glPopMatrix();
	glPopMatrix();
	// Eye left
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(0.5f, BodyRadius / 2, -0.5f);
	gluSphere(sphereQuadric, BodyRadius / 3, 10.0, 10.0);
	glPushMatrix();
	glColor3f(0.0f, 0.0f, 0.0f);
	glTranslatef(0.25f, 0.05f, 0.0f);
	gluSphere(sphereQuadric, BodyRadius / 6, 10.0, 10.0);
	glPopMatrix();
	glPopMatrix();
	// Right Wing
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);

	glRotatef(WingRot, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0, 0.0, 1.0);
	glScalef(1.0, 0.1, 2.0);
	glutSolidCube(1.0);
	glPopMatrix();
	// Left Wing
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);

	glRotatef(-WingRot, 1.0f, 0.0f, 0.0f);
	glTranslatef(0.0, 0.0, -1.0);
	glScalef(1.0, 0.1, 2.0);
	glutSolidCube(1.0);
	glPopMatrix();

	glPopMatrix();

	//glColor3f(1.0f, 0.0f, 0.0f);
	//gluCylinder(cylinderQuadric, BaseRadius, TopRadius, Height, 10.0, 10.0);

	glutSwapBuffers();
		/*
	      TEMPLATE: REPLACE THIS COMMENT WITH YOUR DRAWING CODE
	      Separate reusable pieces of drawing code into functions, which you can
	      add
	      to the "Animation-Specific Functions" section below.
	      Remember to add prototypes for any new functions to the "Animation-
	      Specific
	      Function Prototypes" section near the top of this template.
	      */
}

/*
Called when the OpenGL window has been resized.
*/
void reshape(int width, int h)
{
	windowWidth = width;
	windowHeight = h;

	glViewport(0, 0, windowWidth, windowHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60, (double)windowWidth / (double)windowHeight, 1.0, 20.0); // fov, Aspect ratio, near, far

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
/*
Called each time a character key (e.g. a letter, number, or symbol) is
pressed.
*/
void keyPressed(unsigned char key, int x, int y)
{
	switch (tolower(key)) {
				/*
		            Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION
		            Whenever one of our movement keys is pressed, we do two things:
		            (1) Update motionKeyStates to record that the key is held down. We use
		            this later in the keyReleased callback.
		            (2) Update the relevant axis in keyboardMotion to set the new direction
		            we should be moving in. The most recent key always "wins" (e.g.
		            if
		            you're holding down KEY_MOVE_LEFT then also pressed
		            KEY_MOVE_RIGHT,
		            our object will immediately start moving right).
		            */
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_FORWARD;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_DOWN;
		keyboardMotion.Surge = MOTION_BACKWARD;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_LEFT;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_DOWN;
		keyboardMotion.Sway = MOTION_RIGHT;
		break;
				/*
		            Other Keyboard Functions (add any new character key controls here)
		            Rather than using literals (e.g. "t" for spotlight), create a new KEY_
		            definition in the "Keyboard Input Handling Setup" section of this file.
		            For example, refer to the existing keys used here (KEY_MOVE_FORWARD,
		            KEY_MOVE_LEFT, KEY_EXIT, etc).
		            */
	case KEY_RENDER_FILL:
		renderFillEnabled = !renderFillEnabled;
		break;

	case KEY_CHANGE_VIEW: // Changing view of camera
		if (camerastate == 0)
		{
			cameraLookAt[0] = 5.0f;
			cameraLookAt[1] = 0.0f;
			cameraLookAt[2] = 0.0f;
			camerastate = 1;
		}
		else if (camerastate == 1)
		{
			cameraLookAt[0] = 0.0f;
			cameraLookAt[1] = 0.0f;
			cameraLookAt[2] = -5.0f;
			camerastate = 2;
		}
		else if (camerastate == 2)
		{
			cameraLookAt[0] = -5.0f;
			cameraLookAt[1] = 0.0f;
			cameraLookAt[2] = 0.0f;
			camerastate = 3;
		}
		else if (camerastate == 3)
		{
			cameraLookAt[0] = 0.0f;
			cameraLookAt[1] = 0.0f;
			cameraLookAt[2] = 5.0f;
			camerastate = 4;
		}
		else if (camerastate == 4)
		{
			cameraLookAt[0] = 10.0f;
			cameraLookAt[1] = 10.0f;
			cameraLookAt[2] = 10.0f;
			camerastate = 0;
		}
		break;

	case KEY_TOGGLE_ORIGIN:
		if (originActive == 1)
		{
			originActive = 0;
		}
		else if (originActive == 0)
		{
			originActive = 1;
		}
		break;

	case KEY_EXIT:
		gluDeleteQuadric(sphereQuadric);
		exit(0);
		break;
	}
}
/*
Called each time a "special" key (e.g. an arrow key) is pressed.
*/
void specialKeyPressed(int key, int x, int y)
{
	switch (key) {
					/*
		            Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION
		            This works as per the motion keys in keyPressed.
		            */
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_UP;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_DOWN;
		keyboardMotion.Heave = MOTION_DOWN;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_ANTICLOCKWISE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_DOWN;
		keyboardMotion.Yaw = MOTION_CLOCKWISE;
		break;
					/*
		            Other Keyboard Functions (add any new special key controls here)
		            Rather than directly using the GLUT constants (e.g. GLUT_KEY_F1),
		            create
		            a new SP_KEY_ definition in the "Keyboard Input Handling Setup" section
		            of
		            this file. For example, refer to the existing keys used here
		            (SP_KEY_MOVE_UP,
		            SP_KEY_TURN_LEFT, etc).
		            */
	}
}
/*
Called each time a character key (e.g. a letter, number, or symbol) is
released.
*/
void keyReleased(unsigned char key, int x, int y)
{
	switch (tolower(key)) {
					/*
		            Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION
		            Whenever one of our movement keys is released, we do two things:
		            (1) Update motionKeyStates to record that the key is no longer held
		            down;
		            we need to know when we get to step (2) below.
		            (2) Update the relevant axis in keyboardMotion to set the new direction
		            we should be moving in. This gets a little complicated to ensure
		            the controls work smoothly. When the user releases a key that
		            moves
		            in one direction (e.g. KEY_MOVE_RIGHT), we check if its
		            "opposite"
		            key (e.g. KEY_MOVE_LEFT) is pressed down. If it is, we begin
		            moving
		            in that direction instead. Otherwise, we just stop moving.
		            */
	case KEY_MOVE_FORWARD:
		motionKeyStates.MoveForward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveBackward ==
			KEYSTATE_DOWN) ? MOTION_BACKWARD : MOTION_NONE;
		break;
	case KEY_MOVE_BACKWARD:
		motionKeyStates.MoveBackward = KEYSTATE_UP;
		keyboardMotion.Surge = (motionKeyStates.MoveForward == KEYSTATE_DOWN) ?
			MOTION_FORWARD : MOTION_NONE;
		break;
	case KEY_MOVE_LEFT:
		motionKeyStates.MoveLeft = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveRight == KEYSTATE_DOWN) ?
			MOTION_RIGHT : MOTION_NONE;
		break;
	case KEY_MOVE_RIGHT:
		motionKeyStates.MoveRight = KEYSTATE_UP;
		keyboardMotion.Sway = (motionKeyStates.MoveLeft == KEYSTATE_DOWN) ?
			MOTION_LEFT : MOTION_NONE;
		break;
					/*
		            Other Keyboard Functions (add any new character key controls here)
		            Note: If you only care when your key is first pressed down, you don't
		            have to
		            add anything here. You only need to put something in keyReleased if you
		            care
		            what happens when the user lets go, like we do with our movement keys
		            above.
		            For example: if you wanted a spotlight to come on while you held down
		            "t", you
		            would need to set a flag to turn the spotlight on in keyPressed, and
		            update the
		            flag to turn it off in keyReleased.
		            */
	}
}
/*
Called each time a "special" key (e.g. an arrow key) is released.
*/
void specialKeyReleased(int key, int x, int y)
{
	switch (key) {
		/*
		            Keyboard-Controlled Motion Handler - DON'T CHANGE THIS SECTION
		            This works as per the motion keys in keyReleased.
		            */
	case SP_KEY_MOVE_UP:
		motionKeyStates.MoveUp = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveDown == KEYSTATE_DOWN) ?
			MOTION_DOWN : MOTION_NONE;
		break;
	case SP_KEY_MOVE_DOWN:
		motionKeyStates.MoveDown = KEYSTATE_UP;
		keyboardMotion.Heave = (motionKeyStates.MoveUp == KEYSTATE_DOWN) ?
			MOTION_UP : MOTION_NONE;
		break;
	case SP_KEY_TURN_LEFT:
		motionKeyStates.TurnLeft = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnRight == KEYSTATE_DOWN) ?
			MOTION_CLOCKWISE : MOTION_NONE;
		break;
	case SP_KEY_TURN_RIGHT:
		motionKeyStates.TurnRight = KEYSTATE_UP;
		keyboardMotion.Yaw = (motionKeyStates.TurnLeft == KEYSTATE_DOWN) ?
			MOTION_ANTICLOCKWISE : MOTION_NONE;
		break;
			/*
            Other Keyboard Functions (add any new special key controls here)
            As per keyReleased, you only need to handle the key here if you want
            something
            to happen when the user lets go. If you just want something to happen
            when the
            key is first pressed, add you code to specialKeyPressed instead.
            */
	}
}
/*
Called by GLUT when it's not rendering a frame.
Note: We use this to handle animation and timing. You shouldn't need to
modify
this callback at all. Instead, place your animation logic (e.g. moving or
rotating
things) within the think() method provided with this template.
*/
void idle(void)
{
	// Wait until it's time to render the next frame.
	unsigned int frameTimeElapsed = (unsigned int)glutGet(GLUT_ELAPSED_TIME) -
		frameStartTime;
	if (frameTimeElapsed < FRAME_TIME)
	{
		// This frame took less time to render than the ideal FRAME_TIME: we'll suspend this thread for the remaining time,
			  // so we're not taking up the CPU until we need to render another frame.
		unsigned int timeLeft = FRAME_TIME - frameTimeElapsed;
		Sleep(timeLeft);
	}
	// Begin processing the next frame.
	frameStartTime = glutGet(GLUT_ELAPSED_TIME); // Record when we started work on the new frame.
	think(); // Update our simulated world before the next call to display().
	glutPostRedisplay(); // Tell OpenGL there's a new frame ready to be drawn.
}
/******************************************************************************
* Animation-Specific Functions (Add your own functions at the end of this section)
******************************************************************************/
/*
Initialise OpenGL and set up our scene before we begin the render loop.
*/
void init(void)
{
	glEnable(GL_DEPTH_TEST);
	initLights();
	// Anything that relies on lighting or specifies normals must be initialised after initLights.

	sphereQuadric = gluNewQuadric();
}

/*
Advance our animation by FRAME_TIME milliseconds.
Note: Our template's GLUT idle() callback calls this once before each new
frame is drawn, EXCEPT the very first frame drawn after our application
starts. Any setup required before the first frame is drawn should be placed
in init().
*/
void think(void)
{
	if (UpDown == 0)
	{
		WingRot += 1;
		if (WingRot == 20)
		{
			UpDown = 1;
		}
	}
	else if (UpDown == 1)
	{
		WingRot -= 1;
		if (WingRot == -20)
		{
			UpDown = 0;
		}
	}
	/*
	      TEMPLATE: REPLACE THIS COMMENT WITH YOUR ANIMATION/SIMULATION CODE
	      In this function, we update all the variables that control the animated
	      parts of our simulated world. For example: if you have a moving box,
	      this is
	      where you update its coordinates to make it move. If you have something
	      that
	      spins around, here's where you update its angle.
	      NOTHING CAN BE DRAWN IN HERE: you can only update the variables that
	      control
	      how everything will be drawn later in display().
	      How much do we move or rotate things? Because we use a fixed frame
	      rate, we
	      assume there's always FRAME_TIME milliseconds between drawing each
	      frame. So,
	      every time think() is called, we need to work out how far things should
	      have
	      moved, rotated, or otherwise changed in that period of time.
	      Movement example:
	      * Let's assume a distance of 1.0 GL units is 1 metre.
	      * Let's assume we want something to move 2 metres per second on the x
	      axis
	      * Each frame, we'd need to update its position like this:
	      x += 2 * (FRAME_TIME / 1000.0f)
	      * Note that we have to convert FRAME_TIME to seconds. We can skip this
	      by
	      using a constant defined earlier in this template:
	      x += 2 * FRAME_TIME_SEC;
	      Rotation example:
	      * Let's assume we want something to do one complete 360-degree rotation
	      every
	      second (i.e. 60 Revolutions Per Minute, or RPM).
	      * Each frame, we'd need to update our object's angle like this (we'll
	      use the
	      FRAME_TIME_SEC constant as per the example above):
	      a += 360 * FRAME_TIME_SEC;
	      This works for any type of "per second" change: just multiply the
	      amount you'd
	      want to move in a full second by FRAME_TIME_SEC, and add or subtract
	      that
	      from whatever variable you're updating.
	      You can use this same approach to animate other things like color,
	      opacity,
	      brightness of lights, etc.
		  Keyboard motion handler: complete this section to make your "player-
		  controlled"
		  object respond to keyboard input.
		  */
	if (keyboardMotion.Yaw != MOTION_NONE) { // TEMPLATE: Turn your object right (clockwise) if .Yaw < 0, or left (anticlockwise) if .Yaw > 0

		BirdRot += 5 * FRAME_TIME_SEC * keyboardMotion.Yaw;
	}
	if (keyboardMotion.Surge != MOTION_NONE) { // TEMPLATE: Move your object backward if .Surge < 0, or forward if .Surge > 0 
		BirdPos[0] += 0.5 * FRAME_TIME_SEC * keyboardMotion.Surge;
	}
	if (keyboardMotion.Sway != MOTION_NONE) { 		// TEMPLATE: Move (strafe) your object left if .Sway < 0, or right if .Sway > 0 
		BirdPos[2] += 0.5 * FRAME_TIME_SEC * keyboardMotion.Sway;
	}
	if (keyboardMotion.Heave != MOTION_NONE) { // TEMPLATE: Move your object down if .Heave < 0, or up if .Heave > 0 
		BirdPos[1] += 0.5 * FRAME_TIME_SEC * keyboardMotion.Heave;
	}
	cameraLookAt[1] = BirdPos[1];
}
/*
Initialise OpenGL lighting before we begin the render loop.
Note (advanced): If you're using dynamic lighting (e.g. lights that move
around, turn on or
off, or change colour) you may want to replace this with a drawLights
function that gets called
at the beginning of display() instead of init().
*/
void initLights(void)
{
	// Simple lighting setup
	GLfloat globalAmbient[] = { 0.4f, 0.4f, 0.4f, 1 };

	// lights
	GLfloat lightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f }; // point light
	GLfloat ambientLight[] = { 0, 0, 0, 1 }; // no ambient
	GLfloat diffuseLight[] = { 1, 1, 1, 1 }; // white
	GLfloat specularLight[] = { 1, 1, 1, 1 }; // white

	// Configure global ambient lighting.
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	// Configure Light 0.
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

	// Enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// Make GL normalize the normal vectors we supply.
	glEnable(GL_NORMALIZE);

	// Enable use of simple GL colours as materials.
	glEnable(GL_COLOR_MATERIAL);
}

void drawOriginMarker() // marks the origin of the space
{
	glBegin(GL_LINES); // x axis red
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(2.0f, 0.0f, 0.0f);
	glEnd();

	glBegin(GL_LINES); // y axis green
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 2.0f, 0.0f);
	glEnd();

	glBegin(GL_LINES); // z axis blue
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 2.0f);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.5f);
	glutWireSphere(0.1, 10, 10); //origin marker
}

void loadTextures(void) {
	glGenTextures(3, texID);  // Get the texture object IDs.

	for (int j = 0; j < 3; j++) {
		GLubyte* imageData;        // Image data
		int imageWidth, imageHeight;  // Image size
		FILE* fileID;              // Image file
		int maxValue;              // Maximum pixel value
		int totalPixels;           // Total number of pixels in the image
		char tempChar;             // Temporary character for parsing
		int i;                     // Counter for current pixel
		char headerLine[100];      // Header line buffer
		float RGBScaling;          // Scaling factor for RGB values
		int red, green, blue;      // RGB components for each pixel

		// Open the image file (hardcoded filename)
		fileID = fopen(textureFileNames[j], "r");

		// Read the first header line
		if (fscanf(fileID, "%[^\n] ", headerLine));

		// Verify the file is a PPM file (must start with "P3")
		if ((headerLine[0] != 'P') || (headerLine[1] != '3')) {
			printf("This is not a PPM file!\n");
			exit(0);
		}

		// We have a PPM file
		printf("This is a PPM file\n");

		// Read the first character of the next line
		if (fscanf(fileID, "%c", &tempChar));

		// Skip comment lines (lines starting with '#')
		while (tempChar == '#') {
			if (fscanf(fileID, "%[^\n] ", headerLine));
			printf("%s\n", headerLine);
			if (fscanf(fileID, "%c", &tempChar));
		}

		// Undo the last character read if it's not a comment
		ungetc(tempChar, fileID);

		// Read the image width, height, and max value
		if (fscanf(fileID, "%d %d %d", &imageWidth, &imageHeight, &maxValue));
		printf("%d rows  %d columns  max value= %d\n", imageHeight, imageWidth, maxValue);

		// Calculate total number of pixels
		totalPixels = imageWidth * imageHeight;

		// Allocate memory for the image data (3 bytes per pixel for RGB)
		imageData = malloc(3 * sizeof(GLuint) * totalPixels);

		// Determine scaling factor for RGB values
		RGBScaling = 255.0f / maxValue;

		// Read pixel data
		if (maxValue == 255) {
			for (i = 0; i < totalPixels; i++) {
				if (fscanf(fileID, "%d %d %d", &red, &green, &blue));
				imageData[3 * totalPixels - 3 * i - 3] = red;
				imageData[3 * totalPixels - 3 * i - 2] = green;
				imageData[3 * totalPixels - 3 * i - 1] = blue;
			}
		}
		else {  // Scale RGB values if maxValue is not 255
			for (i = 0; i < totalPixels; i++) {
				if (fscanf(fileID, "%d %d %d", &red, &green, &blue));
				imageData[3 * totalPixels - 3 * i - 3] = red * (GLubyte)RGBScaling;
				imageData[3 * totalPixels - 3 * i - 2] = green * (GLubyte)RGBScaling;
				imageData[3 * totalPixels - 3 * i - 1] = blue * (GLubyte)RGBScaling;
			}
		}

		// Close the image file
		fclose(fileID);

		// If image data was loaded successfully, generate OpenGL texture
		if (imageData) {
			glBindTexture(GL_TEXTURE_2D, texID[j]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // No mipmaps
		}
		else {
			printf("Failed to get texture data from %s\n", textureFileNames[j]);
		}

		// Free allocated memory for the image data
		free(imageData);
	}
}


/******************************************************************************/