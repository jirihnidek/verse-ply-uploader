/*
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 * Contributor(s): Jiri Hnidek <jiri.hnidek@tul.cz>.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "main.h"

static struct CTX *ctx = NULL;

/**
 * \brief Initialize OpenGL context
 */
static void gl_init(void)
{
	static float light_ambient[4] = {0.7, 0.7, 0.7, 1.0};
	static float light_diffuse[4] = {0.9, 0.9, 0.9, 1.0};
	static float light_specular[4] = {0.5, 0.5, 0.5, 1.0};
	static float material_ambient[4] = {0.2, 0.2, 0.2, 1.0};
	static float material_diffuse[4] = {0.4, 0.4, 0.4, 1.0};
	static float material_specular[4] = {0.5, 0.5, 0.5, 1.0};

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPointSize(2.0);
	glLineWidth(1.0);
	glEnable(GL_POINT_SMOOTH);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_specular);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0);	/* TODO: add to ctx */
}

/**
 * \brief This function displays 3d scene
 */
static void glut_on_display(void)
{
	uint64_t i, j;
	uint64_t *quad;

	glViewport(0, 0, ctx->window_width, ctx->window_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, ctx->window_width, 0, ctx->window_height);
	glScalef(1, -1, 1);
	glTranslatef(0, -ctx->window_height, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* BEGIN: Drawing of 2D stuff */

	/* display_text_info(); */

	/* END: Drawing of 2D stuff */

	/* Set projection matrix for 3D stuff here */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(45.0f,
			(double)ctx->window_width/(double)ctx->window_height,
			0.01f,
			1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	gluLookAt(0.5, -0.5, 0.5,
			0.0, 0.0, 0.0,
			0.0, 0.0, 1.0);
	glPushMatrix();

	/* BEGIN: Drawing of 3d staff */

	glBegin(GL_QUADS);
		glNormal3f(1.0, 1.0, 1.0);
		for(i = 0; i < ctx->nquads; i++) {
			printf("q: %ld\n", i);
			quad = &ctx->quads[4*i];
			for(j = 0; j < 4; j++) {
				if(j==3 && quad[3]==0) {
					glVertex3dv(&ctx->vertices[quad[2]]);
				} else {
					printf("v (%ld): %6.3f, %6.3f, %6.3f\n",
							quad[j],
							ctx->vertices[quad[j] + 0],
							ctx->vertices[quad[j] + 1],
							ctx->vertices[quad[j] + 2]);
					glVertex3dv(&ctx->vertices[quad[j]]);
				}
			}
		}
	glEnd();

	/* END: Drawing of 3d staff */

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glFlush();
	glutSwapBuffers();
}

/**
 * Callback function called on window resize
 */
static void glut_on_resize(int w, int h)
{
	ctx->window_width = w;
	ctx->window_height = h;
}

/**
 * \brief Display particles in regular periods
 */
static void glut_on_timer(int value) {
	glutPostRedisplay();
	glutTimerFunc(40, glut_on_timer, value);
}

/**
 * \brief This function set up glut callback functions and basic settings
 */
static void glut_init(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("PLY Loader");
	glutDisplayFunc(glut_on_display);
	glutReshapeFunc(glut_on_resize);
	glutTimerFunc(40, glut_on_timer, 0);
	gl_init();
}

/**
 * \brief This function start Glut never ending loop
 */
void *display_loop(void *arg)
{
	ctx = (struct CTX*) arg;

	glut_init(ctx->argc, ctx->argv);

	if(ctx != NULL) {
		glutMainLoop();
	}

	return NULL;
}
