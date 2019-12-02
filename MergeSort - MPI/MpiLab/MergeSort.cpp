#include <iostream>
#include <stdio.h>
#include <string>
#include <algorithm>

#include "mpi.h"

#include <math.h>
#include <vector>

using namespace std;

#pragma comment(lib, "msmpi.lib")

int get_tag(int source, int destination)
{
	int num_of_digits = 0;
	int temp = destination;

	while (temp)
	{
		num_of_digits++;
		temp /= 10;
	}

	return source * pow(10, num_of_digits + 1) + destination;
}

void convert_string_to_array(string str, vector<int>& v)
{
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(", ", prev);
		if (pos == string::npos)
			pos = str.length();
		string token = str.substr(prev, pos - prev);
		if (!token.empty())
			v.push_back(stoi(token));
		prev = pos + 2;
	} while (pos < str.length() && prev < str.length());
}

void merge(int* begin1, int* end1, int* begin2, int* end2, int* merged)
{
	int* curr1 = begin1;
	int* curr2 = begin2;
	while (curr1 < end1 || curr2 < end2)
	{
		if (curr2 >= end2 || (curr1 < end1 && *curr1 < *curr2))
		{
			*merged = *curr1;
			++curr1;
		}
		else
		{
			*merged = *curr2;
			++curr2;
		}
		++merged;
	}
}

void merge_sort_rec(int rank, vector<int>& buffer, int& max_rank_arg)
{
	vector<int> tmp;
	tmp.resize(buffer.size());
	copy(buffer.data(), buffer.data() + buffer.size(), tmp.data());

	printf("Tmp Buffer: [ ");
	for (int i = 0; i < tmp.size() - 1; i++)
		printf("%d ; ", tmp[i]);
	printf("%d ] ", tmp[tmp.size() - 1]);

	int half_size = tmp.size() / 2;

	int left_child = 2 * rank + 1;
	int right_child = 2 * rank + 2;

	printf("\nHalf size: %d , Left child rank: %d, Right child rank: %d", half_size, left_child, right_child);

	return;
	if (left_child <= max_rank_arg)
	{
		printf("Send half data to left child %d to process\n", left_child);
		int tag = get_tag(rank, left_child);
		MPI_Send(tmp.data(), half_size, MPI_INT, left_child, tag, MPI_COMM_WORLD);
		printf("Left child received the data\n");
	}

	if (right_child <= max_rank_arg)
	{
		printf("Send half data to right child %d to process\n", right_child);
		int tag = get_tag(rank, right_child);
		MPI_Send(tmp.data() + half_size, tmp.size() - half_size, MPI_INT, right_child, tag, MPI_COMM_WORLD);
		printf("Right child received the data\n");
	}

	MPI_Status status;
	if (left_child <= max_rank_arg)
	{
		printf("Waiting for left child %d to send processed data\n", left_child);
		int tag = get_tag(left_child, rank);
		MPI_Recv(tmp.data(), half_size, MPI_INT, left_child, tag, MPI_COMM_WORLD, &status);
		printf("Parent received data from left child\n");
	}

	if (right_child <= max_rank_arg)
	{
		printf("Waiting for right child %d to send processed data\n", right_child);
		int tag = get_tag(right_child, rank);
		MPI_Recv(tmp.data() + half_size, tmp.size() - half_size, MPI_INT, right_child, tag, MPI_COMM_WORLD, &status);
		printf("Parent received data from right child\n");
	}

	printf("Merge these two halfs processed by children and save it back in buffer\n");
	vector<int> result(buffer.size());
	merge(tmp.data(), tmp.data() + half_size, tmp.data() + half_size, tmp.data() + tmp.size(), result.data());
	copy(result.data(), result.data() + result.size(), buffer.data());
	printf("Finished merging\n");
}

