/*Geçen zamanı nanosaniye cinsinden hesaplayan program*/
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#define BILLION 1000000000L

int main(){
	struct timespec start,end;
	long time1, time2;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&start);//Zamanı saniye + nanosaniye cinsinden al
	printf("start time in nanoseconds:%ld\n",(long)time2);
	/*Sleep for 1 seconds*/
	sleep(1);

	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&end);//Zamanı saniye + nanosaniye cinsinden al
	time1  = BILLION * start.tv_sec; //Saniyeleri nanosaniyeye ekle
	time1 += start.tv_nsec; // Nanosaniyeleri ekle, böylece toplam nanosaniyeleri bul
	time2  = BILLION * end.tv_sec; //Saniyeleri nanosaniyeye ekle
	time2 += end.tv_nsec; // Mikrosaniyeleri ekle, böylece toplam mikrosaniyeleri bul
	printf("End   time in nanoseconds:%ld\n",(long)time2);

	printf("Elapsed time (nanoseconds):%ld\n",(long)(time2 - time1));
	printf("Elapsed time (seconds):%ld\n",(long)(time2 - time1)/BILLION);

	return 0;
}