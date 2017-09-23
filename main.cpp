#include "gl/glut.h"
#include "lol_reader.h"
#include "lol_render.h"
#include "SOIL.h"
#include "gutility.h"
#include "gstd.h"
#include "g_gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#define VIRTUAL_KEYBOARD_RANGE_LT(mx, my, ox, oy, w, h) ((((mx) >= (ox)) && ((mx) <= ((ox) + (w)))) && (((my) >= (oy)) && ((my) <= ((oy) + (h)))))
#define VIRTUAL_KEYBOARD_RANGE_RT(mx, my, ox, oy, w, h) ((((mx) >= (ox - w)) && ((mx) <= (ox))) && (((my) >= (oy)) && ((my) <= ((oy) + (h)))))
#define VIRTUAL_KEYBOARD_RANGE_LB(mx, my, ox, oy, w, h) ((((mx) >= (ox)) && ((mx) <= ((ox) + (w)))) && (((my) >= ((oy) - (h))) && ((my) <= (oy))))
#define VIRTUAL_KEYBOARD_RANGE_RB(mx, my, ox, oy, w, h) ((((mx) >= (ox - w)) && ((mx) <= (ox))) && (((my) >= (oy)) && ((my) <= ((oy) - (h)))))

#define CHANGE_COORD_X_TO_GL(y, h, w) ((h) - (y) - (w))

/*
#include <OpenEXR/ImathMatrix.h>
	 Matrix44<float> y_r_matrix;
	 Matrix44<float> x_r_matrix;
	 y_r_matrix.setAxisAngle(Vec3<float>(0.0f, 1.0f, 0.0), y_r / 180.0f * (float)M_PI);
	 x_r_matrix.setAxisAngle(Vec3<float>(1.0f, 0.0f, 0.0), x_r / 180.0f * (float)M_PI);
	 Matrix44<float> r_matrix = x_r_matrix * y_r_matrix;
	 Matrix44<float> t_matrix;
	 t_matrix[3][0] = x_t;
	 t_matrix[3][1] = z_t;
	 t_matrix[3][2] = y_t;
	 Matrix44<float> matrix = t_matrix * r_matrix;
	 glMultMatrixf((GLfloat *)matrix.x);
	 */

enum DrawWhat
{
	DrawStatic = 1,
	DrawBone = 2,
	DrawAnimation = 4
};

struct Setting
{
	GLboolean fullscreen;
	GLuint drawWhat; // 1 - static 2 - bone 4 - animation
	GLboolean drawVirtualKey;
}setting = {
	GL_FALSE,
	DrawStatic | DrawBone | DrawAnimation,
	GL_FALSE
};

//using namespace lol;

static LolSknHeader* sknHeader = NULL;
static SklHeader *sklHeader = NULL;
static AnmHeader *anmHeader = NULL;

typedef struct _dds_link
{
	char *dds;
	struct _dds_link *link;
}dds_link;

static dds_link *dds = NULL;

static char *skn = NULL;
//static char *dds = NULL;
static char *skl = NULL;
static char *anm = NULL;

static GLuint *g_tex = NULL;

float yAxisRotate = 0;
float xAxisRotate = 0;

static float x_t = 0;
static float y_t = -350;
static float z_t = -100.0;

static float x_r = 0;
static float y_r = 0;

static const GLfloat rotation_unit = 2;
static const GLfloat move_unit = 20;
static const GLfloat turn_unit = 10;
static int frame = 0;

#define BUTTON_LEN 64
#define BUTTON_SPACING 20

extern int width;
extern int height;

static GLboolean move[TotalPosition] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
static GLboolean turn[TotalOrientation] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};
static GLboolean function[4] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};

static GLint delta_x = 0;
static GLint delta_y = 0;

static Bool keyboardHandler(KeySym key, Bool pressed, int x, int y)
{
	Position p = TotalPosition;
	Orientation o = TotalOrientation;
	switch(key)
	{
		case 'h':
			//exit(EXIT_SUCCESS);
			karinPostExit();
			return False;
		case 'w':
			p = Forward;
			break;
		case 's':
			p = Backward;
			break;
		case 'a':
			p = Left;
			break;
		case 'd':
			p = Right;
			break;
		case 'e':
			p = Up;
			break;
		case 'q':
			p = Down;
			break;
		case XK_Up:
			o = TurnUp;
			break;
		case XK_Down:
			o = TurnDown;
			break;
		case XK_Left:
			o = TurnLeft;
			break;
		case XK_Right:
			o = TurnRight;
			break;
		case 'n':
			function[1] = (GLboolean)pressed;
			function[0] = GL_FALSE;
			break;
		case 'p':
			function[0] = (GLboolean)pressed;
			function[1] = GL_FALSE;
			break;
		case 'r':
			if(pressed)
				function[3] = !function[3];
				function[2] = GL_FALSE;
			break;
		case 't':
			if(pressed)
				function[2] = !function[2];
				function[3] = GL_FALSE;
			break;
		default:
			return False;
	}
	if(p != TotalPosition)
		move[p] = (GLboolean)pressed;
	if(o != TotalOrientation)
		turn[o] = (GLboolean)pressed;
	return True;
}

