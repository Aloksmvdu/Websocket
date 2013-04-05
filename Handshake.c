/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "Handshake.h"
#include "Errors.h"
#include "md5.h"
#include "sha1.h"
#include "base64.h"

/**
 * Returns whether x is a integral multiple of y.
 */
uint32_t isIntergralMultiple(long x, int y) {
	double d = x/y;

	if (d == (int) d) {
		return (uint32_t) x/y;
	}

	return 0;
}

uint32_t getNumbersInStringDividedByNumberOfSpaces(char *key, int length) {	
	char result[length];
	char chr[2];
	int i, spaces = 0;

	memset(result, '\0', length);

	for(i = 0; i < length; i++) {
		if ( isdigit((unsigned char) key[i]) != 0 ) {	
			memset(chr, '\0', 2);
			sprintf(chr, "%c", key[i]);
			strcat(result, chr);
		} else if (isblank((unsigned char) key[i]) != 0 && key[i] != '\t') {
			spaces++;	
		}
	}

	if (spaces < 1) {
		return 0;
	}

	return isIntergralMultiple(strtol(result, (char **) NULL, 10), spaces);
}

void concate(uint32_t key1, uint32_t key2, char *key3, char *result){
	memset(result, '\0', KEYSIZE);
	memcpy(result, (unsigned char *) &key1, sizeof(uint32_t));
	memcpy(result+4, (unsigned char *) &key2, sizeof(uint32_t));
	memcpy(result+8, key3, 8);
}

char *getMemory(char *token, int length) {
	char *temp = (char *) malloc(length);

	if (temp == NULL) {	
		return NULL;
	}

	memset(temp, '\0', length);	
	memcpy(temp, token, length);
	return temp;
}