void merge_sort_rec_worker(int rank, int& max_rank_arg, vector<int>& leaves)
{
	int parent = (rank - 1) / 2;
	int received_size;

	printf("\n----------------------------------\n");
	printf("Parent of this process: %d", parent);
	printf("\n----------------------------------\n");

	if (find(leaves.begin(), leaves.end(), rank) != leaves.end())
	{
		printf("This process has nothing to do!");
		return;
	}

	return;
	printf("Probe to catch a send from parent\n");

	MPI_Status stats;
	int tag = get_tag(parent, rank);

	MPI_Probe(parent, tag, MPI_COMM_WORLD, &stats);
	MPI_Get_count(&stats, MPI_INT, &received_size);

	vector<int> tmp;
	tmp.resize(received_size);

	MPI_Status status;

	printf("Waiting for parent %d to send data\n", parent);
	MPI_Recv(tmp.data(), received_size, MPI_INT, parent, tag, MPI_COMM_WORLD, &status);
	printf("Child %d received data from parent\n", rank);

	if (received_size == 1)
	{
		printf("Send processed data to parent %d\n", parent);
		int tag = get_tag(rank, parent);
		MPI_Send(tmp.data(), received_size, MPI_INT, parent, tag, MPI_COMM_WORLD);
		printf("Waiting for parent %d to receive data\n", parent);
	}
	else if (received_size == 2)
	{
		if (tmp[0] > tmp[1])
			swap(tmp[0], tmp[1]);

		printf("Send processed data to parent %d\n", parent);
		int tag = get_tag(rank, parent);
		MPI_Send(tmp.data(), received_size, MPI_INT, parent, tag, MPI_COMM_WORLD);
		printf("Waiting for parent %d to receive data\n", parent);
	}
	else
	{
		merge_sort_rec(rank, tmp, max_rank_arg);
		printf("Send processed data to parent %d\n", parent);
		int tag = get_tag(rank, parent);
		MPI_Send(tmp.data(), tmp.size(), MPI_INT, parent, tag, MPI_COMM_WORLD);
		printf("Waiting for parent %d to receive data\n", parent);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		printf("\nInsufficient number of arguments!\n");
		return -1;
	}

	int numprocs, rank, rc;
	rc = MPI_Init(&argc, &argv);
	if (rc != MPI_SUCCESS)
	{
		printf("Error starting MPI program. Terminating.\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}

	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	printf("##################################\n");
	printf("		I'm rank %d", rank);

	int ign;
	vector<int> buffer;
	int nr_words_arg;

	int max_rank_arg;
	vector<int> leaves;

	if (rank == 0)
	{
		ign = sscanf(argv[2], "%d", &nr_words_arg);

		buffer.resize(nr_words_arg);
		for (int i = 0; i < buffer.size(); i++)
			buffer[i] = rand() % 123456;

		printf("\n----------------------------------\n");
		printf("Buffer: [ ");
		for (int i = 0; i < buffer.size() - 1; i++)
			printf("%d ; ", buffer[i]);
		printf("%d ] ", buffer[buffer.size() - 1]);

		ign = sscanf(argv[1], "%d", &max_rank_arg);
		convert_string_to_array(string(argv[3]), leaves);

		printf("\n----------------------------------\n");
		printf("Program arguments:\n");
		printf(" - Number of words: %d\n", nr_words_arg);
		printf(" - Number of processes: %d\n", numprocs);
		printf(" - Leaves: [ ");
		for (int i = 0; i < leaves.size() - 1; i++)
			printf("%d ; ", leaves[i]);
		printf("%d ] ", leaves[leaves.size() - 1]);
		printf("\n----------------------------------\n");
	}
	else
	{
		ign = sscanf(argv[1], "%d", &max_rank_arg);
		convert_string_to_array(string(argv[3]), leaves);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0)
	{
		merge_sort_rec(rank, buffer, max_rank_arg);
		printf("\n##################################\n\n");
	}
	else
	{
		merge_sort_rec_worker(rank, max_rank_arg, leaves);
		printf("\n##################################\n\n");
	}

	MPI_Finalize();
}
