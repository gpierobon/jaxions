#include <cstdio>
#include <cstdlib>
#include <memory>
#include <chrono>
#include "scalar/scalarField.h"
#include "scalar/folder.h"
#include "enum-field.h"
#include "propagator/propClass.h"
#include "utils/utils.h"

#include <omp.h>

std::unique_ptr<PropBase> prop;

template<VqcdType pot>
class	PropLeap : public PropClass<1, true, pot> {

	public:
		PropLeap(Scalar *field, const double LL, const double nQcd, const double delta, const bool spec) : PropClass<1, true, pot>(field, LL, nQcd, delta, spec) {
		//	Set up Leapfrog parameters

		double nC[2] = { 0.5, 0.5 };
		double nD[1] = { 1.0 };

		this->setCoeff(nC, nD);

		if (spec && field->Device() == DEV_CPU) {
			this->setBaseName("Leapfrog spectral ");
		} else {
			if (field->LowMem())
				this->setBaseName("Lowmem Leapfrog ");
			else
				this->setBaseName("Leapfrog ");
		}
	}
};

template<VqcdType pot>
class	PropOmelyan2 : public PropClass<2, true, pot> {

	public:
		PropOmelyan2(Scalar *field, const double LL, const double nQcd, const double delta, const bool spec) : PropClass<2, true, pot>(field, LL, nQcd, delta, spec) {
		constexpr double chi = +0.19318332750378360;

		//	Set up Omelyan parameters for BABAB

		double nC[3] = { chi, 1.-2.*chi, chi };
		double nD[2] = { 0.5, 0.5 };

		this->setCoeff(nC, nD);

		if (spec && field->Device() == DEV_CPU) {
			this->setBaseName("Omelyan2 spectral ");
		} else {
			if (field->LowMem())
				this->setBaseName("Lowmem Omelyan2 ");
			else
				this->setBaseName("Omelyan2 ");
		}
	}
};

template<VqcdType pot>
class	PropOmelyan4 : public PropClass<4, true, pot> {

	public:
		PropOmelyan4(Scalar *field, const double LL, const double nQcd, const double delta, const bool spec) : PropClass<4, true, pot>(field, LL, nQcd, delta, spec) {
		constexpr double xi  = +0.16449865155757600;
		constexpr double lb  = -0.02094333910398989;
		constexpr double chi = +1.23569265113891700;

		//	Set up Omelyan parameters for BABABABAB

		double nC[5] = { xi, chi, 1.-2.*(xi+chi), chi, xi };
		double nD[4] = { 0.5*(1.-2.*lb), lb, lb, 0.5*(1.-2.*lb) };

		this->setCoeff(nC, nD);

		if (spec && field->Device() == DEV_CPU) {
			this->setBaseName("Omelyan4 spectral ");
		} else {
			if (field->LowMem())
				this->setBaseName("Lowmem Omelyan4 ");
			else
				this->setBaseName("Omelyan4 ");
		}
	}
};

template<VqcdType pot>
class	PropRKN4 : public PropClass<4, false, pot> {

	public:
		PropRKN4(Scalar *field, const double LL, const double nQcd, const double delta, const bool spec) : PropClass<4, false, pot>(field, LL, nQcd, delta, spec) {
		//	Set up RKN parameters for BABABABA

		const double nC[4] = { +0.1344961992774310892, -0.2248198030794208058, +0.7563200005156682911, +0.3340036032863214255 };
		const double nD[4] = { +0.5153528374311229364, -0.085782019412973646,  +0.4415830236164665242, +0.1288461583653841854 };

		this->setCoeff(nC, nD);

		if (spec && field->Device() == DEV_CPU) {
			this->setBaseName("RKN4 spectral ");
		} else {
			if (field->LowMem())
				this->setBaseName("Lowmem RKN4 ");
			else
				this->setBaseName("RKN4 ");
		}
	}
};

