#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT1 2201
#define PORT2 2202
#define BUFFER_SIZE 1024
#define MAX_PLAYERS 2

typedef struct {
    int piece_type;      // Piece type (from 0 to 6)
    int piece_rotation;  // Rotation (0, 1, 2, or 3)
    int piece_column;    // Column position
    int piece_row;       // Row position
} Piece;

typedef struct {
    int socket;
    Piece pieces[5];
} Player;

// Helper function to send the same message to both clients
void send_to_both_players(Player *players, const char *message) {
    // Send message to Player 1
    send(players[0].socket, message, strlen(message), 0);
    // Send message to Player 2
    send(players[1].socket, message, strlen(message), 0);
}

int main() {
    int listen_fd1, listen_fd2, new_socket;
    struct sockaddr_in server_addr1, server_addr2, player1_addr, player2_addr;
    socklen_t addr_len;
    int opt = 1;
    char buffer[BUFFER_SIZE] = {0};
    Player players[MAX_PLAYERS];

    // Create two separate server sockets: one for Player 1, one for Player 2
    if ((listen_fd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[Server] socket() failed for Player 1.");
        exit(EXIT_FAILURE);
    }

    if ((listen_fd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[Server] socket() failed for Player 2.");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listen_fd2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    // Set up server addresses for both Player 1 and Player 2
    memset(&server_addr1, 0, sizeof(server_addr1));
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_addr.s_addr = INADDR_ANY;
    server_addr1.sin_port = htons(PORT1);  // Listen on PORT1 for Player 1

    memset(&server_addr2, 0, sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_addr.s_addr = INADDR_ANY;
    server_addr2.sin_port = htons(PORT2);  // Listen on PORT2 for Player 2

    // Bind the first server socket to PORT1
    if (bind(listen_fd1, (struct sockaddr *)&server_addr1, sizeof(server_addr1)) < 0) {
        perror("[Server] bind() failed for Player 1.");
        exit(EXIT_FAILURE);
    }

    // Bind the second server socket to PORT2
    if (bind(listen_fd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0) {
        perror("[Server] bind() failed for Player 2.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd1, 1) < 0) {
        perror("[Server] listen() failed for Player 1.");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd2, 1) < 0) {
        perror("[Server] listen() failed for Player 2.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Waiting for players to connect...\n");

    // Accept Player 1 connection (on PORT1)
    addr_len = sizeof(player1_addr);
    if ((players[0].socket = accept(listen_fd1, (struct sockaddr *)&player1_addr, &addr_len)) < 0) {
        perror("[Server] accept() failed for Player 1.");
        exit(EXIT_FAILURE);
    }
    printf("[Server] Player 1 connected on PORT1.\n");

    // Accept Player 2 connection (on PORT2)
    addr_len = sizeof(player2_addr);
    if ((players[1].socket = accept(listen_fd2, (struct sockaddr *)&player2_addr, &addr_len)) < 0) {
        perror("[Server] accept() failed for Player 2.");
        exit(EXIT_FAILURE);
    }
    printf("[Server] Player 2 connected on PORT2.\n");
    char *board;
    int current_player = 0;
    // Main game loop
    while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int nbytes = read(players[current_player].socket, buffer, BUFFER_SIZE);
            if (nbytes <= 0) {
                perror("[Server] read() failed.");
                exit(EXIT_FAILURE);
            }
        
            printf("[Server] Received from Player %d: %s\n",current_player, buffer);
            if (strncmp(buffer, "F", 1) == 0 && current_player == 0) {
                printf("[Server] Player %d forfeits\n",current_player);
                send(players[0].socket, "H 0\0", 3, 0);
                send(players[1].socket, "H 1\0", 3, 0);
                break;
            }
            else if (strncmp(buffer, "F", 1) == 0 && current_player == 1) {
                printf("[Server] Player %d forfeits\n",current_player);
                send(players[0].socket, "H 1\0", 3, 0);
                send(players[1].socket, "H 0\0", 3, 0);
                break;
            }
            else if(strncmp(buffer, "B", 1) == 0 && current_player == 0){
                int width = 0;
                int height = 0;
                sscanf(buffer, "B %d %d", &width, &height);
                board = malloc(sizeof(char)*width*height);
                send_to_both_players(players, "board initialized\0");
                //initialize pieces
            }
            else if(strncmp(buffer, "B", 1) == 0){
                send_to_both_players(players, "Player 2 joined\0");
            }
            else{
                send_to_both_players(players, "A\0");
            }
            // printf("[Server] Enter message: ");
            // memset(buffer, 0, BUFFER_SIZE);
            // fgets(buffer, BUFFER_SIZE, stdin);
            // buffer[strlen(buffer)-1] = '\0';
            // if (strcmp(buffer, "F") == 0) {
            //     printf("[Server] Quitting...\n");
            //     send(players[0].socket, buffer, strlen(buffer), 0);
            //     send(players[1].socket, buffer, strlen(buffer), 0);
            //     break;
            // }
            current_player = 1 - current_player;
    }

    // Close the connections
    close(players[0].socket);
    close(players[1].socket);
    // close sockets
    close(listen_fd1); 
    close(listen_fd2);  
    printf("[Server] Shutting down.\n");

    return 0;
}




