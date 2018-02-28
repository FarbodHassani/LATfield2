#ifndef LATFIELD2_PARALLEL2D_HPP
#define LATFIELD2_PARALLEL2D_HPP


#include "LATfield2_parallel2d_decl.hpp"

/////////// add verif for the 2*2 process and dim>2.



/*! \file LATfield2_parallel2d.hpp
 \brief LATfield2_parallel2d.hpp contains the class Parallel2d implementation.
 \author David Daverio, edited by Wessel Valkenburg
 */


Parallel2d::Parallel2d() : neverFinalizeMPI(false)
{


	int argc=1;
	char** argv = new char*[argc];
	for(int i=0; i<argc; i++) { argv[i]=new char[20]; }
	node_grid_size_[0] = -1;
	node_grid_size_[1] = -1;
	node_size_ = -1;
#ifndef EXTERNAL_IO
	lat_world_comm_ = MPI_COMM_WORLD;
    world_comm_ = MPI_COMM_WORLD;
	MPI_Init( &argc, &argv );
	MPI_Comm_rank( lat_world_comm_, &lat_world_rank_ );
	MPI_Comm_size( lat_world_comm_, &lat_world_size_ );
    MPI_Comm_rank( world_comm_, &world_rank_ );
	MPI_Comm_size( world_comm_, &world_size_ );
#else
    world_comm_ = MPI_COMM_WORLD;
	MPI_Init( &argc, &argv );
    MPI_Comm_rank( world_comm_, &world_rank_ );
	MPI_Comm_size( world_comm_, &world_size_ );

#endif

}
void Parallel2d::setNodeGeometry(int size0,int size1)
{
	node_size_ = size0*size1;
	node_grid_size_[0] = size0;
	node_grid_size_[1] = size1;
}

void Parallel2d::initialize(int proc_size0, int proc_size1)
{
    this->initialize(proc_size0, proc_size1,0,0);
}


