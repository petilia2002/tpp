#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <sys/neutrino.h>
#include <unistd.h>

using namespace std;

/*
 * Модуль M2:
 *  - запускается в двух экземплярах (роль P2 или P3);
 *  - создаёт свой канал;
 *  - отправляет INFO в P1;
 *  - принимает сообщения от P4, отвечает ему;
 *  - уведомляет P1 о получении сообщения.
 */

int main(int argc, char *argv[])
{
	// Если переданных аргументов меньше, чем ожидалось - выводится ошибка
	if(argc < 4)
	{
		cerr << "M2: usage: M2 <role:2|3> <parentPid> <parentChid>" << endl;
		exit(EXIT_FAILURE);
	}

	// Получение аргументов командной строки
	const char *role = argv[1];
	int parentPid = atoi(argv[2]);
	int parentChid = atoi(argv[3]);

	// Получение pid текущего процесса
	int myPid = getpid();
	cout << "P" << role << ": started, pid = " << myPid << endl;

	// Создание канала для приёма сообщений от P4
	int myChid = ChannelCreate(_NTO_CHF_SENDER_LEN);
	if(myChid == -1)
	{
		cout << "Error P" << role << ": ChannelCreate" << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "P" << role << ": channel created, chid = " << myChid << endl;
	}

	// Подключение к каналу P1 и отправка информации (INFO)
	int coidToP1 = ConnectAttach(0, parentPid, parentChid, _NTO_SIDE_CHANNEL, 0);
	if(coidToP1 == -1)
	{
		cout << "Error P" << role << ": ConnectAttach to P1" << endl;
		ChannelDestroy(myChid);
		exit(EXIT_FAILURE);
	} else {
		cout << "P" << role << ": connected to P1 channel" << endl;
    }

	// Формирование сообщения INFO: "INFO <pid> <chid>"
	char infoMsg[128] = {};
	snprintf(infoMsg, sizeof(infoMsg), "INFO %d %d", myPid, myChid);

	// Отправка сообщения с pid и chid
	char reply[128] = {};
	int msgSend = MsgSend(coidToP1, infoMsg, strlen(infoMsg) + 1, reply, sizeof(reply));
	if(msgSend == -1)
	{
		cout << "Error P" << role << ": MsgSend to P1" << endl;
		ConnectDetach(coidToP1);
		ChannelDestroy(myChid);
		exit(EXIT_FAILURE);
	}
	cout << "P" << role << ": sent INFO to P1, got reply: " << reply << endl;

	// Ожидание сообщений на своём канале (от P4)
	char buf[256] = {};
	_msg_info info;
	int rcvid = MsgReceive(myChid, buf, sizeof(buf), &info);
	if(rcvid == -1)
	{
		cout << "Error P" << role << ": MsgReceive from P4" << endl;
		ConnectDetach(coidToP1);
		ChannelDestroy(myChid);
		exit(EXIT_FAILURE);
	}
	string s = buf;
	cout << "P" << role << ": received on myChid from pid=" << info.pid << " : \"" << s << "\"" << endl;

	// Ответ отправителю (P4)
	char replyToSender[64];
	snprintf(replyToSender, sizeof(replyToSender), "P%s ACK", role);
	int msgReply = MsgReply(rcvid, 0, replyToSender, strlen(replyToSender) + 1);
	// Возможные ошибки выводятся на экран
	if(msgReply == -1)
	{
		cout << "Error P" << role << ": MsgReply to P4" << endl;
		ConnectDetach(coidToP1);
		ChannelDestroy(myChid);
		exit(EXIT_FAILURE);
	}

	// Формирование текста уведомления для P1: "P? received message from P4"
	char notify[128];
	snprintf(notify, sizeof(notify), "P%s received message from P4", role);

	// После этого посылает уведомление
	char notifyReply[128];
	msgSend = MsgSend(coidToP1, notify, strlen(notify) + 1, notifyReply, sizeof(notifyReply));
	if(msgSend == -1)
	{
		cout << "Error P" << role << ": MsgSend notify to P1" << endl;
		ConnectDetach(coidToP1);
		ChannelDestroy(myChid);
		exit(EXIT_FAILURE);
	}

	// После взаимодействия P2(или P3) завершается (по условию)
	cout << "P" << role << ": sent notify to P1 and got reply: " << notifyReply << endl;

	// Удаление канала и соединения
	ConnectDetach(coidToP1);
	ChannelDestroy(myChid);
	cout << "P" << role << ": executed" << endl;
	return EXIT_SUCCESS;
}








