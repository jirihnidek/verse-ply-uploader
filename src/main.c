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
#include <verse.h>
#include <rply.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

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

static int print_debug = 0;

/* My session ID */
static uint8_t my_session_id = -1;

/**
 * Username passed from command line
 */
static char *my_username = NULL;

/**
 * Password passed from command line
 */
static char *my_password = NULL;

/**
 * ID of user used for connection to verse server
 */
static int64_t my_user_id = -1;

/**
 * ID of avatar representing this client on verse server
 */
static int64_t my_avatar_id = -1;

/**
 * ID of object node
 */
static int64_t my_object_node_id = -1;

/**
 * ID of mesh node
 */
static int64_t my_mesh_node_id = -1;

/**
 * ID of layer containing vertices
 */
static int64_t my_vertex_layer_id = -1;

/**
 * ID of layer containing faces
 */
static int64_t my_face_layer_id = -1;

/**
 * File path to PLY file
 */
static char *my_filename = NULL;

/**
 * Hostname of Verse server
 */
static char *my_verse_server = NULL;

/**
 * Current vertex position
 */
static double vertex[3] = {0.0, 0.0, 0.0};

/**
 * Indexes of current face (quat)
 */
static long *face;

static long face_size = 0;

/**
* \brief Callback function for handling signals.
* \details Only SIGINT (Ctrl-C) is handled. When first SIGINT is received,
* then client tries to terminate connection and it set handling to SIGINT
* to default behavior (terminate application).
* \param sig identifier of signal
*/
static void handle_signal(int sig)
{
	if(sig == SIGINT) {
		printf("%s() try to terminate connection: %d\n",
				__FUNCTION__, my_session_id);
		vrs_send_connect_terminate(my_session_id);
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	}
}

/**
 *
 * @param argument
 * @return
 */
static int vertex_cb(p_ply_argument argument)
{
	long xyz, *vert_num;
	ply_get_argument_user_data(argument, (void**)&vert_num, &xyz);
	switch(xyz) {
	case 0:
		/* printf("%ld ", *vert_num); */
		vertex[0] = ply_get_argument_value(argument);
		break;
	case 1:
		vertex[1] = ply_get_argument_value(argument);
		break;
	case 2:
		vertex[2] = ply_get_argument_value(argument);
		/* printf("(%g, %g, %g)\n", vertex[0], vertex[1], vertex[2]); */
		vrs_send_layer_set_value(my_session_id, VRS_DEFAULT_PRIORITY, my_mesh_node_id, my_vertex_layer_id, *vert_num, VRS_VALUE_TYPE_REAL64, 3, (void*)vertex);
		*vert_num = *vert_num + 1;
		break;
	}
	return 1;
}

/**
 *
 * @param argument
 * @return
 */
static int face_cb(p_ply_argument argument)
{
	long length, value_index, *face_num;

	ply_get_argument_user_data(argument, (void**)&face_num, NULL);
	ply_get_argument_property(argument, NULL, &length, &value_index);

	/* When first index is loaded */
	if(value_index == 0) {
		int size;
		face_size = length;
		size = (face_size < 4) ? 4 : face_size;
		face = (long*)calloc(size, sizeof(long));
		/* printf("%ld, %ld, ", *face_num, length); */
	}

	if(face != NULL) {
		face[value_index] = (long)ply_get_argument_value(argument);
	}

	/* When last face index is loaded */
	if(value_index >= 0 && value_index == (face_size - 1)) {
		/*
		long i;
		printf("{");
		for(i = 0; i < face_size; i++) {
			if(i != (face_size - 1)) {
				printf("%ld, ", face[i]);
			} else {
				printf("%ld", face[i]);
			}
		}
		printf("}\n");
		*/

		vrs_send_layer_set_value(my_session_id, VRS_DEFAULT_PRIORITY, my_mesh_node_id, my_face_layer_id, *face_num, VRS_VALUE_TYPE_UINT64, 4, (void*)face);

		if(face != NULL) {
			free(face);
			face = NULL;
		}
		*face_num = *face_num + 1;
	}

	return 1;
}

/**
 * @brief The callback function or command layer set_value
 *
 * @param session_id
 * @param node_id
 * @param layer_id
 * @param item_id
 * @param data_type
 * @param count
 * @param value
 */