void Parallel2d::initialize(int proc_size0, int proc_size1,int IO_total_size, int IO_node_size)
{
	if(lat_world_rank_==0)cout<<"init parallel"<<endl;
  grid_size_[0]=proc_size0;
	grid_size_[1]=proc_size1;

	dim0_comm_ = (MPI_Comm *)malloc(grid_size_[1]*sizeof(MPI_Comm));
	dim1_comm_ = (MPI_Comm *)malloc(grid_size_[0]*sizeof(MPI_Comm));

	dim0_group_ = (MPI_Group *)malloc(grid_size_[1]*sizeof(MPI_Group));
	dim1_group_ = (MPI_Group *)malloc(grid_size_[0]*sizeof(MPI_Group));

	int rang[3],i,j,comm_rank;



#ifdef EXTERNAL_IO

    if(world_rank_==0)
	{
		if(proc_size0*proc_size1+IO_total_size!=world_size_)
		{
			cerr<<"Latfield::Parallel2d::initialization - wrong number of process"<<endl;
			cerr<<"Latfield::Parallel2d::initialization - Number of total process must be equal to proc_size0*proc_size1+IO_total_size"<<endl;
			cerr<<"Latfield::Parallel2d::initialization - Within the call : Parallel2d(int proc_size0, int proc_size1, int IO_total_size)"<<endl;
			this->abortForce();
		}



	}

    MPI_Comm_group(world_comm_,&world_group_);

    rang[0]=0;
    rang[1]=proc_size0*proc_size1-1;
    rang[2]=1;

    MPI_Group_range_incl(world_group_,1,&rang,&lat_world_group_);
    MPI_Comm_create(world_comm_,lat_world_group_ , &lat_world_comm_);

    rang[0]=proc_size0*proc_size1;
    rang[1]=proc_size0*proc_size1 + IO_total_size - 1;
    rang[2]=1;

    MPI_Group_range_incl(world_group_,1,&rang,&IO_group_);
    MPI_Comm_create(world_comm_,IO_group_ , &IO_comm_);


    MPI_Group_rank(lat_world_group_, &comm_rank);
    if(comm_rank!=MPI_UNDEFINED)
    {
        lat_world_rank_=comm_rank;
        MPI_Comm_size( lat_world_comm_, &lat_world_size_ );

        rang[2]=1;
        for(j=0;j<grid_size_[1];j++)
        {
            rang[0] = j * grid_size_[0];
            rang[1] = grid_size_[0] - 1 + j*grid_size_[0];
            MPI_Group_range_incl(lat_world_group_,1,&rang,&dim0_group_[j]);
            MPI_Comm_create(lat_world_comm_, dim0_group_[j], &dim0_comm_[j]);
        }


        rang[2]=grid_size_[0];
        for(i=0;i<grid_size_[0];i++)
        {
            rang[0]=i;
            rang[1]=i+(grid_size_[1]-1)*grid_size_[0];
            MPI_Group_range_incl(lat_world_group_,1,&rang,&dim1_group_[i]);
            MPI_Comm_create(lat_world_comm_, dim1_group_[i], &dim1_comm_[i]);
        }


        for(i=0;i<grid_size_[0];i++)
        {
            MPI_Group_rank(dim1_group_[i], &comm_rank);
            if(comm_rank!=MPI_UNDEFINED)grid_rank_[1]=comm_rank;
        }

        for(j=0;j<grid_size_[1];j++)
        {
            MPI_Group_rank(dim0_group_[j], &comm_rank);
            if(comm_rank!=MPI_UNDEFINED)grid_rank_[0]=comm_rank;
        }


        root_=0;
        isIO_=false;
    }
    else
    {
        lat_world_rank_=-1;
        root_=0;
        grid_rank_[1]=-1;
        grid_rank_[0]=-1;
        isIO_=true;
    }

    if(grid_rank_[0]==grid_size_[0]-1)last_proc_[0]=true;
    else last_proc_[0]=false;
    if(grid_rank_[1]==grid_size_[1]-1)last_proc_[1]=true;
    else last_proc_[1]=false;




    ioserver.initialize(proc_size0,proc_size1,IO_total_size,IO_node_size);

#else

	if(lat_world_rank_==0)
	{
		if(proc_size0*proc_size1!=lat_world_size_)
		{
			cerr<<"Latfield::Parallel2d::initialization - wrong number of process"<<endl;
			cerr<<"Latfield::Parallel2d::initialization - Number of total process must be equal to proc_size0*proc_size1"<<endl;
			cerr<<"Latfield::Parallel2d::initialization - Within the call : Parallel2d(int proc_size0, int proc_size1)"<<endl;
			this->abortForce();
		}
	}


  MPI_Comm_group(lat_world_comm_,&lat_world_group_);


	if(node_size_ == -1)
	{
		rang[2]=1;
		for(j=0;j<grid_size_[1];j++)
		{
			rang[0] = j * grid_size_[0];
			rang[1] = grid_size_[0] - 1 + j*grid_size_[0];
			MPI_Group_range_incl(lat_world_group_,1,&rang,&dim0_group_[j]);
			MPI_Comm_create(lat_world_comm_, dim0_group_[j], &dim0_comm_[j]);
		}


		rang[2]=grid_size_[0];
		for(i=0;i<grid_size_[0];i++)
		{
			rang[0]=i;
			rang[1]=i+(grid_size_[1]-1)*grid_size_[0];
			MPI_Group_range_incl(lat_world_group_,1,&rang,&dim1_group_[i]);
			MPI_Comm_create(lat_world_comm_, dim1_group_[i], &dim1_comm_[i]);
		}
	}
	else
	{
		if(lat_world_rank_==0)cout<<"initializing parallel object with node geometry"<<endl;
		if(grid_size_[0]%node_grid_size_[0]!=0 ||grid_size_[1]%node_grid_size_[1]!=0 )
		{
			if(lat_world_rank_==0)
			{
				cerr << "node geometry not well suited... aborting"<<endl;
			}
			this->abortForce();
		}
		int n_nodes[2];
		n_nodes[0] = grid_size_[0] / node_grid_size_[0];
		n_nodes[1] = grid_size_[1] / node_grid_size_[1];

		int rangs0[n_nodes[0]][3];
		int rangs1[n_nodes[1]][3];

		int icomm;
		int first_in_comm;

		//setting dim0 comm
		for(int i = 0;i<n_nodes[1];i++)
		{
			for(int j = 0;j<node_grid_size_[1];j++)
			{
				icomm = j + i*node_grid_size_[1];
				first_in_comm  = j*node_grid_size_[0] + i*n_nodes[0]*node_size_;

				rangs0[0][0]=first_in_comm;
				rangs0[0][1]=first_in_comm + node_grid_size_[0]-1;
				rangs0[0][2]=1;
				for(int k = 1;k<n_nodes[0];k++)
				{
					rangs0[k][0]=rangs0[k-1][0]+node_size_;
					rangs0[k][1]=rangs0[k-1][1]+node_size_;
					rangs0[k][2]=1;
				}
				/*
				if(lat_world_rank_==0)
				{
					cout<<"comm0 :"<<icomm<< " rangs: ";
					for(int k = 0;k<n_nodes[0];k++)cout<<"("<<rangs0[k][0]<<","<<rangs0[k][1]<<","<<rangs0[k][2]<<")";
					cout<<endl;
				}
				*/
				MPI_Group_range_incl(lat_world_group_,n_nodes[0],rangs0,&dim0_group_[icomm]);
				MPI_Comm_create(lat_world_comm_, dim0_group_[icomm], &dim0_comm_[icomm]);
			}
		}

		for(int i = 0;i<n_nodes[0];i++)
		{
			for(int j = 0;j<node_grid_size_[0];j++)
			{
				icomm = j + i*node_grid_size_[0];
				first_in_comm  = j + i*node_size_;

				rangs1[0][0]=first_in_comm;
				rangs1[0][1]=first_in_comm + node_grid_size_[0]*(node_grid_size_[1]-1);
				rangs1[0][2]=node_grid_size_[0];
				for(int k = 1;k<n_nodes[1];k++)
				{
					rangs1[k][0]=rangs1[k-1][0]+node_size_*n_nodes[0];
					rangs1[k][1]=rangs1[k-1][1]+node_size_*n_nodes[0];
					rangs1[k][2]=node_grid_size_[0];
				}
				/*
				if(lat_world_rank_==0)
				{
					cout<<"comm1 :"<<icomm<< " rangs: ";
					for(int k = 0;k<n_nodes[1];k++)cout<<"("<<rangs1[k][0]<<","<<rangs1[k][1]<<","<<rangs1[k][2]<<")";
					cout<<endl;
				}
				*/
				MPI_Group_range_incl(lat_world_group_,n_nodes[1],rangs1,&dim1_group_[icomm]);
				MPI_Comm_create(lat_world_comm_, dim1_group_[icomm], &dim1_comm_[icomm]);
			}
		}

	}

	for(i=0;i<grid_size_[0];i++)
	{
		MPI_Group_rank(dim1_group_[i], &comm_rank);
		if(comm_rank!=MPI_UNDEFINED)grid_rank_[1]=comm_rank;
	}

	for(j=0;j<grid_size_[1];j++)
	{
		MPI_Group_rank(dim0_group_[j], &comm_rank);
		if(comm_rank!=MPI_UNDEFINED)grid_rank_[0]=comm_rank;
	}

	root_=0;

    if(grid_rank_[0]==grid_size_[0]-1)last_proc_[0]=true;
	else last_proc_[0]=false;
	if(grid_rank_[1]==grid_size_[1]-1)last_proc_[1]=true;
	else last_proc_[1]=false;



#endif

	if(root_ == lat_world_rank_)isRoot_=true;
    else isRoot_=false;


}


