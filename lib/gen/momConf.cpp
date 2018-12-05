#include<cstdio>
// #include<cstdlib>
#include<cstdlib>
#include<math.h>	/* pow */
#include<cstring>
#include<complex>
#include<random>

#include <omp.h>

#include "scalar/scalarField.h"
#include "enum-field.h"
#include "utils/memAlloc.h"
#include "comms/comms.h"
#include "utils/parse.h"

using namespace std;

template<typename Float, MomConfType Moco>
void	momXeon (complex<Float> * __restrict__ fM, complex<Float> * __restrict__ fV, const size_t kMax, const Float kCrat, const size_t Lx, const size_t Lz, const size_t Tz, const size_t S, const size_t V)
{
	LogMsg(VERB_NORMAL,"[momXeon] Called with kMax %zu kCrit %f (kCrit es %f)", kMax, kCrat, kCrit);
	long long kmax;
	int adp = 0;
	if (kMax > Lx/2 - 1)
	{
		kmax = Lx/2 - 1;
		adp = 1;
	}
	else {
		kmax = kMax;
	}
	size_t kmax2 = kmax*kmax;

	constexpr Float Twop = 2.0*M_PI;
	complex<Float> II = complex<Float>{0,1} ;
	Float kcrit = (Float) kCrat;

	int	maxThreads = omp_get_max_threads();
	int	*sd;

	trackAlloc((void **) &sd, sizeof(int)*maxThreads);

	std::random_device seed;		// Totally random seed coming from memory garbage

	for (int i=0; i<maxThreads; i++)
		sd[i] = seed()*(1 + commRank());

	#pragma omp parallel default(shared)
	{
		int nThread = omp_get_thread_num();


		std::mt19937_64 mt64(sd[nThread]);		// Mersenne-Twister 64 bits, independent per thread
		std::uniform_real_distribution<Float> uni(0.0, 1.0);
		std::normal_distribution<Float> distri(0.0,1.0);

		#pragma omp for schedule(static)
		for (size_t oz = 0; oz < Tz; oz++)
		{
			if (oz/Lz != ((size_t) commRank()))
				continue;

			long long pz = oz - (oz/(Tz >> 1))*Tz;

			for(long long py = -kmax; py <= kmax + adp; py++)
			{
				for(long long px = -kmax ; px <= kmax + adp; px++)
				{
					size_t idx  = ((px + Lx)%Lx) + ((py+Lx)%Lx)*Lx + ((pz+Tz)%Tz)*S - commRank()*V;
					size_t modP = px*px + py*py + pz*pz;

					if (modP <= 3*(kmax2 + adp*(1+Lx)))
					{
						Float vl = Twop*(uni(mt64));
						Float al = distri(mt64);
						Float mP = sqrt(((Float) modP))/(kcrit);
						complex<Float> marsa = exp( complex<Float>(0,vl) )*al;

						switch (Moco)
						{
							case(MOM_MFLAT):
								// fM[idx] = complex<Float>(cos(vl), sin(vl))*al;
								fM[idx] = marsa;
							break;

							case(MOM_MSIN):
								{
									Float sc = (modP == 0) ? 1.0 : sin(mP)/mP;
								// fM[idx] = complex<Float>(cos(vl), sin(vl))*al*sc;
								fM[idx] = marsa*sc;
								}
							break;

							case(MOM_MVSINCOS):
								{
									// v to m2 m to v
									Float sc = (modP == 0) ? 1.0 : sin(mP)/mP;
									fV[idx] = marsa*sc;
									sc = (modP == 0) ? 0.0 : (cos(mP) - sc) ;
									fM[idx] = marsa*sc;
								}
							break;

							default:
							case(MOM_MEXP):
								{
									Float sc = (modP == 0) ? 1.0 : exp(-mP);
									fM[idx] = marsa*sc;
								}
							break;
						}
						 // Float mP = (sqrt((Float) modP))/((Float) (kCrat));
						 // Float sc = (modP == 0) ? 1.0 : exp(-mP);


						//printf("mom (%d,%d,%d) = %f %f*I\n",pz,py,px,fM[idx].real(),fM[idx].imag());

					}
				} // END  px loop
			} // END  py loop
		} // END oz FOR
	}

	// zero mode
	if (commRank() == 0)
	{
		if ( mode0 < 3.141597 )
		{
			LogMsg (VERB_NORMAL, "mode0 set to %f in rank %d", mode0, commRank());
			// fM[0] = complex<Float>(cos(mode0), sin(mode0));
			if (Moco == MOM_MVSINCOS)
				fV[0] = exp( complex<Float>(0,mode0) );
				else
				fM[0] = exp( complex<Float>(0,mode0) );
		}
		else
		{
			if (Moco == MOM_MVSINCOS)
				mode0 = atan2(fV[0].imag(),fV[0].real());
				else
				mode0 = atan2(fM[0].imag(),fM[0].real());

			LogMsg (VERB_NORMAL, "mode0 is been randomly set to %f by rank %d", mode0, commRank());
		}
	}

	trackFree((void *) sd);
}

