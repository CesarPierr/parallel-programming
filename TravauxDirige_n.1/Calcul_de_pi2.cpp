# include <chrono>
# include <random>
# include <cstdlib>
# include <sstream>
# include <string>
# include <fstream>
# include <iostream>
# include <iomanip>
# include <mpi.h>

// Attention , ne marche qu'en C++ 11 ou supérieur :
double approximate_pi( unsigned long nbSamples ) 
{
    typedef std::chrono::high_resolution_clock myclock;
    myclock::time_point beginning = myclock::now();
    myclock::duration d = beginning.time_since_epoch();
    unsigned seed = d.count();
    std::default_random_engine generator(seed);
    std::uniform_real_distribution <double> distribution ( -1.0 ,1.0);
    unsigned long nbDarts = 0;
    // Throw nbSamples darts in the unit square [-1 :1] x [-1 :1]
    for ( unsigned sample = 0 ; sample < nbSamples ; ++ sample ) {
        double x = distribution(generator);
        double y = distribution(generator);
        // Test if the dart is in the unit disk
        if ( x*x+y*y<=1 ) nbDarts ++;
    }
    // Number of nbDarts throwed in the unit disk
    double ratio = double(nbDarts)/double(nbSamples);
    return double(nbDarts);
}

int main( int nargs, char* argv[] )
{
	// On initialise le contexte MPI qui va s'occuper :
	//    1. Créer un communicateur global, COMM_WORLD qui permet de gérer
	//       et assurer la cohésion de l'ensemble des processus créés par MPI;
	//    2. d'attribuer à chaque processus un identifiant ( entier ) unique pour
	//       le communicateur COMM_WORLD
	//    3. etc...
	MPI_Init( &nargs, &argv );
	// Pour des raisons de portabilité qui débordent largement du cadre
	// de ce cours, on préfère toujours cloner le communicateur global
	// MPI_COMM_WORLD qui gère l'ensemble des processus lancés par MPI.
	// On interroge le communicateur global pour connaître le nombre de processus
	// qui ont été lancés par l'utilisateur :
	int nbp;
	MPI_Comm_size(MPI_COMM_WORLD, &nbp);
    MPI_Request resq[2*(nbp-1)];

	// On interroge le communicateur global pour connaître l'identifiant qui
	// m'a été attribué ( en tant que processus ). Cet identifiant est compris
	// entre 0 et nbp-1 ( nbp étant le nombre de processus qui ont été lancés par
	// l'utilisateur )
	MPI_Status Stat[2*(nbp-1)];

	int number_samples = 1000000;
	int rank;
    double buf[nbp-1];
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Création d'un fichier pour ma propre sortie en écriture :
	std::stringstream fileName;
	
	if(rank == 0) {
		double recept;
		for(int i =0; i< nbp-1; i++){
			MPI_Irecv(&buf[i], 1,MPI_DOUBLE, i+1,0,MPI_COMM_WORLD,&resq[nbp + i-2]);
		}
        fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	    std::ofstream output( fileName.str().c_str() );
	    double send = approximate_pi(number_samples);

        MPI_Waitall(2*(nbp-1),resq,Stat);

        for(int i = 0; i<nbp-1;i++){
            send += buf[i];
        }
		send /= (double)nbp*(double)number_samples;
		send*=4;
		std::cout << "pi = " << send << std::endl;
        output.close();
		
	}else{
        fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	    std::ofstream output( fileName.str().c_str() );
	    double send = approximate_pi(number_samples);
		MPI_Isend(&send, 1, MPI_DOUBLE, 0,0,MPI_COMM_WORLD,&resq[rank-1]);
        std::cout << "sent" << std::endl;
        output.close();
	}
	// Rajout de code....

	
	// A la fin du programme, on doit synchroniser une dernière fois tous les processus
	// afin qu'aucun processus ne se termine pendant que d'autres processus continue à
	// tourner. Si on oublie cet instruction, on aura une plantage assuré des processus
	// qui ne seront pas encore terminés.
	MPI_Finalize();
	return EXIT_SUCCESS;
}