Parallel2d::~Parallel2d()
{
	int finalized;
  MPI_Finalized(&finalized);
  if((!finalized) && (!neverFinalizeMPI)) { MPI_Finalize(); }
}

//ABORT AND BARRIER===============================

void Parallel2d::abortForce()
{
	MPI_Abort( world_comm_, EXIT_FAILURE);
}

void Parallel2d::abortRequest()
{
	char failure;
	if(isRoot())
	{
		failure=char(1);
		broadcast(failure, root_);
		exit(EXIT_FAILURE);
	}
	else
	{
		cout<<"Parallel::abortRequest() called from non-Root process."<<endl;
		cout<<"Parallel::abortRequest() calling abortForce()..."<<endl;
		abortForce();
	}
}

void Parallel2d::barrier()
{

    MPI_Barrier( lat_world_comm_ );
}


template<class Type> void Parallel2d::sum(Type& number)
{
	sum( &number,1 );
}

template<class Type> void Parallel2d::sum(Type* array, int len)
{
	//Gather numbers at root
	Type* gather;
	if( rank() == root() ) gather = new Type[len*size()];
	MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
			   gather, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);

	//Sum on root
	if( isRoot() )
	{
		int i, j;
		for(i=0; i<size(); i++)
		{
			if( i!=root() ) for(j=0; j<len; j++)
			{
				array[j] = array[j] + gather[len*i+j];
			}
		}
	}

	//Broadcast result
	MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);
	// Tidy up (bug found by MDP 12/4/06)
	if(rank() == root() ) delete[] gather;
}

