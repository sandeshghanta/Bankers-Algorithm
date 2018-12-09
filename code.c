#include <stdio.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
void sem_lock(int sem_id,int sem_num){
	struct sembuf sem_op; 
	sem_op.sem_num = sem_num;
	sem_op.sem_op = -1;
	sem_op.sem_flg = 0;
	semop(sem_id, &sem_op, 1);
}
void sem_unlock(int sem_id,int sem_num){
	struct sembuf sem_op; 
	sem_op.sem_num = sem_num;
	sem_op.sem_op = 1;
	sem_op.sem_flg = 0;
	semop(sem_id, &sem_op, 1);
}
int main(){
	//6,7,0 are used to synchronise processes init value of 7 and 0 should be 0. 6 is just a mutex init value should be 1. 8 should be 0
	int semid = semget(IPC_PRIVATE, 11, IPC_CREAT | 0777);	
	int maxne = shmget(IPC_PRIVATE,66*sizeof(int),IPC_CREAT|0777);	//maxneed array
	int alloc = shmget(IPC_PRIVATE,66*sizeof(int),IPC_CREAT|0777);	//allocation array
	int req = shmget(IPC_PRIVATE,6*sizeof(int),IPC_CREAT|0777);
	int avail = shmget(IPC_PRIVATE,6*sizeof(int),IPC_CREAT|0777);	//available array
	int var = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0777);	//counting no of processes
	int proc = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0777);	//for communication
	int res = shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0777);	//for communicating the result
	semctl(semid,7,SETVAL,0);
	semctl(semid,6,SETVAL,1);
	semctl(semid,8,SETVAL,0);
	semctl(semid,9,SETVAL,0);
	semctl(semid,10,SETVAL,0);
	if (fork() != 0){
		sem_lock(semid,7);
		printf("\nstarting parent\n");
		int *process_num = (int*)shmat(proc,0,0);
		int *result = (int*)shmat(res,0,0);
		int *request = (int*)shmat(req,0,0);
		int *maxneed = (int*)shmat(maxne,0,0);
		int *allocation = (int*)shmat(alloc,0,0);
		int *available = (int*)shmat(avail,0,0);
		printf("\navailable vector is ");
		for (int i=1;i<=5;i++){
			available[i] = rand()%100 + 50;
			printf("%d ",available[i]);
		}
		printf("\n");
		bool complete[11];
		for (int i=1;i<11;i++){
			complete[i] = false;
		}
		while (1){
			bool flag = true;
			for (int j=1;j<=10;j++){
				bool flag1 = true;
				for (int i=1;i<=5;i++){
					if (available[i] > maxneed[5*j + i] && !complete[j]){
						
					}
					else{
						flag1 = false;
					}
				}
				if (flag1){
					complete[j] = true;
					flag = false;
				}
			}
			if (flag){
				*result = 1;
				for (int i=1;i<11;i++){
					if (!complete[i]){
						printf("Safe sequence does not exist\n");
						*result = 0;
					}
				}
				break;
			} 
		}
		for (int i=1;i<11;i++){
			sem_unlock(semid,8);
		}
		if (*result == 0){
			return 0;
		}
		bool complete1[6];
		for (int i=1;i<11;i++){
			complete[i] = false;
		}
		while (true){
			int flag4;
		lab:
			flag4 = 1;
			for (int i=1;i<=10;i++){
				if (!complete[i]){
					flag4 = 0;
				}
			}
			if (flag4){
				printf("\nThanks for running the code\n");
				return 0;
			}
			sem_lock(semid,10);
			printf("\nprocess %d has requested for ",*process_num);
			for (int i=1;i<=10;i++){
				complete1[i] = complete[i];
			}
			for (int i=1;i<=5;i++){
				if (available[i] < request[i]){
					*result = 0;
					sem_unlock(semid,9);
					goto lab;
				}
				available[i] = available[i] - request[i];
				allocation[5 * *process_num + i] = allocation[5 * *process_num + i] + request[i];
				printf("%d ",request[i]);
			}
			printf("\n");
			*result = 1;
			while (1){
				bool flag = true;
				for (int j=1;j<=10;j++){
					bool flag1 = true;
					for (int i=1;i<=5;i++){
						if (available[i] > maxneed[5*j + i] - allocation[5*j + i] && !complete1[j]){
							
						}
						else{
							flag1 = false;
						}
					}
					if (flag1){
						complete1[j] = true;
						for (int l=1;l<=5;l++){
							available[l] = available[l] + allocation[5*j + l];
						}
						flag = false;
					}
				}
				if (flag){
					*result = 1;
					for (int l=1;l<11;l++){
						if (!complete1[l]){
							printf("Request cannot be granted\n");
							*result = 0;
							break;
						}
					}
					break;
				} 
			}
			if (*result == 1){
				printf("Request has been granted\n");
				for (int i=1;i<11;i++){
					if (!complete[i]){
						for (int j=1;j<=5;j++){
							available[j] = available[j] - allocation[i*5 + j];
						}	
					}
				}
				bool flag3 = true;
				for (int i=1;i<=5;i++){
					if (maxneed[*process_num * 5 + i] != allocation[*process_num * 5 + i]){
						flag3 = false;
					}
				}
				if (flag3){
					printf("Process %d is complete\n",*process_num);		
					for (int j=1;j<=5;j++){
						available[j] = available[j] + allocation[*process_num * 5+j];
					}
					*result = 2;
					complete[*process_num] = true;
				}
				printf("New Available is ");
				for (int i=1;i<=5;i++){
					printf("%d ",available[i]);
				}
				printf("\n");
			}
			else{
				for (int i=1;i<11;i++){
					if (!complete[i] && complete1[i]){
						for (int j=1;j<=5;j++){
							available[j] = available[j] - allocation[i*5 + j];
						}	
					}
				}
				for (int j=1;j<=5;j++){
					allocation[*process_num * 5 + j] = allocation[*process_num * 5 + j] - request[j];
				}
			}
			sem_unlock(semid,9);
		}
	}
	else{
		int process_num;
		int *share = (int*)shmat(proc,0,0);
		int *result = (int*)shmat(res,0,0);
		int *request = (int*)shmat(req,0,0);
		int *maxneed = (int*)shmat(maxne,0,0);
		int *allocation = (int*)shmat(alloc,0,0);
		int *num = (int*)shmat(var,0,0);
		*num = 0;
		bool complete[11];
		for (int i=1;i<11;i++){
			complete[i] = false;
		}
		for (int i=1;i<=10;i++){
			process_num = 10;
			if (i == 10 || fork() == 0){
				process_num = i;
				for (int j=1;j<=5;j++){
					int temp = rand()%10;
					while (temp == 0){
						temp = rand()%10;
					}
					maxneed[i*5 + j] = temp;
					allocation[i*5 + j] = 0;
				}
				break;
			}
		}
		sem_lock(semid,6);
		*num = *num + 1;
		printf("process %d requires ",process_num);
		for (int i=1;i<=5;i++){
			printf("%d ",maxneed[5*process_num + i]);
		}
		printf("\n");
		if (*num == 10){
			sem_unlock(semid,7);
		}
		sem_unlock(semid,6);
		sem_lock(semid,8);
		if (*result == 0){
			printf("process %d is exiting due to lack of safe sequence\n",process_num);
			return 0;
		}
		int localrequest[6];
		for (int i=1;i<=5;i++){
			localrequest[i] = -1;
		}
		while (!complete[process_num]){
			sem_lock(semid,6);
			*share = process_num;
			for (int i=1;i<=5 && localrequest[i] == -1;i++){
				int b = maxneed[process_num * 5 + i] - allocation[process_num * 5 + i];
				if (b == 1 || b == 0){
					localrequest[i] = b;
				}
				else{
					localrequest[i] = rand()%b;
				}
			}
			for (int i=1;i<=5;i++){
				request[i] = localrequest[i];
			}
			sem_unlock(semid,10);
			sem_lock(semid,9);
			if (*result == 0 && rand()%3 == 0){
				printf("Process has not been granted it's request it is letting go of its request\n");
				for (int i=1;i<=5;i++){
					localrequest[i] = -1;
				}
			}
			else if (*result == 0){
				printf("Process has not been granted the request\n");
			}
			else if (*result == 1){
				printf("Process has been granted the request\n");
				for (int i=1;i<=5;i++){
					localrequest[i] = -1;
				}
			}
			else if (*result == 2){
				for (int i=1;i<=5;i++){
					localrequest[i] = -1;
				}
				complete[process_num] = true;
			}
			sem_unlock(semid,6);
		}
	}
	return 0;
}