void	initPropagator	(PropType pType, Scalar *field, const double nQcd, const double delta, const double LL, VqcdType pot) {

	LogMsg	(VERB_HIGH, "Initializing propagator");

	bool	spec = (pType & PROP_SPEC) ? true : false, wasTuned = false;

	unsigned int xBlock, yBlock, zBlock;

	if (prop != nullptr)
		if (prop->IsTuned()) {
			wasTuned = true;
			xBlock = prop->TunedBlockX();
			yBlock = prop->TunedBlockY();
			zBlock = prop->TunedBlockZ();
		}		

	switch (pType & PROP_MASK) {
		case PROP_OMELYAN2:
			switch (pot) {
				case VQCD_1:
					prop = std::make_unique<PropOmelyan2<VQCD_1> >    (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2:
					prop = std::make_unique<PropOmelyan2<VQCD_1_PQ_2> >(field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2_RHO:
					prop = std::make_unique<PropOmelyan2<VQCD_1_PQ_2_RHO> >(field, LL, nQcd, delta, spec);
					break;

				case VQCD_2:
					prop = std::make_unique<PropOmelyan2<VQCD_2> >    (field, LL, nQcd, delta, spec);
					break;

				case VQCD_NONE:
					prop = std::make_unique<PropOmelyan2<VQCD_NONE> > (field, LL, nQcd, delta, spec);
					break;
			}
			break;

		case PROP_OMELYAN4:
			switch (pot) {
				case VQCD_1:
					prop = std::make_unique<PropOmelyan4<VQCD_1> >    (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2:
					prop = std::make_unique<PropOmelyan4<VQCD_1_PQ_2> >    (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2_RHO:
					prop = std::make_unique<PropOmelyan4<VQCD_1_PQ_2_RHO> >    (field, LL, nQcd, delta, spec);
					break;

				case VQCD_2:
					prop = std::make_unique<PropOmelyan4<VQCD_2> >    (field, LL, nQcd, delta, spec);
					break;

				case VQCD_NONE:
					prop = std::make_unique<PropOmelyan4<VQCD_NONE> > (field, LL, nQcd, delta, spec);
					break;
			}
			break;

		case PROP_LEAP:
			switch (pot) {
				case VQCD_1:
					prop = std::make_unique<PropLeap<VQCD_1> >    (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2:
					prop = std::make_unique<PropLeap<VQCD_1_PQ_2> >    (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2_RHO:
					prop = std::make_unique<PropLeap<VQCD_1_PQ_2_RHO> >    (field, LL, nQcd, delta, spec);
					break;

				case VQCD_2:
					prop = std::make_unique<PropLeap<VQCD_2> >    (field, LL, nQcd, delta, spec);
					break;

				case VQCD_NONE:
					prop = std::make_unique<PropLeap<VQCD_NONE> > (field, LL, nQcd, delta, spec);
					break;
			}
			break;

		case PROP_RKN4:
			switch (pot) {
				case VQCD_1:
					prop = std::make_unique<PropRKN4<VQCD_1> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_RHO:
					prop = std::make_unique<PropRKN4<VQCD_1_RHO> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_DRHO:
					prop = std::make_unique<PropRKN4<VQCD_1_DRHO> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2:
					prop = std::make_unique<PropRKN4<VQCD_1_PQ_2> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2_RHO:
					prop = std::make_unique<PropRKN4<VQCD_1_PQ_2_RHO> >  (field, LL, nQcd, delta, spec);
					break;
				case VQCD_1_PQ_2_DRHO:
					prop = std::make_unique<PropRKN4<VQCD_1_PQ_2_DRHO> > (field, LL, nQcd, delta, spec);
					break;
				case VQCD_2:
					prop = std::make_unique<PropRKN4<VQCD_2> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_2_RHO:
					prop = std::make_unique<PropRKN4<VQCD_2_RHO> >	     (field, LL, nQcd, delta, spec);
					break;
				case VQCD_2_DRHO:
					prop = std::make_unique<PropRKN4<VQCD_2_DRHO> >	     (field, LL, nQcd, delta, spec);
					break;


				case VQCD_NONE:
					prop = std::make_unique<PropRKN4<VQCD_NONE> >	     (field, LL, nQcd, delta, spec);
					break;
			}

			break;

		default:
			LogError ("Error: unrecognized propagator %d", pType);
			exit(1);
			break;
	}

	prop->getBaseName();

	if (wasTuned) {
		prop->SetBlockX(xBlock);
		prop->SetBlockY(yBlock);
		prop->SetBlockZ(zBlock);
		prop->UpdateBestBlock();
	}

	LogMsg	(VERB_HIGH, "Propagator %ssuccessfully initialized", prop->Name().c_str());
}

using	namespace profiler;

void	propagate	(Scalar *field, const double dz)
{
	LogMsg	(VERB_HIGH, "Called propagator");
	Profiler &prof = getProfiler(PROF_PROP);

	if	(!field->Folded() && !(pType & PROP_SPEC))
	{
		Folder	munge(field);
		munge(FOLD_ALL);
	}

	prop->getBaseName();

	prof.start();

	switch (field->Field()) {
		case FIELD_AXION:
			prop->appendName("Axion");
			(prop->propAxion)(dz);
			break;

		case FIELD_AXION_MOD:
			prop->appendName("Axion Mod");
			(prop->propAxion)(dz);
			break;

		case FIELD_SAXION:
			prop->appendName("Saxion");
			(prop->propSaxion)(dz);
			break;

		default:
			LogError ("Error: invalid field type");
			prof.stop();
			return;
	}

	auto mFlops = prop->cFlops((pType & PROP_SPEC) ? true : false);
	auto mBytes = prop->cBytes((pType & PROP_SPEC) ? true : false);

	prop->add(mFlops, mBytes);

	prof.stop();

	prof.add(prop->Name(), prop->GFlops(), prop->GBytes());

	prop->reset();

	LogMsg	(VERB_HIGH, "Propagator %s reporting %lf GFlops %lf GBytes", prop->Name().c_str(), prof.Prof()[prop->Name()].GFlops(), prof.Prof()[prop->Name()].GBytes());

	return;
}

void	tunePropagator (Scalar *field) {
	// Hash CPU model so we don't mix different cache files

	int myRank   = commRank();
	int nThreads = 1;
	bool newFile = false, found = false;

	if (prop == nullptr) {
		LogError("Error: propagator not initialized, can't be tuned.");
		return;
	}

	Profiler &prof = getProfiler(PROF_TUNER);

	std::chrono::high_resolution_clock::time_point start, end;
	std::chrono::nanoseconds bestTime, lastTime;

	LogMsg (VERB_HIGH, "Started tuner");
	prof.start();

	prop->InitBlockSize(field->Length(), field->Depth(), field->DataSize(), field->DataAlign());

	/*	Check for a cache file	*/

	if (myRank == 0) {
		FILE *cacheFile;
		char tuneName[2048];
		sprintf (tuneName, "%s/tuneCache.dat", wisDir);
		if ((cacheFile = fopen(tuneName, "r")) == nullptr) {
			LogMsg (VERB_NORMAL, "Missing tuning cache file %s, will create a new one", tuneName);
			newFile = true;
		} else {
			size_t       rMpi, rThreads, rLx, rLz;
			unsigned int rBx, rBy, rBz, fType, myField = (field->Field() == FIELD_SAXION) ? 0 : 1;

			do {
				fscanf (cacheFile, "%lu %lu %lu %lu %u %u %u %u\n", &rMpi, &rThreads, &rLx, &rLz, &fType, &rBx, &rBy, &rBz);

				if (rMpi == commSize() && rThreads == omp_get_max_threads() && rLx == field->Length() && rLz == field->Depth() && fType == myField) {
					if (rBx <= prop->BlockX() && rBy <= field->Surf()/prop->BlockX() && rBz <= field->Depth()) {
						found = true;
						prop->SetBlockX(rBx);
						prop->SetBlockY(rBy);
						prop->SetBlockZ(rBz);
						prop->UpdateBestBlock();
					}
				}
			}	while(!feof(cacheFile) && !found);
			fclose (cacheFile);
		}
	}

	MPI_Bcast (&found, sizeof(found), MPI_BYTE, 0, MPI_COMM_WORLD);

	commSync();

	// If a cache file was found, we broadcast the best block and exit
	if (found) {
		unsigned int block[3];

		if (myRank == 0) {
			block[0] = prop->TunedBlockX();
			block[1] = prop->TunedBlockY();
			block[2] = prop->TunedBlockZ();
printf ("Sending %u %u %u\n", block[0], block[1], block[2]); fflush(stdout);
		}

		MPI_Bcast (&block, sizeof(int)*3, MPI_BYTE, 0, MPI_COMM_WORLD);
		commSync();

		if (myRank != 0) {
printf ("Receiving %u %u %u\n", block[0], block[1], block[2]); fflush(stdout);
			prop->SetBlockX(block[0]);
			prop->SetBlockY(block[1]);
			prop->SetBlockZ(block[2]);
			prop->UpdateBestBlock();
		}

		LogMsg (VERB_NORMAL, "Tuned values read from cache file. Best block %u x %u x %u", prop->TunedBlockX(), prop->TunedBlockY(), prop->TunedBlockZ());
		prof.stop();
		prof.add(prop->Name(), 0., 0.);
		return;
	}

	// Otherwise we start tuning

	start = std::chrono::high_resolution_clock::now();
	propagate(field, 0.);
	end   = std::chrono::high_resolution_clock::now();

	bestTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);

	LogMsg (VERB_HIGH, "Block %u x %u done in %lu ns", prop->BlockY(), prop->BlockZ(), bestTime.count());

	while (!prop->IsTuned()) {
		prop->AdvanceBlockSize();

		start = std::chrono::high_resolution_clock::now();
		propagate(field, 0.);
		end   = std::chrono::high_resolution_clock::now();

		lastTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start);

		LogMsg (VERB_HIGH, "Block %u x %u done in %lu ns", prop->BlockY(), prop->BlockZ(), lastTime.count());

		if (lastTime < bestTime) {
			bestTime = lastTime;
			prop->UpdateBestBlock();
			LogMsg (VERB_HIGH, "Best block updated");
		}
	}

	prop->getBaseName();

	switch (field->Field()) {
		case FIELD_AXION:
			prop->appendName("Axion");
			break;

		case FIELD_AXION_MOD:
			prop->appendName("Axion Mod");
			break;

		case FIELD_SAXION:
			prop->appendName("Saxion");
			break;

		default:
			LogError ("Error: invalid field type");
			return;
	}

	Profiler &propProf = getProfiler(PROF_PROP);
	propProf.reset(prop->Name());

	prop->SetBestBlock();
	LogMsg (VERB_NORMAL, "Propagator tuned! Best block %u x %u x %u in %lu ns", prop->TunedBlockX(), prop->TunedBlockY(), prop->TunedBlockZ(), bestTime.count());

	/*	Write cache file if necessary, block of rank 0 prevails		*/

	if (myRank == 0) {
		FILE *cacheFile;
		char tuneName[2048];
		sprintf (tuneName, "%s/tuneCache.dat", wisDir);

		// We distinguish between opening and appending a new line
		if (!newFile) {
			if ((cacheFile = fopen(tuneName, "a")) == nullptr) {
				LogError ("Error: can't open cache file, can't save tuning results");
				commSync();
				prof.stop();
				prof.add(prop->Name(), 0., 0.);
			}
		} else {
			if ((cacheFile = fopen(tuneName, "w")) == nullptr) {
				LogError ("Error: can't create cache file, can't save tuning results");
				commSync();
				prof.stop();
				prof.add(prop->Name(), 0., 0.);
			}
		}

		unsigned int fType = (field->Field() == FIELD_SAXION) ? 0 : 1;
		fprintf (cacheFile, "%lu %lu %lu %lu %u %u %u %u\n", commSize(), omp_get_max_threads(), field->Length(), field->Depth(),
								     fType, prop->TunedBlockX(), prop->TunedBlockY(), prop->TunedBlockZ());
		fclose  (cacheFile);
	}

	commSync();
	prof.stop();
	prof.add(prop->Name(), 0., 0.);
}
