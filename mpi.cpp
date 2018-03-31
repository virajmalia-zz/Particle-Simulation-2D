#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include <vector>
#include <algorithm>    // std::reverse
#include <set>
//#include "mpi_tools.h"

//#define DEBUG 

MPI_Datatype PARTICLE;
std::vector <particle_t> ScatterParticlesToProcs(particle_t *particles, const int NumofParticles, const int NumofBinsEachSide, const int NumberofProcessors, const int rank);
void GhostParticles(const int rank,const int n,const int NumberofProcessors, const int NumberoflocalBins, const int LocalNumofBinsEachSide, std::vector <particle_t> & GhostParticleTopVector,std::vector <particle_t> & GhostParticleBottomVector, const std::vector< std::vector<int> > & LocalBins, const std::vector <particle_t> & localParticleVec);
void MoveParticles(std::vector <particle_t> & localparticleVector,const int rank, int n,const  int NumberofProcessors, const int NumofBinsEachSide);

//
//  benchmarking program
//
int main(int argc, char **argv)
{
    int navg, nabsavg=0;
    double dmin, absmin=1.0,davg,absavg=0.0;
    double rdavg,rdmin;
    int rnavg; 

    //
    //  process command line parameters
    //
    if( find_option( argc, argv, "-h" ) >= 0 )
    {
        printf( "Options:\n" );
        printf( "-h to see this help\n" );
        printf( "-n <int> to set the number of particles\n" );
        printf( "-o <filename> to specify the output file name\n" );
        printf( "-s <filename> to specify a summary file name\n" );
        printf( "-no turns off all correctness checks and particle output\n");
        return 0;
    }
    
    int n = read_int( argc, argv, "-n", 1000 );
    char *savename = read_string( argc, argv, "-o", NULL );
    char *sumname = read_string( argc, argv, "-s", NULL );

    //This will probably stay the same. 
    set_size( n );
    
    //
    //  set up MPIS
    //
    int n_proc, rank;
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &n_proc );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    // apparently needed for Ibsend but yet the documentation never points this out. 
    // int BufferSize = n * sizeof(particle_t);
    // particle_t buf[BufferSize];
    // MPI_Buffer_attach( buf, BufferSize);
    #ifdef DEBUG
    printf(" We are rank %d running with %d processors\n",rank, n_proc );
    #endif
    //
    //  allocate generic resources
    //
    FILE *fsave = savename && rank == 0 ? fopen( savename, "w" ) : NULL;
    FILE *fsum = sumname && rank == 0 ? fopen ( sumname, "a" ) : NULL;

    particle_t *particles = (particle_t*) malloc( n * sizeof(particle_t) );
    MPI_Type_contiguous( 6, MPI_DOUBLE, &PARTICLE );
    MPI_Type_commit( &PARTICLE );

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// probably have to set stuff up in here 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // //
    // //  set up the data partitioning across processors
    // //
    // int particle_per_proc = (n + n_proc - 1) / n_proc;
    // int *partition_offsets = (int*) malloc( (n_proc+1) * sizeof(int) );
    // for( int i = 0; i < n_proc+1; i++ )
    //     partition_offsets[i] = min( i * particle_per_proc, n );
    
    // int *partition_sizes = (int*) malloc( n_proc * sizeof(int) );
    // for( int i = 0; i < n_proc; i++ )
    //     partition_sizes[i] = partition_offsets[i+1] - partition_offsets[i];
    
    //
    //  initialize and distribute the particles (that's fine to leave it unoptimized)
    //


    if (rank == 0)
    { // if we are the master node. 
        init_particles(n, particles);

    }

    double size = getSize(); 
    int NumofBinsEachSide = getNumberofBins(size);
    int NumofBins = NumofBinsEachSide*NumofBinsEachSide; // global bins 
    // allocate storage for local partition
    //
    //int nlocal;
    //particle_t *local; //  = (particle_t*) malloc( nlocal * sizeof(particle_t) );


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// BEGIN LOCAL ///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // send the assign the particles to each procssor based on which bin they are located in. local particles will be populated from this array. 
    std::vector <particle_t> localParticleVector = ScatterParticlesToProcs(particles, n, NumofBinsEachSide, n_proc,rank); // bug in this function!

    #ifdef DEBUG
    printf("ParticleVector is %d\n", localParticleVector.size());
    #endif
    //MPI_Scatterv( particles, partition_sizes, partition_offsets, PARTICLE, local, nlocal, PARTICLE, 0, MPI_COMM_WORLD );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // probaly going to have to change this as well 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int LocalNumberofBins = getNumberofBinsLocal(NumofBinsEachSide, rank,n_proc);
    
    set_local_space(size, rank, NumofBinsEachSide, n_proc);

    std::vector< std::vector<int> > Bins(LocalNumberofBins, std::vector<int>(0));


    // for ghost particles. 
    std::vector <particle_t> GhostParticleTopVector;
    std::vector <particle_t> GhostParticleBottomVector;

    std::vector< std::vector<int> > GhostBinTop(LocalNumberofBins, std::vector<int>(0));
    std::vector< std::vector<int> > GhostBinBottom(LocalNumberofBins, std::vector<int>(0));

    // int LocalNumofBinsEachSide = 0; ///FIXME.
    // int NumberOfBinsLocally = 0;
    //  /// Make the vector of vectors. The bins are vectors
    // std::vector< std::vector<int> > Bins(NumofBinsLocally, std::vector<int>(0));

    //  simulate a number of time steps
    //
    double simulation_time = read_timer();
    for (int step = 0; step < NSTEPS; step++)
    {
        navg = 0;
        dmin = 1.0;
        davg = 0.0;

        // clear bins 
        for(int clear = 0; clear < LocalNumberofBins; clear++ )
        {
            Bins[clear].clear();
        }


        std::set<int> BinsWithParticles;
        for(int particleIndex = 0; particleIndex < localParticleVector.size(); ++particleIndex)
        {
            // CHECKED///////////////////////
              double binsize = getBinSize();
              //printf("Test4\n");
              // get the bin index

               int BinX = (int)(localParticleVector[particleIndex].x/binsize);
               int BinY = (int)(localParticleVector[particleIndex].y/binsize);
               // int BinX = (int)(particlesSOA->x[particle]/binsize);
               // int BinY = (int)(particlesSOA->y[particle]/binsize);
               //printf("Adding particle\n");
               int GlobalBinNum = BinX + NumofBinsEachSide*BinY;


               int LocalBinNumber = MapGlobalBinToLocalBin(rank,GlobalBinNum,NumofBinsEachSide,n_proc);

               //printf("Particle added to Bin %d", BinNum);
               Bins[LocalBinNumber].push_back(particleIndex);

               // store the bin which contain a particle. We will ignore the empty ones
               BinsWithParticles.insert(LocalBinNumber);

               //printf("P %d X: %f Y: %f BinNum: %d \n", particle,particles[particle].x, particles[particle].y,BinNum );
               //printf("There are %d particles in bin %d\n",Bins[BinNum].size(),BinNum );
        //     //addParticleToBin(n,BinX,BinY);
        }
        // //  collect all global data locally (not good idea to do)
        // //
        // MPI_Allgatherv( local, nlocal, PARTICLE, particles, partition_sizes, partition_offsets, PARTICLE, MPI_COMM_WORLD );
        
        // send out ghost particles to other processors // get ghost particles from other processors 

        GhostParticles(rank,n,n_proc, LocalNumberofBins, NumofBinsEachSide,GhostParticleTopVector,GhostParticleBottomVector, Bins, localParticleVector);

        // ghost particle binning......
        for( int TopGhostIndex = 0; TopGhostIndex < GhostParticleTopVector.size(); TopGhostIndex++ )
        { // already sorted in the Y
            double binsize = getBinSize();
            int BinX = (int)(GhostParticleTopVector[TopGhostIndex].x/binsize);
            GhostBinTop[BinX].push_back(BinX);
        }

        for( int BottomGhostIndex = 0; BottomGhostIndex < GhostParticleBottomVector.size(); BottomGhostIndex++ )
        {
            double binsize = getBinSize();
            int BinX = (int)(GhostParticleBottomVector[BottomGhostIndex].x/binsize);
            GhostBinBottom[BinX].push_back(BinX);
        }

        // store the particle indices from each surrounding bin.
        std::vector<int> BinMembers;

        // used for the ghost particles
        std::vector<int> TopGhostBinMembers;

        std::vector<int> BottomGhostBinMembers;


        // for each bin apply forces from particles within the cutoff distance.
        //for(int BinIndex = 0; BinIndex < NumofBins; BinIndex++ )
        std::set<int>::iterator it;
        for( it = BinsWithParticles.begin(); it != BinsWithParticles.end(); it++ )
        {

            // this is a vector of the bins with particles. We don't care about empty bins.
            int BinIndex = *it;

         // if(Bins[BinIndex].size() > 0)
         // {
            //printf("#########################################################\n");
            //printf("Bin number %d \n",BinIndex );
            //printf("#########################################################\n");
            /////////CHECKED !!!

            Bin_Location_t Location =  GetBinLocation(BinIndex,NumofBinsEachSide,LocalNumberofBins );

            //printf("BinIndex %d Left %d Right %d Top %d Bottom %d\n",BinIndex,Left,Right,Top,Bottom);

            Neighbor_Indexes_t NeighborIndex = GetNeighborBinIndexes(BinIndex,NumofBinsEachSide);

            //printf(" BinIndex %d North %d NorthEast %d NorthWest %d, East %d, West %d, South %d, SouthEast %d, SouthWest %d\n",BinIndex, North,NorthEast,NorthWest,East, West,South, SouthEast,SouthWest  );

            // Note: Left = West
            // Right = East

            /// NOW! Take all the particles in each of the bins and apply forces across all 9 bins.
            // We are always going to apply forces within a bin we are in
            //BinMembers.insert(BinMembers.end(),Bins[BinIndex].begin(),Bins[BinIndex].end());


            // we are not at an edge or a corner. -- Most common case
            if( (Location.Left | Location.Right | Location.Top | Location.Bottom) == false)
            {
                ///////////////// CHECKED
                //printf("ALL %d \n", BinIndex );
                //NumofPeerBins = 8;
                //N NE NW E W S SE SW
                //printf("Test1 %d ", BinIndex);
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthWest].begin(),Bins[NeighborIndex.NorthWest].end());
                // printf("Test2");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                // printf("Test3");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthEast].begin(),Bins[NeighborIndex.NorthEast].end());
                // printf("Test4");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                // printf("Test5");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                // printf("Test6");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthWest].begin() ,Bins[NeighborIndex.SouthWest].end());
                //printf("Test7");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                //printf("Test8");
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthEast].begin(),Bins[NeighborIndex.SouthEast].end());
                //printVector(BinMembers);
                //printf("Testlast");
            }
            //printf("After");
            // Top Row
            else if( Location.Top )
            {
                // most common case for the top row -- Not in a corner.
                if( (Location.Left | Location.Right) == false)
                {
                    //printf("Top Row %d \n", BinIndex );
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthWest].begin(),Bins[NeighborIndex.SouthWest].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthEast].begin(),Bins[NeighborIndex.SouthEast].end());
                    //printVector(BinMembers);
                }
                else if( (!Location.Left) && Location.Right ) // Yes this would be called a corner case!!!
                {
                    //printf("Top Row Right %d \n", BinIndex );
                    // Right == East
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthWest].begin(),Bins[NeighborIndex.SouthWest].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                    //printVector(BinMembers);
                }
                else if ( Location.Left && (!Location.Right) )// left corner
                {
                    //printf("Top Row Left %d \n", BinIndex );
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthEast].begin(),Bins[NeighborIndex.SouthEast].end());
                    //printVector(BinMembers);
                }

                ///////////////////////////////////////////////////////// MPI GHOST bins //////////////////////////////////////////////////////////

                if(rank > 0) // we are not the top most space
                {
                    Neighbor_Indexes_t GhostTopIndex = GetGhostBinLocations(BinIndex);
                    if( (Location.Left | Location.Right) == false)
                    {
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.North].begin(),GhostBinTop[GhostTopIndex.North].end());
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.NorthWest].begin(),GhostBinTop[GhostTopIndex.NorthWest].end());
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.NorthEast].begin(),GhostBinTop[GhostTopIndex.NorthEast].end());
                    }
                    else if( (!Location.Left) && Location.Right ) // Yes this would be called a corner case!!!
                    {
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.North].begin(),GhostBinTop[GhostTopIndex.North].end());
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.NorthWest].begin(),GhostBinTop[GhostTopIndex.NorthWest].end());

                    }
                    else if ( Location.Left && (!Location.Right) )// left corner
                    {

                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.North].begin(),GhostBinTop[GhostTopIndex.North].end());
                        TopGhostBinMembers.insert(TopGhostBinMembers.end(),GhostBinTop[GhostTopIndex.NorthEast].begin(),GhostBinTop[GhostTopIndex.NorthEast].end());
                    }
                    
                }

            }
            else if( Location.Bottom )
            {
                // most common case for the top row -- Not in a corner.
                if((Location.Left | Location.Right) == false)
                {
                    //printf("Bottom Row %d \n", BinIndex );
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthWest].begin(),Bins[NeighborIndex.NorthWest].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthEast].begin(),Bins[NeighborIndex.NorthEast].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                    //printVector(BinMembers);
                }
                else if( (!Location.Left) && Location.Right ) // Yes this would be called a corner case!!!
                {
                    // Right == East
                    //printf("Bottom Row Right %d \n", BinIndex );
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthWest].begin(),Bins[NeighborIndex.NorthWest].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                    //printVector(BinMembers);
                }
                else if( Location.Left && (!Location.Right) )// left corner
                {
                    //printf("Bottom Row Left %d ", BinIndex );
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthEast].begin(),Bins[NeighborIndex.NorthEast].end());
                    BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                    //printVector(BinMembers);
                }


                if(rank < (n_proc -1) ) // we are not the bottom most space
                {
                    Neighbor_Indexes_t GhostBottomIndex = GetGhostBinLocations(BinIndex);
                    if( (Location.Left | Location.Right) == false)
                    {
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.South].begin(),GhostBinBottom[GhostBottomIndex.South].end());
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.SouthWest].begin(),GhostBinBottom[GhostBottomIndex.SouthWest].end());
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.SouthEast].begin(),GhostBinBottom[GhostBottomIndex.SouthEast].end());
                    }
                    else if( (!Location.Left) && Location.Right ) // Yes this would be called a corner case!!!
                    {
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.North].begin(),GhostBinBottom[GhostBottomIndex.North].end());
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.SouthWest].begin(),GhostBinBottom[GhostBottomIndex.SouthWest].end());

                    }
                    else if ( Location.Left && (!Location.Right) )// left corner
                    {

                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.South].begin(),GhostBinBottom[GhostBottomIndex.South].end());
                        BottomGhostBinMembers.insert(BottomGhostBinMembers.end(),GhostBinBottom[GhostBottomIndex.SouthEast].begin(),GhostBinBottom[GhostBottomIndex.SouthEast].end());
                    }   
                }

            }
            else if(Location.Left)  // not in a corner on the left side
            {
                //printf("Left %d \n", BinIndex );
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthEast].begin(),Bins[NeighborIndex.NorthEast].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.East].begin(),Bins[NeighborIndex.East].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthEast].begin(),Bins[NeighborIndex.SouthEast].end());
                //printVector(BinMembers);
            }
            else if(Location.Right) // must be the right side
            {
                //printf("Right %d \n", BinIndex );
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.NorthWest].begin(),Bins[NeighborIndex.NorthWest].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.North].begin(),Bins[NeighborIndex.North].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.West].begin(),Bins[NeighborIndex.West].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.SouthWest].begin(),Bins[NeighborIndex.SouthWest].end());
                BinMembers.insert(BinMembers.end(),Bins[NeighborIndex.South].begin(),Bins[NeighborIndex.South].end());
                //printVector(BinMembers);
            }
            else
            {
              printf("Getting another bin case\n");
            }

            for(int Inside_Bin = 0; Inside_Bin < Bins[BinIndex].size(); Inside_Bin++)
            {
                int Index = Bins[BinIndex][Inside_Bin];
                // particles[Index].ax = particles[Index].ay = 0;

                for(int Inside_BinJ =0; Inside_BinJ < Bins[BinIndex].size(); Inside_BinJ++)
                {
                    int Index2 = Bins[BinIndex][Inside_BinJ];
                    
                    apply_force( particles[Index], particles[Index2], &dmin, &davg, &navg);
                    //apply_force_SOA( particlesSOA,Index, Inside_BinJ, &dmin, &davg, &navg);
                }

            }

            //printf(" There will be %d x %d interactions\n",Bins[BinIndex].size(), BinMembers.size());
            // force to neighbors
            for(int calcForceindexI = 0; calcForceindexI < Bins[BinIndex].size(); calcForceindexI++ )
            {
                // apply forces

                int ParticleThisBin = Bins[BinIndex][calcForceindexI];    ////// PERFECT
                    // once the force is applied awesome the accel is zero.
                // particlesSOA->ax[ParticleThisBin] = 0;
                // particlesSOA->ay[ParticleThisBin] = 0;

                for (int calcForceindexJ = 0; calcForceindexJ < BinMembers.size(); calcForceindexJ++ )
                {
                    //printf("Interaction!\n");
                    //apply_force_SOA( particlesSOA,ParticleThisBin, BinMembers[calcForceindexJ], &dmin, &davg, &navg);
                    apply_force( particles[ParticleThisBin], particles[ BinMembers[calcForceindexJ] ], &dmin, &davg, &navg);
                    //apply_force_SOA( particlesSOA,ParticleThisBin, BinMembers[calcForceindexJ], &dmin, &davg, &navg);
                 }
            }

            /// ghost particles
            // top row
            if((rank > 0) && (Location.Top == true) )
            {
                for(int GhostTopBin = 0; GhostTopBin < Bins[BinIndex].size(); GhostTopBin++)
                {
                    int Index = Bins[BinIndex][GhostTopBin];
                    // particles[Index].ax = particles[Index].ay = 0;

                    for (int calcForceindexJ = 0; calcForceindexJ < TopGhostBinMembers.size(); calcForceindexJ++ )
                    {

                        apply_force( particles[Index], GhostParticleTopVector[TopGhostBinMembers[calcForceindexJ]], &dmin, &davg, &navg);
                        //apply_force_SOA( particlesSOA,Index, Inside_BinJ, &dmin, &davg, &navg);
                    }

                }
            }

            /// bottom row
            if( (rank < (n_proc -1) ) && (Location.Bottom == true) )
            {
                for(int GhostBottomBin = 0; GhostBottomBin < Bins[BinIndex].size(); GhostBottomBin++)
                {
                    int Index = Bins[BinIndex][GhostBottomBin];
                    // particles[Index].ax = particles[Index].ay = 0;

                    for (int calcForceindexJ = 0; calcForceindexJ < BottomGhostBinMembers.size(); calcForceindexJ++ )
                    {
                       
                        apply_force( particles[Index], GhostParticleBottomVector[BottomGhostBinMembers[calcForceindexJ]], &dmin, &davg, &navg);
                        //apply_force_SOA( particlesSOA,Index, Inside_BinJ, &dmin, &davg, &navg);
                    }

                }
            }

            BinMembers.clear();

            ///////// MPI

            BottomGhostBinMembers.clear();
            TopGhostBinMembers.clear();

            // printf("##########################################################################################\n");
            // printf("##########################################################################################\n");
            // printf("##########################################################################################\n");
            // printf("\n");

        //}// Bins[BinNum].size() > 0

    } // end of bin calcs

    BinsWithParticles.clear();

        // //
        // //  save current step if necessary (slightly different semantics than in other codes)
        // //
        if( find_option( argc, argv, "-no" ) == -1 )
          if( fsave && (step%SAVEFREQ) == 0 )
            save( fsave, n, particles );
        
        // //
        // //  compute all forces
        // //
        // for( int i = 0; i < nlocal; i++ )
        // {
        //     local[i].ax = local[i].ay = 0;
        //     for (int j = 0; j < n; j++ )
        //         apply_force( local[i], particles[j], &dmin, &davg, &navg );
        // }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// new apply forces function that works across processors. 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if( find_option( argc, argv, "-no" ) == -1 )
        { //This should stay the same 
          
          MPI_Reduce(&davg,&rdavg,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
          MPI_Reduce(&navg,&rnavg,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
          MPI_Reduce(&dmin,&rdmin,1,MPI_DOUBLE,MPI_MIN,0,MPI_COMM_WORLD);

          if (rank == 0)
          {
            //
            // Computing statistical data
            //
            if (rnavg) 
            {
              absavg +=  rdavg/rnavg;
              nabsavg++;
            }
            if (rdmin < absmin)
            { 
                absmin = rdmin;
            }
          }
        }

    MoveParticles(localParticleVector,rank, n, n_proc,NumofBinsEachSide);
    //             //
    //     //  move particles
    //     //
    //     for( int i = 0; i < nlocal; i++ )
    //         move( local[i] );
    // }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ////new move function that handles cross processor particles 
}
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    simulation_time = read_timer() - simulation_time;

    if (rank == 0) 
    {
        printf("n = %d, simulation time = %g seconds", n, simulation_time);

        if (find_option(argc, argv, "-no") == -1) 
        {
            if (nabsavg) 
            {
                absavg /= nabsavg;
            }
            //
            //  -the minimum distance absmin between 2 particles during the run of the simulation
            //  -A Correct simulation will have particles stay at greater than 0.4 (of cutoff) with typical values between .7-.8
            //  -A simulation were particles don't interact correctly will be less than 0.4 (of cutoff) with typical values between .01-.05
            //
            //  -The average distance absavg is ~.95 when most particles are interacting correctly and ~.66 when no particles are interacting
            //
            printf(", absmin = %lf, absavg = %lf", absmin, absavg);
            if (absmin < 0.4) printf("\nThe minimum distance is below 0.4 meaning that some particle is not interacting");
            if (absavg < 0.8) printf("\nThe average distance is below 0.8 meaning that most particles are not interacting");
        }
        printf("\n");

        //
        // Printing summary data
        //
        if (fsum)
        {
            fprintf(fsum,"%d %d %g\n",n,n_proc,simulation_time);
        }
    }

    //
    //  release resources
    //
    if ( fsum )
        fclose( fsum );
    //free( partition_offsets );
    //free( partition_sizes );
    //free( local );
    //free(nlocal);
    free( particles );
    if( fsave )
        fclose( fsave );
    
    MPI_Finalize( );
    
    return 0;
}






