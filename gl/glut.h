#ifndef _KARIN_GLUT_H
#define _KARIN_GLUT_H

/* ASCI-C std */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

/* GNU Unix std */
#include <sys/time.h>
#include <unistd.h>

/* X11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* OpenGL & GLX & GLU */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

/* GLX */
#include <GL/glx.h>

/* Macros */
// utility macros
#define KARIN_BUFFER_OFFSET(x, y) ((GLubyte *)NULL + sizeof(y) * (x))
#define KARIN_ENDL printf("\n");
#define KARIN_CASEANDEQUAL(value, index) \
	case value:\
		i = index;\
		break;

// X11
#define KEY_MASK	(KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK	(ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask)
#define X_MASK		(KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask)
#define _NET_WM_STATE_REMOVE        0	/* remove/unset property */
#define _NET_WM_STATE_ADD           1	/* add/set property */
#define _NET_WM_STATE_TOGGLE        2	/* toggle property  */

/* function pointer typedef*/
// init function
typedef void (*initGLFunc) (void);
// draw GL function
typedef void (*drawGLFunc) (void);
// reshape function
typedef void (*reshapeGLFunc) (int w, int h);
// mouse and key event function
// return True if event has been handled
// key: key sym
// x, y: mouse x and y
// pressed: mouse or key is pressed
// dx, dy: mouse motion delta position
typedef Bool (*keyHandlerFunc) (KeySym key, Bool pressed, int x, int y);
typedef Bool (*mouseHandlerFunc) (int button, Bool pressed, int x, int y);
typedef Bool (*motionHandlerFunc) (int button, int pressed, int x, int y, int dx, int dy);
// window resize event function
typedef void (*resizeHandlerFunc) (void);
// atexit function
typedef void (*exitXFunc) (void);
// idle function
// return True if need re-draw GL
typedef Bool (*idleFunc) (void);

/* extern variant */
Display *dpy = NULL;
Window win = 0;

GLXWindow glw = 0;
GLXContext glc = 0;

int width = 0;
int height = 0;

int x = 0;
int y = 0;

static unsigned long loop_interval = 3000;

/* static variant */
static int *attr = NULL;
static unsigned int attr_count = 0;

/* register function pointer */
static initGLFunc initGL = NULL;
static drawGLFunc drawGL = NULL;
static reshapeGLFunc reshapeGL = NULL;

static keyHandlerFunc keyHandler = NULL;
static mouseHandlerFunc mouseHandler = NULL;
static resizeHandlerFunc resizeHandler = NULL;
static motionHandlerFunc motionHandler = NULL;

static idleFunc idleHandler = NULL;
static exitXFunc exitX = NULL;

/* static attribute */
// default: auto reshape is GL_TRUE auto swap buffer is GL_TRUE
// if running is GL_TRUE application is in loop
// glxHasInit is True when GLX has initial
static GLboolean autoReshape = GL_TRUE;
static GLboolean autoSwapBuffers = GL_TRUE;
static Bool glxHasInit = False;
static GLboolean running = GL_TRUE;

/* extern function */
// GLX init function
Bool karinSetGLXInitAttributeArray(const int args[]);
Bool karinSetGLXInitAttribute(int param, ...);
Bool karinAddGLXInitAttribute(int param, ...);
void karinSetAutoReshapeWhenResize(GLboolean b);
Bool karinInitX11GLObject(void);
Bool karinInitX11GLObjectAndGLUT(int *argc, char *argv[]);
void karinSetWindowPosiotionAndSize(int lx, int ly, int w, int h);
Bool karinCreateGLXWindow(const char *title);

// register function
void karinRegisterDrawFunc(drawGLFunc f);
void karinRegisterReshapeFunc(reshapeGLFunc f);
void karinRegisterInitFunc(initGLFunc f);
void karinRegisterKeyFunc(keyHandlerFunc f);
void karinRegisterMouseFunc(mouseHandlerFunc f);
void karinRegisterIdleFunc(idleFunc f);
void karinRegisterExitFunc(exitXFunc f);
void karinRegisterMotionFunc(motionHandlerFunc f);
void karinRegisterResizeFunc(resizeHandlerFunc f);

// application function
void karinPostExit(void);
void karinMainLoop(void);
void karinFullscreen(Bool fs);

