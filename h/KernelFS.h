#pragma once

#include <mutex>
#include <iostream>
#include <vector>
using namespace std;
class Partition;
class File;
class Semaphore;
class KernelFS {
	Partition* part;
	struct f {
		char name[8];
		char ext[3];
		char mode;
		unsigned long index1;
		unsigned long clust_size;
		unsigned long start_pos;
		int state;
		long my_clust;
		File* file=nullptr;
		Semaphore* sem = nullptr;
		thread::id tid;
	};//predstavlja ulaz fajla u korenom direktorijumu
	char buffer[2048];//posrednik za particiju
	vector<unsigned long> file_clusters;//klasteri u kojima se nalaze fajlovi
	f** files;//ulazi fajlova u kesu-mogu biti otvoreni(1),zatvoreni(0)
	int cache_num;//kes kapacitet
	int* nums;//brojevi za zamenu u LRU
	int num_of_files_in;//trenutni broj fajlova u kesu
	char* vec;//vektor svih bitova ulaza
	long max_file_num;//maksimalan broj fajlova
	long num_of_clusters;//broj klastera u particiji
	mutex* op_mutex;//Obezbedjuje medjusobno iskljucenje
	Semaphore* mount_sem;//Blokada za montiranje
	Semaphore* format_sem;//Blokada za formatiranje
	Semaphore* unmount_sem;//Blokada za demontiranje
	int opened;//oznacava ima li otvorenih fajlova ili ne
	bool formatted;//oznacava je li formatirana particija vec ili ne 
	bool format_open;
	int file_num;//broj svih fajlova u particiji
	void write_bit_vec(char* v, char buffer[]);//pise bit-vektor u root klaster
	void read_bit_vec(char* v, char buffer[]);//dobija bit-vektor iz root klastera
	void write_file_str(f* piece, char buffer[]);//pise fajl u bafer
	void read_file_str(f* piece, char buffer[],int pos);//cita fajl iz bafera
	unsigned long read_long(char buffer[], int pos);//cita indeks iz bafera
	void write_long(char buffer[], unsigned long l, int pos);//pise indeks u bafer
	unsigned long free_cluster();//pronalazi slobodan klaster
	void mark_cluster(unsigned long cluster,bool mark);//oznacava klaster slobodnim ili zauzetim
	bool is_free_file_cluster(unsigned long cluster);//ima li klaster mesta za ulaz fajla(32B)
	bool is_free_index_cluster(unsigned long cluster);//ima li klaster mesta za indeks(4B)
	void write_index_in_cluster(unsigned long index, char buffer[]);//trazi slobodno mesto u klasteru i upisuje indeks
	bool write_in_cash(f* piece);//upisivanje u kesu
	bool search_name(f* piece, char* fname);//provera imena fajla
	friend class KernelFile;
public:
	KernelFS();
	~KernelFS();
	char mount(Partition* partition); //montira particiju
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha 
	char unmount(); //demontira particiju
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	char format(); //formatira particiju;
	// vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
	long readRootDir();
	// vraca -1 u slucaju neuspeha ili broj fajlova u slucaju uspeha
	char doesExist(char* fname); //argument je naziv fajla sa
	//apsolutnom putanjom
	File* open(char* fname, char mode);
	char deleteFile(char* fname);
};