static void cb_receive_layer_set_value(const uint8_t session_id,
	     const uint32_t node_id,
	     const uint16_t layer_id,
	     const uint32_t item_id,
	     const uint8_t data_type,
	     const uint8_t count,
	     const void *value)
{
	int i;

	if(print_debug) {
		printf("%s(): session_id: %u, node_id: %u, layer_id: %d, item_id: %d, data_type: %d, count: %d, value(s): ",
				__FUNCTION__, session_id, node_id, layer_id, item_id, data_type, count);

		switch(data_type) {
		case VRS_VALUE_TYPE_UINT8:
			for(i=0; i<count; i++) {
				printf("%d, ", ((uint8_t*)value)[i]);
			}
			break;
		case VRS_VALUE_TYPE_UINT16:
			for(i=0; i<count; i++) {
				printf("%d, ", ((uint16_t*)value)[i]);
			}
			break;
		case VRS_VALUE_TYPE_UINT32:
			for(i=0; i<count; i++) {
				printf("%d, ", ((uint32_t*)value)[i]);
			}
			break;
		case VRS_VALUE_TYPE_UINT64:
			for(i=0; i<count; i++) {
#ifdef __APPLE__
				printf("%llu, ", ((uint64_t*)value)[i]);
#else
				printf("%lu, ", ((uint64_t*)value)[i]);
#endif
			}
			break;
		case VRS_VALUE_TYPE_REAL16:
			for(i=0; i<count; i++) {
				/* TODO: convert half-float to float and print it as float value */
				printf("%x, ", ((uint16_t*)value)[i]);
			}
			break;
		case VRS_VALUE_TYPE_REAL32:
			for(i=0; i<count; i++) {
				printf("%6.3f, ", ((float*)value)[i]);
			}
			break;
		case VRS_VALUE_TYPE_REAL64:
			for(i=0; i<count; i++) {
				printf("%6.3f, ", ((double*)value)[i]);
			}
			break;
		default:
			printf("Unknown type");
			break;
		}
		printf("\n");
	}
}

/**
 * @brief The callback function or command layer create
 *
 * @param session_id
 * @param node_id
 * @param parent_layer_id
 * @param layer_id
 * @param data_type
 * @param count
 * @param custom_type
 */
static void cb_receive_layer_create(const uint8_t session_id,
		const uint32_t node_id,
		const uint16_t parent_layer_id,
		const uint16_t layer_id,
		const uint8_t data_type,
		const uint8_t count,
		const uint16_t custom_type)
{
	if(print_debug) {
		printf("%s(): session_id: %u, node_id: %u, parent_layer_id: %d, layer_id: %d, data_type: %d, count: %d, custom_type: %d\n",
			__FUNCTION__, session_id, node_id, parent_layer_id, layer_id, data_type, count, custom_type);
	}

	if(node_id == my_mesh_node_id && custom_type == LAYER_VERTEXES_CT) {
		vrs_send_layer_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, layer_id, 0, 0);
		my_vertex_layer_id = layer_id;
	}

	if(node_id == my_mesh_node_id && custom_type == LAYER_QUADS_CT) {
		vrs_send_layer_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, layer_id, 0, 0);
		my_face_layer_id = layer_id;
	}

	if(my_vertex_layer_id != -1 && my_face_layer_id != -1) {
		long nvertices, ntriangles, vert_num = 0, face_num = 0;
		p_ply ply;

		ply = ply_open(my_filename, NULL, 0, NULL);
		if (!ply) return;
		if (!ply_read_header(ply)) return;
		nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, &vert_num, 0);
		ply_set_read_cb(ply, "vertex", "y", vertex_cb, &vert_num, 1);
		ply_set_read_cb(ply, "vertex", "z", vertex_cb, &vert_num, 2);
		ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, &face_num, 0);
		if (!ply_read(ply)) return;
		ply_close(ply);
	}
}

/**
 *
 * @param session_id
 * @param node_id
 * @param parent_id
 * @param user_id
 * @param custom_type
 */
