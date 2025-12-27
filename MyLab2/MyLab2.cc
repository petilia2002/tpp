#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

using namespace std;

/* Определение объектов управления синхронизацией*/
pthread_barrier_t barrier; // барьер
pthread_mutex_t condMutex; // мутекс
pthread_cond_t condvar; // условная переменная
sem_t sem; // неименнованый семафор

/* Определение функций для создания нитей Т1 и Т2 */
void* f1(void* args); // функция для нити Т1
void* f2(void* args); // функция для нити Т2

/* Функция добавления текста по буквам */
void add_text_by_letters(char* buffer, const char* text);

// Разделяемые нитями буфер формирования текста и флаг готовности текста для T2
char result_text[200];
bool flag_f2 = false; // флаг готовности буфера текста для нити Т2

/***************************************< MAIN >********************************************/
int main(int argc, char *argv[]) {
  cout << "Старт нити MAIN" << endl;

  /* Создание объектов управления синхронизацией */
  pthread_barrier_init(&barrier, NULL, 3); // создание барьера со значением счетчика равным 3
  pthread_mutex_init(&condMutex, NULL); // инициализация мутекса по умолчанию
  pthread_cond_init(&condvar, NULL); //  инициализация условной переменной по умолчанию
  sem_init(&sem, 0, 0); // неименованный локальный семафор со значением счетчика равным 0

  // Запускаем нить T1(F1)
  pthread_t thread1;
  int result = pthread_create(&thread1, NULL, &f1, result_text); // создание и запуск нити Т1 с атрибутами по умолчанию
  if(result != 0) {
    cout << "Ошибка при создании нити T1(F1)" << endl;
    return EXIT_FAILURE;
  }
  sleep(2);

  // Инициализация буфера (начинаем с пустой строки)
  result_text[0] = '\0';

  // Нить main формирует свою часть текста "Text0, " по буквам
  const char* main_text = "Text0, ";
  add_text_by_letters(result_text, main_text);
  sem_post(&sem);

  /* Ждет, когда текст окончательно сформируется.. */
  cout << "Main: добавила текст и ждет у барьера" << endl;
  pthread_barrier_wait(&barrier); // нить main ждет у барьера

  cout << "Сформированный текст: \"" << result_text << "\"" << endl;
  cout << "MAIN: завершение работы" << endl;

  // Освобождаем ресурсы
  pthread_barrier_destroy(&barrier);
  sem_destroy(&sem);
  pthread_mutex_destroy(&condMutex);
  pthread_cond_destroy(&condvar);

  return EXIT_SUCCESS;
}

/***************************************< F1 >**********************************************/
void* f1(void* args) {
	cout << "Старт нити T1(F1)" << endl;

	pthread_t thread2;
	int result = pthread_create(&thread2, NULL, &f2, result_text);
	if(result != 0) {
	    cout << "Ошибка при создании нити T2(F2)" << endl;
	    pthread_exit((void*)EXIT_FAILURE);
	}
	sleep(1);

	/* Ждет, когда Main закончит писать в буффер.. */
	cout << "T1: ждёт строку текста от нити Main" << endl;
	sem_wait(&sem);
	cout << "T1: готова записать в буфер свою строку текста" << endl;

	// Нить T1 формирует свою часть текста "Text1, " по буквам
	const char* t1_text = "Text1, ";
	add_text_by_letters(result_text, t1_text);

	// Устанавливаем flag_f2 и сигнализируем об этом ожидающей нити Т2
	pthread_mutex_lock(&condMutex);
	flag_f2 = true;
	pthread_cond_signal(&condvar);
	pthread_mutex_unlock(&condMutex);

	/* Т1 ожидает завершения выполнения нити Т2.. */
	cout << "T1: добавила текст и ждет у барьера" << endl;
	pthread_barrier_wait(&barrier); // нить T1 ждет у барьера

	cout << "T1: завершение работы" << endl;
	return EXIT_SUCCESS;
}

/***************************************< F2 >**********************************************/
void* f2(void* args) {
	cout << "Старт нити T2(F2)" << endl;

	/* Ждет, когда T1 закончит писать в буффер.. */
	pthread_mutex_lock(&condMutex);
	cout << "T2: ждёт строку текста от нити T1" << endl;
	while(!flag_f2) {
	  cout << "T2: флаг flag_f2 не готов, жду сигнал уведомления" << endl;
	  pthread_cond_wait(&condvar, &condMutex);
	  cout << "T2: сигнал пришёл" << std::endl;
	}
	cout << "T2: готова записать в буфер свою строку текста" << endl;
	pthread_mutex_unlock(&condMutex);

	// Нить T2 формирует свою часть текста "Text2.\n" по буквам
	const char* t2_text = "Text2.";
	add_text_by_letters(result_text, t2_text);

	cout << "T2: добавила текст и ждет у барьера" << endl;
	pthread_barrier_wait(&barrier); // нить T2 ждет у барьера

	cout << "T2: завершение работы" << endl;
	return EXIT_SUCCESS;
}

/***************************************< ФУНКЦИЯ ДОБАВЛЕНИЯ ТЕКСТА ПО БУКВАМ >************/
/**
 * Функция добавляет текст в буфер по одной букве за раз
 * @param buffer - указатель на буфер для записи
 * @param text   - текст для добавления
 */
void add_text_by_letters(char* buffer, const char* text) {
	int current_length = strlen(buffer);
	int text_length = strlen(text);

	for(int i = 0; i < text_length; ++i) {
		buffer[current_length + i] = text[i];
		buffer[current_length + i + 1] = '\0';

		for(int j = 0; j < 1000; ++j) {}

        // Дополнительная задержка 300ms (300000 микросекунд)
        usleep(300000);
	}
}

