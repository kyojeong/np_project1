#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <fcntl.h>

using namespace std;

void init_npshell();

int main(){
	int save_stdout;
        save_stdout=dup(STDOUT_FILENO);
	init_npshell();
	while(true){
		dup2(save_stdout,STDOUT_FILENO);
		cout << "% ";
		string inputstr;
		getline(cin, inputstr);
		
		
		char *inputstr2=new char[1000];

		for(int i=0;i<inputstr.length();i++){
			inputstr2[i]=inputstr[i];
		}
		
		char *block[10000];
		
		
		int count=0;
		char *token;
		token=strtok(inputstr2," ");
		
		while(token!=NULL){
			block[count]=token;
			count++;
			token=strtok(NULL," ");
		}
		
		if(!strcmp(block[0],"printenv")){
                        cout<<getenv(block[1])<<endl;
                }

		else if(!strcmp(block[0],"setenv")){
			setenv(block[1],block[2],1);
		}
		else if(!strcmp(block[0],"exit")||!strcmp(block[0],"EOF")){
			break;
		}

		else{
			pid_t pid;
			int pfd[2];
			if(pipe(pfd)){
				fprintf(stderr,"Pipe failed\n");
				
			}

			pid=fork();
			if(pid==0){
				/*this is child*/
				/*fd=1 is read, fd=0 is write*/
				
				close(pfd[0]);
				dup2(pfd[1],STDOUT_FILENO);
				

				if(!strcmp(block[0],"ls")){
					execlp("ls","ls",NULL);
				}
				else{
					if(block[2]!=NULL){
						if(!strcmp(block[2],">")){
                                        		int outfd=open(block[3],O_RDWR|O_CREAT|O_TRUNC|O_CLOEXEC,0600);
							pid=fork();
							if(pid==0){
                                       				dup2(outfd,STDOUT_FILENO);
								close(outfd);	
								execlp(block[0],block[0],block[1],NULL);
								

							}
							else if(pid>0){
								int status;
								int len;
								char buffer[1024];
								
								close(outfd);
				                                while((len=read(pfd[0],buffer,1023))>0){
                                				        buffer[len]='\0';
                                      					printf("%s",buffer);
                                				}


								waitpid(pid,&status,0);
								
								
								if(WIFEXITED(status)) return WEXITSTATUS(status);
								else return EXIT_FAILURE;
							}
						
							
							
							
                                		}

						
					}
					else{
					
						execlp(block[0],block[0],block[1],NULL);
					}
					
					
				
				}
				close(pfd[1]);
			
				
			}
			else if(pid>0){
				/*this is parent*/
				char buffer[1024];
				int status;
				close(pfd[1]);
				int len;
				
				
				while((len=read(pfd[0],buffer,1023))>0){
					buffer[len]='\0';
					printf("%s",buffer);
				}

				waitpid((pid_t)pid,&status,0);
				close(pfd[0]);
				
			}
			
			
		
		}
	}

}

void init_npshell()
{
        setenv("PATH","bin:.",1);
}


