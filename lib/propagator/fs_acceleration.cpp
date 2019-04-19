#include <string>
#include <complex>
#include <memory>
#include "scalar/scalarField.h"
#include "scalar/folder.h"
#include "enum-field.h"

#ifdef	USE_GPU
	#include <cuda.h>
	#include <cuda_runtime.h>
	#include <cuda_device_runtime_api.h>
#endif

#include "utils/utils.h"
#include "fft/fftCode.h"
#include "comms/comms.h"

class	Acceleration : public Tunable
{
	private:

	const FieldPrecision	precision;
	const size_t		Lx;
	const size_t		Ly;
	const size_t		Lz;
	const size_t		Tz;
	const size_t		Sf;
	const size_t		npoints;
	const size_t		norma;

	Scalar			*field;

	template<class cFloat, const bool hCmplx>
	void			accCpu(std::string name);

	template<class cFloat, const bool hCmplx>
	void			accGpu(std::string name);

	public:

		Acceleration (Scalar *field) : precision(field->Precision()), Lx(field->Length()), Ly(field->Length()/commSize()), Lz(field->Depth()), Tz(field->TotalDepth()),
					    Sf(field->Surf()), field(field), npoints(field->Size()), norma(field->eSize()) {
		if (field->LowMem()) {
			LogError ("Error: Acceleration not supported in lowmem runs");
			exit(0);
		}
	}

	void	sRunCpu	();	// Saxion Acceleration in FS
	void	sRunGpu	();

	void	tRunCpu	();	// Axion Acceleration in FS
	void	tRunGpu	();
};

void	Laplacian::sRunGpu	()
{
#ifdef	USE_GPU
#else
	LogError ("Error: gpu support not built");
	exit(1);
#endif
}


template<class cFloat, const bool hCmplx>
void	Acceleration::accCpu	(std::string name)
{
	// fetch the plan to take FTm into m@m2
	// note that FWD is from m to m2 but with the wrong sign
	auto &planFFT = AxionFFT::fetchPlan(name);
	planFFT.run(FFT_FWD);

	//complex in Saxion mode, real in Axion mode
	cFloat *mData = static_cast<cFloat*> (field->m2Cpu());

	const size_t zBase = Ly*commRank();

	const int hLx = Lx>>1;
	const int hTz = Tz>>1;

	const uint   maxLx = (hCmplx == true) ? hLx+1 : Lx;
	const size_t maxSf = maxLx*Tz;

  // this can be vectorised
	#pragma omp parallel for schedule(static) default(shared)
	for (uint idx = 0; idx < npoints; idx++)	// As Javier pointed out, the transposition makes y the slowest coordinate
		{
			/* First adjust the norm, it was FTed */
			mData[idx] /= (double) norma;
			/* now m2 is Saxion or Axion field */
			switch(vqcd)
			{
				case VQCD_1:
				default:
					
				break ;


			}

		}

	planFFT.run(FFT_BCK);
	field->setM2     (M2_DIRTY);
}

void	Laplacian::sRunCpu	()
{
	switch (precision) {
		case FIELD_SINGLE:
			lapCpu<std::complex<float>, false>(std::string("SpSx"));
			break;

		case FIELD_DOUBLE:
			lapCpu<std::complex<double>,false>(std::string("SpSx"));
			break;

		default:
			LogError ("Couldn't calculate laplacian: Wrong precision");
			break;
	}
}

void    Laplacian::tRunGpu	()
{
#ifdef  USE_GPU
#else
	LogError ("Error: gpu support not built");
	exit(1);
#endif
}

void    Laplacian::tRunCpu	()
{
	switch (precision) {
		case FIELD_SINGLE:
			lapCpu<std::complex<float>, true>(std::string("SpAx"));
			break;

		case FIELD_DOUBLE:
			lapCpu<std::complex<double>,true>(std::string("SpAx"));
			break;

		default:
			LogError ("Couldn't calculate laplacian: Wrong precision");
			break;
	}
}

using	namespace profiler;

void	applyLaplacian	(Scalar *field)
{
	LogMsg	(VERB_HIGH, "Called laplacian");
	profiler::Profiler &prof = getProfiler(PROF_PROP);

	auto lap = std::make_unique<Laplacian>(field);

	if	(field->Folded())
	{
		Folder	munge(field);
		munge(UNFOLD_ALL);
	}

	//prof.start();

	switch (field->Field()) {
		case FIELD_AXION_MOD:
		case FIELD_AXION:
			//lap->setName("Laplacian Axion");

			switch (field->Device()) {
				case DEV_GPU:
					lap->tRunGpu ();
					break;
				case DEV_CPU:
					lap->tRunCpu ();
					break;
				default:
					LogError ("Error: invalid device");
					prof.stop();
					return;
			}

			//lap->add(16.*4.*field->Size()*1.e-9, 10.*4.*field->DataSize()*field->Size()*1.e-9);

			break;

		case FIELD_SAXION:
			//lap->setName("Laplacian Saxion");

			switch (field->Device()) {
				case DEV_GPU:
					lap->sRunGpu ();
					break;
				case DEV_CPU:
					lap->sRunCpu ();
					break;
				default:
					LogError ("Error: invalid device");
					//prof.stop();
					return;
			}
			break;

		default:
			LogError ("Error: invalid field type");
			//prof.stop();
			return;
	}

	//prof.stop();

	//prof.add(lap->Name(), lap->GFlops(), lap->GBytes());

	//LogMsg	(VERB_HIGH, "%s reporting %lf GFlops %lf GBytes", lap->Name().c_str(), prof.Prof()[lap->Name()].GFlops(), prof.Prof()[lap->Name()].GBytes());

	return;
}