void	momConf (Scalar *field, const size_t kMax, const double kCrt, MomConfType Moco)
{
	const size_t n1 = field->Length();
	const size_t n2 = field->Surf();
	const size_t n3 = field->Size();
	const size_t Lz = field->Depth();
	const size_t Tz = field->TotalDepth();

	const size_t offset = field->DataSize()*n2;

	switch (field->Precision())
	{
		case FIELD_DOUBLE:
		{
			complex<double>* ma;
			complex<double>* va = static_cast<complex<double>*> (field->vCpu());
			if (field->LowMem())
				ma = static_cast<complex<double>*> (field->mStart());
			else
				ma = static_cast<complex<double>*> (field->m2Cpu());

			switch(Moco)
			{
				case MOM_MFLAT:
				momXeon<double, MOM_MFLAT> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				case MOM_MSIN:
				momXeon<double, MOM_MSIN> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				case MOM_MVSINCOS:
				momXeon<double, MOM_MVSINCOS> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				default:
				case MOM_MEXP:
				momXeon<double, MOM_MEXP> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
			}

		// if (field->LowMem())
		// 	momXeon<double, Moco> (static_cast<complex<double>*> (field->mStart()), static_cast<complex<double>*> (field->vCpu()), kMax, kCrt,  n1, Lz, Tz, n2, n3);
		// else
		// 	momXeon<double, Moco> (static_cast<complex<double>*> (field->m2Cpu()), static_cast<complex<double>*> (field->vCpu()), kMax, kCrt,  n1, Lz, Tz, n2, n3);
		}
		break;

		case FIELD_SINGLE:
		{
			complex<float>* ma;
			complex<float>* va = static_cast<complex<float>*> (field->vCpu());
			if (field->LowMem())
				ma = static_cast<complex<float>*> (field->mStart());
			else
				ma = static_cast<complex<float>*> (field->m2Cpu());

			switch(Moco)
			{
				case MOM_MFLAT:
				momXeon<float, MOM_MFLAT> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				case MOM_MSIN:
				momXeon<float, MOM_MSIN> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				case MOM_MVSINCOS:
				momXeon<float, MOM_MVSINCOS> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
				default:
				case MOM_MEXP:
				momXeon<float, MOM_MEXP> (ma, va, kMax, kCrt,  n1, Lz, Tz, n2, n3);
				break;
			}
		// if (field->LowMem())
		// 	momXeon<float, Moco> (static_cast<complex<float> *> (field->mStart()), static_cast<complex<double>*> (field->vCpu()), kMax, static_cast<float>(kCrt), n1, Lz, Tz, n2, n3);
		// else
		// 	momXeon<float, Moco> (static_cast<complex<float> *> (field->m2Cpu()), static_cast<complex<double>*> (field->vCpu()), kMax, static_cast<float>(kCrt), n1, Lz, Tz, n2, n3);
		// break;
		}
		break;

		default:
		break;
	}
}
