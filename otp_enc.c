#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PASSWORD "encrypt123"
#define ACCEPTED "accepted"
#define MAX_BUFFER 196000

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int sendall(int fd, char *buf, int *length)
{
	int total = 0;
	int bytesleft = *length;
	int n;

	while (total < *length)
	{
		n = send(fd, buf+total, 512, 0);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
	}
	
	*length = total;

	return n==-1?-1:0;
}

/*Takes argv[1]: plaintext, argv[2]: key, argv[3]: port*/
int main(int argc, char *argv[])
{
	int socketFD;
	int portNumber;
	int charsWritten;
	int charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[MAX_BUFFER];
	
	/*Check usage and args*/
	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
		exit(0);
	}
	
	/*Concatenate the arguments into 1 big string to be sent. Use "|" as a delimiter*/		
	char bigString[MAX_BUFFER];
	memset(bigString, '\0', sizeof(bigString));
	
	char *arg1 = argv[1];
	FILE *fp = fopen(arg1, "r");
	
	if (fp == NULL)
	{
		error("Could not open plaintext file");
	}
	
	/*Read the contents of fp into plaintext*/	
	char plaintext[MAX_BUFFER];
	memset(plaintext, '\0', sizeof(plaintext));
	fgets(plaintext, sizeof(plaintext), fp);
	fclose(fp);
	
	char *arg2 = argv[2];	
	char key[MAX_BUFFER];
	memset(key, '\0', sizeof(key));
	fp = fopen(argv[2], "r");
	
	if (fp == NULL)
	{
		error("Could not open key file");
	}
	
	/*Read the contents of fp into key*/
	//memset(key, '\0', sizeof(key));
	fgets(key, sizeof(key), fp);
	fclose(fp);

	if (strlen(plaintext) > strlen(key))
	{
		fprintf(stderr, "Key file is too short\n");
		exit(1);
	}
	
	/*These 2 for loops check for any unwanted characters*/	
	for (int i = 0; i < strlen(plaintext); i++)
	{
		if((int)plaintext[i] < 65 && (int)plaintext[i] != 32 && (int)plaintext[i] != 10)
		{
			fprintf(stderr, "Invalid character found: %c\n", plaintext[i]);
			exit(1);
		} 
		else if ((int)plaintext[i] > 90)
		{
			fprintf(stderr, "Invalid character found: %c\n", plaintext[i]);
			exit(1);
		}
	}
	for (int i = 0; i < strlen(key); i++)
	{
		if((int)key[i] < 65 && (int)key[i] != 32 && (int)key[i] != 10)
		{
			fprintf(stderr, "Invalid character found: %c\n", key[i]);
			exit(1);
		} 
		else if ((int)key[i] > 90)
		{
			fprintf(stderr, "Invalid character found: %c\n", key[i]);
			exit(1);
		}
	}
	
	strcpy(bigString, plaintext);
	strcat(bigString, "|");
	strcat(bigString, key);
	strcat(bigString, "#");
	
	/*Set up the server address struct*/
	/*Clear out the address struct*/
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	
	/*Get the port number and convert from string to integer*/
	portNumber = atoi(argv[3]);
	
	/*Create a network-capable socket*/
	serverAddress.sin_family = AF_INET;
	
	/*Store the port number*/
	serverAddress.sin_port = htons(portNumber);
	
	/*Conver the machine name into a special form of address*/
	serverHostInfo = gethostbyname("localhost");
	
	if (serverHostInfo == NULL)
	{
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}
	
	/*Copy in the address*/
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr_list[0], serverHostInfo->h_length);
	
	/*Setup the socket*/
	/*Create the socket*/
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0)
	{
		fprintf(stderr, "CLIENT: ERROR opening socket\n");
		exit(2);
	}
	
	/*Connect to server*/
	/*Connect socket to address*/
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	{
		fprintf(stderr, "CLIENT: ERROR connecting to port: %d\n", portNumber);
		exit(2);
	}
	
	/*Send the password*/	
	charsWritten = send(socketFD, PASSWORD, strlen(PASSWORD), 0);
	if (charsWritten < 0)
	{
		fprintf(stderr, "CLIENT: ERROR writing to socket\n");
		exit(1);
	}
	
/*	if (charsWritten < strlen(buffer))
	{
		fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
	}
	*/
	/*Get return message from server*/
	memset(buffer, '\0', sizeof(buffer));
	
	/*Read data from the socket, leaving \0 at the end*/
	charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0);
	if (charsRead < 0)
	{
		error("CLIENT: ERROR reading from socket");
	}
	
	char buffer1[MAX_BUFFER];
	/*After being accepted, send the plaintext and key contents*/
	if (strcmp(buffer, ACCEPTED) == 0)
	{
		/*Send the plaintext and key*/
		//charsWritten = send(socketFD, bigString, strlen(bigString), 0);
		
		char *temp = malloc(strlen(bigString)*sizeof(char)+1);
		//memset(temp, '\0', sizeof(temp));
		strcpy(temp, bigString);
		int *len = malloc(sizeof(int));
		*len = strlen(bigString);
	//	printf("C: Sending bigString: %s\n", temp);
		int stats = sendall(socketFD, temp, len);	
	//	printf("C: status: %d\n", stats);
		if (charsWritten < 0)
		{
			error("CLIENT: ERROR writing to socket");
		}
	
	/*	if (charsWritten < strlen(buffer))
		{
			printf("CLIENT: WARNING: Not all data written to socket!\n");
		}
	*/	
		memset(buffer, '\0', sizeof(buffer));
		//sleep(1);
	//	printf("C: Receving cypher\n");
		char *tempC = malloc(MAX_BUFFER*sizeof(char));
		while (strstr(tempC, "#") == NULL)
		{
			memset(buffer, '\0', sizeof(buffer));
			charsRead = recv(socketFD, buffer, sizeof(buffer), 0);
			//printf("C: received: %s\n", buffer);
			if (charsRead < 0)
			{
				error("CLIENT: ERROR reading from socket");
			}
			strcat(tempC, buffer);
		}

		tempC = strtok(tempC, "#");

	//	printf("C: RECEIVED cypher: %s\n", tempC);	
		if (strcmp(buffer, "denied") == 0)
		{
			error("CLIENT: ERROR attempting to access wrong service");
			close(socketFD);
			exit(2);
		}


	printf("%s\n", tempC);
	}
	
	//printf("%s\n", tempC);
	//printf("%s\n", buffer);
	
	
	/*Close the socket*/
	close(socketFD);
}