int parseHeaders(char *string, struct node *n, int port){
	struct header *h = n->headers;
	char* token = strtok(string, "\r\n");

	(void) port;

	/**
	 * Make the choose of port and server dynamical
	 */	
	char* host[HOSTS];
	host[0] = "localhost:4567";
	host[1] = "127.0.0.1:4567";
	host[2] = "192.168.87.103:4567";
	host[3] = "192.168.1.100:4567";
	host[4] = "192.168.0.21:4567";

	/**
	 * Make the choose of server dynamical
	 */
	char* origin[HOSTS];
	origin[0] = "http://localhost";
	origin[1] = "http://127.0.0.1";
	origin[2] = "http://192.168.87.103";
	origin[3] = "http://192.168.1.100";
	origin[4] = "http://192.168.0.21";

	/**
	 * Split the header string into pieces that we place in the header struct
	 * in the node parameter.
	 */
	if (token != NULL) {
		h->get = token;
		h->get_len = strlen(h->get);

		while ( token != NULL ) {

			printf("%s\n", token);
			fflush(stdout);

			if ( strncasecmp("Sec-WebSocket-Version: ", token, 23) == 0 ) {
				h->version = token + 23;
				
				if ( strncmp("7", h->version, 1) == 0 ) {
					h->type = "hybi-07";
				} else if( strncmp("8", h->version, 1) == 0 ) {
					h->type = "hybi-10";
				} else if( strncmp("13", h->version, 2) == 0 ) {
					h->type = "RFC6455";
				} else {
					client_error("Unknown websocket version.", ERROR_VERSION, 
							n);
					return -1;
				}
			} else if ( strncasecmp("Upgrade: ", token, 9) == 0) {
				h->upgrade = token + 9;
				h->upgrade_len = strlen(h->upgrade);
			} else if ( strncasecmp("Origin: ", token, 8) == 0 ) {
				h->origin = token + 8;
				h->origin_len = strlen(h->origin);
			} else if ( strncasecmp("Connection: ", token, 12) == 0 ) {
				h->connection = token + 12;
			} else if ( strncasecmp("Sec-WebSocket-Protocol: ", token, 24) 
					== 0 ) {
				h->protocol = token + 24;
				h->protocol_len = strlen(h->protocol);
			} else if ( strncasecmp("Sec-WebSocket-Origin: ", token, 22) 
					== 0) {
				h->origin = token + 22;
				h->origin_len = strlen(h->origin);
			} else if ( strncasecmp("Sec-WebSocket-Key: ", token, 19) == 0 ) {
				h->key = token + 19;
			} else if ( strncasecmp("Sec-WebSocket-Extensions: ", token, 26) 
					== 0 ) {
				h->extension = token + 19;
				h->extension_len = strlen(h->extension);
			} else if ( strncasecmp("Host: ", token, 6) == 0 ) {
				h->host = token + 6;
				h->host_len = strlen(h->host);
			} else if ( strncasecmp("WebSocket-Protocol: ", token, 20) == 0 ) {
				h->type = "hixie-75";
				h->protocol = token + 20;
				h->protocol_len = strlen(h->protocol);
			} else if ( strncasecmp("Sec-WebSocket-Key1: ", token, 20) == 0 ) {
				h->type = "hybi-00";
				h->key1 = token + 20;
			} else if ( strncasecmp("Sec-WebSocket-Key2: ", token, 20) == 0 ) {
				h->type = "hybi-00";
				h->key2 = token + 20;
			}
			
			char *temp = strtok(NULL, "\r\n");

			if ( temp == NULL && h->type != NULL 
					&& strncasecmp(h->type, "hybi-00", 7) == 0 ) {
				h->key3 = token;
			}

			token = temp;
		}
		
	} else {
		client_error("The parsing of the headers went wrong.", ERROR_BAD, n);
		return -1;
	}

	/**
	 * If all these conditions is true, then we assume that the websocket
	 * protocol that is trying to be reached is hixie-75.
	 */
	if (h->key1 == NULL && h->key2 == NULL && h->key3 == NULL && h->key == NULL
			&& h->upgrade != NULL && h->connection != NULL && h->host != NULL
			&& h->origin != NULL && h->get != NULL) {
		h->type = "hixie-75";
	}

	/**
	 * Generate the accept key by the type of websocketprotocol the client is 
	 * using. We found calculated this protocol above.
	 */
	if ( h->type == NULL) {

		/**
		 * Remove these 2 lines when we are ready to test HTTP Requests
		 */
		client_error("Received a HTTP Request. We are not yet ready to handle" 
				"these.\n", ERROR_NOT_IMPL, n);
		return -1;

		if (h->get == NULL || (h->host == NULL 
				&& strncasecmp(" HTTP/1.1", h->get+(h->get_len-9), 9) == 0)) {
			client_error("HTTP Request: didn't receive all the headers required"
					"to validate and evaluate the http request", ERROR_BAD, n);
			return -1;
		}	

		if ( (strncasecmp(" HTTP/1.1", h->get+(h->get_len-9), 9) != 0 
				&& strncasecmp(" HTTP/1.0", h->get+(h->get_len-9), 9) != 0 
				&& strncasecmp(" HTTP/0.9", h->get+(h->get_len-9), 9) != 0)
				|| strncasecmp("GET /", h->get, 5) != 0) {
			client_error("The headerline of the request was invalid.", 
					ERROR_BAD, n);
			return -1;
		}

		int i;
		bool ok = false;

		for (i = 0; i < HOSTS; i++) {
			if (strncasecmp(host[i], h->host, strlen(host[i])) == 0) {
				ok = true;
			}
		}

		if (!ok) {
			client_error("The requested host is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}

	} else if ( strncasecmp(h->type, "hybi-00", 7) == 0 ) {
		/**
		 * Checking that all headers required has been catched
		 */
		if (h->key1 == NULL || h->key2 == NULL || h->key3 == NULL 
				|| h->host == NULL || h->origin == NULL || h->upgrade == NULL
				|| h->connection == NULL) {
			client_error("hybi-00: didn't receive all the headers required"
					"to evaluate if the handshake was valid", ERROR_BAD, n);
			return -1;
		}

		/**
		 * Acquiring the accept key.
		 */
		uint32_t key1 = ntohl(getNumbersInStringDividedByNumberOfSpaces(
					h->key1, strlen(h->key1)));
		uint32_t key2 = ntohl(getNumbersInStringDividedByNumberOfSpaces(
					h->key2, strlen(h->key2)));

		if (key1 > 0 && key2 > 0) {
			char unhashedKey[KEYSIZE];
			concate(key1, key2, h->key3, unhashedKey);	

			md5_state_t state;
			md5_byte_t hashedKey[KEYSIZE];

			md5_init(&state);
			md5_append(&state, (const md5_byte_t *) unhashedKey, KEYSIZE);
			md5_finish(&state, hashedKey);

			h->accept = getMemory((char *) hashedKey, KEYSIZE);
			h->accept_len = KEYSIZE; 

			if (h->accept == NULL) {
				client_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
				return -1;
			}
		} else {
			client_error("The keys sent in the header is invalid.", ERROR_BAD, 
					n);
			return -1;
		}
	} else if ( strncasecmp(h->type, "hixie-75", 8) == 0 ) {
		int i = 0;
		bool ok = false;
		
		/**
		 * Checking that all headers required has been catched
		 */
		if (h->get == NULL || h->upgrade == NULL || h->connection == NULL
				|| h->host == NULL || h->origin == NULL) {
			client_error("hixie-75: didn't receive all the headers required"
					"to evaluate if the handshake was valid.", ERROR_BAD, n);
			return -1;		
		}

		if (strncasecmp(" HTTP/1.1", h->get+(h->get_len-9), 9) != 0 
				|| strncasecmp("GET /", h->get, 5) != 0) {
			client_error("The headerline of the request was invalid.", 
					ERROR_BAD, n);
			return -1;
		}
		
		if (strncmp("WebSocket", h->upgrade, 9) != 0) {
			client_error("Unknown upgrade sequence.", ERROR_BAD, n);
			return -1;
		}

		if (strstr(h->connection, "Upgrade") == NULL) {
			client_error("Unknown connection sequence.", ERROR_BAD, n);
			return -1;
		}

		for (i = 0; i < HOSTS; i++) {
			if (strncasecmp(host[i], h->host, strlen(host[i])) == 0) {
				ok = true;
			}
		}

		if (!ok) {
			client_error("The requested host is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}

		ok = false;
	
		for (i = 0; i < HOSTS; i++) {
			if (strncasecmp(origin[i], h->origin, strlen(origin[i])) == 0) {
				ok = true;
			}
		}

		if (!ok) {
			client_error("The origin requested from is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}

	} else if ( strncasecmp(h->type, "hybi-07", 7) == 0 
			|| strncasecmp(h->type, "RFC6455", 7) == 0 
			|| strncasecmp(h->type, "hybi-10", 7) == 0 ) {
		/**
		 * Acquiring the accept key.
		 */
		SHA1Context sha;
		char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		int i, magic_len = 36, length = magic_len + strlen(h->key);
		uint32_t number;
		char key[length], sha1Key[20];
		char *acceptKey = NULL;
		bool ok = false;
		
		memset(key, '\0', length);
		memset(sha1Key, '\0', 20);

		memcpy(key, h->key, (length-magic_len));
		memcpy(key+(length-magic_len), magic, magic_len);

		SHA1Reset(&sha);
		SHA1Input(&sha, (const unsigned char*) key, length);

		if ( !SHA1Result(&sha) ) {
			client_error("The sha1 hash of the key went wrong!", 
					ERROR_INTERNAL, n);
			return -1;
		}

		for(i = 0; i < 5; i++) {
			number = ntohl(sha.Message_Digest[i]);
			memcpy(sha1Key+(4*i), (unsigned char *) &number, 4);
		}

		if (base64_encode_alloc((const char *) sha1Key, 20, &acceptKey) == 0) {
			client_error("The input length was greater than the output length",
					ERROR_BAD, n);
			return -1;
		}

		if (acceptKey == NULL) {
			client_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}

		h->accept = acceptKey;
		h->accept_len = strlen(h->accept);

		/**
		 * Check the header fields are valid
		 */
		if (strncasecmp(" HTTP/1.1", h->get+(h->get_len-9), 9) != 0 
				|| strncasecmp("GET /", h->get, 5) != 0) {
			client_error("The headerline of the request was invalid", 
					ERROR_BAD, n);
			return -1;
		}
		
		if (strncasecmp("websocket", h->upgrade, 9) != 0) {
			client_error("Unknown upgrade sequence.", ERROR_BAD, n);
			return -1;
		}

		if (strstr(h->connection, "Upgrade") == NULL) {
			client_error("Unknown connection sequence.", ERROR_BAD, n);
			return -1;
		}

		for (i = 0; i < HOSTS; i++) {
			if (strncasecmp(host[i], h->host, strlen(host[i])) == 0) {
				ok = true;
			}
		}

		if (!ok) {
			client_error("The requested host is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}

		ok = false;
	
		for (i = 0; i < HOSTS; i++) {
			if (strncasecmp(origin[i], h->origin, strlen(origin[i])) == 0) {
				ok = true;
			}
		}

		if (!ok) {
			client_error("The origin requested from is not accepted by us.", 
					ERROR_FORBIDDEN, n);
			return -1;
		}

	} else {
		client_error("Something very wierd happened??", ERROR_INTERNAL, n);
		return -1; 
	}

	return 0;
}

int sendHandshake(struct node *n) {
	int memlen = 0, length = 0;
	char* response = NULL;

	if ( strncasecmp(n->headers->type, "hybi-07", 7) == 0 
			|| strncasecmp(n->headers->type, "RFC6455", 7) == 0 
			|| strncasecmp(n->headers->type, "hybi-10", 7) == 0 ) {
		length = ACCEPT_HEADER_V3_LEN  
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN
			+ ACCEPT_KEY_LEN + n->headers->accept_len 
			+ (2*3);
		if (n->headers->protocol != NULL) {
			length += ACCEPT_PROTOCOL_V2_LEN + n->headers->protocol_len+2;
		}
		response = malloc(length);
		
		if (response == NULL) {
			client_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}

		memset(response, '\0', length);
	
		memcpy(response + memlen, ACCEPT_HEADER_V3, ACCEPT_HEADER_V3_LEN);
		memlen += ACCEPT_HEADER_V3_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;
		
		if (n->headers->protocol != NULL) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V2, ACCEPT_PROTOCOL_V2_LEN);
			memlen += ACCEPT_PROTOCOL_V2_LEN;

			memcpy(response + memlen, n->headers->protocol, n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, ACCEPT_KEY, ACCEPT_KEY_LEN);
		memlen += ACCEPT_KEY_LEN;
		
		memcpy(response + memlen, n->headers->accept, n->headers->accept_len);
		memlen += n->headers->accept_len;

		memcpy(response + memlen, "\r\n\r\n", 4);
		memlen += 4;

		if (memlen != length) {
			free(response);
			client_error("We've fuck the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	} else if ( strncasecmp(n->headers->type, "hybi-00", 7) == 0 ) {
		length = ACCEPT_HEADER_V2_LEN  
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN 
			+ ACCEPT_ORIGIN_V2_LEN + n->headers->origin_len
			+ ACCEPT_LOCATION_V2_LEN + 5 
			+ n->headers->host_len
			+ n->headers->accept_len 
			+ (2*4) + 1;
		
		if (n->headers->protocol != NULL) {
			length += ACCEPT_PROTOCOL_V2_LEN + n->headers->protocol_len 
				+ 2;
		}

		response = malloc(length);
		
		if (response == NULL) {
			client_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}

		memset(response, '\0', length);
	
		memcpy(response + memlen, ACCEPT_HEADER_V2, ACCEPT_HEADER_V2_LEN);
		memlen += ACCEPT_HEADER_V2_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;

		memcpy(response + memlen, ACCEPT_ORIGIN_V2, ACCEPT_ORIGIN_V2_LEN);
		memlen += ACCEPT_ORIGIN_V2_LEN;

		memcpy(response + memlen, n->headers->origin, n->headers->origin_len);
		memlen += n->headers->origin_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_LOCATION_V2, ACCEPT_LOCATION_V2_LEN);
		memlen += ACCEPT_LOCATION_V2_LEN;

		memcpy(response + memlen, "ws://", 5);
		memlen += 5;

		memcpy(response + memlen, n->headers->host, n->headers->host_len);
		memlen += n->headers->host_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		if (n->headers->protocol != NULL) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V2, ACCEPT_PROTOCOL_V2_LEN);
			memlen += ACCEPT_PROTOCOL_V2_LEN;

			memcpy(response + memlen, n->headers->protocol, n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, n->headers->accept, n->headers->accept_len);
		memlen += n->headers->accept_len;

		response[memlen] = '\0';
		memlen++;

		printf("%d = %d\n", memlen, length);
		printf("%s\n", response);
		fflush(stdout);

		if (memlen != length) {
			free(response);
			client_error("We've fuck the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	} else if ( strncasecmp(n->headers->type, "hixie-75", 8) == 0 ) {
		/**
		 * This protocol has not been tested.
		 */

		length = ACCEPT_HEADER_V1_LEN 
			+ ACCEPT_UPGRADE_LEN + n->headers->upgrade_len
			+ ACCEPT_CONNECTION_LEN 
			+ ACCEPT_ORIGIN_V1_LEN + n->headers->origin_len
			+ ACCEPT_LOCATION_V1_LEN + 5 
			+ n->headers->host_len
			+ (2*4) + 1;
		
		if (n->headers->protocol != NULL) {
			length += ACCEPT_PROTOCOL_V1_LEN + n->headers->protocol_len + 2;
		}

		response = malloc(length);
		
		if (response == NULL) {
			client_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
			return -1;
		}

		memset(response, '\0', length);
	
		memcpy(response + memlen, ACCEPT_HEADER_V1, ACCEPT_HEADER_V1_LEN);
		memlen += ACCEPT_HEADER_V1_LEN;

		memcpy(response + memlen, ACCEPT_UPGRADE, ACCEPT_UPGRADE_LEN);
		memlen += ACCEPT_UPGRADE_LEN;

		memcpy(response + memlen, n->headers->upgrade, n->headers->upgrade_len);
		memlen += n->headers->upgrade_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_CONNECTION, ACCEPT_CONNECTION_LEN);
		memlen += ACCEPT_CONNECTION_LEN;

		memcpy(response + memlen, ACCEPT_ORIGIN_V1, ACCEPT_ORIGIN_V1_LEN);
		memlen += ACCEPT_ORIGIN_V1_LEN;

		memcpy(response + memlen, n->headers->origin, n->headers->origin_len);
		memlen += n->headers->origin_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		memcpy(response + memlen, ACCEPT_LOCATION_V1, ACCEPT_LOCATION_V1_LEN);
		memlen += ACCEPT_LOCATION_V1_LEN;

		memcpy(response + memlen, "ws://", 5);
		memlen += 5;

		memcpy(response + memlen, n->headers->host, n->headers->host_len);
		memlen += n->headers->host_len;

		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		if (n->headers->protocol != NULL) {
			memcpy(response + memlen, ACCEPT_PROTOCOL_V1, ACCEPT_PROTOCOL_V1_LEN);
			memlen += ACCEPT_PROTOCOL_V1_LEN;

			memcpy(response + memlen, n->headers->protocol, n->headers->protocol_len);
			memlen += n->headers->protocol_len;

			memcpy(response + memlen, "\r\n", 2);
			memlen += 2;
		}
		
		memcpy(response + memlen, "\r\n", 2);
		memlen += 2;

		printf("%d = %d\n", memlen, length);
		printf("%s\n", response);
		fflush(stdout);

		if (memlen != length) {
			free(response);
			client_error("We've fuck the counting up!", ERROR_INTERNAL, n);
			return -1;
		}
	} else {
		client_error("The type of protocol used was unknown.", ERROR_BAD, n);
		return -1;
	}

	send(n->socket_id, response, length, 0);

	if (response != NULL) {
		free(response);
	}

	return 0;
}
