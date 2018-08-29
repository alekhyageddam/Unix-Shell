//Objective: Write a C program to design a shell in Linux.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

//function to execute a system command such as grep, ls, pwd etc
int execute(char* cmd, char** args){
	pid_t pid = fork();
	if(pid == -1){//error
		printf("This is an error.\n");
	}
	else if(pid == 0){//child
		//exec
		execvp(cmd, args);
	}
	else{ //parent
			printf("Process created with PID: %d\n",pid);
			signal(SIGCHLD, SIG_IGN);
			//wait(NULL);
	}
	return 0;
}


int execute2(char* cmd, char ** args, char* inp, char*out){	
	//set the initial input
	pid_t ret = fork();

	if(ret == 0){ //child process
		int fdin = 0;
		int fdout = 1;
		if(inp != NULL){
			fdin = open(inp, O_RDWR | O_CREAT|O_TRUNC, S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP );
			//redirect input
			dup2(fdin, 0);
			close(fdin);
		}
		if(out != NULL){
			fdout = open(out,O_RDWR | O_CREAT|O_TRUNC, S_IREAD | S_IWRITE | S_IRGRP | S_IWGRP);
			//redirect output
			dup2(fdout, 1); //overwrites stdout with the new file
			close(fdout);
		}
		execvp(cmd, args);
	}
	else{ //parent
		wait(NULL);
	}
		return 0;
  }


//function to read from command line
char * processInput(char *str)
{
	char *buf = str;
	while((*buf = getchar()) != '\n')
		buf++;
	*buf = 0;
	return buf;
}	

//parses various characters, also does quote checking

void detAction(char * command, char ** args){
	int quote = 0;
    while (*command != '\0' && *command != '>' && *command != '<')
    {
        while (*command == ' ' || *command == '\t' || *command == '\n'){
						*command++ = '\0';
						if(*command == '\"'){
							if(quote == 1){
								quote = 0;
							}
							else
								quote = 1;
						}
				}
        *args++ = command;
        while (*command != '\0' && (*command != ' ' || quote == 1) && *command != '\t' && *command != '\n'){
					command++;
					if(*command == '\"'){
						if(quote == 1){
							quote = 0;
						}
						else
							quote = 1;
					}
				}
    }
    *args = '\0';
}


//function to execute piped system commands, including checking for I/O direction
int execPipes(char ** args){
	
	int fds[2];	//file descriptors
	pid_t pid;	//pid to call fork()

	int noOfCmds = 0;	//number of commands
	char* commands[100];  //array to hold the commands

	int m = 0;
	int n = 0;
	int o = 0;
	int p = 0;
	int fd_in = 0;

	int exit = 0;

	while(args[m] != NULL){
		if(strcmp(args[m], "|") == 0){
			noOfCmds++;
		}
		m++;
	}
	noOfCmds++;


	//loop through the arguments
	while(args[n] != NULL & exit != 1){
		o = 0;
		//store the commands into char commands[]
		while(strcmp(args[n], "|") != 0){
			commands[o] = args[n];
			n++;
			if(args[n] == NULL){
				exit = 1;
				o++;
				break; 
			}
			o++;
		}
		commands[o] = NULL; //after all the cmds have been stored
		n++;

		pipe(fds);
		pid=fork();
		if(pid == -1){
			printf("Error: could not make child process\n");
			return -1;
			
		}
		if(pid == 0){
			/*	if (p % 2 != 0){
					dup2(fds[1], STDOUT_FILENO);
				}
				else{ */
					dup2(fds[0], STDIN_FILENO);
				}
			
			if(execvp(commands[0],commands)== -1){
				kill(getpid(), SIGTERM);
			} 
		
		else if(p == noOfCmds-1){
				close(fds[0]);
		}
		else{
			if(p % 2 != 0){
				close(fds[1]);
			}
		/*	else{
				close(fds[0]);
			} */
		}
		waitpid(pid,NULL,0);//wait to run completely
		p++;
	}
		 return 0;
	} //end of while loop	
	 



int changeDir (char* args){	//parse and execute the changeDirectory (changeDcd) command
	if(args != NULL){
		char* test = '-';
		if(strcmp(args,test) == 0){ //if both strings are equal
			chdir(getenv("HOME"));	//change directory -> get home envir. variable
			char cwd[256];
			if(getcwd(cwd,sizeof(cwd)) != NULL) //check if the pathname of cwd exists
				printf("%s\n", cwd);
			return 0;
		}

		else if(chdir(args) == -1){
			printf("\nThere is no such directory.");
			return -1;
		}
	}
	else
		chdir(getenv("HOME"));
	return 0;
}


//parse the command being read; checks for exit/cd/I.O redirection handling/pipeling/background process
void parseCmd(char *cmd){
	char*args[150];
	detAction(cmd, args);		
	int strlen;
	int execcmd = 1;

	//determines the length of the args
	for(int strlen = 0; args[strlen]; strlen++);

	//loop through the args and execute depending on its type
	for(int i = 0; args[i]; i++)
	{
		if(strcmp(args[i], "cd") == 0){
			char* nxt = args[i+1];
			if(i+1 < strlen){
				if(args[i+1][0] == '<'){
					int fd_in = 0;
					char buffer [100];
					if(args[i+2] != NULL){
						fd_in = open(args[i+2], O_RDWR);
						read(fd_in,buffer,100);
						nxt = buffer;
						close(fd_in);	
					}
				}
			}
			execcmd = 0;
			changeDir(nxt); //execute the change directory 
		}

		if(strcmp(args[i],"exit") == 0){
			exit(0); //clean exit
		}	
		
		if(args[i][0] == '|'){	//handle shell piping
			execcmd = 0;
			execPipes(args);
			return;
		}			
		char* inputFile = NULL;
		char* outputFile = NULL;
		if(args[i][0] == '>'){	//handle output redirection
			execcmd = 1; 
			if(i + 1 < strlen)
				outputFile = args[i+1];
			args[i] = NULL;
			i++; //increment to retrieve the filename 
		}

		if(args[i][0] == '<'){	//handle input redirection
			if(i + 1 < strlen)
				inputFile = args[i+1];
			args[i] = NULL;
			i++;
			execcmd = 1;
		}

		if(args[i][0] == '&'){	//handle background process (&)
			args[i] = NULL;
			i++;
			strlen--;
			execcmd = 1;
		}

		if(execcmd == 0) execute(cmd,args);
		if(execcmd == 1) execute2(cmd, args,inputFile, outputFile);		
	}
}


int main(int agrc, char** argv)
{
	char cmd[256];
	//int test = 1;
	while(1){ //Main loop		
		//1.command = get command from user using cin
		printf("$>");
		processInput(cmd);
		//2.PARSE Command
		while(cmd[0] == '\0')
		{
			printf("$>");
			processInput(cmd);
		}
		parseCmd(cmd);
	}
	return 0;
}
