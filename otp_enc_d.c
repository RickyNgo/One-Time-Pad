#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define PASSWORD "encrypt123"
#define ACCEPTED "accepted"
#define MAX_BUFFER 196000

/*Keeps track of the child processes*/
pid_t childProcess[5] = {0};
int childCount = 0;

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
		bytesleft -=n;
	}
	
	*length = total;
	
	return n==-1?-1:0;
}

/*Wrapper for the fork() call to update the process list*/
pid_t newProcess()
{
	pid_t spawnPID = fork();

	for (int i = 0; i < 5; i++)
	{
		if (childProcess[i] == 0)
		{
			childProcess[i] = spawnPID;
			childCount++;
			return spawnPID;
		}
	}
	
	perror("MAX connections met");
	return -1;
}

/*Cleans up any defunct children*/
void exitProgram()
{
	printf("Daemon shutting down...\n");
	if (childProcess != NULL)
	{
		for (int i = 0; i < 5; i++)
		{	
			int status = -5;
			int result = kill(childProcess[i], SIGTERM);
			waitpid(childProcess[i], &status, 0);
		}
	}
	fflush(NULL);
	exit(0);
}

void error(const char *msg) 
{
	perror(msg);
	exit(1);
}

/*Takes a port number as the only argument*/
int main(int argc, char *argv[])
{
	int port = atoi(argv[1]);

	int listenSocketFD;
	int establishedConnectionFD;
	int portNumber;
	int charsRead;
	int stayAwake = 1;
	socklen_t sizeOfClientInfo;
	char buffer[MAX_BUFFER];
	char key[MAX_BUFFER];
	memset(key, '\0', sizeof(key));
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	
		
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}
	
	/*Setup the server address struct*/
	/*Clear out the address struct*/
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	
	/*Get the port number, convert to an integer from a string*/
	portNumber = atoi(argv[1]);
	
	/*Create a network-capable socket*/
	serverAddress.sin_family = AF_INET;
	
	/*Store the port number*/
	serverAddress.sin_port = htons(portNumber);
	
	/*Any address is allowed for connection to this process*/
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	
	/*Set up the socket*/
	/*Create the socket*/
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocketFD < 0)
	{
		fprintf(stderr, "ERROR opening socket\n");
		exit(2);
	}
	
	/*Enable the socket to begin listening*/
	/*Connect socket to port*/
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	{
		fprintf(stderr, "ERROR on binding to port: %d\n", portNumber);
		exit(2);
	}
	
	/*Flip the socket on, can support up to 5 connections*/
	listen(listenSocketFD, 5);
	
	/*Loop to keep listening for connections*/
	while (stayAwake)
	{
		/*Accept a connection, blocking if one is not available until one connects*/
		/*Get the size of the address for the client that will connect*/
		sizeOfClientInfo = sizeof(clientAddress);
		
		/*Accept*/
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		
		/*If the port is already in use*/
		if (establishedConnectionFD < 0)
		{
			fprintf(stderr, "ERROR on accept to port: %d\n", portNumber);
			exit(2);
		}	
		else
		{	
			/*Fork a new process to do everything*/
			int exitStatus = -5;
			pid_t spawnPID = newProcess();
			
			if (spawnPID == -1)
			{
				perror("Hull Breach!");
				exit(1);
			}
			else if (spawnPID == 0)
			{
				/*Get the message from the client and display it*/
				memset(buffer, '\0', sizeof(buffer));
				
				/*Read the client's message from the socket*/
				charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
				
				/*-1 for a bad read*/
				if (charsRead < 0)
				{
					fprintf(stderr, "ERROR reading from socket\n");
					continue;
				}
				
				/*Setup the strings*/
				char *secret = malloc(MAX_BUFFER*sizeof(char));
				char *plaintxt = malloc(MAX_BUFFER*sizeof(char));
				char *key = malloc(MAX_BUFFER*sizeof(char));
				
				/*The secret is the password to be able to connect and use the server*/
				secret = strtok(buffer, " |\n");
				
				/*Deny if wrong password*/
				if (strcmp(secret, PASSWORD) != 0)
				{
					fprintf(stderr, "ERROR wrong service\n");
					charsRead = send(establishedConnectionFD, "denied", 6, 0);
					close(establishedConnectionFD);
					continue;
				}
				
				/*Send a success message back to the client*/
				charsRead = send(establishedConnectionFD, ACCEPTED, sizeof(ACCEPTED), 0);
				if (charsRead < 0)
				{
					fprintf(stderr, "ERROR writing to socket\n");
					continue;
				}
					
				/*Get the actual data*/
				memset(buffer, '\0', sizeof(buffer));
				//charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
				
				char *tempC = malloc(MAX_BUFFER*sizeof(char));
				
			//	printf("S: RECEIVING bigString\n");

				while (strstr(tempC, "#") == NULL)
				{
				memset(buffer, '\0', sizeof(buffer));
				charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), MSG_DONTWAIT);
				//printf("%d\n", charsRead);
				strcat(tempC, buffer);
				}		
				if (charsRead < 0)
				{
					fprintf(stderr, "ERROR writing to socket\n");
					continue;
				}
				
			//	printf("S: RECEIVED bigString: %s\n", tempC);	
				plaintxt = strtok(tempC, "#|\n");
				key = strtok(NULL, "#|\n");
				
				/*ENCRYPTION*/
				char value[27] = {'A', 'B', 'C', 'D', 'E', 'F', 
						'G', 'H', 'I', 'J', 'K', 'L', 
						'M','N', 'O', 'P', 'Q', 'R', 
						'S', 'T', 'U', 'V', 'W', 'X', 
						'Y', 'Z', ' '};
				
				int textLength = strlen(plaintxt);
				int temp[textLength];
				memset(temp, -1, sizeof(temp));
				
				char *cypherText = malloc(textLength*sizeof(char));
				
				/*Each index is the corresponding value for each char*/
				for (int i = 0; i < strlen(plaintxt); i++)
				{
					for (int j = 0; j < 27; j++)
					{
						if (plaintxt[i] == value[j])
						{
							temp[i] = j;
						}
					}
					
					for (int j = 0; j < 27; j++)
					{
						if (key[i] == value[j])
						{
							temp[i] += j;
						}
					}
					temp[i] = temp[i]%27;
				}
				
				/*Based on the new value, use that as the index for the cypher char*/
				for (int i = 0; i < strlen(plaintxt); i++)
				{
					int tempVal = temp[i];
					cypherText[i] = value[tempVal];
				}
				
				strcat(cypherText, "#");
				/*END ENCRYPTION*/
				
				
				int *len = malloc(sizeof(int));
				*len = strlen(cypherText);
			//	printf("S: Sending cypher: %s\n", cypherText);
				int stats = sendall(establishedConnectionFD, cypherText, len);
			//	printf("S: status: %d\n", stats);
			//	charsRead = send(establishedConnectionFD, cypherText, strlen(cypherText), 0);
			
			/*	if (charsRead < strlen(cypherText))
				{
					fprintf(stderr, "SERVER: ERROR not all data written");	
				}*/
			
	
				/*Close the existing socket which is connected to the client*/
				close(establishedConnectionFD);
	
				/*Close the listening socket*/
				close(listenSocketFD);
				exit(0);
			}	
			else
			{
				sleep(1);
			}
		}
		
		/*After each iteration, try and clean up the processes*/	
		int status = -5;
		for (int i = 0; i < 5; i++)
		{
			if (childProcess[i] != 0)
			{
				int status;
				pid_t pid = waitpid(childProcess[i], &status, WNOHANG);
				if (pid != 0 || pid != -1)
				{
					childProcess[i] = 0;
					childCount--;
				}
			}
		}
	}
	
	/*Final clean up*/
	exitProgram();

	return 0;

}
