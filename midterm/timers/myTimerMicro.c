
/*
time functions

time_t time(time_t *tloc);
Bu fonksiyon Epoch'tan beri geçen zamanı saniyeler
cinsinden return eder. Döndürülen time_t genelde 
long bir integer olur.
Kullanım:
	time_t curTime;
	curTime = time(NULL); 
	or
	time(&curTime);

double difftime(time_t endTime, time_t startTime);
Aradaki zaman farkını saniye cinsinden return eder.

char *ctime(const time_t *timer);
Verilen zamanı gün ay yıl ve saat saniye olarak verir.
!!!!!STATIC STORAGE!!!!!
Örnek: Sun Oct 06 02:21:35 1986\n\0



*/

/*Geçen zamanı mikrosaniye cinsinden hesaplayan program*/
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#define MILLION 1000000L

int main(){
	struct timeval start,end;
	long time1, time2;

	gettimeofday(&start,NULL);//Zamanı saniye + mikrosaniye cinsinden al
	time1  = MILLION * start.tv_sec; //Saniyeleri mikrosaniyeye ekle
	time1 += start.tv_usec; // Mikrosaniyeleri ekle, böylece toplam mikrosaniyeleri bul
	printf("Start time in microseconds:%ld\n",(long)time1);

	/*Sleep for 2 seconds*/
	sleep(1);

	gettimeofday(&end,NULL);//Zamanı saniye + mikrosaniye cinsinden al
	time2  = MILLION * end.tv_sec; //Saniyeleri mikrosaniyeye ekle
	time2 += end.tv_usec; // Mikrosaniyeleri ekle, böylece toplam mikrosaniyeleri bul
	printf("End time in microseconds:%ld\n",(long)time2);

	printf("Elapsed time (microseconds):%ld\n",(long)(time2 - time1));
	//printf("Elapsed time (seconds):%ld\n",(long)(time2 - time1)/MILLION);

	return 0;
}