template<class Type> void Parallel2d::sum_dim0(Type& number)
{
	sum_dim0( &number,1 );
}

template<class Type> void Parallel2d::sum_dim0(Type* array, int len)
{
	int i,j,comm_rank;

	//Gather numbers at root
	Type* gather;
	if( grid_rank()[0] == 0 ) gather = new Type[len*grid_size_[0]];

    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,gather, len*sizeof(Type), MPI_BYTE, 0,dim0_comm_[grid_rank_[1]]);

	//Sum on root
	if( grid_rank()[0] == 0)
	{
		for(i=0; i<grid_size()[0]; i++)
		{
			if( i!=0 ) for(j=0; j<len; j++)
			{
				array[j] = array[j] + gather[len*i+j];
			}
		}
	}

	//Broadcast result

    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0, dim0_comm_[grid_rank_[1]]);

	// Tidy up (bug found by MDP 12/4/06)
	if( grid_rank()[0] == 0 ) delete[] gather;
}
template<class Type> void Parallel2d::sum_dim1(Type& number)
{
	sum_dim1( &number,1 );
}

template<class Type> void Parallel2d::sum_dim1(Type* array, int len)
{
	int i,j,comm_rank;

	//Gather numbers at root
	Type* gather;
	if( grid_rank_[1] == 0 ) gather = new Type[len*grid_size_[1]];


    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,gather, len*sizeof(Type), MPI_BYTE, 0,dim1_comm_[grid_rank_[0]]);

	//Sum on root
	if( grid_rank()[1] == 0)
	{
		for(i=0; i<grid_size()[1]; i++)
		{
			if( i!=0 ) for(j=0; j<len; j++)
			{
				array[j] = array[j] + gather[len*i+j];
			}
		}
	}


    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0, dim1_comm_[grid_rank_[0]]);

	if( grid_rank()[1] == 0 ) delete[] gather;
}

template<class Type> void Parallel2d::max(Type& number)
{
    max( &number,1 );
}

template<class Type> void Parallel2d::max(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( rank() == root() ) gather = new Type[len*size()];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);

    //Find max on root
    if( isRoot() )
    {
        int i, j;
        for(i=0; i<size(); i++)
        {
            if( i!=root() ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] > array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);
    // Tidy up (bug found by MDP 12/4/06)
    if( rank() == root() ) delete[] gather;
}

template<class Type> void Parallel2d::max_dim0(Type& number)
{
    max_dim0( &number,1 );
}
template<class Type> void Parallel2d::max_dim0(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( grid_rank_[0] == 0 ) gather = new Type[len*grid_size_[0]];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, 0,dim0_comm_[grid_rank_[1]]);

    //Find max on root
    if( grid_rank_[0] == 0  )
    {
        int i, j;
        for(i=0; i<grid_size_[0]; i++)
        {
            if( i!=0 ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] > array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0,dim0_comm_[grid_rank_[1]]);
    // Tidy up (bug found by MDP 12/4/06)
    if( grid_rank_[0] == 0  ) delete[] gather;
}

template<class Type> void Parallel2d::max_dim1(Type& number)
{
    max_dim1( &number,1 );
}
template<class Type> void Parallel2d::max_dim1(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( grid_rank_[1] == 0 ) gather = new Type[len*grid_size_[1]];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, 0,dim1_comm_[grid_rank_[0]]);

    //Find max on root
    if( grid_rank_[1] == 0  )
    {
        int i, j;
        for(i=0; i<grid_size_[1]; i++)
        {
            if( i!=0 ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] > array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0,dim1_comm_[grid_rank_[0]]);
    // Tidy up (bug found by MDP 12/4/06)
    if( grid_rank_[1] == 0  ) delete[] gather;
}