///////////////////////////////////////////////////////////// MPI FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////


// would be much cleaner to use multiple returns in C++ 14 but I am not placing faith in compiler support. It's a very new feature
std::vector <particle_t> ScatterParticlesToProcs(particle_t *particles, const int NumofParticles, const int NumofBinsEachSide, const int NumberofProcessors, const int rank)
{ // send particles to the correct processor based on which bin the particle is in and what processor the bin belongs to. 


    //int *partition_offsets = (int*) malloc( (n_proc+1) * sizeof(int) );
    //std::vector<int> partition_offsets(NumberofProcessors);
    int partition_offsets[NumberofProcessors];
    //int *partition_sizes = (int*) malloc( NumberofProcessors * sizeof(int) );
    int partition_sizes[NumberofProcessors];
    //std::vector<int> partition_sizes(NumberofProcessors);
    

    std::vector< std::vector<particle_t> > ParticlesPerProcecesor(NumberofProcessors,std::vector<particle_t>() );
    std::vector<particle_t>  particlesassignedtoproc;

    if(rank == 0) // only the master node should do this 
    {   //sort the particles based on the bin location 
        //returns a proc number 
        for(int particleIndex = 0; particleIndex < NumofParticles; particleIndex++)
        {

            int ProcNumber = MapParticleToProc(particles[particleIndex],NumofBinsEachSide,NumberofProcessors);
            ParticlesPerProcecesor[ProcNumber].push_back(particles[particleIndex]); // add the the vector at each index
            //printf("Particle assigned to procnum %d\n",ProcNumber );
        }

        // append all the particles to particlesassignedtoproc 0 to NumberofProcessors
        int ProcNumCount = 0;
        int runningoffset = 0;

        for(auto ParticleVector : ParticlesPerProcecesor)
        {
            //printf("Particle Array for procnum %d\n",ProcNumCount);
            particlesassignedtoproc.insert(particlesassignedtoproc.end(),ParticleVector.begin(),ParticleVector.end());
            partition_sizes[ProcNumCount] = ParticleVector.size();
            partition_offsets[ProcNumCount] = runningoffset; 
            runningoffset += ParticleVector.size();
            ProcNumCount +=1; 
        }
    }

    //nlocal = (int *) malloc( NumberofProcessors * sizeof(int) );

    /// THIS IS REQUIRED TO MAKE SCATTERV WORK!! SINCE THE OFFSETS ARE CALCULATED BY RANK 0. ScatterV does not send the partion sizes or offsets to the other processors. 
    MPI_Bcast(&partition_offsets, NumberofProcessors, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&partition_sizes, NumberofProcessors, MPI_INT, 0, MPI_COMM_WORLD);

    int nlocal = partition_sizes[rank];
    particle_t * local = (particle_t*) malloc( nlocal * sizeof(particle_t) );

    // only process 0 will get the correct number :(
    #ifdef DEBUG
    printf("Rank: %d Will get %d particles with an offset of %d\n",rank, partition_sizes[rank],partition_offsets[rank]);
    #endif

    // scatter the particles to the processors. More scattered than the programmer's brain. 
    MPI_Scatterv( particlesassignedtoproc.data(), partition_sizes, partition_offsets, PARTICLE, local, nlocal, PARTICLE, 0, MPI_COMM_WORLD );

    // printf("nlocal is %d \n", nlocal);
    // printf("local is %d \n", local);

    std::vector <particle_t> temp (local , local + nlocal);

    free(local);

    return temp;

    // pharaoh, let my particles go!!!!
    // free(partition_offsets);
    // free(partition_sizes);
    //free(particlesassignedtoproc);
}