static Bool mouseClickHandler(int button, Bool pressed, int x, int y)
{
	Bool res = False;
	if(!pressed)
	{
		delta_x = 0;
		delta_y = 0;
		res = True;
	}
	if(setting.drawVirtualKey)
	{
		char ch = '\0';
		if(VIRTUAL_KEYBOARD_RANGE_RT(x, y, width, 0, BUTTON_LEN, BUTTON_LEN))
			ch = 'h';
		else if(VIRTUAL_KEYBOARD_RANGE_LT(x, y, 0, 0, BUTTON_LEN, BUTTON_LEN))
			ch = 'r';
		else if(VIRTUAL_KEYBOARD_RANGE_LB(x, y, BUTTON_LEN + BUTTON_SPACING * 2, height - BUTTON_SPACING, BUTTON_LEN, BUTTON_LEN))
			ch = 's';
		else if(VIRTUAL_KEYBOARD_RANGE_LB(x, y, BUTTON_SPACING, height - BUTTON_LEN - BUTTON_SPACING * 2, BUTTON_LEN, BUTTON_LEN))
			ch = 'a';
		else if(VIRTUAL_KEYBOARD_RANGE_LB(x, y, BUTTON_LEN + BUTTON_SPACING * 2, height - (BUTTON_LEN + BUTTON_SPACING) * 2 - BUTTON_SPACING, BUTTON_LEN, BUTTON_LEN))
			ch = 'w';
		else if(VIRTUAL_KEYBOARD_RANGE_LB(x, y, (BUTTON_LEN + BUTTON_SPACING) * 2 + BUTTON_SPACING, height - BUTTON_LEN - BUTTON_SPACING * 2, BUTTON_LEN, BUTTON_LEN))
			ch = 'd';
		if(ch != '\0')
		{
			keyboardHandler(ch, pressed, x, y);
			res = True;
		}
	}
	return res;
}

static Bool mouseMotionHandler(int button, Bool pressed, int x, int y, int dx, int dy)
{
	if(pressed)
	{
		delta_x = dx;
		delta_y = dy;
		return True;
	}
	return False;
}

static void init()
{
	if(!skn)
	{
		fprintf(stderr, "No skn file.\n");
		exit(EXIT_FAILURE);
	}
	glClearColor(1.0,1.0,1.0,1.0);
	//glClearColor(0.0,0.1,0.5,1.0);
	glShadeModel(GL_SMOOTH);
	//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH);

	/*
	glEnable(GL_ALPHA);
	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GREATER, 0.9);
	*/

	//glEnable(GL_LINE_SMOOTH);
	//glEnable(GL_POINT_SMOOTH);
	//glEnable(GL_POLYGON_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	//glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	//glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sknHeader = loadSkn(skn);
	if(!sknHeader)
	{
		fprintf(stderr, "Unsupported skn file.\n");
		exit(EXIT_FAILURE);
	}

	if(dds && sknHeader -> numMaterials)
	{
		int i;
		dds_link *d = dds;
		g_tex = (GLuint *)malloc(sizeof(GLuint) * sknHeader -> numMaterials);
		memset(g_tex, 0, sizeof(GLuint) * sknHeader -> numMaterials);
		GLuint *texids = (GLuint *)malloc(sizeof(GLuint) * sknHeader -> numMaterials);
		memset(texids, 0, sizeof(GLuint) * sknHeader -> numMaterials);
		for(i = 0; i < sknHeader -> numMaterials; i++)
		{
			if(d)
			{
				texinfo *tex = loadDds(d -> dds);
				if(tex)
				{
					texids[i] = tex -> texid;
					free(tex);
				}
				else
					texids[i] = 0;

				if(d -> link)
					d = d -> link;
				else
					break;
			}
		}

		GLuint last = 0;
		int j = sknHeader -> numMaterials - 1;
		for(i = sknHeader -> numMaterials - 1; i >= 0; i--)
		{
			for(; j >= 0; j--)
			{
				if(texids[j] != 0)
				{
					last = texids[j];
					j--;
					break;
				}
			}
			g_tex[i] = last;
		}

		free(texids);
	}
	else
		printf("No dds file.\n");

	// --------------
	if(skl)
		sklHeader = gLoadSkl(skl, sknHeader -> version);
	else
		printf("No skl file.\n");
	//LolAnmHeader*anm = loadAnm("Leblanc_Run.anm");
	if(anm)
		anmHeader = gLoadAnm(anm, sklHeader);
	else
		printf("No anm file.\n");
}

