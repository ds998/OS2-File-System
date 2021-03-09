#include "part.h"
#include "KernelFS.h"
#include "Semaphore.h"
#include "file.h"
#include <string>

bool KernelFS::search_name(f* piece, char* fname) {
	string name = fname;
	string file = "/";
	for (int i = 0; i < 8; i++) {
		if (piece->name[i] != 0x00)file += piece->name[i];
	}
	file += ".";
	for (int i = 0; i < 3; i++) {
		if (piece->ext[i] != 0x00)file += piece->ext[i];
	}
	return name == file;
}//provereno
bool KernelFS::write_in_cash(f* piece) {
	if (num_of_files_in < cache_num) {
		files[num_of_files_in] = piece;
		nums[num_of_files_in] = num_of_files_in;
		num_of_files_in++;
		return true;
	}
	else {
		if (opened < cache_num) {
			int ind = -1;
			for (int j = 0; j < cache_num; j++) {
				if (nums[j] == 0) {
					ind = j;
					break;
				}
			}
			if (files[ind]->state == 0) {
				part->readCluster(files[ind]->my_clust, buffer);
				write_file_str(files[ind], buffer);
				part->writeCluster(files[ind]->my_clust, buffer);
				delete files[ind];
				files[ind] = nullptr;
				files[ind] = piece;
				for (int j = 0; j < cache_num; j++) {
					nums[j]--;
				}
				nums[ind] = cache_num-1;
				return true;
			}
		}
	}
	return false;
}
//provereno
void KernelFS::write_index_in_cluster(unsigned long index, char buffer[]) {
	int ind = -1;
	for (int i = 0; i < 2048; i += 4) {
		int xef = 1;
		for (int j = i; j < i + 4; j++) {
			if (buffer[j] != 0x00) {
				xef = 0;
				break;
			}
		}
		if (xef) {
			ind = i;
			break;
		}
	}
	write_long(buffer, index, ind);
}//provereno
bool KernelFS::is_free_file_cluster(unsigned long cluster) {
	part->readCluster(cluster, buffer);
	for (int i = 0; i < 2048; i += 32) {
		int xef = 1;
		for (int j = i; j <i+ 32; j++) {
			if (buffer[j] != 0x00) {
				xef = 0;
				break;
			}
		}
		if (xef != 0) return true;
	}
	return false;
}//provereno
bool KernelFS::is_free_index_cluster(unsigned long cluster) {
	part->readCluster(cluster, buffer);
	for (int i = 0; i < 2048; i += 4) {
		int xef = 1;
		for (int j = i; j < i +4; j++) {
			if (buffer[j] != 0x00) {
				xef = 0;
				break;
			}
		}
		if (xef != 0) return true;
	}
	return false;
}//provereno
unsigned long KernelFS::free_cluster() {
	unsigned long sz = num_of_clusters / 8 + ((num_of_clusters % 8) ? 1 : 0);
	int index = -1;
	for (long i = 0; i < sz; i++) {
		if (vec[i] != 0x00) {
			index = i;
			break;
		}
	}
	if (index == -1)return 0;
	unsigned char f = 0x80;
	int x = 0;
	while (!(vec[index] & f)) {
		f >>= 1;
		x++;
	}
	return index * 8 + x;
}//provereno
void KernelFS::mark_cluster(unsigned long cluster,bool mark) {
	long index = cluster/8;
	unsigned char f = 0x80;
	int sef = cluster % 8;
	f >>= sef;
	if (mark) vec[index] &= ~f;
	else vec[index] |= f;
	
}//provereno
unsigned long KernelFS::read_long(char buffer[], int pos) {
	unsigned long f = 0x00;
	unsigned char c = 0x00;
	for (int i = pos+3; i >= pos; i--) {
		c = buffer[i];
		f |= c;
		if (i != pos)f <<= 8;
	}
	return f;
}//provereno
void KernelFS::write_long(char buffer[],unsigned long l, int pos) {
	unsigned long f = l;
	unsigned char c = 0x00;
	for (int i = pos; i < pos + 4; i++) {
		c = 0x00;
		c |= f;
		buffer[i] = c;
		f >>= 8;
	}
}//provereno
void KernelFS::write_bit_vec(char* vec, char buffer[]) {
	long sz = num_of_clusters / 8 + ((num_of_clusters % 8) ? 1 : 0);
	for (int i = 1; i <= sz; i++) {
		buffer[sz - i] = vec[i - 1];
	}
}//provereno
void KernelFS::read_bit_vec(char* vec, char buffer[]) {
	long sz = num_of_clusters / 8 + ((num_of_clusters % 8) ? 1 : 0);
	for (int i = 1; i <= sz; i++) {
		vec[i - 1] = buffer[sz - i];
	}
}//provereno
void KernelFS::write_file_str(f* piece, char buffer[]) {
	for (int i = piece->start_pos; i < piece->start_pos + 12; i++) {
		buffer[i] = 0x00;
	}
	write_long(buffer, piece->clust_size, piece->start_pos + 12);
	write_long(buffer, piece->index1, piece->start_pos + 16);
	buffer[piece->start_pos + 20] = 0x00;
	for (int i = piece->start_pos + 21; i < piece->start_pos + 24; i++) {
		buffer[i] = piece->ext[piece->start_pos + 24 - i - 1];
	}
	for (int i = piece->start_pos + 24; i < piece->start_pos + 32; i++) {
		buffer[i] = piece->name[piece->start_pos + 32 - i - 1];
	}
}//provereno
void KernelFS::read_file_str(f* piece, char buffer[], int pos) {
	piece->clust_size = read_long(buffer, pos + 12);
	piece->index1 = read_long(buffer, pos + 16);
	for (int i = pos + 21; i < pos + 24; i++) {
		piece->ext[pos + 24 - i - 1] = buffer[i];
	}
	for (int i = pos + 24; i < pos + 32; i++) {
		piece->name[pos + 32 - i - 1] = buffer[i];
	}
	piece->start_pos = pos;
}
//provereno
KernelFS::KernelFS() {
	part = nullptr;
	mount_sem = new Semaphore(0);
	unmount_sem = new Semaphore(0);
	format_sem = new Semaphore(0);
	vec = nullptr;
	files = nullptr;
	file_clusters.resize(0);
	opened = false;
	formatted = false;
	files = nullptr;
	nums = nullptr;
	op_mutex = new mutex();
	format_open = false;
}
//provereno

