//서버 프로그램

#include "msq.h"
#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void signalHandler(int signum);

//메시지큐 생성과 동작에 필요한 key와 msq 지시자
key_t msq_key = 0;//(=31)

//(메시지 큐를 관리자용과 클라이언트용으로 나누어 부모와 자식프로세스에서 각각 운용 -> 2개의 프로그램이나 다름없음)
int msq_client_id = 0;//클라이언트용
int msq_admin_id = 0;//관리자용

int main(int argc, char const* argv[]) {

	MsgClient client;
	MsgAdmin admin;
	pid_t pid = 0;

	//ipc사용을 위한 키 획득
	msq_key = ftok(".", 31);
	if(msq_key == -1){
		perror("ftok() error!(ipc 키생성 실패) : ");
		kill(getpid(), SIGINT);
	}

	//시그널 동작 재정의
	signal(SIGINT, signalHandler);	//ctrl + c 입력 시그널
	signal(SIGUSR1, signalHandler);	//클라이언트 메시지큐 에러 시그널
	signal(SIGUSR2, signalHandler);	//관리자 메시지큐 에러 시그널
	
	//자식프로세스를 생성하여 부모프로세스에서는 클라이언트와 통신, 자식프로세스에서는 관리자와 통신하도록 구성
	pid = fork();

	//fork() 예외처리
	if(pid < 0){
		perror("fork() error! ");
		exit(-1);
	}
	//부모프로세스 - 고객(클라이언트)와 통신
	if(pid > 0){

		//메시지큐 생성
		msq_client_id = msgget(msq_key, IPC_CREAT | 0666);
		if(msq_client_id == -1) {
			perror("msgget() error!(클라이언트 메시지큐 생성 실패) : ");
			kill(getpid(), SIGUSR1);
		}
		cout<< "clientserver is working... "<< endl;

		double money = 315400;//DB가 없어서 임시로 만든 잔액 저장 변수
		
		while(1) {
			//client와 통신할 메시지 구조체 초기화
			memset(&client, 0x00, sizeof(MsgClient));
			
			//메시지 수신(type 우선순위 X, 메시지가 올 때까지 대기)
			int rcvSize = msgrcv(msq_client_id, &client, MSG_SIZE_CLIENT, MSG_TYPE_CLIENT, 0);
			//메시지 수신 실패 시
			if(rcvSize == -1) {
				perror("msgrcv() error!(작업 대기 중 - 클라이언트로부터 메시지수신 실패) : ");
				kill(getpid(), SIGUSR1);
			}
			//메시지 수신 성공 시
			if(rcvSize != -1) {
				//클라이언트에서 날아온 작업코드에 따라 다른 동작 수행
				//작업코드는 MsgClient.cmd에 저장되어있고, 클라이언트가 MsgClient구조체에 담아서 보내줌
				//1 : 고객 로그인
				//2 : 고객 회원가입
				//3 : 입금
				//4 : 출금
				switch(client.cmd){
					case 1:	{	//고객 로그인(임시 테스트)
								//날아온 메시지의 ID와 PW 일치 여부에 따라 정보를 제공하거나, 제공을 거부함 
								//(추후 DB파일에서 읽을 예정)
						//ID/PW가 일치하면
						if((strcmp(client.data.clientId, "hmschlng") == 0) && (strcmp(client.data.clientPw, "asdf1234!@") == 0)){			
							//MsgClient구조체에 정보(더미 데이터)를 담아 메시지 송신
							client.mtype = MSG_TYPE_CLIENT;
							strcpy(client.data.clientName, "이방환");
							strcpy(client.data.clientResRegNum, "940430-1");
							strcpy(client.data.clientAccountNum, "123123-01-987654");
							client.data.clientBalance = money;
							int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
							//msgsnd() 예외처리
							if(sndSize != 0){
								perror("msgsnd() error!(로그인 - 회원정보 송신 실패) : ");
								kill(getpid(), SIGUSR1);
							}
						}
						//ID/PW가 일치하지 않으면 로그인 거부 메시지 송신
						else{
							//(bool) MsgClient.is_error을 참으로 변경한 뒤 메시지 송신
							client.is_error = true;
							int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
							//송신 실패 시
							if(sndSize != 0) {
								perror("msgsnd() error!(로그인 - 로그인 승인거부 메시지 송신 실패) ");
								kill(getpid(),SIGUSR1);
							}
						}
						break;
					}	
					case 2: {	//고객 회원가입
								//클라이언트에게서 ID/PW/이름/주민번호가 날아오면, 계좌번호를 생성, DB에 저장하고 클라이언트에게 메시지 송신
						//ID 중복체크(DB 구현 후 작성 예정) - 중복되면 client.is_error = true로 설정 후 메시지 송신 예정
						
						//ID 중복이 아니면 계좌번호 생성 후 메시지 송신
						//(계좌번호 생성은 추후 구현 예정)
						strcpy(client.data.clientAccountNum, "123456-12-1234567");	
						int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
						//송신 실패 시
						if(sndSize != 0){
							perror("msgsnd() error!(회원가입 - 회원가입 거부 메시지 송신 실패) ");
							kill(getpid(), SIGUSR1);
						}
						break;
					}
					case 3: {	//고객 입금
									//입금할 액수가 MsgClient.data.clientBalance에 담겨서 메시지가 날아옴
									//DB에 추가하는 부분은 추후 구현 예정
						client.mtype = MSG_TYPE_CLIENT;
						strcpy(client.data.clientId, "hmschlng");
						strcpy(client.data.clientPw, "asdf1234!@");
						strcpy(client.data.clientName, "이방환");
						strcpy(client.data.clientResRegNum, "940430-1");
						strcpy(client.data.clientAccountNum, "123123-01-987654");
						client.data.clientBalance = money;
						int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
						if(sndSize != 0) {
							perror("msgsnd() error!(입금 - 회원정보 송신 실패) : ");
							kill(getpid(), SIGUSR1);
						}
						else {
							int rcvSize = msgrcv(msq_client_id, &client, MSG_SIZE_CLIENT, MSG_TYPE_CLIENT, 0);
							if(rcvSize == -1) {
								perror("msgrcv() error!(입금 - 입금액 정보 수신 실패) : ");
								kill(getpid(), SIGUSR1);
							}
							else { 
								cout << "입금액 : " << client.data.clientBalance << "원" << endl;	//입금할 액수 출력
								client.data.clientBalance += money;
								money = client.data.clientBalance;
								int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
								if(sndSize != 0) {
									perror("msgsnd() error!(입금 - 갱신정보 송신 실패) : ");
									kill(getpid(), SIGUSR1);
								}
							}
						}
						break;
					}
					case 4: {	//고객 출금
									//출금할 액수가 MsgClient.data.clientBalancl에 담겨서 메시지가 날아옴
									//DB에 추가하는 부분은 추후 구현 예정

						//로그인 후 입금을 해도 정보가 남아있지만, 그냥 동기화 목적으로 더미데이터를 넘겨봄
						client.mtype = MSG_TYPE_CLIENT;
						strcpy(client.data.clientId, "hmschlng");
						strcpy(client.data.clientPw, "asdf1234!@");
						strcpy(client.data.clientName, "이방환");
						strcpy(client.data.clientResRegNum, "940430-1");
						strcpy(client.data.clientAccountNum, "123123-01-987654");
						client.data.clientBalance = money;
						int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
						if(sndSize != 0) {
							perror("msgsnd() error!(출금 - 회원정보 송신 실패) : ");
							kill(getpid(), SIGUSR1);
						}
						else {
							int rcvSize = msgrcv(msq_client_id, &client, MSG_SIZE_CLIENT, MSG_TYPE_CLIENT, 0);
							if(rcvSize == -1) {
								perror("msgrcv() error!(출금 - 입금액 정보 수신 실패) : ");
								kill(getpid(), SIGUSR1);
							}
							else { 
								cout << "출금액 : " << client.data.clientBalance << "원" << endl;	//입금할 액수 출력
								client.data.clientBalance = money - client.data.clientBalance;
								money = client.data.clientBalance;
								int sndSize = msgsnd(msq_client_id, &client, MSG_SIZE_CLIENT, 0);
								if(sndSize != 0) {
									perror("msgsnd() error!(출금 - 갱신정보 송신 실패) : ");
									kill(getpid(), SIGUSR1);
								}
							}
						}
						break;
					}
					default: {	//예상치 못한 오류
						cout << "에러 : 예상치 못한 클라이언트의 작업 요청." << endl;
						kill(getpid(), SIGUSR1);
					}
				}
			}
			cout.clear();	//ostream 초기화
		}
	}
	//자식 프로세스(관리자와 통신)
	if(pid == 0){	
		//메시지큐 생성
		msq_admin_id = msgget(msq_key, IPC_CREAT | 0666);
		if(msq_admin_id == -1) {
			perror("msgget() error!(관리자 메시지큐 생성 실패) : ");
			kill(getpid(), SIGUSR2);
		}
		cout << "adminserver is working.." << endl;	//추후 구현 예정
		//관리자는 DB파트가 구현이 되어야 기능을 추가할 수 있습니다.
		/*
		while(1){
			memset(&admin, 0x00, sizeof(MsgAdmin));

			int adminMsgSize = msgrcv(msq_id, &admin, MSG_SIZE_ADMIN, MSG_TYPE_ADMIN, 0);
			if(adminMsgSize != -1) {//메시지 수신에 성공하면
				
				//관리자프로그램에서 날아온 작업 코드(admin.cmd)에 따라 해당 동작 수행
				switch(admin.cmd){
				case 1:		//관리자 회원가입
				case 2:		//관리자 로그인
				case 3:		//관리자 고객정보 조회
				case 4:		//관리자 고객정보 수정요청
				default:	//예상치 못한 오류
				}
			}
		}
		cout.clear();*/
	}

	return 0;
}

void signalHandler(int signum) {
	switch(signum){
		case SIGINT: {//인터럽트 시그널 수신 시 모든 메시지큐 종료 후 프로그램 종료.
			msgctl(msq_client_id, IPC_RMID, NULL);
			msgctl(msq_admin_id, IPC_RMID, NULL);
			exit(0);
		}
		case SIGUSR1: {//클라이언트에 문제 발생 시 메시지큐 제거했다가 다시 생성
			msgctl(msq_client_id, IPC_RMID, NULL);
			msq_client_id = msgget(msq_key, IPC_CREAT | 0666);	
			if(msq_client_id == -1) {
				perror("msgget() error!(클라이언트 메시지큐 복구 실패) : ");
				kill(getpid(), SIGUSR1);
			}
		}
		case SIGUSR2: {//관리자에 문제 발생 시 메시지큐 제거했다가 다시 생성
			msgctl(msq_admin_id, IPC_RMID, NULL);
			msq_admin_id = msgget(msq_key, IPC_CREAT | 0666);
			if(msq_admin_id == -1) {
				perror("msgget() error!(관리자 메시지큐 복구 실패) : ");
				kill(getpid(), SIGUSR2);
			}
		}
	}
}