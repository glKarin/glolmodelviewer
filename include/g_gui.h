#ifndef _G_GUI_H
#define _G_GUI_H

#include <math.h>
#include "gutility.h"

static GLuint font_list = 0;

static void load_font()
{
	if(font_list == 0)
	{
		font_list = 1;
		//font_list = glGenLists(26);
		karinUseXFont("fixed", 1, '~', font_list);
		//glListBase('!');
	}
}

void draw_string(GLfloat x, GLfloat y, const GLfloat color[], const char *str)
{
	if(!str)
		return;
	load_font();

	PFNGLWINDOWPOS2FPROC glWindowPos2f = (PFNGLWINDOWPOS2FPROC)karinGetProcAddress((const GLubyte *)"glWindowPos2f");
	if(glWindowPos2f)
	{
		GLfloat current_color[4];
		glGetFloatv(GL_CURRENT_COLOR, current_color);
		glColor3f((GLfloat)color[0], (GLfloat)color[1], (GLfloat)color[2]);
		glWindowPos2f(x, y);
		for(; *str; str++)
		{
			glCallList(*str);
		}

		glColor4fv(current_color);
	}
}

void draw_digit(GLfloat x, GLfloat y, const GLfloat color[], GLint num)
{
	GLuint size = 1;
	GLuint base = 10;
	for(; num / base; base *= 10)
		size++;
	char *str = (char *)malloc(size + 1);
	memset(str, 0, size + 1);
	sprintf(str, "%d", num);
	str[size] = '\0';
	draw_string(x, y, color, str);
	//printf("|%s|\n", str);
	free(str);
}

void draw_button(GLfloat x, GLfloat y, GLfloat width, GLfloat height, const GLfloat color[], const GLfloat font_color[], const char *str)
{
	GLfloat current_color[4];
	glGetFloatv(GL_CURRENT_COLOR, current_color);
	glColor3f((GLfloat)color[0], (GLfloat)color[1], (GLfloat)color[2]);

	glBegin(GL_LINES);
	{
		glVertex2f(x, y);
		glVertex2f(x, y + height);

		glVertex2f(x, y + height);
		glVertex2f(x + width, y + height);

		glVertex2f(x + width, y + height);
		glVertex2f(x + width, y);

		glVertex2f(x + width, y);
		glVertex2f(x, y);
	}
	glEnd();

	draw_string(x + width / 2, y + height / 2, font_color, str);
	
	glColor4fv(current_color);
}

void draw_slider(GLfloat x, GLfloat y, GLfloat width, GLfloat height, GLuint value, GLuint min, GLuint max, const GLfloat line_color[], GLfloat block_width, GLfloat block_height, GLfloat block_color[], GLboolean draw_text)
{
	GLfloat current_color[4];
	glGetFloatv(GL_CURRENT_COLOR, current_color);
	glColor3f((GLfloat)line_color[0], (GLfloat)line_color[1], (GLfloat)line_color[2]);

	glBegin(GL_LINES);
	{
		glVertex2f(x, y);
		glVertex2f(x, y + height);

		glVertex2f(x, y + height);
		glVertex2f(x + width, y + height);

		glVertex2f(x + width, y + height);
		glVertex2f(x + width, y);

		glVertex2f(x + width, y);
		glVertex2f(x, y);
	}
	glEnd();

	if(draw_text)
	{
		draw_digit(x - 20, y, line_color, KARIN_MIN(max, min));
		draw_digit(x + width + 5, y, line_color, KARIN_MAX(max, min));
	}

	GLfloat per = (GLfloat)value / (GLfloat)abs(max - min);
	
	GLfloat block_x = x + width * per - block_width / 2;
	GLfloat block_y = y + height / 2 - block_height / 2;

	glColor3f((GLfloat)block_color[0], (GLfloat)block_color[1], (GLfloat)block_color[2]);
	glBegin(GL_LINES);
	{
		glVertex2f(block_x, block_y);
		glVertex2f(block_x, block_y + block_height);
		glVertex2f(block_x, block_y + block_height);
		glVertex2f(block_x + block_width, block_y + block_height);
		glVertex2f(block_x + block_width, block_y + block_height);
		glVertex2f(block_x + block_width, block_y);
		glVertex2f(block_x + block_width, block_y);
		glVertex2f(block_x, block_y);
	}
	glEnd();
	if(draw_text)
	{
		draw_digit(block_x, block_y + block_height + 5, block_color, value);
	}


	glColor4fv(current_color);
}

#endif
