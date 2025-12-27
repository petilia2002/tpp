#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <sys/neutrino.h>
#include <unistd.h>

using namespace std;

/*
 * Модуль M1 (процесс P1):
 *  - создаёт канал;
 *  - запускает процессы P2, P3 (из M2) и P4 (из M3);
 *  - принимает от P2 и P3 сообщения INFO;
 *  - обрабатывает запрос REQUEST от P4, возвращает сведения о P2 и P3;
 *  - принимает уведомления о том, что P2 и P3 получили сообщения от P4.
 */

int main(int argc, char *argv[])
{
	// Получение id текущего процесса
	int pid1 = getpid();
	cout << "P1: started, pid = " << pid1 << endl;

	// Создание канала для приёма сообщений
	// Флаг _NTO_CHF_SENDER_LEN используется для того, чтобы получить реальную длину посланного сообщения в _msg_info.srcmsglen
	int chid1 = ChannelCreate(_NTO_CHF_SENDER_LEN);
	if(chid1 == -1)
	{
		cout << "Error P1: ChannelCreate" << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "P1: " << "channel created, chid = " << chid1 << endl;
	}

	// Подготовка строк для передачи в spawn: pid и chid1
	char pid1Str[20], chid1Str[20];
	snprintf(pid1Str, sizeof(pid1Str), "%d", pid1);
	snprintf(chid1Str, sizeof(chid1Str), "%d", chid1);

	// Пути к исполняемым файлам M2 и M3
	const char *pathM2 = "/home/host/M2/x86/o/M2";
	const char *pathM3 = "/home/host/M3/x86/o/M3";

	// Запуск P2 (M2), P3 (M2) и P4 (M3)
	// Для M2 передается: argv[0]=имя, argv[1]=role ("2" или "3"), argv[2]=parentPid, argv[3]=parentChid
	char role2[] = "2";
	char role3[] = "3";

	// Запуск процесса P2(M2)
	pid_t pid2 = spawnl(P_NOWAITO, pathM2, "M2", role2, pid1Str, chid1Str, NULL);
	if(pid2 == -1)
	{
		cout << "Error P1: spawn M2(P2)" << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "P1: " << "run P2 process, pid = " << pid2 << endl;
	}

	// Запуск процесса P3(M2)
	pid_t pid3 = spawnl(P_NOWAITO, pathM2, "M2", role3, pid1Str, chid1Str, NULL);
	if(pid3 == -1)
	{
		cout << "Error P1: spawn M2(P3)" << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "P1: " << "run P3 process, pid = " << pid3 << endl;
	}

	// Запуск процесса P4(M3)
	pid_t pid4 = spawnl(P_NOWAITO, pathM3, "M3", pid1Str, chid1Str, NULL);
	if(pid4 == -1)
	{
		cout << "Error P1: spawn M3(P4)" << endl;
		exit(EXIT_FAILURE);
	} else {
		cout << "P1: " << "run P4 process, pid = " << pid4 << endl;
	}

	// Здесь реализован приём сообщений INFO, REQUEST и уведомлений.
	int stored_pid2 = -1, stored_chid2 = -1;
	int stored_pid3 = -1, stored_chid3 = -1;

	// Далее процесс принимает сообщения: сначала ожидает получить INFO от P2 и P3, затем запрос от P4
	// После ответа P4 ожидает уведомления от P2 и P3 о приёме ими сообщений от P4.

	// 1) Принимает три стартовых сообщения: INFO<pid, chid> от P2, INFO<pid, chid> от P3, и REQUEST от P4
	int gotInfoCount = 0;
	while(gotInfoCount < 3)
	{
		// Создание буфера для приема сообщения
		char buf[256] = {};
		_msg_info info;
		// Прием сообщения
		int rcvid = MsgReceive(chid1, buf, sizeof(buf), &info);
		if(rcvid == -1)
		{
			cout << "Error P1: MsgReceive" << endl;
			ChannelDestroy(chid1);
			exit(EXIT_FAILURE);
		}

		string s = buf;
		cout << "P1: received from pid = " << info.pid << " : \"" << s << "\"" << endl;

		// Обработка сообщения:
		// INFO messages от P2/P3: "INFO <pid> <chid>"
		// REQUEST message от P4: "REQUEST"

		// Для информационных сообщений от P2 и P3
		if(s.find("INFO") == 0)
		{
			int pid_v = 0, chid_v = 0;
			// Достает из сообщения pid и chid
			if(sscanf(buf, "INFO %d %d", &pid_v, &chid_v) == 2)
			{
				if(pid_v == pid2)
				{
					stored_pid2 = pid_v;
					stored_chid2 = chid_v;
					cout << "P1: stored P2 info: pid=" << stored_pid2 << " chid=" << stored_chid2 << endl;
				}
				if(pid_v == pid3)
				{
					stored_pid3 = pid_v;
					stored_chid3 = chid_v;
					cout << "P1: stored P3 info: pid=" << stored_pid3 << " chid=" << stored_chid3 << endl;
				}

				// Отправка ответа клиенту
				const char *reply = "INFO ACK";
				int msgRep = MsgReply(rcvid, 0, reply, strlen(reply) + 1);
				if(msgRep == -1)
				{
					cout << "Error P1: MsgReply" << endl;
					ChannelDestroy(chid1);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				cout << "P1: bad INFO format" << endl;
			}
		}
		// Для запроса от P4
		else if(s.find("REQUEST") == 0)
		{
			// REQUEST от P4 -> P1 отправляет ответ с информацией о P2 и P3
			char reply[256] = {};
			snprintf(reply, sizeof(reply), "P2 %d %d P3 %d %d", stored_pid2, stored_chid2, stored_pid3, stored_chid3);
			MsgReply(rcvid, 0, reply, strlen(reply) + 1);
			cout << "P1: replied to P4 with P2/P3 info" << endl;
		}
		else
		{
			cout << "P1: bad message format" << endl;
			ChannelDestroy(chid1);
			exit(EXIT_FAILURE);
		}

		++gotInfoCount;
	}

	// 2) Теперь ожидаются два финальных уведомления от P2 и P3: "P2 received message from P4" и "P3 received message from P4"
	int gotNotifyCount = 0;
	while(gotNotifyCount < 2)
	{
		// Прием уведомления от P2 (P3)
		char buf[256] = {};
		_msg_info info;
		int rcvid = MsgReceive(chid1, buf, sizeof(buf), &info);
		if(rcvid == -1)
		{
			cout << "Error P1: MsgReceive" << endl;
			ChannelDestroy(chid1);
			exit(EXIT_FAILURE);
		}

		string s = buf;
		cout << "P1: received from pid = " << info.pid << " : \"" << s << "\"" << endl;

		// Отправка ответа
		const char *notifyReply = "P1 final ACK";
		int notifyRep = MsgReply(rcvid, 0, notifyReply, strlen(notifyReply) + 1);
		if(notifyRep == -1)
		{
			cout << "Error P1: MsgReply" << endl;
			ChannelDestroy(chid1);
			exit(EXIT_FAILURE);
		}

		++gotNotifyCount;
	}

	// Удаляем канал
	ChannelDestroy(chid1);

	cout << "P1: executed" << endl;
	return EXIT_SUCCESS;
}






