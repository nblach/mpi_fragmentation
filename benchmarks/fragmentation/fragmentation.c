#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int frag_size;

// TODO: FRAGMENTATION_FRAG_SIZE is the number of elements per fragment 
//		 which is not necessarily the number of bytes per fragment
void read_env(){
	char* frag_size_s = getenv("FRAGMENTATION_FRAG_SIZE");
	if(!frag_size_s){
		frag_size = 1024 * 8;
	}else{
		frag_size = atoi(frag_size_s);
	}
}

int MPI_Init(int *argc, char ***argv){
	read_env();
	return PMPI_Init(argc, argv);
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ){
	read_env();
	return PMPI_Init_thread(argc, argv, required, provided);
}

// Wrapper for Send that uses fragmentation
int MPI_Send(const void *buf_, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
	// Convert void buffer to char buffer
	char *buf = (char*) buf_;
	// Compute number of fragments needed to send that data
	int frag_count = count / frag_size;
	// Get the size in bytes of the datatype to be transmitted
	int datatype_size = 0;
	MPI_Type_size(datatype, &datatype_size)
	// Counts the remaining items to be sent 
	int remaining_count = count;
	// Array of MPI requests
	MPI_Request reqs[frag_count];
	// Stuff used inside the loop
	int r = MPI_SUCCESS;
	int sub_count = 0;
	char *sub_buf = 0;
	// Loop through the fragments
	for(int i = 0; i < frag_count; i++){
		// Compute the size of the current fragment (the last fragment can be longer than the fragment size)
		sub_count = (i == frag_count -1) ? remaining_count : frag_size;
		// Adjust the pointer to the start of the current fragment
		sub_buf = buf + (i * frag_size * datatype_size);	
		// Send current fragment
		r = PMPI_Isend(sub_buf, sub_count, datatype, dest, tag, comm, &reqs[i]);
		// Handle return value of send
		if(r != MPI_SUCCESS){
			return r;
		}
		// Update number of remaining items to be transmitted
		remaining_count -= frag_size;
	}
	// Wait until all fragments have been sent.
	return PMPI_Waitall(frag_count, reqs, MPI_STATUSES_IGNORE);
}


// Wrapper for Receive that uses fragmentation
int MPI_Recv(void *buf_, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){
	// Convert void buffer to char buffer
	char *buf = (char*) buf_;
	// Compute number of fragments needed to send that data
	int frag_count = count / frag_size;
	// Get the size in bytes of the datatype to be transmitted
	int datatype_size = 0;
	MPI_Type_size(datatype, &datatype_size)
	// Counts the remaining items to be sent 
	int remaining_count = count;
	// Array of MPI requests
	MPI_Request reqs[frag_count];
	// Stuff used inside the loop
	int r = MPI_SUCCESS;
	int sub_count = 0;
	char *sub_buf = 0;
	// Loop through the fragments
	for(int i = 0; i < frag_count; i++){
		// Compute the size of the current fragment (the last fragment can be longer than the fragment size)
		sub_count = (i == frag_count -1) ? remaining_count : frag_size;
		// Adjust the pointer to the start of the current fragment
		sub_buf = buf + (i * frag_size * datatype_size);	
		// Send current fragment
		r = PMPI_Irecv(sub_buf, sub_count, datatype, source, tag, comm, &reqs[i]);
		// Handle return value of send
		if(r != MPI_SUCCESS){
			return r;
		}
		// Update number of remaining items to be transmitted
		remaining_count -= frag_size;
	}
	// Wait until all fragments have been sent.
	return PMPI_Waitall(frag_count, reqs, MPI_STATUSES_IGNORE);
}

int MPI_Sendrecv(const void *sendbuf_, int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
					   void *recvbuf_, int recvcount, MPI_Datatype recvtype, int source, int recvtag, 
					   MPI_Comm comm, MPI_Status * status){
	// Convert void buffer to char buffer
	char *send_buf = (char*) sendbuf_;
	char *recv_buf = (char*) recvbuf_;
	// Compute number of fragments needed to send / receive the data
	int send_frag_count = sendcount / frag_size;
	int recv_frag_count = recvcount / frag_size;
	// Get the size in bytes of the datatype to be transmitted
	int send_datatype_size = 0;
	int recv_datatype_size = 0;
	MPI_Type_size(sendtype, &send_datatype_size)
	MPI_Type_size(recvtype, &recv_datatype_size)
	// Counts the remaining items to be sent / received 
	int send_remaining_count = sendcount;
	int recv_remaining_count = recvcount;
	// Array of MPI requests
	MPI_Request reqs[send_frag_count + recv_frag_count];
	// Stuff used inside the loop
	int r = MPI_SUCCESS;
	int sub_count = 0;
	char *sub_buf = 0;
	// More Stuff needed to alternate between send and receive
	int frags_sent = 0;
	int frags_received = 0;
	// Loop through the fragments
	for(int i = 0; i < (send_frag_count + recv_frag_count); i++){
		// Perform a send
		if(((i % 2 == 0) && (frags_sent != send_frag_count)) || frags_received == recv_frag_count){
			// Compute the size of the current fragment (the last fragment can be longer than the fragment size)
			sub_count = (frags_sent == (send_frag_count - 1)) ? send_remaining_count : frag_size;
			// Adjust the pointer to the start of the current fragment
			sub_buf = send_buf + (i * frag_size * send_datatype_size);	
			// Send a fragment
			r = PMPI_Isend(sub_buf, sub_count, sendtype, dest, sendtag, comm, &reqs[i]);
			// Update counters
			send_remaining_count -= frag_size;
			frags_sent++;
		}
		// Perform a receive
		else{
			// Compute the size of the current fragment (the last fragment can be longer than the fragment size)
			sub_count = (frags_received == (recv_frag_count - 1)) ? remaining_recv_count : frag_size;
			// Adjust the pointer to the start of the current fragment
			sub_buf = recv_buf + (i * frag_size * recv_datatype_size);	
			// Receive a fragment
			r = PMPI_Irecv(sub_buf, sub_count, recvtype, source, recvtag, comm, &reqs[i]);
			// Update counters
			recv_remaining_count -= frag_size;
			frags_received++;
		}
		// Handle return value (of either send or receive)
		if(r != MPI_SUCCESS){
			return r;
		}
	}
	// Wait until all fragments have been sent.
	return PMPI_Waitall(send_frag_count + recv_frag_count, reqs, MPI_STATUSES_IGNORE);
}
