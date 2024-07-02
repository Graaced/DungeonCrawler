// Include declaration ----------------------------------------------------
#include "dungeon.h"
// ------------------------------------------------------------------------

// Struct declaration -----------------------------------------------------

// ------------------------------------------------------------------------

int set_nb(int s)
{
#ifdef _WIN32
    unsigned long nb_mode = 1;
    return ioctlsocket(s, FIONBIO, &nb_mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    return fcntl(s, F_SETFL, flags);
#endif
}

int init_socket(struct sockaddr_in *sin)
{
#ifdef _WIN32
    // This part is only required on windows, init the winsock2 dll
    WSADATA wsa_data;

    // Init winsock library on windows
    if (WSAStartup(0x0202, &wsa_data))
    {
        printf("unable to initialize winsock2 \n");
        return -1;
    }
#endif

    // Create a UDP socket
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Check if the socket creation is successful
    if (s < 0)
    {
        printf("Unable to initialize the UDP socket \n");
        return -1;
    }

    set_nb(s);

    printf("---------------------------\n");
    printf("Socket %d created \n", s);

    // Convert the IP address string to a binary representation
    inet_pton(AF_INET, "188.152.81.212", &sin->sin_addr); // this will create a big endian 32 bit address

    // Set the address family of the sin structure to AF_INET, which indicates that the address is an IPv4 address.
    sin->sin_family = AF_INET;

    // Set the port number to 9999 and convert it to big-endian format
    sin->sin_port = htons(9999);

    return s;
}

// Entry point ------------------------------------------------------------
int send_packet(const int s, const unsigned int p_id, const float x, const float y, const unsigned int order, struct sockaddr_in *sin, const unsigned int message_type)
{
    // Init packet struct
    maze_packet packet;

    packet.x = x;
    packet.y = y;
    packet.player_id = p_id;
    packet.order = order;
    packet.command_type = message_type;

    // Send the packet to the server
    int sent_bytes = sendto(s, (const char *)&packet, sizeof(packet), 0, (struct sockaddr *)sin, sizeof(struct sockaddr_in));

    // Print the number of bytes sent
    // printf("---------------------------\n");
    // printf("Sent %d bytes via UDP \n", sent_bytes);

    // Returned packet
    return sent_bytes;
}

int receive_packet(const int s, char *buffer, const int len)
{
    struct sockaddr_in sin;
    int sin_len = sizeof(struct sockaddr_in);

    return recvfrom(s, buffer, len, 0, (struct sockaddr *)&sin, &sin_len);
}