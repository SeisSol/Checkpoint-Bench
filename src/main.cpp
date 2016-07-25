/**
 * @file
 * This file is part of SeisSol.
 *
 * @author Sebastian Rettenberger (sebastian.rettenberger AT tum.de, http://www5.in.tum.de/wiki/index.php/Sebastian_Rettenberger)
 *
 * @section LICENSE
 * Copyright (c) 2016, SeisSol Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 */

#ifdef USE_MPI
#include <mpi.h>
#endif // USE_MPI

#include <cstdlib>
#include <string>

#include "utils/args.h"

#include "Checkpoint/Manager.h"
#include "Monitoring/instrumentation.fpp"
// FIXME Use stopwatch from SeisSol repository as soon as we upgrade to the lastest version
#include "Monitoring/Stopwatch.h"
#include "Parallel/MPI.h"

int main(int argc, char* argv[])
{
	SCOREP_USER_REGION("checkpoint-bench", SCOREP_USER_REGION_TYPE_FUNCTION);

	// Set up MPI
	seissol::MPI mpi;
	mpi.init(argc, argv);

	// Parse command line arguements
	utils::Args args;
	const char* backends[] = {"posix", "hdf5", "mpio", "mpio-async", "sionlib"};
	args.addEnumOption("backend", backends, 'b', "the checkpoint back-end");
	args.addOption("file", 'f', "the file name prefix");
	args.addOption("elements", 'e', "number of elements per rank", utils::Args::Required, false);
	args.addOption("total", 't', "total number of elements", utils::Args::Required, false);
	args.addOption("iterations", 'i', "number of iterations (Default: 10)", utils::Args::Required, false);

	switch (args.parse(argc, argv, mpi.rank() == 0)) {
	case utils::Args::Success:
		break;
	case utils::Args::Help:
		mpi.finalize();
		return 0;
	case utils::Args::Error:
		mpi.finalize();
		return 1;
	}

	// Set up checkpoint manager
	seissol::checkpoint::Manager manager;

	std::string backend = backends[args.getArgument<unsigned int>("backend")];
	if (backend == "posix")
		manager.setBackend(seissol::checkpoint::POSIX);
	else if (backend == "hdf5")
		manager.setBackend(seissol::checkpoint::HDF5);
	else if (backend == "mpio")
		manager.setBackend(seissol::checkpoint::MPIO);
	else if (backend == "mpio-async")
		manager.setBackend(seissol::checkpoint::MPIO_ASYNC);
	else if (backend == "sionlib")
		manager.setBackend(seissol::checkpoint::SIONLIB);
	else
		logError() << "Unknown back-end";

	manager.setFilename(args.getArgument<const char*>("file"));

	unsigned long elements = args.getArgument("elements", 0ul);
	unsigned long total = args.getArgument("total", 0ul);

	if (elements > 0) {
		if (total > 0)
			logWarning(mpi.rank()) << "Elements per rank and total number of elements set, ignoring total number of elements";

#ifdef USE_MPI
		MPI_Reduce(&elements, &total, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
#endif // USE_MPI
	} else if (total > 0) {
		elements = (total + mpi.size() - 1) / mpi.size();
		if (mpi.rank() == mpi.size() - 1)
			elements = total - (elements * (mpi.size()-1));
	} else {
		logError() << "Elements per rank or total number of elements required";
	}

	// Total number of elements
	elements *= NUMBER_OF_ALIGNED_DOFS;
	total *= NUMBER_OF_ALIGNED_DOFS;

	// Allocate array
	real* dofs = new real[elements];

	// Initialize the checkpoint
	SCOREP_USER_REGION_DEFINE(r_init_checkpoint);
	SCOREP_USER_REGION_BEGIN(r_init_checkpoint, "init_checkpoint", SCOREP_USER_REGION_TYPE_COMMON);
	double time;
	int waveFieldTimestep;
	int faultTimestep;
	manager.init(dofs, elements, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0, 0, time, waveFieldTimestep, faultTimestep);
	SCOREP_USER_REGION_END(r_init_checkpoint);

	// Iterate
	unsigned int iterations = args.getArgument("iterations", 10u);
	SCOREP_USER_REGION_DEFINE(r_iterate);
	SCOREP_USER_REGION_BEGIN(r_iterate, "init_iterate", SCOREP_USER_REGION_TYPE_COMMON);

	SCOREP_USER_REGION_DEFINE(r_fill);
	SCOREP_USER_REGION_DEFINE(r_write);

	double totalTime = 0;

	for (unsigned int i = 0; i < iterations; i++) {
		logInfo(mpi.rank()) << "Iteration" << i;
		SCOREP_USER_REGION_BEGIN(r_fill, "fill_dofs", SCOREP_USER_REGION_TYPE_COMMON);
#ifdef _OPENMP
		#pragma omp parallel for
#endif // _OPENMP
		for (unsigned long j = 0; j < elements; j++) {
			dofs[j] = i * elements + j;
		}
		SCOREP_USER_REGION_END(r_fill);

#ifdef USE_MPI
		MPI_Barrier(MPI_COMM_WORLD);
#endif // USE_MPI

		Stopwatch watch;
		SCOREP_USER_REGION_BEGIN(r_write, "write_dofs", SCOREP_USER_REGION_TYPE_COMMON);
		watch.start();
		manager.write(i, 0, 0);
#ifdef USE_MPI
		MPI_Barrier(MPI_COMM_WORLD);
#endif // USE_MPI
		double t = watch.stop();
		SCOREP_USER_REGION_END(r_write);

		totalTime += t;

		logInfo(mpi.rank()) << "Time:" << t << "s, bandwidth:" << (total*sizeof(real)) / t / (1024 * 1024) << "MiB/s";
	}

	SCOREP_USER_REGION_END(r_iterate);

	// Statistics
	logInfo(mpi.rank()) << "Avg time:" << totalTime/iterations
			<< "s, avg Bandwidth:" << (total*sizeof(real)*iterations) / totalTime / (1024 * 1024) << "MiB/s";

	// Finalize checkpoint manager
	SCOREP_USER_REGION_DEFINE(r_finalize_checkpoint);
	SCOREP_USER_REGION_BEGIN(r_finalize_checkpoint, "finalize_checkpoint", SCOREP_USER_REGION_TYPE_COMMON);
	manager.close();
	SCOREP_USER_REGION_END(r_finalize_checkpoint)

	// Finalize MPI
	mpi.finalize();

	return 0;
}