static void draw2D()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, (GLdouble)width, 0, (GLdouble)height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}

static void draw3D()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45,(GLdouble)width / (GLdouble)height, 10, 10000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);

}

static void reshape(int w, int h)
{
	if(w <= 0)
		w = 1;
	if(h <= 0)
		h = 1;
	glViewport(0, 0, w, h);

}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_COLOR);
	if(anmHeader)
	{
		if(frame < 0)
			frame = anmHeader -> frameCount - 1;
		if(frame >= anmHeader -> frameCount)
			frame = 0;
	}

	if(setting.drawVirtualKey)
	{
		draw2D();
		{
			GLfloat color[] = {1.0f, 0.5f, 1.0f};
			GLfloat font_color[] = {0.0f, 0.0f, 0.0f};
			draw_button(0, CHANGE_COORD_X_TO_GL(0, height, BUTTON_LEN), BUTTON_LEN, BUTTON_LEN, color, font_color, "R");
			draw_button(width - BUTTON_LEN, CHANGE_COORD_X_TO_GL(0, height, BUTTON_LEN), BUTTON_LEN, BUTTON_LEN, color, font_color, "Q");
			draw_button(BUTTON_LEN + BUTTON_SPACING * 2, BUTTON_SPACING, BUTTON_LEN, BUTTON_LEN, color, font_color, "S");
			draw_button(BUTTON_LEN + BUTTON_SPACING * 2, (BUTTON_LEN + BUTTON_SPACING) * 2 + BUTTON_SPACING, BUTTON_LEN, BUTTON_LEN, color, font_color, "W");
			draw_button(BUTTON_SPACING, BUTTON_LEN + BUTTON_SPACING * 2, BUTTON_LEN, BUTTON_LEN, color, font_color, "A");
			draw_button((BUTTON_LEN + BUTTON_SPACING) * 2 + BUTTON_SPACING, BUTTON_LEN + BUTTON_SPACING * 2, BUTTON_LEN, BUTTON_LEN, color, font_color, "D");
			if(anmHeader)
			{
				GLfloat block_color[] = {0.0f, 0.0f, 1.0f};
				GLfloat line_color[] = {0.0f, 1.0f, 0.0f};
				draw_slider(width / 2 - 200, height - 40, 400, 2, frame, 0, anmHeader -> frameCount - 1, line_color, 15, 30, block_color, GL_TRUE);
			}
		}
	}

	draw3D();

	if(g_tex)
		glColor3f(1.0,1.0,1.0);
	else
		glColor3f(1.0,0.5,1.0);
	glPushMatrix();
	{
		//gluLookAt(x_t, z_t, y_t, 0, 0, 0, 0, 1, 0);
		glRotatef(y_r,0,1,0);
		glRotatef(x_r,1,0,0);

		glTranslatef(x_t, z_t, y_t);

		//glScalef(1.0, 1.5, 1.0);

		if(sknHeader && (setting.drawWhat & DrawStatic))
		{
			glPushMatrix();
			{
				glTranslatef(140.0, 0.0, 0.0);
				glRotatef(yAxisRotate,0,1,0);
				glRotatef(xAxisRotate,1,0,0);
				drawSkn(sknHeader, g_tex);
			}
			glPopMatrix();
		}

		if(sklHeader)
		{
			if(anmHeader && (setting.drawWhat & DrawAnimation))
			{
				updateBone(sklHeader, anmHeader, frame);
				glPushMatrix();
				{
					glTranslatef(-20.0, 0.0, 0.0);
					glRotatef(yAxisRotate,0,1,0);
					glRotatef(xAxisRotate,1,0,0);
					drawSkn(sknHeader, g_tex, sklHeader, anmHeader, frame);
				}
				glPopMatrix();
			}

			if(setting.drawWhat & DrawBone)
			{
				glPushMatrix();
				{
					glTranslatef(-140.0, 0.0, 0.0);
					glRotatef(yAxisRotate,0,1,0);
					glRotatef(xAxisRotate,1,0,0);
					drawBone(sklHeader);
				}
				glPopMatrix();
			}
		}

	}
	glPopMatrix();

	glFlush();

	//glutSwapBuffers();
	//Sleep(10);
	//usleep(10);
	//glutPostRedisplay();
}