// GL function
void karinSwapBuffers(void);
void karinSetAutoSwapBuffers(GLboolean b);
GLboolean karinQueryExtension(const char *extName);
void karinPostDrawGL(void);
void (*karinGetProcAddress(const GLubyte *procname))(void);
Bool karinUseXFont(const char *name, int start, int size, GLuint list_start);

// utility function
float karinCastFPS(void);

/* static function */
// default handler
static void karinGLInit(void);
static void karinGLDraw(void);
static void karinGLReshape(int w, int h);

static void karinXResizeHandler(void);
static int karinXErrorHandler(Display *d, XErrorEvent *eev);
static void karinGLXErrorHandler(void);

// application function
static Bool karinCreateGLContext(const char* title);
static Bool karinXEventLoop(void);
static void karinShutdown(void);
static void karinPrintGLInfo(void);

//static void GLimp_DisableComposition(void);

Bool karinSetGLXInitAttributeArray(const int args[])
{
	size_t len = sizeof(args) / sizeof(int);
	if(len == 0)
	{
		fprintf(stderr, "No configure attritube.\n");
		return False;
	}
	free(attr);
	attr = NULL;
	attr_count = 0;
	void *ptr = calloc(len, sizeof(int));
	if(!ptr)
	{
		fprintf(stderr, "Alloc fail.\n");
		return False;
	}
	attr = (int *)ptr;
	memcpy(attr, args, sizeof(args));
	attr_count = len - 1;
	return True;
}

Bool karinSetGLXInitAttribute(int param, ...)
{
	if(param < 2)
	{
		fprintf(stderr, "Need 1 pair of configure attritube.\n");
		return False;
	}
	free(attr);
	attr = NULL;
	attr_count = 0;
	void *ptr = calloc(param + 1, sizeof(int));
	if(!ptr)
	{
		fprintf(stderr, "Alloc fail.\n");
		return False;
	}
	attr = (int *)ptr;
	va_list args;
	va_start(args, param);
		int i;
		for(i = 0; i < param; i++)
			attr[i] = va_arg(args, int);
	va_end(args);
	attr[i] = None;
	attr_count = param;
	return True;
}

Bool karinAddGLXInitAttribute(int param, ...)
{
	if(param < 2)
	{
		fprintf(stderr, "Need 1 pair of configure attritube.\n");
		return False;
	}
	void *ptr = NULL;
	if(attr_count == 0)
	{
		int arg_arr[param + 1];
		va_list args;
		va_start(args, param);
		int i;
		for(i = 0; i < param; i++)
			arg_arr[i] = va_arg(args, int);
		va_end(args);
		arg_arr[i] = None;
		return karinSetGLXInitAttributeArray(arg_arr);
	}
	int old_conf[attr_count];
	memcpy(old_conf, attr, sizeof(old_conf));
	ptr = realloc((void *)attr, sizeof(int) * (attr_count + param + 1));
	if(!ptr)
	{
		fprintf(stderr, "ReAlloc fail.\n");
		return False;
	}
	attr = (int *)ptr;
	memcpy(attr, old_conf, sizeof(old_conf));
	va_list args;
	va_start(args, param);
		int i;
		for(i = 0; i < param; i++)
			attr[attr_count + i] = va_arg(args, int);
	va_end(args);
	attr[attr_count + i] = None;
	attr_count += param;
	return True;
}

void karinSetAutoReshapeWhenResize(GLboolean b)
{
	autoReshape = b;
}

static void karinGLInit(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glShadeModel(GL_SMOOTH);
	//glEnable(GL_DEPTH_TEST);
}

static void karinXResizeHandler(void)
{
	if(autoReshape)
		if(reshapeGL)
			reshapeGL((GLsizei)width, (GLsizei)height);
}

static void karinGLDraw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFlush();
}

