//메시지큐에 사용할 자료형 정의
//작업코드를 추후 열거형(enum)으로 작성해 가독성을 높일 예정


#ifndef __MYMSG_H__
#define __MYMSG_H__

#define INPUT_LEN 20

//메시지큐 사용을 위한 매크로
#define MSG_TYPE_CLIENT 1
#define MSG_TYPE_ADMIN 2
#define MSG_SIZE_CLIENT (sizeof(MsgClient) - sizeof(long))
#define MSG_SIZE_ADMIN (sizeof(MsgAdmin) - sizeof(long))

//고객 정보 양식
typedef struct __ClientInfo {	//나중에 STL 사용을 위해 클래스로 변경할 예정
	char clientId[20];				//고객 ID
	char clientPw[20];				//고객 PW
	char clientName[20];			//고객 이름
	char clientResRegNum[20];		//고객 주민번호
	char clientAccountNum[20];		//고객 계좌번호
	double clientBalance;			//고객 계좌잔액
} ClientInfo;

//관리자 정보 양식 
typedef struct __AdminInfo{
	char adminId[20];
	char adminPw[20];
} AdminInfo;

#endif