static Bool idle()
{
	if(turn[TurnUp])
		x_r -= turn_unit;
	if(turn[TurnDown])
		x_r += turn_unit;
	if(turn[TurnLeft])
		y_r -= turn_unit;
	if(turn[TurnRight])
		y_r += turn_unit;
	y_r = formatAngle(y_r);
	x_r = formatAngle(x_r);
	if(move[Forward])
		y_t += abs(y_r) < 90 || abs(y_r) >= 270  ? move_unit : -move_unit;
	if(move[Backward])
		y_t -= abs(y_r) < 90 || abs(y_r) >= 270 ? move_unit : -move_unit;
	if(move[Left])
		x_t += abs(x_r) < 90 || abs(x_r) >= 270  ? move_unit : -move_unit;
	if(move[Right])
		x_t -= abs(x_r) < 90 || abs(x_r) >= 270  ? move_unit : -move_unit;
	if(move[Up])
		z_t -= move_unit;
	if(move[Down])
		z_t += move_unit;
	if(function[0])
		frame--;
	if(function[1])
		frame++;
	if(function[2])
		frame--;
	if(function[3])
		frame++;

	if(delta_x != 0 || delta_y != 0)
	{
		xAxisRotate += delta_y * rotation_unit;
		yAxisRotate += delta_x * rotation_unit;
		return True;
	}

	int i;
	for(i = 0; i < TotalPosition; i++)
		if(move[i])
			return True;
	for(i = 0; i < TotalOrientation; i++)
		if(turn[i])
			return True;
	for(i = 0; i < 4; i++)
		if(function[i])
			return True;
	return False;
}

static void shutdown()
{
	if(sknHeader && g_tex)
	{
		glDeleteTextures(sknHeader -> numMaterials, g_tex);
	}
	free(anm);
	free(skl);
	free(dds);
	free(skn);
	freeSkn(sknHeader);
	gFreeSkl(sklHeader);
	gFreeAnm(anmHeader);
	free(g_tex);
}

static void printVersion()
{
	printf("%s\nversion: %s  state: %s\n", _G_VIEWER_NAME, _G_VIEWER_VERSION, _G_VIEWER_STATE); KARIN_ENDL
}

static void printHelp()
{
	printf("Usage: %s [<arguments> <file> ...]", _G_VIEWER_NAME); KARIN_ENDL
	printf(" version: %s(%s)", _G_VIEWER_VERSION, _G_VIEWER_STATE); KARIN_ENDL
	printf(" arguments:"); KARIN_ENDL
	printf("  -n <file>:\n  --skn=<file>: Load skn file."); KARIN_ENDL
	printf("  -d <file>:\n  --dds=<file, file, ...>: Load dds files."); KARIN_ENDL
	printf("  -l <file>:\n  --skl=<file>: Load skl file."); KARIN_ENDL
	printf("  -a <file>:\n  --anm=<file>: Load anm file."); KARIN_ENDL
	printf("  -f:\n  --fullscreen: Show fullscreen."); KARIN_ENDL
	printf("  -r <type>:\n  --draw=<type>: 1, static: draw static model, 2 - bone: draw bone, 3 - animation: draw animation model, 4 - all: draw all. use ',' split."); KARIN_ENDL
	printf("  -v:\n  --version: Print version."); KARIN_ENDL
	printf("  -h:\n  --help: Print this text."); KARIN_ENDL
}

GLuint parseDrawWhat(const char *arg)
{
	GLuint drawWhat = DrawStatic | DrawBone | DrawAnimation;
	if(!arg)
		return drawWhat;
	if(strcmp(arg, "all") == 0)
		return drawWhat;

	size_t len = strlen(arg);
	char str[len + 1];
	strcpy(str, arg);
	str[len] = '\0';
	char *ptr = NULL;
	const char *split = ",";
	ptr = strtok(str, split);
	drawWhat = 0;
	while(ptr != NULL)
	{
		if(strcmp(ptr, "static") == 0)
			drawWhat |= DrawStatic;
		else if(strcmp(ptr, "bone") == 0)
			drawWhat |= DrawBone;
		else if(strcmp(ptr, "animation") == 0)
			drawWhat |= DrawAnimation;
		else if(strcmp(ptr, "all") == 0)
		{
			drawWhat = DrawStatic | DrawBone | DrawAnimation;
			break;
		}
		else
			fprintf(stderr, "unknow draw type: %s\n", ptr);
		ptr = strtok(NULL, split);
	}
	if(drawWhat == 0)
		drawWhat = DrawStatic | DrawBone | DrawAnimation;
	return drawWhat;
}

