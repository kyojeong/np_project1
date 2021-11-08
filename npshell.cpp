#include <iostream>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex>

using namespace std;

typedef struct block{
	string cmd;	
	vector<string> argv;
	int has_fd_in,has_fd_out;
	int fd_in,fd_out;
	int ptype; //0:no pipe 1:pipe 2:error pipe
}block;

typedef struct Pipe{
	int count;
	int fd[2]; //1:read 0:write
}Pipe;

vector<Pipe> pipes;
void splitpipe(string inputstr,vector<block> &blocks);
void splitcmd(block &block);
void makepipe(block &block);
void execution(block &block);
void vtoa(vector<string> &vec,char *arr[],int index);

int main()
{
	setenv("PATH","bin:.",1); //initial
	string inputstr;
	vector<block> blocks;
	int first_cmd=1;

	while(true){
		cout<<"% ";
		getline(cin,inputstr);
		if(inputstr.length()==0) continue;
		splitpipe(inputstr,blocks);
		first_cmd=1;
		
		while(!blocks.empty()){
			splitcmd(blocks[0]);
			makepipe(blocks[0]);

			if(first_cmd==0){
				for(int i=0;i<pipes.size();i++){
					if(pipes[i].count==-1){
						blocks[0].fd_in=pipes[i].fd[0];
						break;
					}
				}	
				blocks[0].has_fd_in=1;			
			}
			else if(first_cmd==1){
				for(int i=0;i<pipes.size();i++){
					if(pipes[i].count==0){
						blocks[0].fd_in=pipes[i].fd[0];
						blocks[0].has_fd_in=1;
						break;
					}
				}
				
			}
			execution(blocks[0]);
			blocks.erase(blocks.begin());
			first_cmd=0;
			
		}
		
		for(int i=0;i<pipes.size();i++){
			pipes[i].count--;
		}		
	}
}

void splitpipe(string inputstr,vector<block> &blocks){

	
	int begin =0;
	int end;
	block block;
	inputstr=inputstr+"| ";
	
	while((end=inputstr.find(' ',inputstr.find_first_of("|!",begin)))!=-1){
		//initial block
		block.has_fd_in=0;
		block.has_fd_out=0;
		block.fd_in=0;
		block.fd_out=0;
		block.ptype=0;

		//split pipe
		block.cmd=inputstr.substr(begin,end-begin);
		while(block.cmd[0]==' ')block.cmd=block.cmd.substr(1);//remove front space
		if(end==inputstr.length()-1)block.cmd=block.cmd.substr(0,block.cmd.length()-1);
		while(block.cmd[block.cmd.length()-1]==' ')block.cmd=block.cmd.substr(0,block.cmd.length()-1);//remove back space
		blocks.push_back(block);
		begin=end+1;
	}
	
}

void splitcmd(block &block){
	char *temp=new char[1000];
	char *token;
	for(int i=0;i<block.cmd.length();i++){
		temp[i]=block.cmd[i];
	}
	
	token=strtok(temp," ");
	while(token!=NULL){
		block.argv.push_back(token);
		token=strtok(NULL," ");
	}
}
 
	
 
void makepipe(block &block)
{
	string back=block.argv.back();
	Pipe pipenew;
	
	pipenew.count=0;
	
	static regex reg("[|!][0-9]+");	
	if(regex_match(back,reg)){ 
		//check |1 |2
		if(back[0]=='|'){
			block.ptype=1;
		}
		else if(back[0]=='!'){
			block.ptype=2;
		}
		pipenew.count=stoi(back.substr(1));
		int  checkpipe=0;

		for(int i=0;i<pipes.size();i++){
			if(pipes[i].count==pipenew.count){
				block.fd_out=pipes[i].fd[1];
				checkpipe=1;
				break;
			}
		}

		if(checkpipe==0){
			pipe(pipenew.fd);
			block.fd_out=pipenew.fd[1];
			pipes.push_back(pipenew);
		}
	}
	
	else{
		if(back=="|"){
			block.ptype=1;
			pipenew.count=-1;
			pipe(pipenew.fd);
			block.fd_out=pipenew.fd[1];
			pipes.push_back(pipenew);
		}
		else if(back=="!"){
			block.ptype=2;
			pipenew.count=-1;
                        pipe(pipenew.fd);
                        block.fd_out=pipenew.fd[1];
                        pipes.push_back(pipenew);
		}
		else{
			block.ptype=0;
		}
	}

	if(block.ptype!=0){
		block.has_fd_out=1;
	}	
}

void execution(block &block)
{
	char *array[1000];
	string cmd=block.argv[0];
	int pid;
		
	if(cmd=="printenv"){
		if (getenv(block.argv[1].data()) != NULL){
			cout<<getenv(block.argv[1].data())<<endl;
		}
	}
	else if(cmd=="setenv"){
		setenv(block.argv[1].data(),block.argv[2].data(),1);
	}
	else if(cmd=="exit"||cmd=="EOF"){
		exit(0);
	}
	else{
		
		int status;
		while((pid=fork())<0){
			while(waitpid(-1,&status,WNOHANG)>0);
		}
		
		if(pid==0){
			//child pid
			if(block.has_fd_in==1){
				dup2(block.fd_in,STDIN_FILENO);
			}
			if(block.has_fd_out==1){
				if(block.ptype==1){
					dup2(block.fd_out,STDOUT_FILENO);
				}
				else if(block.ptype==2){
					dup2(block.fd_out,STDOUT_FILENO);
					dup2(block.fd_out,STDERR_FILENO);
				}
				
			}
			//close all pipe fd
	                for(int i=0;i<pipes.size();i++){
        	                close(pipes[i].fd[0]);
                	        close(pipes[i].fd[1]);
               		}

                	//output redirection
                	int outfd,index;

              		for(index=0;index<block.argv.size();index++){
                        	if(block.argv[index]==">"){
                                	break;
                        	}
                	}
               		if(index==block.argv.size()){
                        	index=-1;
               		}

                	if(index==-1){
                        	if(block.ptype==0){
					vtoa(block.argv,array,block.argv.size());
                       		}
                        	else{
					vtoa(block.argv,array,block.argv.size()-1);

                        	}

                	}
			else{
                        	outfd=open(block.argv.back().data(),O_RDWR|O_CREAT|O_TRUNC,0600);
                        	dup2(outfd,STDOUT_FILENO);
                        	dup2(outfd,STDERR_FILENO);
                        	close(outfd);
				vtoa(block.argv,array,index);
                	}
			
                	if(execvp(cmd.data(),array)==-1){
                        	cerr<<"Unknown command: [" << cmd << "]."<<endl;
                	}
                	

		}

		else if(pid>0){
                	//parent close fd
                	if(block.has_fd_in){
                        	for(int i=0;i<pipes.size();i++){
                                	if(pipes[i].fd[0]==block.fd_in){
                                        	close(pipes[i].fd[0]);
                                        	close(pipes[i].fd[1]);
                                        	pipes.erase(pipes.begin()+i);
                                        	break;
                                	}
                        	}
                	}

                	if(block.ptype==0){
                        	waitpid(pid,&status,0);
                	}
               		else{
                        	waitpid(-1,&status,WNOHANG);
                	}
        	}
	}		
}

void vtoa(vector<string> &vec,char *arr[],int index)
{
	for(int i=0;i<index;i++){
		arr[i]=(char*)vec[i].data();
	}
	arr[vec.size()]=NULL;
}
