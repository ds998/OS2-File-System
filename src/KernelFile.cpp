#include "KernelFile.h"
#include "part.h"
#include "fs.h"
#include "Semaphore.h"
KernelFile::KernelFile() {
	FS::get_impl()->op_mutex->lock();
	part = FS::get_impl()->part;
	KernelFS::f** files = FS::get_impl()->files;
	int* n = FS::get_impl()->nums;
	int n_of_files = FS::get_impl()->num_of_files_in;
	int index = -1;
	int spec;
	for (int i = 0; i < n_of_files; i++) {
		if (n[i] == n_of_files-1) {
			index = i;
			break;
		}
	}
	file = files[index];
	sec_indexes.resize(0);
	file_blocks.resize(0);
	part->readCluster(file->index1, shift_buffer);
	for (int i = 0; i < 2048; i += 4) {
		BytesCnt fef = FS::get_impl()->read_long(shift_buffer, i);
		if (fef) {
			block* bl = new block();
			bl->block = fef;
			bl->pos = i;
			sec_indexes.push_back(bl);
		}
	}
	for (int i = 0; i < sec_indexes.size(); i++) {
		part->readCluster(sec_indexes.at(i)->block, shift_buffer);
		for (int j = 0; j < 2048; j += 4) {
			BytesCnt fef = FS::get_impl()->read_long(shift_buffer, j);
			if (fef) {
				block* bl = new block();
				bl->block = fef;
				bl->pos = j;
				bl->origin_block = sec_indexes.at(i)->block;
				file_blocks.push_back(bl);
			}
		}
	}
	char final_buffer[2048];
	part->readCluster(file_blocks.back()->block, final_buffer);
	BytesCnt pes = 0;
	for (int i = 2047; i >=0; i--) {
		if (final_buffer[i] != 0x00) {
			pes = i+1;
			break;
		}
	}
	end = ((file_blocks.size() - 1) * 2048)+pes;
	if (file->mode == 'a') {
		pos =  end;
	}
	unread = true;
	FS::get_impl()->op_mutex->unlock();
}
//provereno
KernelFile::~KernelFile() {
	FS::get_impl()->op_mutex->lock();
	int n_of_files = FS::get_impl()->num_of_files_in;
	int index = -1;
	for (int i = 0; i < n_of_files; i++) {
		if (FS::get_impl()->files[i]->index1 == file->index1) {
			index = i;
			break;
		}
	}
	FS::get_impl()->files[index]->clust_size = file->clust_size;
	FS::get_impl()->files[index]->state = 0;
	FS::get_impl()->opened--;
	sec_indexes.clear();
	file_blocks.clear();
	part = nullptr;
	bool file_sem = false;
	if (file->sem->val() < 0) {
		file_sem = true;
	}
	bool mounting_sem = false;
	if (FS::get_impl()->unmount_sem->val() < 0 && FS::get_impl()->opened == 0) {
		mounting_sem = true;
	}
	bool format_sem = false;
	if (FS::get_impl()->format_sem->val() < 0 && FS::get_impl()->opened == 0) {
		format_sem = true;
	}
	if (file_sem) file->sem->notify();
	else {
		if (format_sem) {
			FS::get_impl()->format_open = false;
			FS::get_impl()->format_sem->notify();
		}
		if (mounting_sem) FS::get_impl()->unmount_sem->notify();
	}
	file = nullptr;
	FS::get_impl()->op_mutex->unlock();
}
//provereno
char KernelFile::write(BytesCnt cnt, char* buffer) {
	if (file->tid != this_thread::get_id())return 0;
	if (part == nullptr || file->mode == 'r')return 0;
	if (cnt > (file_blocks.size()*2048 - pos)) {
		FS::get_impl()->op_mutex->lock();
		BytesCnt num_of_blocks = (cnt - (file_blocks.size()*2048 - pos))/2048+((cnt - (file_blocks.size()*2048 - pos)%2048?1:0));
		vector<block*> where_to_put_file_blocks;
		vector<BytesCnt> fi_pos;
		vector<BytesCnt> secs_to_mark;
		vector<BytesCnt> new_file_blocks;
		where_to_put_file_blocks.resize(0);
		int ix = 0;
		char buffer[2048];
		while (where_to_put_file_blocks.size() < num_of_blocks && ix < sec_indexes.size()) {
			part->readCluster(sec_indexes.at(ix)->block, buffer);
			int jx = 0;
			while(where_to_put_file_blocks.size() < num_of_blocks && jx<2048) {
				BytesCnt fef = FS::get_impl()->read_long(buffer, jx);
				if (!fef) {
					block* bl = new block();
					bl->block = sec_indexes.at(ix)->block;
					bl->pos = jx;
					where_to_put_file_blocks.push_back(bl);
				}
				jx += 4;
			}
			ix++;
		}
		if (where_to_put_file_blocks.size() < num_of_blocks) {
			BytesCnt remaining_num_of_blocks = num_of_blocks - where_to_put_file_blocks.size();
			BytesCnt num_of_sec = remaining_num_of_blocks / 512 + ((remaining_num_of_blocks % 512) ? 1 : 0);
			fi_pos.resize(0);
			int mx = 0;
			part->readCluster(file->index1, buffer);
			while (fi_pos.size() < num_of_sec && mx < 2048) {
				BytesCnt fef = FS::get_impl()->read_long(buffer, mx);
				if (!fef) {
					fi_pos.push_back(mx);
				}
				mx += 4;
			}
			if (fi_pos.size() < num_of_sec) {
				where_to_put_file_blocks.clear();
				fi_pos.clear();
				FS::get_impl()->op_mutex->unlock();
				return 0;
			}
			secs_to_mark.resize(0);
			for (BytesCnt i = 0; i < num_of_sec; i++) {
				BytesCnt free_cluster = FS::get_impl()->free_cluster();
				if (free_cluster == 0) {
					for (BytesCnt j = 0; j < secs_to_mark.size(); j++) {
						FS::get_impl()->mark_cluster(secs_to_mark.at(i), false);
						
					}
					secs_to_mark.clear();
					where_to_put_file_blocks.clear();
					fi_pos.clear();
					FS::get_impl()->op_mutex->unlock();
					return 0;

				}
				FS::get_impl()->mark_cluster(free_cluster, true);
				secs_to_mark.push_back(free_cluster);

			}
			BytesCnt fx = 0;
			while (where_to_put_file_blocks.size()<num_of_blocks) {
				int tx = 0;
				while (where_to_put_file_blocks.size() < num_of_blocks && tx < 2048) {
					block* bl = new block();
					bl->block = secs_to_mark.at(fx);
					bl->pos = tx;
					where_to_put_file_blocks.push_back(bl);
					tx += 4;

				}
				fx++;
			}


		}
		for (BytesCnt i = 0; i < num_of_blocks; i++) {
			BytesCnt free_cluster = FS::get_impl()->free_cluster();
			if (free_cluster == 0) {
				for (BytesCnt j = 0; j < new_file_blocks.size(); j++) {
					FS::get_impl()->mark_cluster(new_file_blocks.at(i), false);
					
				}
				new_file_blocks.clear();
				for (BytesCnt j = 0; j < secs_to_mark.size(); j++) {
					FS::get_impl()->mark_cluster(secs_to_mark.at(i), false);

				}
				secs_to_mark.clear();
				where_to_put_file_blocks.clear();
				fi_pos.clear();
				FS::get_impl()->op_mutex->unlock();
				return 0;

			}
			FS::get_impl()->mark_cluster(free_cluster, true);
			new_file_blocks.push_back(free_cluster);
		}
		if (fi_pos.size() > 0) {
			part->readCluster(file->index1, buffer);
		}
		for (int i = 0; i < fi_pos.size(); i++) {
			FS::get_impl()->write_long(buffer, secs_to_mark.at(i), fi_pos.at(i));
			block* bl = new block();
			bl->block = secs_to_mark.at(i);
			bl->pos = fi_pos.at(i);
			sec_indexes.push_back(bl);
		}
		if (fi_pos.size() > 0) {
			part->writeCluster(file->index1, buffer);
		}
		for (int i = 0; i < where_to_put_file_blocks.size(); i++) {
			part->readCluster(where_to_put_file_blocks.at(i)->block, buffer);
			FS::get_impl()->write_long(buffer, new_file_blocks.at(i), where_to_put_file_blocks.at(i)->pos);
			block* bl = new block();
			bl->block = new_file_blocks.at(i);
			bl->pos = where_to_put_file_blocks.at(i)->pos;
			bl->origin_block = where_to_put_file_blocks.at(i)->block;
			file_blocks.push_back(bl);
			part->writeCluster(where_to_put_file_blocks.at(i)->block, buffer);
		}
		where_to_put_file_blocks.clear();
		fi_pos.clear();
		secs_to_mark.clear();
		new_file_blocks.clear();
		file->clust_size += num_of_blocks;
		if (pos == (file_blocks.size() - num_of_blocks) * 2048) unread = true;
		FS::get_impl()->op_mutex->unlock();

	}
	if (unread) {
		part->readCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
		unread = false;
	}
	BytesCnt ret_cnt = 0;
	BytesCnt start_pos = pos;
	while (ret_cnt < cnt) {
		if (pos % 2048 == 0 && pos!=start_pos) {
			part->writeCluster(file_blocks.at((pos - 1) / 2048)->block, shift_buffer);
			part->readCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
		}
		shift_buffer[pos % 2048] = buffer[ret_cnt];
		ret_cnt++;
		pos++;
		if (pos > end)end = pos;
	}
	if (pos % 2048 == 0 && pos!=0) {
		part->writeCluster(file_blocks.at((pos - 1) / 2048)->block, shift_buffer);
	}
	else {
		part->writeCluster(file_blocks.at((pos) / 2048)->block, shift_buffer);
	}
	return 1;
}
//provereno
BytesCnt KernelFile::read(BytesCnt cnt, char* buffer) {
	if (file->tid != this_thread::get_id())return 0;
	if (part == nullptr)return 0;
	if (unread) {
		part->readCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
		unread = false;
	}
	BytesCnt ret_cnt = 0;
	while (!eof() && ret_cnt < cnt) {
		if (pos % 2048 == 0) {
			part->readCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
		}
		buffer[ret_cnt] = shift_buffer[pos%2048];
		ret_cnt++;
		pos++;
		
	}
	return ret_cnt;

}
//provereno
char KernelFile::seek(BytesCnt address) {
	if (file->tid != this_thread::get_id())return 0;
	if (part==nullptr || address<0 || address>getFileSize()) return 0;
	pos = address;
	unread = true;
	return 1;
}
//provereno
BytesCnt KernelFile::filePos() { return pos; }
//provereno
char KernelFile::eof() {
	if (file->tid != this_thread::get_id())return 1;
	if (part == nullptr) return 1;
	if (pos != end)return 0;
	return 2;
}
//provereno
BytesCnt KernelFile::getFileSize() {
	if (file->tid != this_thread::get_id())return 0;
	if (part == nullptr)return 0;
	else return end;
}
//provereno
char KernelFile::truncate() {
	if (file->tid != this_thread::get_id())return 0;
	if (eof() || file->mode=='r')return 0;
	if (unread) {
		part->readCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
		unread = false;
	}
	BytesCnt idx = pos / 2048;
	for (int i = pos % 2048; i < 2048; i++) {
		shift_buffer[i] = 0x00;
	}
	if(pos%2048)part->writeCluster(file_blocks.at(pos / 2048)->block, shift_buffer);
	char empty_buffer[2048];
	char buffer[2048];
	for (int i = 0; i < 2048; i++) empty_buffer[i] = 0x00;
	BytesCnt mes = (pos % 2048) ? idx+1 : idx;
	if (mes < file_blocks.size()) {
		FS::get_impl()->op_mutex->lock();
		for (BytesCnt i = mes; i < file_blocks.size(); i++) {
			part->writeCluster(file_blocks.at(i)->block, empty_buffer);
			part->readCluster(file_blocks.at(i)->origin_block, buffer);
			FS::get_impl()->write_long(buffer, 0, file_blocks.at(i)->pos);
			FS::get_impl()->mark_cluster(file_blocks.at(i)->block, false);
			part->writeCluster(file_blocks.at(i)->origin_block, buffer);
			file->clust_size--;
		}
		file_blocks.resize(mes);
		vector<block*> true_sec_indexes;
		true_sec_indexes.resize(0);
		part->readCluster(file->index1, empty_buffer);
		for (BytesCnt i = 0; i < sec_indexes.size(); i++) {
			part->readCluster(sec_indexes.at(i)->block, buffer);
			int xof = 1;
			for (int i = 0; i < 2048; i++) {
				if (buffer[i] != 0x00) {
					xof = 0;
					break;
				}
			}
			if (xof) {
				FS::get_impl()->mark_cluster(sec_indexes.at(i)->block, false);
				FS::get_impl()->write_long(empty_buffer, 0, sec_indexes.at(i)->block);
			}
			else {
				true_sec_indexes.push_back(sec_indexes.at(i));
			}
		}
		part->writeCluster(file->index1, empty_buffer);
		sec_indexes.resize(0);
		for (int i = 0; i < true_sec_indexes.size(); i++) {
			sec_indexes.push_back(true_sec_indexes.at(i));
		}
		true_sec_indexes.clear();
		FS::get_impl()->op_mutex->unlock();
	}
	end = pos;
	return 1;
	
}//provereno