KernelFS::~KernelFS() {
	delete mount_sem;
	mount_sem = nullptr;
	delete unmount_sem;
	unmount_sem = nullptr;
	delete format_sem;
	format_sem = nullptr;
	delete op_mutex;
	op_mutex = nullptr;
}
//provereno
char KernelFS::mount(Partition* partition) {
	op_mutex->lock();
	if (partition == nullptr) {
		op_mutex->unlock();
		return 0;
	}
	if (part != nullptr) {
		op_mutex->unlock();
		mount_sem->wait();
		op_mutex->lock();
	}
	
	part = partition;
	num_of_clusters = part->getNumOfClusters();
	vec = new char[num_of_clusters / 8 + ((num_of_clusters % 8) ? 1 : 0)];
	cache_num = num_of_clusters / 10;
	part->readCluster(0,buffer);
	read_bit_vec(vec, buffer);
	file_num = 0;
	opened = 0;
	files = new f*[cache_num];
	for (int i = 0; i < cache_num; i++)files[i] = nullptr;
	nums = new int[cache_num];
	for (int i = 0; i < cache_num; i++)nums[i] = -1;
	if (!(vec[0] & 0x80) && !(vec[0] & 0x40)) {
		formatted = true;
		part->readCluster(1, buffer);
		for (int pos = 0; pos < 2048; pos += 4) {
			unsigned long l = read_long(buffer, pos);
			if (l) file_clusters.push_back(l);
		}
		for (int i = 0; i < file_clusters.size(); i++) {
			part->readCluster(file_clusters.at(i), buffer);
			
			for (int j = 0; j < 2048; j += 32) {
				f* piece = new f;
				read_file_str(piece, buffer, j);
				if (piece->clust_size>0) {
					file_num++;
					if (num_of_files_in < cache_num) {
						piece->sem = new Semaphore(0);
						piece->state = 0;
						piece->my_clust = file_clusters.at(i);
						files[num_of_files_in] = piece;
						nums[num_of_files_in] = num_of_files_in;
						num_of_files_in++;
					}
					else {
						delete piece;
						piece = nullptr;
					}
					
				}
				else {
					delete piece;
					piece = nullptr;
				}
			}
		}
		
	}
	op_mutex->unlock();
	return 1;
}
//provereno
char KernelFS::unmount() {
	op_mutex->lock();
	if (part == nullptr) {
		op_mutex->unlock();
		return 0;
	}
	if (opened) {
		op_mutex->unlock();
		unmount_sem->wait();
		op_mutex->lock();
	}
	if (formatted) {
		for (int i = 0; i < file_clusters.size(); i++) {
			part->readCluster(file_clusters.at(i), buffer);
			for (int j = 0; j < num_of_files_in; j++) {
				if (files[j]->my_clust == file_clusters.at(i)) {
					write_file_str(files[j], buffer);
				}
			}
			part->writeCluster(file_clusters.at(i), buffer);
		}
		for (int j = 0; j < num_of_files_in; j++) {
			delete files[j];
			files[j] = nullptr;
		}
		char empty_buffer[2048];
		for (int i = 0; i < 2048; i++)empty_buffer[i] = 0x00;
		for (int i = 0; i < file_clusters.size(); i++) {
			write_long(empty_buffer, file_clusters.at(i), i * 4);
		}
		part->writeCluster(1, empty_buffer);
		
		part->readCluster(0, buffer);
		write_bit_vec(vec, buffer);
		part->writeCluster(0, buffer);
		formatted = false;
	}
	part = nullptr;
	file_num = 0;
	num_of_clusters = 0;
	delete vec;
	vec = nullptr;
	delete files;
	files = nullptr;
	file_clusters.clear();
	if (mount_sem->val() < 0) mount_sem->notify();
	op_mutex->unlock();
	return 1;
}
//provereno
char KernelFS::format() {
	op_mutex->lock();
	if (part == nullptr) {
		op_mutex->unlock();
		return 0;
	}
	if (opened) {
		op_mutex->unlock();
		format_open = true;
		format_sem->wait();
		op_mutex->lock();
		if (part == nullptr)return 0;
	}
	file_clusters.resize(0);
	for (int i = 0; i < num_of_files_in; i++) {
		if (files[i] != nullptr) {
			delete files[i];
			files[i] = nullptr;
		}
	}
	num_of_files_in = 0;
	for (int i = 0; i < cache_num; i++)nums[i] = -1;
	vec[0] = 0x3f;
	for (int i = 1; i < num_of_clusters/8; i++) {
		vec[i] = 0xff;
	}
	if (num_of_clusters % 8) {
		char x = 0xff;
		x << (num_of_clusters % 8);
		vec[num_of_clusters / 8] = x;
	}
	for (int i = 0; i < 2048; i++) {
		buffer[i] = 0x00;
	}
	for (unsigned long i = num_of_clusters - 1; i > 0; i--) {
		part->writeCluster(i, buffer);
	}
	write_bit_vec(vec, buffer);
	part->writeCluster(0, buffer);
	file_num = 0;
	formatted = true;
	op_mutex->unlock();
	return 1;
}
//provereno
long KernelFS::readRootDir() {
	op_mutex->lock();
	if (!formatted) {
		op_mutex->unlock();
		return -1;
	}
	op_mutex->unlock();
	return file_num;

}
//provereno