// spoky! These are need for the local caclulations but forces are not computed on them. 
void GhostParticles(const int rank,const int n,const int NumberofProcessors, const int NumberoflocalBins, const int LocalNumofBinsEachSide, std::vector <particle_t> & GhostParticleTopVector,std::vector <particle_t> & GhostParticleBottomVector, const std::vector< std::vector<int> > & LocalBins, const std::vector <particle_t> & localParticleVec)
{ //These are redundant particles that exists on other processors but, are needed for computing forces on the local processor. 
    // since we can't request the particle it make more sense for each processor to send them to it's peers. 
    std::vector<int> BoarderPeers = getBoarderPeers(rank,NumberofProcessors);
    // Send border particles to neighbors
    #ifdef DEBUG
    printf("We are rank: %d with %d peers\n", rank,BoarderPeers.size());
    #endif

    std::vector< std::vector<particle_t> > OutgoingParticles(NumberofProcessors,std::vector<particle_t>() );


    for (auto Peer : BoarderPeers) // believe it or not the auto prevents an unnedded temp object. 
    {

            if(Peer < rank)
            {
                OutgoingParticles[Peer] = getGhostParticlesTop(rank,LocalNumofBinsEachSide, NumberofProcessors,LocalBins, localParticleVec);
            }
            else if(Peer > rank)  // peer is bigger 
            {
                OutgoingParticles[Peer] = getGhostParticlesBottom(rank,LocalNumofBinsEachSide, NumberoflocalBins, NumberofProcessors,LocalBins, localParticleVec);
            }
            else
            {
            #ifdef DEBUG
                printf("We have a bug in the send of GhostParticles Peer: %d Rank %d\n", Peer, rank);
            #endif
            }

            // if(OutgoingParticles[Peer].empty() == false) 
            // {
            #ifdef DEBUG
                printf("Sending out particles: %d Rank %d to Peer %d\n", OutgoingParticles[Peer].size(), rank, Peer);
            #endif
                MPI_Request request;
                //MPI_Ibsend(&OutgoingParticles[Peer].data()[0], OutgoingParticles[Peer].size(), PARTICLE, Peer, 0, MPI_COMM_WORLD, &request);
                MPI_Isend(&OutgoingParticles[Peer][0],OutgoingParticles[Peer].size(),PARTICLE,Peer,1,MPI_COMM_WORLD,&request);
                MPI_Request_free(&request);
    }

    // same code as Move particles recv
    //the largest number of particles we could possibly recieve is n. 
    
    GhostParticleTopVector.clear();
    GhostParticleBottomVector.clear();


    for(auto Peer: BoarderPeers) // only get ghost particles from our peers
    {
        particle_t *GhostParticleRecvBuffer = (particle_t*) malloc( n * sizeof(particle_t) );
        // recieve boarder 
        int RecvCount = 0;
        MPI_Status status;
        //int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status)
        MPI_Recv(GhostParticleRecvBuffer, n, PARTICLE, Peer, 1, MPI_COMM_WORLD, &status);
        //MPI_Irecv(GhostParticleRecvBuffer,n,PARTICLE,Peer,0,MPI_COMM_WORLD,&status);
        
        // get the number of particles we have revieved 
        MPI_Get_count(&status, PARTICLE, &RecvCount);

        // for each recieved particle 

        if(Peer < rank)
        {
            for(int newParticle = 0; newParticle < RecvCount; newParticle++)
            {
                // add to our local collection of particles 
                GhostParticleTopVector.push_back(GhostParticleRecvBuffer[newParticle]);
                //MapParticleToBin(MovedParticleRecvBuffer[i], NumofBinsEachSide)
            }

        }
        else if(Peer > rank) // ranks located below us!
        {
            for(int newParticle = 0; newParticle < RecvCount; newParticle++)
            {
                GhostParticleBottomVector.push_back(GhostParticleRecvBuffer[newParticle]);
            }
        }
        else
        {

        #ifdef DEBUG
            printf("There is a bug in GhostParticles()! Peer: %d rank %d\n", Peer, rank);
        #endif
        }

        // add total to the local count 
        //*nlocal += RecvCount;
        free(GhostParticleRecvBuffer);
    }
    // freee 
    
}