template<class Type> void Parallel2d::min(Type& number)
{
    min( &number,1 );
}

template<class Type> void Parallel2d::min(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( rank() == root() ) gather = new Type[len*size()];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);

    //Find min on root
    if( isRoot() )
    {
        int i, j;
        for(i=0; i<size(); i++)
        {
            if( i!=root() ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] < array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, this->root(), lat_world_comm_);
    // Tidy up (bug found by MDP 12/4/06)
    if( rank() == root() ) delete[] gather;


}

template<class Type> void Parallel2d::min_dim0(Type& number)
{
    min_dim0( &number,1 );
}
template<class Type> void Parallel2d::min_dim0(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( grid_rank_[0] == 0 ) gather = new Type[len*grid_size_[0]];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, 0,dim0_comm_[grid_rank_[1]]);

    //Find min on root
    if( grid_rank_[0] == 0  )
    {
        int i, j;
        for(i=0; i<grid_size_[0]; i++)
        {
            if( i!=0 ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] < array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0,dim0_comm_[grid_rank_[1]]);
    // Tidy up (bug found by MDP 12/4/06)
    if( grid_rank_[0] == 0  ) delete[] gather;
}

template<class Type> void Parallel2d::min_dim1(Type& number)
{
    min_dim1( &number,1 );
}
template<class Type> void Parallel2d::min_dim1(Type* array, int len)
{
    //Gather numbers at root
    Type* gather;
    if( grid_rank_[1] == 0 ) gather = new Type[len*grid_size_[1]];
    MPI_Gather( array, len*sizeof(Type), MPI_BYTE,
               gather, len*sizeof(Type), MPI_BYTE, 0,dim1_comm_[grid_rank_[0]]);

    //Find min on root
    if( grid_rank_[1] == 0  )
    {
        int i, j;
        for(i=0; i<grid_size_[1]; i++)
        {
            if( i!=0 ) for(j=0; j<len; j++)
            {
                if( gather[len*i+j] < array[j] ) array[j] = gather[len*i+j];
            }
        }
    }

    //Broadcast result
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, 0,dim1_comm_[grid_rank_[0]]);
    // Tidy up (bug found by MDP 12/4/06)
    if( grid_rank_[1] == 0  ) delete[] gather;
}


template<class Type> void Parallel2d::broadcast(Type& message, int from)
{
	broadcast( &message, 1, from);
};

template<class Type> void Parallel2d::broadcast(Type* array, int len, int from)
{
	MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, from, lat_world_comm_);
}

template<class Type> void Parallel2d::broadcast_dim0(Type& message, int from)
{
	broadcast_dim0( &message, 1, from);
};

template<class Type> void Parallel2d::broadcast_dim0(Type* array, int len, int from)
{
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, from, dim0_comm_[grid_rank_[1]]);
}

template<class Type> void Parallel2d::broadcast_dim1(Type& message, int from)
{
	broadcast_dim1( &message, 1, from);
};

template<class Type> void Parallel2d::broadcast_dim1(Type* array, int len, int from)
{
    MPI_Bcast( array, len*sizeof(Type), MPI_BYTE, from, dim1_comm_[grid_rank_[0]]);
}


template<class Type> void Parallel2d::send(Type& message, int to)
{
	MPI_Send( &message, sizeof(Type), MPI_BYTE, to, 0, world_comm_ );
}

template<class Type> void Parallel2d::send(Type* array, int len, int to)
{
	MPI_Send( array, len*sizeof(Type), MPI_BYTE, to, 0, world_comm_ );
}

template<class Type> void Parallel2d::send_dim0(Type& message, int to)
{
    MPI_Send( &message, sizeof(Type), MPI_BYTE, to, 0, dim0_comm_[grid_rank_[1]] );
}

template<class Type> void Parallel2d::send_dim0(Type* array, int len, int to)
{
    MPI_Send( array, len*sizeof(Type), MPI_BYTE, to, 0, dim0_comm_[grid_rank_[1]] );
}

