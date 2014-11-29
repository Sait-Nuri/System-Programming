/*   Said Nuri UYANIK  */
/* 		101044067      */
/*    CSE 244 HW01     */

#include <stdio.h>      
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>  
#include <string.h>
#include <unistd.h> 	
#include <sys/types.h>  

int stringmatching(char*, int, char*);
int mystrncmp(char* line, char* word, int size);

int main( int argc, char* argv[] )
{
	FILE* file;						/* File pointer */
	pid_t new_pid;      			/* Fork için pid numarası */
	size_t len = 0;					
	ssize_t read;					/* Dosyadan okunacak olan satırın karakter sayısını tutar. */
	int num_of_line_key_in = 0;   	/* Anahtar kelimenin içerdiği toplam satır sayısı */
	int nline = 1;			    	/* Halihazırdaki satır numarası */
	int file_status = 0;	    	/* Dosyanın boş olup olmadı durumu */
	int same_key_number;	    	/* Bir satırda en az kaç anahtar kelimenin bulunduğu sayısı */
	int child_status;				/* Child process için çıkış durumu		 */
	char *filename = argv[1]; 		/* Dosya adı */
	char *buffer = NULL;			/* Dosyadan çekilen satır stringi */
	char *word = argv[2];			/* Aranacak olan anahtar kelime */


	/* fork burada başlıyor */
	if( (new_pid = fork()) < 0 ){
		perror("fork"); /* hata durumu mesaji */
		return 0;
	}

	/* parent process do nothing */
	if(new_pid > 0){			
		
		/*printf("Baba pid: %d\n", getpid()); */
		wait(&child_status);
		return 0;
	}
	else /* child process */
	{
		/*printf("velet pid: %d\n", getpid());		*/

		/* Eksik arguman girilirse uyarı ver, usage göster */
		if (argc  < 3 ) 
		{
			printf("Missing file operand\n");
			printf("Usage: %s [file] [word]\n", argv[0]);
			exit(-1);
		}

		/* Dosya okunamazsa ya da yoksa hata ver */
		if ( (file = fopen(filename, "r")) == NULL)
		{
			perror(filename);
			exit(-1);
		}

		printf("\n");
		/* Tüm dosyayı satır satır okur */
		while( (read = getline(&buffer, &len, file)) != EOF ){						

			/* eğer boş bir satır yoksa key'i ara */
			if(strlen(buffer)>0){
				file_status = 1;
				/* printf("%d %d\n", strlen(buffer), strlen());	*/
				/* Verilen anahtar kelimeyi satır içinde arar. Kaç tane varsa o sayıyı döndürür. */
				same_key_number = stringmatching(buffer, (int)read, word);

				num_of_line_key_in +=same_key_number;  /* anahtar kelimenin içerdiği toplam satır sayısı */
				
				/* Anahtar kelimenin bulunduğu satırları bas */				
				while(same_key_number--){
					printf("#%d :", nline);
					printf("%s", buffer);
				}			
			}
			nline++; /* Halihazırdaki satır sayısı */
		}
		printf("\nTotal number of line in which key found: %d\n", num_of_line_key_in);

		/* Eğer dosya boşsa uyarı ver */
		if(!file_status){
			printf("File is empty!\n");
			exit(-1);
		}

		fclose(file);
		exit(1);
	}	
}

/* 
	line 	  => Okunan satır
	line_size => Okunan satırın büyüklüğü
	key 	  => Aranacak kelime 
*/
int stringmatching(char* line, int line_size, char* key){
	
	int key_size = strlen(key);
	int amount_of_match = (line_size-key_size);
	int same_key_number = 0, result = 0;	;
	int i;

	for (i = 0; i < amount_of_match; ++i)
	{
		if( mystrncmp(&line[i], key, key_size) ){
			same_key_number++;
		}
	}
	return same_key_number;
}

/*
	str1   => Karşılaştırılacak 1. string
	str2   => Karşılaştırılacak 2. string
	lenght => Karşılaştırılma yapılacak karakter adedi (0 dan lenghte kadar)
*/
int mystrncmp(char* str1, char* str2, int lenght){
	
	int i;

	for (i = 0; i < lenght; ++i)
	{		
		if(str1[i] != str2[i])
			return 0;
	}

	return 1;
}