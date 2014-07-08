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
 * Custom type of new node
 */
static int32_t my_node_ct = -1;

/**
 * File path to PLY file
 */
static char *my_filename = NULL;

/**
 * Hostname of Verse server
 */
static char *my_verse_server = NULL;

/**
 * @brief Callback function for user authentication
 *
 * @param session_id
 * @param username
 * @param auth_methods_count
 * @param methods
 */
void cb_receive_user_authenticate(const uint8_t session_id,
		const char *username,
		const uint8_t auth_methods_count,
		const uint8_t *methods)
{
	static int attempts = 0;	/* Store number of authentication attempt for this session. */
	char name[VRS_MAX_USERNAME_LENGTH + 1];
	char *password;
	int i, is_passwd_supported = 0;

	/* Debug print */
	printf("%s() username: %s, auth_methods_count: %d, methods: ",
			__FUNCTION__, username, auth_methods_count);
	for(i = 0; i < auth_methods_count; i++) {
		printf("%d, ", methods[i]);
		if(methods[i] == VRS_UA_METHOD_PASSWORD)
			is_passwd_supported = 1;
	}
	printf("\n");

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
void cb_receive_connect_accept(const uint8_t session_id,
		const uint16_t user_id,
		const uint32_t avatar_id)
{
	printf("%s() session_id: %d, user_id: %d, avatar_id: %d\n",
			__FUNCTION__, session_id, user_id, avatar_id);

	my_avatar_id = avatar_id;
	my_user_id = user_id;

	/* When client receive connect accept, then it is ready to subscribe
	 * to the root node of the node tree. Id of root node is still 0. This
	 * function is called with level 1. It means, that this client will be
	 * subscribed to the root node and its child nodes (1, 2, 3) */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, 0, 0, 0);

	/* Check if server allow double subscribe? */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, 1, 0, 0);

	/* Try to create new node */
	vrs_send_node_create(session_id, VRS_DEFAULT_PRIORITY, my_node_ct);
}

/**
 * @brief Callback function for connect terminate
 *
 * @param session_id
 * @param error_num
 */
void cb_receive_connect_terminate(const uint8_t session_id,
		const uint8_t error_num)
{
	printf("%s() session_id: %d, error_num: %d\n",
			__FUNCTION__, session_id, error_num);
	switch(error_num) {
	case VRS_CONN_TERM_AUTH_FAILED:
		printf("User authentication failed\n");
		break;
	case VRS_CONN_TERM_HOST_DOWN:
		printf("Host is not accesible\n");
		break;
	case VRS_CONN_TERM_HOST_UNKNOWN:
		printf("Host could not be found\n");
		break;
	case VRS_CONN_TERM_SERVER_DOWN:
		printf("Server is not running\n");
		break;
	case VRS_CONN_TERM_TIMEOUT:
		printf("Connection timout\n");
		break;
	case VRS_CONN_TERM_ERROR:
		printf("Conection with server was broken\n");
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
	exit(EXIT_SUCCESS);
}

/**
 * @brief Print help
 */
static void print_help(char *prog_name)
{
	printf("\n Usage: %s -t filename server_address\n", prog_name);
	printf("\n");
	printf(" This program is Verse client uploading PLY model to \n");
	printf(" to Verse server.\n");
	printf("\n");
	printf(" Options:\n");
	printf(" -f filename    Filename of PLY file.\n");
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

	if(argc > 0) {
		/* Parse all options */
		while( (opt = getopt(argc, argv, "f:h")) != -1) {
			switch(opt) {
			case 'f':
				my_filename = strdup(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				exit(EXIT_SUCCESS);
			case ':':
				exit(EXIT_FAILURE);
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

	/* Register basic callback functions */
	vrs_register_receive_user_authenticate(cb_receive_user_authenticate);
	vrs_register_receive_connect_accept(cb_receive_connect_accept);
	vrs_register_receive_connect_terminate(cb_receive_connect_terminate);

	/* Send connect request to the server */
	error_num = vrs_send_connect_request(my_verse_server, "12345", 0, &my_session_id);
	if(error_num != VRS_SUCCESS) {
		printf("ERROR: %s\n", vrs_strerror(error_num));
		return EXIT_FAILURE;
	}

	/* Never ending loop */
	while(1) {
		vrs_callback_update(my_session_id);
		sleep(1);
	}

	return EXIT_SUCCESS;
}