template<class Type> void Parallel2d::send_dim1(Type& message, int to)
{

    MPI_Send( &message, sizeof(Type), MPI_BYTE, to, 0, dim1_comm_[grid_rank_[0]] );
}

template<class Type> void Parallel2d::send_dim1(Type* array, int len, int to)
{

    MPI_Send( array, len*sizeof(Type), MPI_BYTE, to, 0, dim1_comm_[grid_rank_[0]] );
}





template<class Type> void Parallel2d::receive(Type& message, int from)
{
	MPI_Status  status;
	MPI_Recv( &message, sizeof(Type), MPI_BYTE, from, 0, world_comm_, &status);
}

template<class Type> void Parallel2d::receive(Type* array, int len, int from)
{
	MPI_Status  status;
	MPI_Recv( array, len*sizeof(Type), MPI_BYTE, from, 0, world_comm_, &status);
}

template<class Type> void Parallel2d::receive_dim0(Type& message, int from)
{

    MPI_Status  status;
    MPI_Recv( &message, sizeof(Type), MPI_BYTE, from, 0, dim0_comm_[grid_rank_[1]], &status);
}

template<class Type> void Parallel2d::receive_dim0(Type* array, int len, int from)
{

    MPI_Status  status;
    MPI_Recv( array, len*sizeof(Type), MPI_BYTE, from, 0, dim0_comm_[grid_rank_[1]], &status);
}

template<class Type> void Parallel2d::receive_dim1(Type& message, int from)
{

    MPI_Status  status;
    MPI_Recv( &message, sizeof(Type), MPI_BYTE, from, 0,  dim1_comm_[grid_rank_[0]], &status);
}

template<class Type> void Parallel2d::receive_dim1(Type* array, int len, int from)
{

    MPI_Status  status;
    MPI_Recv( array, len*sizeof(Type), MPI_BYTE, from, 0, dim1_comm_[grid_rank_[0]], &status);
}