static int parseArg(int argc, char *argv[])
{
	int res = 1;
	const char *arg = "n:d:l:a:r:fkvh";
	const option long_arg[] = {
		{"skn", 2, NULL, 'n'},
		{"dds", 2, NULL, 'd'},
		{"skl", 2, NULL, 'l'},
		{"anm", 2, NULL, 'a'},
		{"fullscreen", 0, NULL, 'f'},
		{"draw", 2, NULL, 'r'},
		{"virtual-key", 0, NULL, 'k'},
		{"version", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'}
	};
	int ch;
	dds_link *d = NULL;
	dds_link *last = dds;
	while((ch = getopt_long(argc, argv, arg, long_arg, NULL)) != -1)
	{
		switch(ch)
		{
			case 'n':
				if(!skn)
				{
					skn = (char *)malloc(strlen(optarg) + 1);
					memset(skn, 0, strlen(optarg) + 1);
					strncpy(skn, optarg, strlen(optarg));
					skn[strlen(optarg)] = '\0';
				}
				break;
			case 'd':
				d = (dds_link *)malloc(sizeof(dds_link));
				memset(d, 0, sizeof(dds_link));
				d -> dds = (char *)malloc(strlen(optarg) + 1);
				memset(d -> dds, 0, strlen(optarg) + 1);
				strncpy(d -> dds, optarg, strlen(optarg));
				d -> dds[strlen(optarg)] = '\0';
				d -> link = NULL;
				if(last)
				{
					last -> link = d;
					last = d;
				}
				else
				{
					dds = d;
					last = dds;
				}
				break;
			case 'l':
				if(!skl)
				{
					skl = (char *)malloc(strlen(optarg) + 1);
					memset(skl, 0, strlen(optarg) + 1);
					strncpy(skl, optarg, strlen(optarg));
					skl[strlen(optarg)] = '\0';
				}
				break;
			case 'a':
				if(!anm)
				{
					anm = (char *)malloc(strlen(optarg) + 1);
					memset(anm, 0, strlen(optarg) + 1);
					strncpy(anm, optarg, strlen(optarg));
					anm[strlen(optarg)] = '\0';
				}
				break;
			case 'f':
				setting.fullscreen = GL_TRUE;
				break;
			case 'k':
				setting.drawVirtualKey = GL_TRUE;
				break;
			case 'r':
				setting.drawWhat = parseDrawWhat(optarg);
				break;
			case 'v':
				res = 0;
				printVersion();
				break;
			case 'h':
				res = 0;
				printHelp();
				break;
			default:
				res = -1;
				printHelp();
				break;
		}
	}
	return res;
}

int main(int argc, char *argv[])
{
	int res = parseArg(argc, argv);
	if(res == -1)
		exit(EXIT_FAILURE);
	else if(res == 0)
		exit(EXIT_SUCCESS);
#ifdef _G_TEST
	const char *nargv[] = {
		"",
		"./Leblanc.skn",
		"./Leblanc.dds",
		"./Leblanc.skl",
		"./Leblanc_Run.anm"
	};
	skn = (char *)calloc(strlen(nargv[1]) + 1, sizeof(char));
	strncpy(skn, nargv[1], strlen(nargv[1]));
	dds = (char *)calloc(strlen(nargv[2]) + 1, sizeof(char));
	strncpy(dds, nargv[2], strlen(nargv[2]));
	skl = (char *)calloc(strlen(nargv[3]) + 1, sizeof(char));
	strncpy(skl, nargv[3], strlen(nargv[3]));
	anm = (char *)calloc(strlen(nargv[4]) + 1, sizeof(char));
	strncpy(anm, nargv[4], strlen(nargv[4]));
#endif
	//printf("%s_%s", skn, dds);
	karinInitX11GLObjectAndGLUT(&argc, argv);
	karinSetWindowPosiotionAndSize(0, 0, 854, 480);
	if(!karinCreateGLXWindow("GL lol model viewer"))
	{
		fprintf(stderr, "Init GLX fail.\n");
	}

	karinFullscreen(True);
	karinRegisterInitFunc(init);
	karinRegisterReshapeFunc(reshape);
	karinRegisterDrawFunc(display);

	karinRegisterKeyFunc(keyboardHandler);
	karinRegisterMouseFunc(mouseClickHandler);
	karinRegisterMotionFunc(mouseMotionHandler);

	karinRegisterIdleFunc(idle);
	karinRegisterExitFunc(shutdown);

	if(!setting.fullscreen)
		karinFullscreen(setting.fullscreen);

	karinMainLoop();
	return 0;
	/*
		 glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
		 */
}
