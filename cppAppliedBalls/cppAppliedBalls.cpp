#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <ctime>
#include <ourTimer.h>
#include <GL/freeglut.h>

in_timer tmr;

std::mutex mainLock;

bool exitCall = false;
bool thread1Complete = false;
bool thread2Complete = false;
bool work = true;

float dt = 0;

class gField
{
public:	
	float w, h;
	gField(float nw, float nh) { w = nw; h = nh; }
};

class ball
{
public:
	gField * gfRef;
	int cr, cg, cb;
	float x, y, r, spd;
	int kx, ky;
	void move(float dt);
	ball(gField * vGfRef) { 
		std::cout << "Ball added!\n";
		gfRef = vGfRef;  
		r = rand() % 20 + 5;

		x = rand() % int(gfRef->w - 2 * r); 
		y = rand() % int(gfRef->h - 2 * r);

		cr = rand() % 256;
		cg = rand() % 256;
		cb = rand() % 256;

		spd = 100; 
		
		int d = rand() % 4;
		if (d == 0)
		{
			kx = 1; ky = 1;
		}
		if (d == 1)
		{
			kx = -1; ky = -1;
		}
		if (d == 2)
		{
			kx = 1; ky = -1;
		}
		if (d == 3)
		{
			kx = -1; ky = 1;
		}
	};
};

typedef std::vector<ball> vBalls;

vBalls balls = {};

void addBall(gField * vGfRef)
{
	mainLock.lock();
	balls.push_back(ball(vGfRef));
	mainLock.unlock();
}

void cbDisplay();

void ball::move(float dt)
{
	//move
	x += spd * dt*kx;
	y += spd * dt*ky;

	//std::cout << "Ball moved to: " << x << ";" << "y" << y << "\n";

	//handle wall collisions
	if ((x - r) < 0)
	{
		kx *= -1;
		x = r;
	}
	if ((y - r) < 0)
	{
		ky *= -1;
		y = r;
	}
	if ((x + r) > gfRef->w)
	{
		kx *= -1;
		x = gfRef->w - r;
	}
	if ((y - r) > gfRef->h)
	{ 
		ky *= -1;
		y = gfRef->h - r;
	}
}

void cbIdle()
{
	if (exitCall)
	{
		glutLeaveMainLoop();
	}

	cbDisplay();
}

void cbReshape(int x, int y)
{
	glViewport(0,0,x,y);
}

void drawCircle(float x, float y, float r, int res, float cr, float cg, float cb)
{

	//std::cout << "Drawing circle at " << x << "; " << y << "\n";

	/*glBegin(GL_TRIANGLES);

	glColor3f(1.0f, 0.0f, 0.0f);

	glVertex2f(x - r, y - r);
	glVertex2f(x + r, y - r);
	glVertex2f(x    , y + r);

	glEnd();*/

	glBegin(GL_TRIANGLE_FAN);

	float step = 360.0f / res;
	
	glColor3f(cr, cg, cb);

	glVertex2f(x, y);

	for (int i = 0; i < res; i++)
	{
		float cx = r * sin(step*i*3.14 / 180) + x;
		float cy = r * cos(step*i*3.14 / 180) + y;
		glVertex2f(cx, cy);
	}

	glVertex2f(x, y+r);	

	glEnd();

}


void cbDisplay()
{	

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int l = balls.size();

	if (l > 0)
	{
		gField * gfr = balls[0].gfRef;

		//std::cout << "GFR: x = " << gfr->w << "; y = " << gfr->h << "\n";

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, gfr->w, 0, gfr->h);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glPushMatrix();

		for (int i = 0; i < l; i++)
		{
			drawCircle(balls[i].x, balls[i].y, balls[i].r, 8, 
				(float)balls[i].cr/256.0f, (float)balls[i].cg / 256.0f, (float)balls[i].cb / 256.0f);
		}

		glPopMatrix();

		glutSwapBuffers();

	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(-1, 1, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glPushMatrix();

		drawCircle(0, 0, 1, 8, 1, 0, 0);

		glPopMatrix();

		glutSwapBuffers();
	}
}


void glutThreadFunc(int argcp, char **argv)
{
	glutInit(&argcp, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(800, 600);
	glutCreateWindow("GLUT window :D");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);

	glutIdleFunc(cbIdle);
	glutReshapeFunc(cbReshape);
	glutDisplayFunc(cbDisplay);

	glutMainLoop();

	thread1Complete = true;
}

void ballsThreadFunc()
{
	while (!exitCall)
	{
		dt = tmr.getdeltatime();

		mainLock.lock();
		int l = balls.size();
		for (int i = 0; i < l; i++) balls[i].move(dt);
		mainLock.unlock();

	}
	thread2Complete = true;
}


int main(int argcp, char **argv)
{

	srand(time(NULL));

	gField fld(800, 600);

	std::thread glutThread(glutThreadFunc, argcp, argv);
	glutThread.detach();

	std::thread ballsThread(ballsThreadFunc);
	ballsThread.detach();
	
	while (work)
	{
		std::cout << "Enter Command:\n";
		std::string cmd;
		std::cin >> cmd;

		if (cmd == "exit")
		{
			work = false;
			exitCall = true;
		}
		if (cmd == "add")
		{
			addBall(&fld);
		}
	}

	while ((!thread1Complete) && (!thread2Complete)) { std::cout << "Pending thread...\n"; }
	system("pause");

}