template<class Type> void Parallel2d::sendUp_dim0(Type& bufferSend,Type& bufferRec, long len)
{
    if(this->grid_rank()[0]%2==0)
    {
        if(this->grid_rank()[0]!=this->grid_size()[0]-1)// si pas le dernier alors envoie au +1
        {
            this->send_dim0( bufferSend, len , this->grid_rank()[0]+1);
        }

        if(this->grid_rank()[0] != 0)     // si pas le premier alors recois du -1
        {
            this->receive_dim0( bufferRec, len , this->grid_rank()[0]-1);
        }
        else if(this->grid_size()[0]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->receive_dim0( bufferRec, len , this->grid_size()[0]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->receive_dim0( bufferRec, len , this->grid_rank()[0]-1);

        if(this->grid_rank()[0]!=this->grid_size()[0]-1)//si pas dernier alors envoie au +1
        {
            this->send_dim0( bufferSend, len , this->grid_rank()[0]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->send_dim0( bufferSend, len , 0);
        }

    }


    if(this->grid_size()[0]%2!=0)
    {

        if(this->grid_rank()[0]==this->grid_size()[0]-1)//dernier envoie au 0
        {
            this->send_dim0( bufferSend, len , 0);
        }
        if(this->grid_rank()[0]==0)//0 recoie du dernier
        {
            this->receive_dim0( bufferRec, len , this->grid_size()[0]-1);
        }
    }

}
template<class Type> void Parallel2d::sendDown_dim0(Type& bufferSend,Type& bufferRec, long len)
{
    if(this->grid_rank()[0]%2==0)
    {
        if(this->grid_rank()[0]!=this->grid_size()[0]-1)// si pas le dernier alors envoie au +1
        {
            this->receive_dim0( bufferRec, len , this->grid_rank()[0]+1);
        }

        if(this->grid_rank()[0] != 0)     //si pas le premier alors recois du -1
        {
            this->send_dim0( bufferSend, len , this->grid_rank()[0]-1);
        }
        else if(this->grid_size()[0]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->send_dim0( bufferSend, len , this->grid_size()[0]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->send_dim0( bufferSend, len , this->grid_rank()[0]-1);

        if(this->grid_rank()[0]!=this->grid_size()[0]-1)//si pas dernier alors envoie au +1
        {
            this->receive_dim0( bufferRec, len , this->grid_rank()[0]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->receive_dim0( bufferRec, len , 0);
        }

    }


    if(this->grid_size()[0]%2!=0)
    {

        if(this->grid_rank()[0]==this->grid_size()[0]-1)//dernier envoie au 0
        {
            this->receive_dim0( bufferRec, len , 0);
        }
        if(this->grid_rank()[0]==0)//0 recoie du dernier
        {
            this->send_dim0( bufferSend, len , this->grid_size()[0]-1);
        }
    }

}

template<class Type> void Parallel2d::sendUpDown_dim0(Type& bufferSendUp,Type& bufferRecUp, long lenUp, Type& bufferSendDown,Type& bufferRecDown, long lenDown )
{
    if(this->grid_rank()[0]%2==0)
    {
        if(this->grid_rank()[0]!=this->grid_size()[0]-1)// si pas le dernier alors envoie au +1
        {
            this->send_dim0( bufferSendUp, lenUp , this->grid_rank()[0]+1);
            this->receive_dim0( bufferRecDown, lenDown , this->grid_rank()[0]+1);
        }

        if(this->grid_rank()[0] != 0)     // si pas le premier alors recois du -1
        {
            this->receive_dim0( bufferRecUp, lenUp , this->grid_rank()[0]-1);
            this->send_dim0( bufferSendDown, lenDown , this->grid_rank()[0]-1);
        }
        else if(this->grid_size()[0]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->receive_dim0( bufferRecUp, lenUp , this->grid_size()[0]-1);
            this->send_dim0( bufferSendDown, lenDown , this->grid_size()[0]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->receive_dim0( bufferRecUp, lenUp , this->grid_rank()[0]-1);
        this->send_dim0( bufferSendDown, lenDown , this->grid_rank()[0]-1);

        if(this->grid_rank()[0]!=this->grid_size()[0]-1)//si pas dernier alors envoie au +1
        {
            this->send_dim0( bufferSendUp, lenUp , this->grid_rank()[0]+1);
            this->receive_dim0( bufferRecDown, lenDown , this->grid_rank()[0]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->send_dim0( bufferSendUp, lenUp , 0);
            this->receive_dim0( bufferRecDown, lenDown , 0);
        }

    }


    if(this->grid_size()[0]%2!=0)
    {

        if(this->grid_rank()[0]==this->grid_size()[0]-1)//dernier envoie au 0
        {
            this->send_dim0( bufferSendUp, lenUp , 0);
            this->receive_dim0( bufferRecDown, lenDown , 0);
        }
        if(this->grid_rank()[0]==0)//0 recoie du dernier
        {
            this->receive_dim0( bufferRecUp, lenUp , this->grid_size()[0]-1);
            this->send_dim0( bufferSendDown, lenDown , this->grid_size()[0]-1);
        }
    }

}

template<class Type> void Parallel2d::sendUp_dim1(Type& bufferSend,Type& bufferRec, long len)
{
    if(this->grid_rank()[1]%2==0)
    {
        if(this->grid_rank()[1]!=this->grid_size()[1]-1)// si pas le dernier alors envoie au +1
        {
            this->send_dim1( bufferSend, len , this->grid_rank()[1]+1);
        }

        if(this->grid_rank()[1] != 0)     // si pas le premier alors recois du -1
        {
            this->receive_dim1( bufferRec, len , this->grid_rank()[1]-1);
        }
        else if(this->grid_size()[1]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->receive_dim1( bufferRec, len , this->grid_size()[1]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->receive_dim1( bufferRec, len , this->grid_rank()[1]-1);

        if(this->grid_rank()[1]!=this->grid_size()[1]-1)//si pas dernier alors envoie au +1
        {
            this->send_dim1( bufferSend, len , this->grid_rank()[1]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->send_dim1( bufferSend, len , 0);
        }

    }


    if(this->grid_size()[1]%2!=0)
    {

        if(this->grid_rank()[1]==this->grid_size()[1]-1)//dernier envoie au 0
        {
            this->send_dim1( bufferSend, len , 0);
        }
        if(this->grid_rank()[1]==0)//0 recoie du dernier
        {
            this->receive_dim1( bufferRec, len , this->grid_size()[1]-1);
        }
    }

}
template<class Type> void Parallel2d::sendDown_dim1(Type& bufferSend,Type& bufferRec, long len)
{
    if(this->grid_rank()[1]%2==0)
    {
        if(this->grid_rank()[1]!=this->grid_size()[1]-1)// si pas le dernier alors envoie au +1
        {
            this->receive_dim1( bufferRec, len , this->grid_rank()[1]+1);
        }

        if(this->grid_rank()[1] != 0)     // si pas le premier alors recois du -1
        {
            this->send_dim1( bufferSend, len , this->grid_rank()[1]-1);
        }
        else if(this->grid_size()[1]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->send_dim1( bufferSend, len , this->grid_size()[1]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->send_dim1( bufferSend, len , this->grid_rank()[1]-1);

        if(this->grid_rank()[1]!=this->grid_size()[1]-1)//si pas dernier alors envoie au +1
        {
            this->receive_dim1( bufferRec, len , this->grid_rank()[1]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->receive_dim1( bufferRec, len , 0);
        }

    }


    if(this->grid_size()[1]%2!=0)
    {

        if(this->grid_rank()[1]==this->grid_size()[1]-1)//dernier envoie au 0
        {
            this->receive_dim1( bufferRec, len , 0);
        }
        if(this->grid_rank()[1]==0)//0 recoie du dernier
        {
            this->send_dim1( bufferSend, len , this->grid_size()[1]-1);
        }
    }

}

template<class Type> void Parallel2d::sendUpDown_dim1(Type& bufferSendUp,Type& bufferRecUp, long lenUp, Type& bufferSendDown,Type& bufferRecDown, long lenDown )
{
    if(this->grid_rank()[1]%2==0)
    {
        if(this->grid_rank()[1]!=this->grid_size()[1]-1)// si pas le dernier alors envoie au +1
        {
            this->send_dim1( bufferSendUp, lenUp , this->grid_rank()[1]+1);
            this->receive_dim1 (bufferRecDown, lenDown  , this->grid_rank()[1]+1);

        }

        if(this->grid_rank()[1] != 0)     // si pas le premier alors recois du -1
        {
            this->receive_dim1( bufferRecUp, lenUp  , this->grid_rank()[1]-1);
            this->send_dim1( bufferSendDown, lenDown  , this->grid_rank()[1]-1);
        }
        else if(this->grid_size()[1]%2==0)  // si pair et = 0 alors recois du dernier
        {
            this->receive_dim1( bufferRecUp, lenUp  , this->grid_size()[1]-1);
            this->send_dim1( bufferSendDown, lenDown  , this->grid_size()[1]-1);
        }

    }
    else
    {
        //tous recoivent du -1
        this->receive_dim1( bufferRecUp, lenUp  , this->grid_rank()[1]-1);
        this->send_dim1( bufferSendDown, lenDown  , this->grid_rank()[1]-1);

        if(this->grid_rank()[1]!=this->grid_size()[1]-1)//si pas dernier alors envoie au +1
        {
            this->send_dim1( bufferSendUp, lenUp , this->grid_rank()[1]+1);
            this->receive_dim1 (bufferRecDown, lenDown  , this->grid_rank()[1]+1);
        }
        else //pair et dernier, donc enoie au 0
        {
            this->send_dim1( bufferSendUp, lenUp , 0);
            this->receive_dim1 (bufferRecDown, lenDown  , 0);
        }

    }


    if(this->grid_size()[1]%2!=0)
    {

        if(this->grid_rank()[1]==this->grid_size()[1]-1)//dernier envoie au 0
        {
            this->send_dim1( bufferSendUp, lenUp , 0 );
            this->receive_dim1 (bufferRecDown, lenDown  , 0);
        }
        if(this->grid_rank()[1]==0)//0 recoie du dernier
        {
            this->receive_dim1( bufferRecUp, lenUp  , this->grid_size()[1]-1);
            this->send_dim1( bufferSendDown, lenDown  , this->grid_size()[1]-1);
        }
    }
}
#endif