static void karinGLReshape(int w, int h)
{
	if(w < 1)
		w = 1;
	if(h < 1)
		h = 1;
	glViewport (0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-5.0f, 5.0f, -5.0f, 5.0f, 0.0f, 10.0f);
	gluPerspective(45.0, 2, 0.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static int karinXErrorHandler(Display *d, XErrorEvent *eev)
{
	char str[100];
	XGetErrorText(d, eev -> error_code, str, 99);
	str[99] = '\0';
	fprintf(stderr, "err -> %d, str -> %s\n", eev -> error_code, str);
	return 0;
}

static void karinGLXErrorHandler(void)
{
	fprintf(stderr, "glx error.\n");
}

Bool karinInitX11GLObject(void)
{
	initGL = karinGLInit;
	drawGL = karinGLDraw;
	reshapeGL = karinGLReshape;

	resizeHandler = karinXResizeHandler;

	XSetErrorHandler(karinXErrorHandler);

	return karinSetGLXInitAttribute(8, 
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DOUBLEBUFFER, True,
			/*
				 GLX_RED_SIZE, 5,
				 GLX_GREEN_SIZE, 6,
				 GLX_BLUE_SIZE, 5,
				 */
			GLX_DEPTH_SIZE, 24,
			GLX_BUFFER_SIZE, 16
			);
}

Bool karinInitX11GLObjectAndGLUT(int *argc, char *argv[])
{
	if(karinInitX11GLObject())
	{
		glutInit(argc, argv);
		return True;
	}
	return False;
}

static Bool karinCreateGLContext(const char* title)
{
	Window root;
	XSetWindowAttributes swa;
	XSetWindowAttributes xattr;
	Atom wm_state;
	Colormap cmap;
	XWMHints hints;

	dpy = XOpenDisplay(NULL);
	if ( dpy == NULL )
		return False;
	//root = DefaultRootWindow(dpy);
	root = RootWindow( dpy, DefaultScreen( dpy ));
	//int blackColour = BlackPixel(dpy, DefaultScreen(dpy));
	swa.event_mask = X_MASK;//ExposureMask | PointerMotionMask | KeyPressMask;
	//win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, esContext.width, esContext.height, 0, blackColour, blackColour);
	//XSelectInput(dpy, win, X_MASK);
	//Set the window attribute override allowed
	/*
	xattr.override_redirect = 0;
	XChangeWindowAttributes ( dpy, win, CWOverrideRedirect, &xattr );
	//Sets the window manager hints that include icon information and location, the initial state of the window, and
	hints.input = 1;
	hints.flags = InputHint;
	XSetWMHints(dpy, win, &hints);
	wm_state = XInternAtom (dpy, "_NET_WM_STATE", 0);
	memset ( &xev, 0, sizeof(xev) );
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = 0;
	XSendEvent (dpy,DefaultRootWindow ( dpy ),0,SubstructureNotifyMask,&xev );
	*/
	//XMoveResizeWindow(dpy, win, 0, 0, esContext -> width, esContext -> height);

	int numConfigs;
	GLXFBConfig *config = NULL;
	GLXFBConfig *configs = NULL;
	XVisualInfo *xvi = NULL;
	//init glx
	if (!glXQueryExtension(dpy, NULL, NULL))
	{
		karinGLXErrorHandler();
		return False;
	}
	/*
	//get configs
	configs = glXGetFBConfigs(dpy, DefaultScreen(dpy), &numConfigs);
	if (!configs)
	{
		karinGLXErrorHandler();
		return False;
	}
	printf("%d___\n", numConfigs);
	*/
	//choose config
	config = glXChooseFBConfig(dpy, DefaultScreen(dpy), attr, &numConfigs);
	if(!config)
	{
		karinGLXErrorHandler();
		return False;
	}
	int i;
	printf("glX config -> %d\n", numConfigs);
	for(i = 0; i < numConfigs; i++)
	{
		xvi = glXGetVisualFromFBConfig (dpy, config[i]);
		if(!xvi)
		{
			karinGLXErrorHandler();
			continue;
		}
		else
			break;
	}
	cmap = XCreateColormap(dpy, root, xvi -> visual,AllocNone);
	swa.colormap = cmap;
	win = XCreateWindow(dpy, RootWindow(dpy, xvi -> screen),0, 0, width, height, 0, xvi -> depth, InputOutput,xvi -> visual, CWEventMask,&swa );
	XMapWindow (dpy, win);
	XStoreName (dpy, win, title);
	XFlush(dpy);
	glw = glXCreateWindow (dpy, config[i], win, NULL);
	glc = glXCreateNewContext (dpy, config[i], GLX_RGBA_TYPE, NULL, GL_TRUE);
	printf("glX window -> %#x\nglX context -> %ld\n", glw, glc);
	glXMakeContextCurrent (dpy, win, win, glc);

	return True;
}

static Bool karinXEventLoop(void)
{
	if(!dpy)
	{
		fprintf(stderr, "X is not initialized!\n");
		return False;
	}
	XEvent xev;
	KeySym key;
	char text;
	Bool pressed;
	static char buf[64];
	KeySym keysym;

	static int mx = 0;
	static int my = 0;
	int dx = 0;
	int dy = 0;
	static Bool mousePressed = False;
	Bool handle = False;

	while (XPending(dpy))
	{
		XNextEvent(dpy, &xev);
		switch(xev.type)
		{
			case KeyPress:
			case KeyRelease:
				pressed = (xev.xkey.type == KeyPress ? True : False);
				XLookupString(&(xev.xkey), buf, sizeof buf, &keysym, 0);
				if(keyHandler)
				{
					keyHandler(keysym, pressed, mx, my);
					//handle = True;
				}
					/*
				if (XLookupString(&xev.xkey,&text,1,&key,0)==1)
				{
					if (esContext.keyFunc != NULL)
						esContext.keyFunc(text, 0, 0);
				}
						*/
				break;
			case CreateNotify:
				x = xev.xcreatewindow.x;
				y = xev.xcreatewindow.y;
				width = xev.xcreatewindow.width;
				height = xev.xcreatewindow.height;
				if(resizeHandler)
					resizeHandler();
				handle = True;
				break;
			case ConfigureNotify:
				x = xev.xconfigure.x;
				y = xev.xconfigure.y;
				width = xev.xconfigure.width;
				height = xev.xconfigure.height;
				if(resizeHandler)
					resizeHandler();
				handle = True;
				break;
			case MotionNotify:
				dx = xev.xmotion.x - mx;
				dy = xev.xmotion.y - my;
				mx = xev.xmotion.x;
				my = xev.xmotion.y;
				if(motionHandler)
				{
					motionHandler(xev.xmotion.state, mousePressed, mx, my, dx, dy);
					//handle = True;
				}
				break;
			case ButtonPress:
			case ButtonRelease:
				mx = xev.xbutton.x;
				my = xev.xbutton.y;
				mousePressed = (xev.xbutton.type == ButtonPress ? True : False);
				if(mouseHandler)
				{
					mouseHandler(xev.xbutton.button, mousePressed, mx, my);
					//handle = True;
				}
				break;
			case DestroyNotify:
				break;
			default:
				printf("Unhandle XEvent -> %d\n", xev.type);
				break;
		}
	}
	return handle;
}

void karinRegisterDrawFunc(drawGLFunc f)
{
	drawGL = f;
}

void karinRegisterReshapeFunc(reshapeGLFunc f)
{
	reshapeGL = f;
}

void karinRegisterInitFunc(initGLFunc f)
{
	initGL = f;
}

void karinRegisterKeyFunc(keyHandlerFunc f)
{
	keyHandler = f;
}

void karinRegisterMouseFunc(mouseHandlerFunc f)
{
	mouseHandler = f;
}

void karinRegisterIdleFunc(idleFunc f)
{
	idleHandler = f;
}

void karinRegisterExitFunc(exitXFunc f)
{
	exitX = f;
}

void karinRegisterMotionFunc(motionHandlerFunc f)
{
	motionHandler = f;
}

void karinRegisterResizeFunc(resizeHandlerFunc f)
{
	resizeHandler = f;
}

float karinCastFPS(void)
{
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	float totaltime = 0.0f;
	unsigned int frames = 0;
	gettimeofday ( &t1 , &tz );
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	t1 = t2;
	totaltime += deltatime;
	frames++;
	if (totaltime > 2.0f)
	{
		printf("%4d frames rendered in %1.4f seconds -> FPS=%3.4f\n", frames, totaltime, frames/totaltime);
		totaltime -= 2.0f;
		frames = 0;
	}
	return frames / totaltime;
}

static void karinShutdown(void)
{
	if(exitX)
		exitX();
	if(glxHasInit)
	{
		glXDestroyContext(dpy, glc);
		glXDestroyWindow(dpy, glw);
		glXMakeContextCurrent (dpy, 0, 0, 0);
	}

	if(dpy)
	{
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
}

void karinPostExit(void)
{
	running = GL_FALSE;
}

void karinSwapBuffers(void)
{
	if(glxHasInit)
		glXSwapBuffers(dpy, glw);
}

void karinSetAutoSwapBuffers(GLboolean b)
{
	autoSwapBuffers = b;
}

Bool karinUseXFont(const char *name, int start, int size, GLuint list_start)
{
	if(glxHasInit)
	{
		XFontStruct *fontStruct = XLoadQueryFont(dpy, name);
		if(!fontStruct)
			return False;
		glXUseXFont(fontStruct -> fid, start, size, list_start);
		return True;
	}
	return False;
}

void (*karinGetProcAddress(const GLubyte *procname))(void)
{
	if(glxHasInit)
		return glXGetProcAddress(procname);
	else
		return NULL;
}

void karinPostDrawGL(void)
{
	if(drawGL)
		drawGL();
	if(autoSwapBuffers)
		karinSwapBuffers();
}

void karinMainLoop(void)
{
	if(!glxHasInit)
		return;
	if(initGL)
		initGL();
	while(running)
	{
		if(karinXEventLoop())
		{
			karinPostDrawGL();
		}
		else
		//karinEventHandler();
		{
			if(idleHandler)
			{
				if(idleHandler())
					karinPostDrawGL();
			}
		}
		if(loop_interval)
			usleep(loop_interval);
		//float delta = karinCastFPS();
	}
	karinShutdown();
}

void karinSetWindowPosiotionAndSize(int lx, int ly, int w, int h)
{
	x = lx;
	y = ly;
	if(w < 1)
		w = 1;
	if(h < 1)
		h = 1;
	width = w;
	height = h;
}

Bool karinCreateGLXWindow(const char *title)
{
	if(!attr)
		return False;
	if(glxHasInit)
	{
		glXDestroyContext(dpy, glc);
		glXDestroyWindow(dpy, glw);
		glXMakeContextCurrent (dpy, 0, 0, 0);
		glxHasInit = False;
	}
	if(dpy)
	{
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	glxHasInit = karinCreateGLContext(title);
	karinPrintGLInfo();
	return glxHasInit;
}

static void karinPrintGLInfo()
{
	printf("------------------ OpenGL -----------------"); KARIN_ENDL
	printf("Version -> %s", glGetString(GL_VERSION)); KARIN_ENDL
	printf("Vendor -> %s", glGetString(GL_VENDOR)); KARIN_ENDL
	printf("Renderer -> %s", glGetString(GL_RENDERER)); KARIN_ENDL
	printf("Shading Language Version -> %s", glGetString(GL_SHADING_LANGUAGE_VERSION)); KARIN_ENDL
	printf("Extensions -> %s", glGetString(GL_EXTENSIONS)); KARIN_ENDL

	printf("------------------ GLU -----------------"); KARIN_ENDL
	printf("Version -> %s", gluGetString(GLU_VERSION)); KARIN_ENDL
	printf("Extensions -> %s", gluGetString(GLU_EXTENSIONS)); KARIN_ENDL
	printf("----------------------------------------"); KARIN_ENDL
}

GLboolean karinQueryExtension(const char *extName)
{
	const char *ptr = (const char *)glGetString(GL_EXTENSIONS);
	if(!ptr)
		return GL_FALSE;
	const char *const end = ptr + strlen(ptr);
	size_t len = strlen(extName);

	while(ptr < end)
	{
		size_t l = strcspn(ptr, " ");
		if(l == len && strncmp(ptr, extName, l))
			return GL_TRUE;
		else
			ptr += (l + 1);
	}
	return GL_FALSE;
}

void karinFullscreen(Bool fs)
{
	XClientMessageEvent xclient;

	xclient.type = ClientMessage;
	xclient.window = win;	//GDK_WINDOW_XID (window);
	xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xclient.format = 32;
	xclient.data.l[0] = fs ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", fs);
	xclient.data.l[2] = 0;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	XSendEvent(dpy, DefaultRootWindow(dpy), False,
		   SubstructureRedirectMask | SubstructureNotifyMask,
		   (XEvent *) & xclient);
}

void karinSetLoopInterval(unsigned long i)
{
	loop_interval = i;
}


/*
static void GLimp_DisableComposition(void)
{
	XClientMessageEvent xclient;
	Atom atom;
	int one = 1;

	atom = XInternAtom(dpy, "_HILDON_NON_COMPOSITED_WINDOW", False);
	XChangeProperty(dpy, win, atom, XA_INTEGER, 32, PropModeReplace,
			(unsigned char *)&one, 1);

	xclient.type = ClientMessage;
	xclient.window = win;	//GDK_WINDOW_XID (window);
	xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xclient.format = 32;
	xclient.data.l[0] =
	    //r_fullscreen->integer ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	//gdk_x11_atom_to_xatom_for_display (display, state1);
	//gdk_x11_atom_to_xatom_for_display (display, state2);
	xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xclient.data.l[2] = 0;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *) & xclient);
}
*/

#endif