static void cb_receive_node_create(const uint8_t session_id,
		const uint32_t node_id,
		const uint32_t parent_id,
		const uint16_t user_id,
		const uint16_t custom_type)
{
	if(print_debug) {
		printf("%s() session_id: %d, node_id: %d, parent_id: %d, user_id: %d, custom_type: %d\n",
			__FUNCTION__, session_id, node_id, parent_id, user_id, custom_type);
	}

	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0, 0);

	if(parent_id == my_avatar_id && custom_type == OBJECT_NODE_CT) {
		my_object_node_id = node_id;
		vrs_send_node_link(session_id, VRS_DEFAULT_PRIORITY, VRS_SCENE_PARENT_NODE_ID, node_id);
		if(my_mesh_node_id != -1) {
			vrs_send_node_link(session_id, VRS_DEFAULT_PRIORITY, my_object_node_id, node_id);
		}
	}

	if(parent_id == my_avatar_id && custom_type == MESH_NODE_CT) {
		my_mesh_node_id = node_id;
		if(my_object_node_id != -1) {
			vrs_send_node_link(session_id, VRS_DEFAULT_PRIORITY, my_object_node_id, node_id);
		}
		vrs_send_layer_create(session_id, VRS_DEFAULT_PRIORITY, node_id, -1, VRS_VALUE_TYPE_REAL64, 3, LAYER_VERTEXES_CT);
		vrs_send_layer_create(session_id, VRS_DEFAULT_PRIORITY, node_id, -1, VRS_VALUE_TYPE_UINT64, 2, LAYER_EDGES_CT);
		vrs_send_layer_create(session_id, VRS_DEFAULT_PRIORITY, node_id, -1, VRS_VALUE_TYPE_UINT64, 4, LAYER_QUADS_CT);
	}
}

/**
 * @brief Callback function for user authentication
 *
 * @param session_id
 * @param username
 * @param auth_methods_count
 * @param methods
 */
static void cb_receive_user_authenticate(const uint8_t session_id,
		const char *username,
		const uint8_t auth_methods_count,
		const uint8_t *methods)
{
	static int attempts = 0;	/* Store number of authentication attempt for this session. */
	char name[VRS_MAX_USERNAME_LENGTH + 1];
	char *password;
	int i, is_passwd_supported = 0;

	/* Debug print */
	if(print_debug) {
		printf("%s() username: %s, auth_methods_count: %d, methods: ",
				__FUNCTION__, username, auth_methods_count);
		for(i = 0; i < auth_methods_count; i++) {
			printf("%d, ", methods[i]);
		}
		printf("\n");
	}

	for(i = 0; i < auth_methods_count; i++) {
		if(methods[i] == VRS_UA_METHOD_PASSWORD) {
			is_passwd_supported = 1;
		}
	}

	/* Get username, when it is requested */
	if(username == NULL) {
		int ret = 0;
		attempts = 0;	/* Reset counter of auth. attempt. */
		if(my_username != NULL) {
			vrs_send_user_authenticate(session_id, my_username, 0, NULL);
		} else {
			printf("Username: ");
			ret = scanf("%s", name);
			if(ret == 1) {
				vrs_send_user_authenticate(session_id, name, 0, NULL);
			} else {
				printf("ERROR: Reading username.\n");
				exit(EXIT_FAILURE);
			}
		}
	} else {
		if(is_passwd_supported == 1) {
			attempts++;
			strncpy(name, username, VRS_MAX_USERNAME_LENGTH);
			if(my_password != NULL && attempts == 1) {
				vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_PASSWORD, my_password);
			} else {
				/* Print this warning, when previous authentication attempt failed. */
				if(attempts > 1)
					printf("Permission denied, please try again.\n");
				/* Get password from user */
				password = getpass("Password: ");
				vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_PASSWORD, password);
			}
		} else {
			printf("ERROR: Verse server does not support password authentication method\n");
		}
	}
}

/**
 * @brief Callback function for connect accept
 *
 * @param session_id
 * @param user_id
 * @param avatar_id
 */
static void cb_receive_connect_accept(const uint8_t session_id,
		const uint16_t user_id,
		const uint32_t avatar_id)
{
	if(print_debug) {
		printf("%s() session_id: %d, user_id: %d, avatar_id: %d\n",
			__FUNCTION__, session_id, user_id, avatar_id);
	}

	my_avatar_id = avatar_id;
	my_user_id = user_id;

	/* When client receive connect accept, then it is ready to subscribe
	 * to the root node of the node tree. Id of root node is still 0. This
	 * function is called with level 1. It means, that this client will be
	 * subscribed to the root node and its child nodes (1, 2, 3) */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, 0, 0, 0);

	/* Check if server allow double subscribe? */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, 1, 0, 0);

	/* Try to create new nodes */
	vrs_send_node_create(session_id, VRS_DEFAULT_PRIORITY, OBJECT_NODE_CT);
	vrs_send_node_create(session_id, VRS_DEFAULT_PRIORITY, MESH_NODE_CT);
}