void MoveParticles(std::vector <particle_t> & localparticleVector,const int rank, int n,const  int NumberofProcessors, const int NumofBinsEachSide)
{
    // moved particles 
    // outgoing particles 

    double Xsize = getLocalXSize();
    double Ysize = getLocalYSize();

    std::vector< std::vector<particle_t> > OutgoingParticles(NumberofProcessors,std::vector<particle_t>() );

    std::vector<int> OutgoingIndexes; 
    for( int i = 0; i < localparticleVector.size(); i++ )
    {
        //move( localparticleVector[i] );

        move( localparticleVector[i]);
        //this is the global n=bin number
        // int BinNum = MapParticleToBin(localparticleVector[i],NumofBinsEachSide);
        // int procNum = MapBinToProc(BinNum,NumberofProcessors);
        int procNum = MapParticleToProc(localparticleVector[i],NumofBinsEachSide,NumberofProcessors);

        if(procNum != rank)
        { // populate the list of outgoing particles 
            OutgoingParticles[procNum].push_back(localparticleVector[i]);
            OutgoingIndexes.push_back(i);

        }
    }

    // reverse the direction of the outgoing vector so we don't have to re calculate indexes every loop 
    std::reverse(OutgoingIndexes.begin(),OutgoingIndexes.end());

    for(auto index:OutgoingIndexes)
    { // remove outgoing partices from local buffer
        localparticleVector.erase(localparticleVector.begin()+index); // remove particle from our local group. 
        //*nlocal = (*nlocal)-1; // hope we don't lose comms!
    }


    // send to other processors with a non blocking send so we don't cause a deadlock
    for(int ProcId = 0; ProcId < NumberofProcessors; ProcId++)
    {
        if(ProcId != rank) // we are not sending particles to ourself. This rank's outgoing vector should be empty but, oh well 
        {
            #ifdef DEBUG
                printf("%d Particles moved from Rank %d to Peer %d\n", OutgoingParticles[ProcId].size(), rank, ProcId);
            #endif
                MPI_Request request;
                MPI_Isend(&OutgoingParticles[ProcId][0], OutgoingParticles[ProcId].size(), PARTICLE, ProcId, 0, MPI_COMM_WORLD, &request);
                MPI_Request_free(&request);

        }
        // reset the outgoing buffer. 
        //OutgoingParticles[procNum].clear();
    }

    //the largest number of particles we could possibly recieve is n. 
    particle_t *MovedParticleRecvBuffer = (particle_t*) malloc( n * sizeof(particle_t) );

    for(int ProcIdRecv = 0; ProcIdRecv < NumberofProcessors; ProcIdRecv++)
    {
        if(ProcIdRecv != rank) // don't wait on ourself.. that would be stupid
        {
                // recieve boarder 
            int RecvCount = 0;
            MPI_Status status;
        #ifdef DEBUG
            printf("Waiting for a message from rank %d \n",ProcIdRecv);
        #endif
            //int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status)
            MPI_Recv(MovedParticleRecvBuffer, n, PARTICLE, ProcIdRecv, 0, MPI_COMM_WORLD, &status);
            
            // get the number of particles we have revieved 
            MPI_Get_count(&status, PARTICLE, &RecvCount);

            // for each recieved particle 
            for(int newParticle = 0; newParticle < RecvCount; newParticle++)
            {
                // add to our local collection of particles 
                localparticleVector.push_back(MovedParticleRecvBuffer[newParticle]);
                //MapParticleToBin(MovedParticleRecvBuffer[i], NumofBinsEachSide)
            }
            // add total to the local count 
        }
        
    }
    // freee 
    free(MovedParticleRecvBuffer);
}

