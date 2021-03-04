#include "lwip/opt.h"

#if LWIP_SOCKET

#include "lwip/sockets.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "dnet.h"

#define BUFLEN 128

static void tcp_server_thread(void *arg)
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(sock != -1);

	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	struct sockaddr_in addr_me, addr_other;
	socklen_t slen = sizeof addr_other;

	memset(&addr_me, 0, sizeof addr_me);

	addr_me.sin_family = AF_INET;
	addr_me.sin_port = htons(12345);
	addr_me.sin_addr.s_addr = htonl(INADDR_ANY);

	int error = bind(sock, (struct sockaddr*)&addr_me, sizeof addr_me);
	assert(error != -1);

	error = listen(sock, 8);

	char buf[BUFLEN];

	while (1) {
		PRINTF("Server - Waiting for connection...\n\r");
		int client = accept(sock, (struct sockaddr*)&addr_other, &slen);

		setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout);
		setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof timeout);

		char* addr = inet_ntoa(addr_other);

		PRINTF("Server - Connection established with: %s\n\r", addr);

		while (1) {
			int bytes_received = recv(client, buf, BUFLEN, 0);
			if (bytes_received <= 0)
				break;

			buf[bytes_received] = 0;

			PRINTF("Server - Received: %s\n\r" , buf);

			int bytes_sent = send(client, buf, bytes_received, 0);
			if (bytes_sent <= 0)
				break;
		}
		PRINTF("Server - Closing connection\n\r");
		shutdown(client, SHUT_RDWR);
		close(client);
	}

	shutdown(sock, SHUT_RDWR);
	close(sock);
}

/*!
 * @brief Main function
 */
int main(void)
{
	BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    DnetConfig config = dnet_init("192.168.0.132", NULL, NULL, dnet_get_uid_location());

    dnet_start_new_thread("tcp_server_thread", tcp_server_thread, NULL);

    vTaskStartScheduler();

    /* Will not get here unless a task calls vTaskEndScheduler ()*/
    return 0;
}
#endif
