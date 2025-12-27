#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <sys/neutrino.h>

using namespace std;

/*
 * Модуль M3 (процесс P4):
 *  - запрашивает у P1 сведения о P2 и P3;
 *  - отправляет сообщения в каналы P2 и P3;
 *  - получает ответы от них.
 */

int main(int argc, char *argv[])
{
	// Если переданных аргументов меньше, чем ожидалось - выводится ошибка
	if(argc < 3)
	{
		cerr << "M3: usage: M3 <parentPid> <parentChid>" << endl;
		exit(EXIT_FAILURE);
	}

	// Получение аргументов командной строки
	int parentPid = atoi(argv[1]);
	int parentChid = atoi(argv[2]);

	// Получение pid текущего процесса
	int myPid = getpid();
	cout << "P4" << ": started, pid = " << myPid << endl;

	// Подключение к P1 и запрос на получение информации о P2 и P3
	int coidToP1 = ConnectAttach(0, parentPid, parentChid, _NTO_SIDE_CHANNEL, 0);
    if (coidToP1 == -1) {
        cout << "Error P4: ConnectAttach to P1" << endl;
        exit(EXIT_FAILURE);
    } else {
    	cout << "P4: connected to P1 channel" << endl;
    }

    // Отправка запроса REQUEST
	const char *req = "REQUEST";
	char reply[256] = {};
	int msgSend = MsgSend(coidToP1, req, strlen(req) + 1, reply, sizeof(reply));
	if(msgSend == -1)
	{
		cout << "Error P4: MsgSend to P1" << endl;
		ConnectDetach(coidToP1);
		exit(EXIT_FAILURE);
	}
	cout << "P4: got from P1: \"" << reply << "\"" << endl;

	// Разбор ответа и взаимодействие с P2 и P3
	// Парсим ответ P1: "P2 <pid2> <chid2> P3 <pid3> <chid3>"
	int p2_pid = 0, p2_chid = 0, p3_pid = 0, p3_chid = 0;
	if(sscanf(reply, "P2 %d %d P3 %d %d", &p2_pid, &p2_chid, &p3_pid, &p3_chid) != 4)
	{
		cout << "P4: bad format from P1" << endl;
		ConnectDetach(coidToP1);
		exit(EXIT_FAILURE);
	}

	cout << "P4: parsed P2(pid=" << p2_pid << ", chid=" << p2_chid << "), "
	              << "P3(pid=" << p3_pid << ", chid=" << p3_chid << ")" << endl;

	// Подключение к каналу P2
	int coidToP2 = ConnectAttach(0, p2_pid, p2_chid, _NTO_SIDE_CHANNEL, 0);
	// В случае неудачи выводится ошибка
    if (coidToP2 == -1) {
        cout << "Error P4: ConnectAttach to P2" << endl;
		ConnectDetach(coidToP1);
        exit(EXIT_FAILURE);
    } else {
    	cout << "P4: connected to P2 channel" << endl;
    }

    // Подготовка сообщения для P2
	const char *msgToP2 = "Р4 send message to Р2";
	char replyFromP2[256] = {};

	// Отправка сообщения
	msgSend = MsgSend(coidToP2, msgToP2, strlen(msgToP2) + 1, replyFromP2, sizeof(replyFromP2));
	// В случае ошибки закрывается соединение и процесс завершает свою работу
	if(msgSend == -1)
	{
		cout << "Error P4: MsgSend to P2" << endl;
		ConnectDetach(coidToP1);
		ConnectDetach(coidToP2);
		exit(EXIT_FAILURE);
	}
	cout << "P4: got reply from P2: \"" << replyFromP2 << "\"" << endl;

	// Подключение к каналу P3
	int coidToP3 = ConnectAttach(0, p3_pid, p3_chid, _NTO_SIDE_CHANNEL, 0);
	// В случае неудачи выводится ошибка
    if (coidToP3 == -1) {
    	cout << "Error P4: ConnectAttach to P3" << endl;
		ConnectDetach(coidToP1);
		ConnectDetach(coidToP2);
        exit(EXIT_FAILURE);
    } else {
    	cout << "P4: connected to P3 channel" << endl;
    }

    // Подготовка сообщения для P3
	const char *msgToP3 = "Р4 send message to Р3";
	char replyFromP3[256] = {};

	// Отправка сообщения
	msgSend = MsgSend(coidToP3, msgToP3, strlen(msgToP3) + 1, replyFromP3, sizeof(replyFromP3));
	// В случае ошибки закрывается соединение и процесс завершает свою работу
	if(msgSend == -1)
	{
		cout << "Error P4: MsgSend to P3" << endl;
		ConnectDetach(coidToP1);
		ConnectDetach(coidToP2);
		ConnectDetach(coidToP3);
		exit(EXIT_FAILURE);
	}
	cout << "P4: got reply from P3: \"" << replyFromP3 << "\"" << endl;

	// Закрытие соединений
	ConnectDetach(coidToP1);
	ConnectDetach(coidToP2);
	ConnectDetach(coidToP3);

	cout << "P4" << ": executed" << endl;
	return EXIT_SUCCESS;
}