/**
 * @brief Callback function for connect terminate
 *
 * @param session_id
 * @param error_num
 */
static void cb_receive_connect_terminate(const uint8_t session_id,
		const uint8_t error_num)
{
	if(print_debug) {
		printf("%s() session_id: %d, error_num: %d\n",
			__FUNCTION__, session_id, error_num);
		switch(error_num) {
		case VRS_CONN_TERM_AUTH_FAILED:
			printf("User authentication failed\n");
			break;
		case VRS_CONN_TERM_HOST_DOWN:
			printf("Host is not accessible\n");
			break;
		case VRS_CONN_TERM_HOST_UNKNOWN:
			printf("Host could not be found\n");
			break;
		case VRS_CONN_TERM_SERVER_DOWN:
			printf("Server is not running\n");
			break;
		case VRS_CONN_TERM_TIMEOUT:
			printf("Connection timeout\n");
			break;
		case VRS_CONN_TERM_ERROR:
			printf("Connection with server was broken\n");
			break;
		case VRS_CONN_TERM_SERVER:
			printf("Connection was terminated by server\n");
			break;
		case VRS_CONN_TERM_CLIENT:
			printf("Connection was terminated by client\n");
			break;
		default:
			printf("Unknown error\n");
			break;
		}
	}
	exit(EXIT_SUCCESS);
}

/**
 * @brief Print help
 */
static void print_help(char *prog_name)
{
	printf("\n Usage: %s -f filename server_address\n", prog_name);
	printf("\n");
	printf(" This program is Verse client uploading PLY model to \n");
	printf(" to Verse server.\n");
	printf("\n");
	printf(" Options:\n");
	printf(" -f filename       Filename of PLY file.\n");
	printf(" -d                Print debug prints.\n");
	printf(" -u username       Username used for authentication.\n");
	printf(" -p password       Password used for authentication.\n");
	printf("\n");
}

/**
 * @brief Main function parsing command line arguments and running main loop.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
	int error_num, opt;
	unsigned short flags = VRS_SEC_DATA_NONE;

	if(argc > 0) {
		/* Parse all options */
		while( (opt = getopt(argc, argv, "f:hdu:p:")) != -1) {
			switch(opt) {
			case 'f':
				my_filename = strdup(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case ':':
				exit(EXIT_FAILURE);
			case 'd':
				print_debug = 1;
				break;
			case 'u':
				my_username = strdup(optarg);
				break;
			case 'p':
				my_password = strdup(optarg);
				break;
			case '?':
				exit(EXIT_FAILURE);
			}
		}
		/* The last argument has to be name of server  */
		if( (optind + 1) != argc) {
			printf("ERROR: Bad number of parameters: %d != 1\n", argc - optind);
			print_help(argv[0]);
			return EXIT_FAILURE;
		}
	} else {
		printf("ERROR: Minimal number of arguments: 3\n");
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Set up server name */
	my_verse_server = strdup(argv[optind]);

	/* Handle SIGINT signal. The handle_signal function will try to terminate
	 * connection. */
	signal(SIGINT, handle_signal);

	/* Register basic callback functions */
	vrs_register_receive_user_authenticate(cb_receive_user_authenticate);
	vrs_register_receive_connect_accept(cb_receive_connect_accept);
	vrs_register_receive_connect_terminate(cb_receive_connect_terminate);

	vrs_register_receive_node_create(cb_receive_node_create);
	vrs_register_receive_layer_create(cb_receive_layer_create);
	vrs_register_receive_layer_set_value(cb_receive_layer_set_value);

	/* Send connect request to the server */
	error_num = vrs_send_connect_request(my_verse_server, "12345", flags, &my_session_id);
	if(error_num != VRS_SUCCESS) {
		printf("ERROR: %s\n", vrs_strerror(error_num));
		return EXIT_FAILURE;
	}

	/* Never ending loop */
	while(1) {
		vrs_callback_update(my_session_id);
		usleep(1000000/FPS);
	}

	if(my_filename != NULL) free(my_filename);
	if(my_verse_server != NULL) free(my_verse_server);
	if(my_username != NULL) free(my_username);
	if(my_password != NULL) free(my_username);

	return EXIT_SUCCESS;
}