char KernelFS::doesExist(char* fname) {
	op_mutex->lock();
	if (!formatted) {
		op_mutex->unlock();
		return -1;
	}
	for (int i = 0; i < num_of_files_in; i++) {
		if (search_name(files[i],fname)) {
			op_mutex->unlock();
			return 1;
		}
	}
	for (int i = 0; i <file_clusters.size(); i++) {
		part->readCluster(file_clusters.at(i), buffer);
		f* piece=new f;
		for (int j = 0; j < 2048; j+= 32) {
			read_file_str(piece, buffer, j);
			if (search_name(piece, fname)) {
				delete piece;
				piece = nullptr;
				op_mutex->unlock();
				return 1;
			}
		}
		delete piece;
		piece = nullptr;
	}
	op_mutex->unlock();
	return 0;
}
//provereno
File* KernelFS::open(char* fname, char mode) {
	char x = doesExist(fname);
	op_mutex->lock();
	if (opened == cache_num || format_open) {
		op_mutex->unlock();
		return nullptr;
	}
	if (x == 0 && (mode == 'r' || mode == 'a')) {
		op_mutex->unlock();
		return nullptr;
	}
	if (x == 1 && mode == 'w') {
		op_mutex->unlock();
		return nullptr;
	}
	if(mode=='r' || mode=='a'){
		int index = -1;
		bool created_piece = false;
		for (int i = 0; i < num_of_files_in; i++) {
			if (search_name(files[i], fname)) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			bool created_piece = true;
			bool found = false;
			for (int i = 0; i < file_clusters.size(); i++) {
				part->readCluster(file_clusters.at(i), buffer);
				f* piece=new f;
				for (int j = 0; j < 2048; j += 32) {
					read_file_str(piece, buffer, j);
					if (piece->clust_size != 0) {
						if (search_name(piece,fname)) {
							piece->sem = new Semaphore(0);
							write_in_cash(piece);
							found = true;
							break;
						}
					}
				}
				if (found) {
					piece = nullptr;
					break;
				}
				delete piece;
				piece = nullptr;
			}
			for (int i = 0; i < num_of_files_in; i++) {
				if (search_name(files[i],fname)) {
					index = i;
					break;
				}
			}
		}
		if (index == -1) {
			op_mutex->unlock();
			return nullptr;
		}
		if (files[index]->state == 1) {
			op_mutex->unlock();
			files[index]->sem->wait();
			op_mutex->lock();

		}
		if (!created_piece) {
			if (num_of_files_in == cache_num) {
				if (nums[index] != num_of_files_in - 1) {
					for (int i = 0; i < num_of_files_in; i++) {
						if (nums[i] > 0) nums[i]--;
					}
					nums[index] = num_of_files_in - 1;
				}
				
			}
			else {
				if (nums[index] != num_of_files_in - 1) {
					f* temp = files[num_of_files_in - 1];
					files[num_of_files_in - 1] =files[index];
					files[index] = temp;
					index = num_of_files_in - 1;
				}
			}
		}
		files[index]->mode = mode;
		files[index]->state = 1;
		files[index]->tid = this_thread::get_id();
		opened++;
		op_mutex->unlock();
		return files[index]->file = new File();

	}
	else {
		unsigned long file_cluster = 0;
		bool fc_unmarked = false;
		for (int i = 0; i < file_clusters.size(); i++) {
			if (is_free_file_cluster(file_clusters.at(i))) {
				file_cluster=file_clusters.at(i);
				break;
			}
		}
		if (file_cluster == 0) {
			if (!is_free_index_cluster(1)) {
				op_mutex->unlock();
				return nullptr;
			}
			file_cluster = free_cluster();
			if (file_cluster == 0) {
				op_mutex->unlock();
				return nullptr;
			}
			mark_cluster(file_cluster, true);
			file_clusters.push_back(file_cluster);
			fc_unmarked = true;
		}
		unsigned long findex = free_cluster();
		if (findex == 0) {
			if (fc_unmarked) {
				mark_cluster(file_cluster, false);
				file_clusters.pop_back();
			}
			op_mutex->unlock();
			return nullptr;
		}
		mark_cluster(findex, true);
		unsigned long first = free_cluster();
		if (first == 0) {
			mark_cluster(findex, false);
			if (fc_unmarked) {
				mark_cluster(file_cluster, false);
				file_clusters.pop_back();
			}
			op_mutex->unlock();
			return nullptr;
		}
		mark_cluster(first, true);
		unsigned long second = free_cluster();
		if (second == 0) {
			mark_cluster(first, false);
			mark_cluster(findex, false);
			if (fc_unmarked) {
				mark_cluster(file_cluster, false);
				file_clusters.pop_back();
			}
			op_mutex->unlock();
			return nullptr;
		}
		mark_cluster(second, true);
		f* piece=new f;
		piece->clust_size = 0x01;
		piece->index1 = findex;
		piece->sem = new Semaphore(0);
		for (int i = 0; i < 8; i++) {
			piece->name[i] = 0x00;
		}
		for (int i = 0; i < 3; i++) {
			piece->ext[i] = 0x00;
		}
		string s = string(fname);
		int stef = 1;
		int length = 0;
		string name = "";
		while (s[stef] != '.') {
			name += s[stef];
			stef++;
		}
		string ext = "";
		stef++;
		while (stef < (int)s.length()) {
			ext += s[stef];
			stef++;
		}
		for (int i = 8-(int)name.length(); i<8; i++) {
			piece->name[i] = name[i-8+(int)name.length()];
		}
		for (int i = 0; i < (int)ext.length(); i++) {
			piece->ext[i] = ext[i];
		}
		piece->mode = mode;
		piece->my_clust = file_cluster;
		part->readCluster(file_cluster, buffer);
		int pos = -1;
		for (int i = 0; i < 2048; i += 32) {
			int gef = 1;
			for (int j = i; j < i + 32; j++) {
				if (buffer[j] != 0x00) {
					gef = 0;
					break;
				}
			}
			if (gef) {
				pos = i;
				break;
			}
		}
		piece->start_pos = pos;
		piece->tid = this_thread::get_id();
		write_file_str(piece, buffer);
		part->writeCluster(file_cluster, buffer);
		piece->state = 1;
		part->readCluster(findex, buffer);
		write_long(buffer, first, 0);
		part->writeCluster(findex, buffer);
		part->readCluster(first, buffer);
		write_long(buffer, second, 0);
		part->writeCluster(first, buffer);
		write_in_cash(piece);
		opened++;
		file_num++;
		op_mutex->unlock();
		return piece->file=new File();


	}
	op_mutex->unlock();
	return nullptr;
		
	
}
//provereno
char KernelFS::deleteFile(char* fname) {
	if (!doesExist(fname)) return 0;
	op_mutex->lock();
	int index = -1;
	for (int i = 0; i < num_of_files_in; i++) {
		if (search_name(files[i],fname)) {
			index = i;
			break;
		}
	}
	f* piece=nullptr;
	if(index!=-1)piece = files[index];
	else {
		bool found = false;
		piece = new f;
		for (int i = 0; i < file_clusters.size(); i++) {
			part->readCluster(file_clusters.at(i), buffer);
			for (int j = 0; j < 2048; j += 32) {
				read_file_str(piece, buffer, j);
				if (piece->clust_size != 0) {
					if (search_name(piece,fname)) {
						break;
						found = true;
					}
				}
			}
			if (found)break;
		}
	}
	if (piece->state==1) {
		op_mutex->unlock();
		return 0;
	}
	delete piece->sem;
	piece->sem = nullptr;
	part->readCluster(piece->index1, buffer);
	char extra_buffer1[2048];
	char extra_buffer2[2048];
	char empty_buffer[2048];
	for (int i = 0; i < 2048; i++) empty_buffer[i] = 0x00;
	int xef = 0;
	while (xef<2048) {
		unsigned long tef = read_long(buffer, xef);
		if (tef != 0) {
			part->readCluster(tef, extra_buffer1);
			int mef = 0;
			while (mef < 2048) {
				unsigned long nef = read_long(extra_buffer1, mef);
				if (nef != 0) {
					part->readCluster(nef,extra_buffer2);
					part->writeCluster(nef, empty_buffer);
					mark_cluster(nef, false);
				}
				else break;
				mef += 4;
			}
			part->writeCluster(tef, empty_buffer);
			mark_cluster(tef, false);

		}
		else break;
		xef += 4;
	}
	part->writeCluster(piece->index1, empty_buffer);
	mark_cluster(piece->index1,false);
	part->readCluster(piece->my_clust, buffer);
	for (int i = piece->start_pos; i < piece->start_pos + 32; i++) {
		buffer[i] = 0x00;
	}
	int clust_empty = 1;
	for (int i = 0; i < 2048; i++) {
		if (buffer[i] != 0x00) {
			clust_empty = 0;
			break;
		}
	}
	part->writeCluster(piece->my_clust, buffer);
	if (clust_empty) {
		size_t j = 0;
		for (size_t i = 0; i < file_clusters.size(); ++i) {
			if (file_clusters[i] != piece->my_clust) file_clusters[j++] = file_clusters[i];
		}
		file_clusters.resize(j);
		mark_cluster(piece->my_clust, false);
	}
	if (index != -1) {
		for (int i = index; i < num_of_files_in-1; i++) {
			nums[i] = nums[i + 1];
			files[i] = files[i + 1];
		}
		files[num_of_files_in - 1] = nullptr;
		nums[num_of_files_in-1] = -1;
		num_of_files_in--;
		vector<f*> closed_files;
		vector<f*> opened_files;
		closed_files.resize(0);
		opened_files.resize(0);
		for (int i = 0; i < num_of_files_in; i++) {
			if (files[i]->state == 1) opened_files.push_back(files[i]);
			else closed_files.push_back(files[i]);
		}
		for (int i = 0; i < num_of_files_in; i++) nums[i] = i;
		int mof = 0;
		while (closed_files.size() > 0) {
			files[mof] = closed_files.back();
			closed_files.pop_back();
			mof++;
		}
		while (opened_files.size() > 0) {
			files[mof] = closed_files.back();
			opened_files.pop_back();
			mof++;
		}
	}
	file_num--;
	delete piece;
	piece = nullptr;
	op_mutex->unlock();
	return 1;
}
//provereno

//Jos nije testirano za vise korisnika