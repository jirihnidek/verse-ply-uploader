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

#include <pthread.h>

#ifndef MAIN_H_
#define MAIN_H_

/* Frames per second used by this verse client */
#define FPS 60

/* Custom type of node containing object */
#define OBJECT_NODE_CT 125

/* Custom type of node containing mesh */
#define MESH_NODE_CT 126

/* Custom type of layer containing vertexes, edges and faces */
#define LAYER_VERTEXES_CT 0
/* Custom type of layer containing edges */
#define LAYER_EDGES_CT    1
/* Custom type of layer containing faces */
#define LAYER_QUADS_CT    2

/**
 * Client context
 */
typedef struct CTX {

	/**
	 * Thread for visualization
	 */
#ifdef WITH_GLUT
	pthread_t glut_thread;
#endif

	/**
	 * MY PLY filename
	 */
	char *my_filename;

	/**
	 * Flag of debug print
	 */
	int print_debug;

	/**
	 * My session ID
	 */
	uint8_t my_session_id;

	/**
	 * Username passed from command line
	 */
	char *my_username;

	/**
	 * Password passed from command line
	 */
	char *my_password;

	/**
	 * Hostname of Verse server
	 */
	char *my_verse_server;

	/**
	 * ID of user used for connection to verse server
	 */
	int64_t my_user_id;

	/**
	 * ID of avatar representing this client on verse server
	 */
	int64_t my_avatar_id;

	/**
	 * ID of object node
	 */
	int64_t my_object_node_id;

	/**
	 * ID of mesh node
	 */
	int64_t my_mesh_node_id;

	/**
	 * ID of layer containing vertices
	 */
	int64_t my_vertex_layer_id;

	/**
	 * Number of vertices
	 */
	uint64_t nvertices;

	/**
	 * Array of vertices
	 */
	double *vertices;

	/**
	 * ID of layer containing faces
	 */
	int64_t my_face_layer_id;

	/**
	 * Number of faces
	 */
	uint64_t nquads;

	/**
	 * Array of faces
	 */
	uint64_t *quads;

	/**
	 *
	 */
	int window_width;

	/**
	 *
	 */
	int window_height;

	int argc;

	char **argv;
} CTX;

void init_CTX(struct CTX *ctx);

#endif /* MAIN_H_ */
