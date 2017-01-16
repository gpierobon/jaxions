#include<cstdlib>
#include<cstring>
#include<complex>
#include <chrono>

#include"square.h"
#include"fft/fftCuda.h"
#include"fft/fftCode.h"
#include"enum-field.h"

#include"utils/flopCounter.h"

#include"comms/comms.h"

#ifdef	USE_GPU
	#include<cuda.h>
	#include<cuda_runtime.h>
	#include "cudaErrors.h"
#endif

#include "utils/memAlloc.h"
#include "utils/parse.h"

#include<mpi.h>
#include<omp.h>

#ifdef	USE_XEON
	#include"utils/xeonDefs.h"
#endif

#include "utils/index.h"

#define	_SCALAR_CLASS_

using namespace std;

const std::complex<double> I(0.,1.);
const std::complex<float> If(0.,1.);

class	Scalar
{
	private:

	const size_t n1;
	const size_t n2;
	const size_t n3;

	const size_t Lz;
	const size_t Tz;
	const size_t Ez;
	const size_t v3;

	const int  nSplit;
	const bool lowmem;

	DeviceType	device;
	FieldPrecision	precision;
	FieldType	fieldType;
	LambdaType	lambdaType;

	size_t	fSize;
	size_t	mAlign;
	int	shift;
	bool	folded;

	double	*z;

	void	*m,   *v,   *m2;			// Cpu data
#ifdef	USE_GPU
	void	*m_d, *v_d, *m2_d;			// Gpu data

	void	*sStreams;
#endif
	void	recallGhosts(FieldIndex fIdx);		// Move the fileds that will become ghosts from the Cpu to the Gpu
	void	transferGhosts(FieldIndex fIdx);	// Copy back the ghosts to the Gpu

//	void	scaleField(FieldIndex fIdx, double factor);
//	void	randConf();
//	void	smoothConf(const size_t iter, const double alpha);
	//JAVIER
//	void	normaliseField(FieldIndex fIdx);

//	template<typename Float>
//	void	iteraField(const size_t iter, const Float alpha);

	//JAVIER
	template<typename Float>
	void	ENERGY(const Float zz, FILE *enWrite, Float &Grho1, Float &Gtheta1, Float &Vrho1, Float &Vtheta1, Float &Krho1, Float &Ktheta1); // TEST

	template<typename Float>
	void ENERGY2(const Float zz, FILE *enWrite, double &Grho1, double &Gtheta1, double &Vrho1, double &Vtheta1, double &Krho1, double &Ktheta1); // TEST

	template<typename Float>
	void energymapTheta(const Float zz, const int index, void *contbin, int numbins); // TEST


//	template<typename Float>
//	void	momConf(const int kMax, const Float kCrit);

	public:

			 Scalar(const size_t nLx, const size_t nLz, FieldPrecision prec, DeviceType dev, const double zI, bool lowmem, const int nSp,
				FieldType fType, ConfType cType, const size_t parm1, const double parm2, FlopCounter *fCount);
			~Scalar();

	void		*mCpu() { return m; }
	const void	*mCpu() const { return m; }
	void		*vCpu() { return v; }
	const void	*vCpu() const { return v; }
	void		*m2Cpu() { return m2; }
	const void	*m2Cpu() const { return m2; }

#ifdef	USE_XEON
	__attribute__((target(mic))) void	*mXeon() { return mX; }
	__attribute__((target(mic))) const void	*mXeon() const { return mX; }
	__attribute__((target(mic))) void	*vXeon() { return vX; }
	__attribute__((target(mic))) const void	*vXeon() const { return vX; }
	__attribute__((target(mic))) void	*m2Xeon() { return m2X; }
	__attribute__((target(mic))) const void	*m2Xeon() const { return m2X; }
#endif

#ifdef	USE_GPU
	void		*mGpu() { return m_d; }
	const void	*mGpu() const { return m_d; }
	void		*vGpu() { return v_d; }
	const void	*vGpu() const { return v_d; }
	void		*m2Gpu() { return m2_d; }
	const void	*m2Gpu() const { return m2_d; }
#endif
	bool		LowMem()  { return lowmem; }

	size_t		TotalSize() { return n3*nSplit; }
	size_t		Size()      { return n3; }
	size_t		Surf()      { return n2; }
	size_t		Length()    { return n1; }
	size_t		TotalDepth(){ return Lz*nSplit; }
	size_t		Depth()     { return Lz; }
	size_t		eSize()     { return v3; }
	size_t		eDepth()    { return Ez; }

	FieldPrecision	Precision() { return precision; }
	DeviceType	Device()    { return device; }
	LambdaType	Lambda()    { return lambdaType; }
	FieldType	Field()     { return fieldType; }

	void		SetLambda(LambdaType newLambda) { lambdaType = newLambda; }

	size_t		DataSize () { return fSize; }
	size_t		DataAlign() { return mAlign; }
	int		Shift()     { return shift; }
	bool		Folded()    { return folded; }

	double		*zV() { return z; }
	const double	*zV() const { return z; }

	void		setZ(const double newZ) { *z = newZ; }

	void	setField	(FieldType field);
	void	setFolded	(bool foli);

	void	transferDev(FieldIndex fIdx);		// Move data to device (Gpu or Xeon)
	void	transferCpu(FieldIndex fIdx);		// Move data to Cpu

	void	sendGhosts(FieldIndex fIdx, CommOperation commOp);	// Send the ghosts in the Cpu using MPI, use this to exchange ghosts with Cpus
	void	exchangeGhosts(FieldIndex fIdx);	// Transfer ghosts from neighbouring ranks, use this to exchange ghosts with Gpus

	void	fftCpu(int sign);			// Fast Fourier Transform in the Cpu
	void	fftGpu(int sign);			// Fast Fourier Transform in the Gpu
	void	fftCpuSpectrum(int sign);			// Fast Fourier Transform in the Cpu for m2 [axion spectrum usage]

	void	prepareCpu(int *window);		// Sets the field for a FFT, prior to analysis
	//JAVIER
	//void	thetaz2m2(int *window);		// COPIES dTHETA/dz into m2	//not used
	void	theta2m2();//int *window);	// COPIES THETA + I dTHETA/dz     into m2
	double	maxtheta();								// RETURNS THE MAX VALUE OF THETA [OR IM m]
	double	thetaDIST(int numbins, void *thetabin);							// RETURNS (MAX THETA) AND BINNED DATA FOR THETA DISTRIBUTION
	void	denstom(); 	//

	void	squareGpu();				// Squares the m2 field in the Gpu
	void	squareCpu();				// Squares the m2 field in the Cpu

//	void	genConf	(ConfType cType, const size_t parm1, const double parm2);
	//JAVIER
//	void	writeENERGY (double zzz, FILE *enwrite);
	void	writeENERGY (double zzz, FILE *enwrite, double &Gfr, double &Gft, double &Vfr, double &Vft, double &Kfr, double &Kft); // TEST
	void	writeMAPTHETA (double zzz, int index, void *contbin, int numbins);

#ifdef	USE_GPU
	void	*Streams() { return sStreams; }
#endif
//	template<typename Float>
//	void	normaCOREField(const Float alpha);
};

#include "gen/genConf.h"

	Scalar::Scalar(const size_t nLx, const size_t nLz, FieldPrecision prec, DeviceType dev, const double zI, bool lowmem, const int nSp, FieldType fType, ConfType cType,
		       const size_t parm1, const double parm2, FlopCounter *fCount) : nSplit(nSp), n1(nLx), n2(nLx*nLx), n3(nLx*nLx*nLz), Lz(nLz), Ez(nLz + 2), Tz(Lz*nSp), v3(nLx*nLx*(nLz + 2)), fieldType(fType),
		       precision(prec), device(dev), lowmem(lowmem)
{

	std::chrono::high_resolution_clock::time_point start, current, old;
	std::chrono::milliseconds elapsed;

	start = std::chrono::high_resolution_clock::now();


	size_t nData;

	lambdaType = LAMBDA_Z2;
	folded 	   = false;

	switch (fieldType)
	{
		case FIELD_SAXION:
			nData = 2;
			break;

		case FIELD_AXION:
			nData = 1;
			break;

		default:
			printf("Unrecognized field type\n");
			exit(1);
			break;
	}

	switch (prec)
	{
		case FIELD_DOUBLE:
			fSize = sizeof(double)*nData;
			break;

		case FIELD_SINGLE:
			fSize = sizeof(float)*nData;
			break;

		default:
			printf("Unrecognized precision\n");
			exit(1);
			break;
	}

	switch	(dev)
	{
		case DEV_XEON:
			printf("Using Xeon Phi 64 bytes alignment\n");
			mAlign = 64;
			break;

		case DEV_CPU:
			#if	defined(__AVX__) || defined(__AVX2__)
			printf("Using AVX 32 bytes alignment\n");
			mAlign = 32;
			#else
			printf("Using SSE 16 bytes alignment\n");
			mAlign = 16;
			#endif
			break;

		case DEV_GPU:
			mAlign = 16;
			break;
	}

	shift = mAlign/fSize;

	if (n2*fSize % mAlign)
	{
		printf("Error: misaligned memory. Are you using an odd dimension?\n");
		exit(1);
	}

	const size_t	mBytes = v3*fSize;
	const size_t	vBytes = n3*fSize;

printf("Allocating m and v\n"); fflush(stdout);
#ifdef	USE_XEON
	alignAlloc ((void**) &mX, mAlign, mBytes);
	alignAlloc ((void**) &vX, mAlign, vBytes);

	if (!lowmem)
		alignAlloc ((void**) &m2X, mAlign, mBytes);

	m = mX;
	v = vX;

	if (!lowmem)
		m2 = m2X;
	else
		m2 = m2X = NULL;
#else
	alignAlloc ((void**) &m, mAlign, mBytes);
	alignAlloc ((void**) &v, mAlign, vBytes);

	if (!lowmem)
	{
		printf("Allocating m2\n"); fflush(stdout);
		alignAlloc ((void**) &m2, mAlign, mBytes);
	}
	else
	{
		printf("LOWMEM!\n"); fflush(stdout);
		m2 = NULL;
	}
#endif

current = std::chrono::high_resolution_clock::now();
elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
printf("ARRAY ALLOCATION TIME %f min\n",elapsed.count()*1.e-3/60.);
start = std::chrono::high_resolution_clock::now();

	if (m == NULL)
	{
		printf("\n\nError: Couldn't allocate %lu bytes on host for the m field\n", mBytes);
		exit(1);
	}

	if (v == NULL)
	{
		printf("\n\nError: Couldn't allocate %lu bytes on host for the v field\n", vBytes);
		exit(1);
	}

	if (!lowmem)
	{
		if (m2 == NULL)
		{
			printf("\n\nError: Couldn't allocate %lu bytes on host for the m2 field\n", mBytes);
			exit(1);
		}
	}

	printf("set m,v=0\n"); fflush(stdout);
	memset (m, 0, fSize*v3);
	memset (v, 0, fSize*n3);

	if (!lowmem)
		memset (m2, 0, fSize*v3);

	current = std::chrono::high_resolution_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
	printf("zeroing TIME %f min\n",elapsed.count()*1.e-3/60.);
	start = std::chrono::high_resolution_clock::now();

	alignAlloc ((void **) &z, mAlign, mAlign);//sizeof(double));

	if (z == NULL)
	{
		printf("\n\nError: Couldn't allocate %d bytes on host for the z field\n", sizeof(double));
		exit(1);
	}

	if (device == DEV_GPU)
	{
#ifndef	USE_GPU
		printf ("Gpu support not built\n");
		exit   (1);
#else
		if (cudaMalloc(&m_d,  mBytes) != cudaSuccess)
		{
			printf("\n\nError: Couldn't allocate %lu bytes for the gpu field m\n", mBytes);
			exit(1);
		}

		if (cudaMalloc(&v_d,  vBytes) != cudaSuccess)
		{
			printf("\n\nError: Couldn't allocate %lu bytes for the gpu field v\n", vBytes);
			exit(1);
		}

		if (!lowmem)
			if (cudaMalloc(&m2_d, mBytes) != cudaSuccess)
			{
				printf("\n\nError: Couldn't allocate %lu bytes for the gpu field m2\n", mBytes);
				exit(1);
			}

		if ((sStreams = malloc(sizeof(cudaStream_t)*3)) == NULL)
		{
			printf("\n\nError: Couldn't allocate %lu bytes on host for the gpu streams\n", sizeof(cudaStream_t)*3);
			exit(1);
		}

		cudaStreamCreate(&((cudaStream_t *)sStreams)[0]);
		cudaStreamCreate(&((cudaStream_t *)sStreams)[1]);
		cudaStreamCreate(&((cudaStream_t *)sStreams)[2]);

//		if (!lowmem)
//			initCudaFFT(n1, Lz, prec);
#endif
	}// else {
//		initFFT(static_cast<void *>(static_cast<char *> (m) + n2*fSize), m2, n1, Tz, precision, lowmem);
//	}

	*z = zI;

	/*	If present, read fileName	*/

	if (cType == CONF_NONE)
	{
	} else {
		if (fieldType == FIELD_AXION)
		{
			printf("Configuration generation not supported for Axion fields... yet\n");
		}
		else
		{
				start = std::chrono::high_resolution_clock::now();
				printf("Entering initFFT\n");
			if (cType == CONF_KMAX || cType == CONF_TKACHEV)
			{
				initFFT(static_cast<void *>(static_cast<char *> (m) + n2*fSize), m2, n1, Tz, precision, lowmem);
			}
				current = std::chrono::high_resolution_clock::now();
				elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
			printf("Initialisation FFT TIME %f min\n",elapsed.count()*1.e-3/60.);

			printf("Entering GEN_CONF\n");
				start = std::chrono::high_resolution_clock::now();
			genConf	(this, cType, parm1, parm2, fCount);
				current = std::chrono::high_resolution_clock::now();
				elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
			printf("GEN-CONF TIME %f min\n",elapsed.count()*1.e-3/60.);
		}
	}

	if (dev == DEV_XEON)
	{
#ifndef	USE_XEON
		printf ("Xeon Phi support not built\n");
		exit   (1);
#else
		const int micIdx = commAcc();

		#pragma offload_transfer target(mic:micIdx) nocopy(mX : length(fSize*v3) AllocX)
		#pragma offload_transfer target(mic:micIdx) nocopy(vX : length(fSize*n3) AllocX)

		if (!lowmem)
		{
			#pragma offload_transfer target(mic:micIdx) nocopy(m2X : length(fSize*v3) AllocX)
		}
#endif
	}
	//

		start = std::chrono::high_resolution_clock::now();
	printf("FFTing m2 if no lowmem\n");
	initFFTSpectrum(m2, n1, Tz, precision, lowmem);
		current = std::chrono::high_resolution_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
	printf("Initialisation FFT m2 TIME %f min\n",elapsed.count()*1.e-3/60.);

}

// END SCALAR

	Scalar::~Scalar()
{
	printf ("Calling destructor...\n");
	fflush (stdout);
	if (m != NULL)
		trackFree(&m, ALLOC_ALIGN);

	if (v != NULL && fieldType == FIELD_SAXION)
		trackFree(&v, ALLOC_ALIGN);

	if (m2 != NULL)
		trackFree(&m2, ALLOC_ALIGN);

	if (z != NULL)
		trackFree((void **) &z, ALLOC_ALIGN);

	if (device == DEV_GPU)
	{
		#ifndef	USE_GPU
			printf ("Gpu support not built\n");
			exit   (1);
		#else
			if (m_d != NULL)
				cudaFree(m_d);

			if (v_d != NULL)
				cudaFree(v_d);

			if (m2_d != NULL)
				cudaFree(m2_d);

			cudaStreamDestroy(((cudaStream_t *)sStreams)[2]);
			cudaStreamDestroy(((cudaStream_t *)sStreams)[1]);
			cudaStreamDestroy(((cudaStream_t *)sStreams)[0]);

			if (sStreams != NULL)
				free(sStreams);

//			if (!lowmem)
//				closeCudaFFT();
			closeFFT();
		#endif
	} else {
//		if (!lowmem)
			closeFFT();
	}

	if (device == DEV_XEON)
	{
		#ifndef	USE_XEON
			printf ("Xeon Phi support not built\n");
			exit   (1);
		#else
			const int micIdx = commAcc();

			#pragma offload_transfer target(mic:micIdx) nocopy(mX : length(fSize*v3) FreeX)
			#pragma offload_transfer target(mic:micIdx) nocopy(vX : length(fSize*n3) FreeX)

			if (!lowmem)
			{
				#pragma offload_transfer target(mic:micIdx) nocopy(m2X : length(fSize*v3) FreeX)
			}
		#endif
	}
}

void	Scalar::transferDev(FieldIndex fIdx)	// Transfers only the internal volume
{
	if (device == DEV_GPU)
	{
		#ifndef	USE_GPU
			printf ("Gpu support not built\n");
			exit   (1);
		#else
			if (fIdx & 1)
				cudaMemcpy((((char *) m_d) + n2*fSize), (((char *) m) + n2*fSize),  n3*fSize, cudaMemcpyHostToDevice);

			if (fIdx & 2)
				cudaMemcpy(v_d,  v,  n3*fSize, cudaMemcpyHostToDevice);

			if ((fIdx & 4) & (!lowmem))
				cudaMemcpy((((char *) m2_d) + n2*fSize), (((char *) m2) + n2*fSize),  n3*fSize, cudaMemcpyHostToDevice);
		#endif
	} else if (device == DEV_XEON) {
		#ifndef	USE_XEON
			printf ("Xeon Phi support not built\n");
			exit   (1);
		#else
			const int micIdx = commAcc();

			if (fIdx & 1)
			{
				#pragma offload_transfer target(mic:micIdx) in(mX : length(v3*fSize) ReUseX)
			}

			if (fIdx & 2)
			{
				#pragma offload_transfer target(mic:micIdx) in(vX : length(n3*fSize) ReUseX)
			}

			if ((fIdx & 4) & (!lowmem))
			{
				#pragma offload_transfer target(mic:micIdx) in(m2X : length(v3*fSize) ReUseX)
			}
		#endif
	}
}

void	Scalar::transferCpu(FieldIndex fIdx)	// Transfers only the internal volume
{
	if (device == DEV_GPU)
	{
		#ifndef	USE_GPU
			printf ("Gpu support not built\n");
			exit   (1);
		#else
			if (fIdx & 1)
				cudaMemcpy(m,  m_d,  v3*fSize, cudaMemcpyDeviceToHost);

			if (fIdx & 2)
				cudaMemcpy(v,  v_d,  n3*fSize, cudaMemcpyDeviceToHost);

			if ((fIdx & 4) & (!lowmem))
				cudaMemcpy(m2, m2_d, v3*fSize, cudaMemcpyDeviceToHost);
		#endif
	} else if (device == DEV_XEON) {
		#ifndef	USE_XEON
			printf ("Xeon Phi support not built\n");
			exit   (1);
		#else
			const int micIdx = commAcc();

			if (fIdx & 1)
			{
				#pragma offload_transfer target(mic:micIdx) out(mX : length(v3*fSize) ReUseX)
			}

			if (fIdx & 2)
			{
				#pragma offload_transfer target(mic:micIdx) out(vX : length(n3*fSize) ReUseX)
			}

			if ((fIdx & 4) & (!lowmem))
			{
				#pragma offload_transfer target(mic:micIdx) out(m2X : length(v3*fSize) ReUseX)
			}
		#endif
	}
}

void	Scalar::recallGhosts(FieldIndex fIdx)		// Copy to the Cpu the fields in the Gpu that are to be exchanged
{
	if (device == DEV_GPU)
	{
		#ifndef	USE_GPU
			printf ("Gpu support not built\n");
			exit   (1);
		#else
			if (fIdx & FIELD_M)
			{
				cudaMemcpyAsync(static_cast<char *> (m) + n2*fSize, static_cast<char *> (m_d) + n2*fSize, n2*fSize, cudaMemcpyDeviceToHost, ((cudaStream_t *)sStreams)[0]);
				cudaMemcpyAsync(static_cast<char *> (m) + n3*fSize, static_cast<char *> (m_d) + n3*fSize, n2*fSize, cudaMemcpyDeviceToHost, ((cudaStream_t *)sStreams)[1]);
			}
			else
			{
				cudaMemcpyAsync(static_cast<char *> (m2) + n2*fSize, static_cast<char *> (m2_d) + n2*fSize, n2*fSize, cudaMemcpyDeviceToHost, ((cudaStream_t *)sStreams)[0]);
				cudaMemcpyAsync(static_cast<char *> (m2) + n3*fSize, static_cast<char *> (m2_d) + n3*fSize, n2*fSize, cudaMemcpyDeviceToHost, ((cudaStream_t *)sStreams)[1]);
			}

			cudaStreamSynchronize(((cudaStream_t *)sStreams)[0]);
			cudaStreamSynchronize(((cudaStream_t *)sStreams)[1]);
		#endif
	} else if (device == DEV_XEON) {
		#ifndef	USE_XEON
			printf ("Xeon Phi support not built\n");
			exit   (1);
		#else
			const int micIdx = commAcc();

			if (fIdx & FIELD_M)
			{
				#pragma offload_transfer target(mic:micIdx) out(mX[n2*fSize:n2*fSize] : ReUseX)
				#pragma offload_transfer target(mic:micIdx) out(mX[n3*fSize:n2*fSize] : ReUseX)
			} else {
				#pragma offload_transfer target(mic:micIdx) out(m2X[n2*fSize:n2*fSize] : ReUseX)
				#pragma offload_transfer target(mic:micIdx) out(m2X[n3*fSize:n2*fSize] : ReUseX)
			}
		#endif
	}
}

void	Scalar::transferGhosts(FieldIndex fIdx)	// Transfers only the ghosts to the Gpu
{
	if (device == DEV_GPU)
	{
		#ifndef	USE_GPU
			printf ("Gpu support not built\n");
			exit   (1);
		#else
			if (fIdx & FIELD_M)
			{
				cudaMemcpyAsync(static_cast<char *> (m_d),                 static_cast<char *> (m),                 n2*fSize, cudaMemcpyHostToDevice, ((cudaStream_t *)sStreams)[0]);
				cudaMemcpyAsync(static_cast<char *> (m_d) + (n3+n2)*fSize, static_cast<char *> (m) + (n3+n2)*fSize, n2*fSize, cudaMemcpyHostToDevice, ((cudaStream_t *)sStreams)[1]);
			} else {
				cudaMemcpyAsync(static_cast<char *> (m2_d),                 static_cast<char *> (m2),                    n2*fSize, cudaMemcpyHostToDevice, ((cudaStream_t *)sStreams)[0]);
				cudaMemcpyAsync(static_cast<char *> (m2_d) + (n3+n2)*fSize, static_cast<char *> (m2)  + (n3+n2)*fSize, n2*fSize, cudaMemcpyHostToDevice, ((cudaStream_t *)sStreams)[1]);
			}

			cudaStreamSynchronize(((cudaStream_t *)sStreams)[0]);
			cudaStreamSynchronize(((cudaStream_t *)sStreams)[1]);
		#endif
	} else if (device == DEV_XEON) {
		#ifndef	USE_XEON
			printf ("Xeon Phi support not built\n");
			exit   (1);
		#else
			const int micIdx = commAcc();

			if (fIdx & FIELD_M)
			{
				#pragma offload_transfer target(mic:micIdx) in(mX[0:n2*fSize] : ReUseX)
				#pragma offload_transfer target(mic:micIdx) in(mX[(n2+n3)*fSize:n2*fSize] : ReUseX)
			} else {
				#pragma offload_transfer target(mic:micIdx) in(m2X[0:n2*fSize] : ReUseX)
				#pragma offload_transfer target(mic:micIdx) in(m2X[(n2+n3)*fSize:n2*fSize] : ReUseX)
			}
		#endif
	}
}

void	Scalar::sendGhosts(FieldIndex fIdx, CommOperation opComm)
{
	static const int rank = commRank();
	static const int fwdNeig = (rank + 1) % nSplit;
	static const int bckNeig = (rank - 1 + nSplit) % nSplit;

	const int ghostBytes = n2*fSize;

	static MPI_Request 	rSendFwd, rSendBck, rRecvFwd, rRecvBck;	// For non-blocking MPI Comms

	/* Assign receive buffers to the right parts of m, v */

	void *sGhostBck, *sGhostFwd, *rGhostBck, *rGhostFwd;

	if (fIdx & FIELD_M)
	{
		sGhostBck = static_cast<void *> (static_cast<char *> (m) + fSize*n2);
		sGhostFwd = static_cast<void *> (static_cast<char *> (m) + fSize*n3);
		rGhostBck = m;
		rGhostFwd = static_cast<void *> (static_cast<char *> (m) + fSize*(n3 + n2));
	}
	else
	{
		sGhostBck = static_cast<void *> (static_cast<char *> (m2) + fSize*n2);
		sGhostFwd = static_cast<void *> (static_cast<char *> (m2) + fSize*n3);
		rGhostBck = m2;
		rGhostFwd = static_cast<void *> (static_cast<char *> (m2) + fSize*(n3 + n2));
	}


	switch	(opComm)
	{
		case	COMM_SEND:

			MPI_Send_init(sGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*rank,   MPI_COMM_WORLD, &rSendFwd);
			MPI_Send_init(sGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*rank+1, MPI_COMM_WORLD, &rSendBck);

			MPI_Start(&rSendFwd);
			MPI_Start(&rSendBck);

			break;

		case	COMM_RECV:

			MPI_Recv_init(rGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*fwdNeig+1, MPI_COMM_WORLD, &rRecvFwd);
			MPI_Recv_init(rGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*bckNeig,   MPI_COMM_WORLD, &rRecvBck);

			MPI_Start(&rRecvBck);
			MPI_Start(&rRecvFwd);

			break;

		case	COMM_SDRV:

			MPI_Send_init(sGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*rank,   MPI_COMM_WORLD, &rSendFwd);
			MPI_Send_init(sGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*rank+1, MPI_COMM_WORLD, &rSendBck);
			MPI_Recv_init(rGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*fwdNeig+1, MPI_COMM_WORLD, &rRecvFwd);
			MPI_Recv_init(rGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*bckNeig,   MPI_COMM_WORLD, &rRecvBck);

			MPI_Start(&rRecvBck);
			MPI_Start(&rRecvFwd);
			MPI_Start(&rSendFwd);
			MPI_Start(&rSendBck);

			break;

		case	COMM_WAIT:

			MPI_Wait(&rSendFwd, MPI_STATUS_IGNORE);
			MPI_Wait(&rSendBck, MPI_STATUS_IGNORE);
			MPI_Wait(&rRecvFwd, MPI_STATUS_IGNORE);
			MPI_Wait(&rRecvBck, MPI_STATUS_IGNORE);

			MPI_Request_free(&rSendFwd);
			MPI_Request_free(&rSendBck);
			MPI_Request_free(&rRecvFwd);
			MPI_Request_free(&rRecvBck);

//			MPI_Barrier(MPI_COMM_WORLD);

			break;
	}

/*
	{
		MPI_Send_init(sGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*rank,   MPI_COMM_WORLD, rSendFwd);
		MPI_Send_init(sGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*rank+1, MPI_COMM_WORLD, rSendBck);

		MPI_Recv_init(rGhostFwd, ghostBytes, MPI_BYTE, fwdNeig, 2*fwdNeig+1, MPI_COMM_WORLD, rRecvFwd);
		MPI_Recv_init(rGhostBck, ghostBytes, MPI_BYTE, bckNeig, 2*bckNeig,   MPI_COMM_WORLD, rRecvBck);

		MPI_Start(rRecvBck);
		MPI_Start(rRecvFwd);
		MPI_Start(rSendFwd);
		MPI_Start(rSendBck);
	}
*/
}

void	Scalar::exchangeGhosts(FieldIndex fIdx)
{
	recallGhosts(fIdx);
	sendGhosts(fIdx, COMM_SDRV);
	sendGhosts(fIdx, COMM_WAIT);
	transferGhosts(fIdx);
}

//	USA M2, ARREGLAR LOWMEM
void	Scalar::prepareCpu(int *window)
{
	if (precision == FIELD_DOUBLE)
	{
		#pragma omp parallel for default(shared) schedule(static)
		for(size_t i=0; i < n3; i++)
			((complex<double> *) m2)[i] = I*(((std::complex<double> *) v)[i]/((std::complex<double> *) m)[i]).imag()*((double) window[i]);
	}
	else
	{
		#pragma omp parallel for default(shared) schedule(static)
		for(size_t i=0; i < n3; i++)
			((complex<float> *) m2)[i] = If*(((std::complex<float> *) v)[i]/((std::complex<float> *) m)[i]).imag()*((float) window[i]);
	}
}

// EN TEORIA USA M2, ARREGLAR LOWMEM
void	Scalar::squareGpu()
{
//	square(m2_d, n1, Lz, n3, precision);
}

// USA M2, ARREGLAR LOWMEM
void	Scalar::squareCpu()
{
	if (precision == FIELD_DOUBLE)
	{
		#pragma omp parallel for default(shared) schedule(static)
		for(size_t i=0; i < n3; i++)
			((std::complex<double> *) m2)[i] = pow(abs(((std::complex<double> *) m2)[i]/((double) n3)),2);
	}
	else
	{
		#pragma omp parallel for default(shared) schedule(static)
		for(size_t i=0; i < n3; i++)
			((std::complex<float> *) m2)[i] = pow(abs(((std::complex<float> *) m2)[i]/((float) n3)),2);
	}
}

//	USA M2, ARREGLAR LOWMEM
// void	Scalar::thetaz2m2(int *window)
// {
// 	if (precision == FIELD_DOUBLE)
// 	{
// 		#pragma omp parallel for default(shared) schedule(static)
// 		for(size_t i=0; i < n3; i++)
// 			((complex<double> *) m2)[i] = I*(((std::complex<double> *) v)[i]/((std::complex<double> *) m)[i]).imag();
// 	}
// 	else
// 	{
// 		#pragma omp parallel for default(shared) schedule(static)
// 		for(size_t i=0; i < n3; i++)
// 			((complex<float> *) m2)[i] = If*(((std::complex<float> *) v)[i]/((std::complex<float> *) m)[i]).imag();
// 	}
// }

//	USA M2, ARREGLAR LOWMEM
//	COPIES CONFORMAL FIELD AND DERIVATIVE FROM PQ FIELD
void	Scalar::theta2m2()//int *window)
{
	switch (fieldType)
	{
		case FIELD_SAXION:
					if (precision == FIELD_DOUBLE)
					{
						double za = (*z);
						double massfactor = 3.0 * pow(za, nQcd/2 + 1);

						#pragma omp parallel for default(shared) schedule(static)
						for(size_t i=0; i < n3; i++)
						{
						double thetaaux = arg(((std::complex<double> *) m)[i]);
							((complex<double> *) m2)[i] = thetaaux*massfactor*za
																						+ I*( ((((std::complex<double> *) v)[i]/((std::complex<double> *) m)[i]).imag())*za
																						      + thetaaux ) ;
						}
					}
					else
					{
						float zaf = *z ;
						float massfactor = 3.0 * pow(zaf, nQcd/2 + 1);

						#pragma omp parallel for default(shared) schedule(static)
						for(size_t i=0; i < n3; i++)
						{
						float thetaauxf = arg(((std::complex<float> *) m)[i]);
							((complex<float> *) m2)[i] = thetaauxf*massfactor*zaf
																					 + If*( ((((std::complex<float> *) v)[i]/((std::complex<float> *) m)[i]).imag())*zaf
																					      + thetaauxf);
						}
					}
		break;

		case FIELD_AXION:

					if (precision == FIELD_DOUBLE)
					{
						double massfactor = 3.0 * pow((*z), nQcd/2 + 1);

						#pragma omp parallel for default(shared) schedule(static)
						for(size_t i=0; i < n3; i++)
						{
							((complex<double> *) m2)[i] = ((static_cast<double*> (m))[i])*massfactor + I*((static_cast<double*> (v))[i]);
						}
					}
					else
					{
						float massfactor = 3.0 * pow((*z), nQcd/2 + 1);

						#pragma omp parallel for default(shared) schedule(static)
						for(size_t i=0; i < n3; i++)
						{
						float thetaauxf = arg(((std::complex<float> *) m)[i]);
							((complex<float> *) m2)[i] = ((static_cast<float*> (m))[i])*massfactor + If*((static_cast<float*> (v))[i]);
						}
					}
		break;


	}
}

//

//OLD VERSION, THETA
// void	Scalar::theta2m2()//int *window)
// {
//
// 	if (precision == FIELD_DOUBLE)
// 	{
// 		#pragma omp parallel for default(shared) schedule(static)
// 		for(size_t i=0; i < n3; i++)
// 			((complex<double> *) m2)[i] = arg(((std::complex<double> *) m)[i]) + I*(((std::complex<double> *) v)[i]/((std::complex<double> *) m)[i]).imag();//*((double) window[i]);
// 	}
// 	else
// 	{
// 		#pragma omp parallel for default(shared) schedule(static)
// 		for(size_t i=0; i < n3; i++)
// 			((complex<float> *) m2)[i] = arg(((std::complex<float> *) m)[i]) +  If*(((std::complex<float> *) v)[i]/((std::complex<float> *) m)[i]).imag();//*((float) window[i]);
// 	}
// }


/*	ARREGLAR PARA DIFERENTES PRECISIONES	*/
/*
void	Scalar::addZmom(int pz, int oPz, void *data, int sign)
{
	int zDiff = pz - oPz;
	int zBase = commRank()*Lz;

	if (zDiff == 0)
		return;

	switch (precision)
	{
		case FIELD_DOUBLE:
		{
			complex<double> phase[Lz];
			double sg = sign;

			#pragma omp parallel for default(shared) schedule(static)
			for (int zc = 0; zc < Lz; zc++)
			{
				int zTot = zBase + zc;
				phase[zc] = exp(I*(sg*(pz*zTot)));
			}


			#pragma omp parallel for default(shared) schedule(static)
			for (int idx = 0; idx < n2; idx++)
				for (int zc = 0; zc < Lz; zc++)
					((complex<double> *) data)[n2*(zc+1)+idx] *= phase[zc];
		}

		break;

		case FIELD_SINGLE:
		{
			complex<float> phase[Lz];
			float sg = sign;

			#pragma omp parallel for default(shared) schedule(static)
			for (int zc = 0; zc < Lz; zc++)
			{
				int zTot = zBase + zc;
				phase[zc] = exp(If*(sg*(pz*zTot)));
			}


			#pragma omp parallel for default(shared) schedule(static)
			for (int idx = 0; idx < n2; idx++)
				for (int zc = 0; zc < Lz; zc++)
					((complex<float> *) data)[n2*(zc+1)+idx] *= phase[zc];
		}

		break;
	}
}
*/

void	Scalar::fftCpu	(int sign)
{
	runFFT(sign);
}

void	Scalar::fftCpuSpectrum	(int sign)
{
	runFFTSpectrum(sign);
}

/*	CODIGO VIEJO INUTIL, IGUAL PARA FFT GPU...
void	Scalar::fftCpu	(int sign)
{
	int	  oldPz = 0;

	runFFT(sign);

	switch (precision)
	{
		case FIELD_DOUBLE:

		for (int cz = 0; cz < Tz; cz++)
		{
			int pz = cz - (cz/(Tz >> 1))*Tz;
			int rk = cz/Lz;
			int vk = cz%Lz;

			double *mDs = (((double *) m) + (2*n2*(vk+1)));

			addZmom (pz, oldPz, m2, sign);

			oldPz = pz;

			#pragma omp parallel for default(shared) schedule(static)
			for (int idx = 0; idx < n2; idx++)
			{
				((complex<double> *) m)[idx] = complex<double>(0.,0.);

				for (int mz = 0; mz < Lz; mz++)
					((complex<double> *) m)[idx] += ((complex<double> *) m2)[idx + (mz+1)*n2];
			}

			MPI_Reduce(m, mDs, 2*n2, MPI_DOUBLE, MPI_SUM, rk, MPI_COMM_WORLD);
		}

		break;

		case FIELD_SINGLE:

		for (int cz = 0; cz < Tz; cz++)
		{
			int pz = cz - (cz/(Tz >> 1))*Tz;
			int rk = cz/Lz;
			int vk = cz%Lz;

			float *mDs = (((float *) m) + (n2*(vk+1)));

			addZmom (pz, oldPz, m2, sign);

			oldPz = pz;

			#pragma omp parallel for default(shared) schedule(static)
			for (int idx = 0; idx < n2; idx++)
			{
				((complex<float> *) m)[idx] = complex<float>(0.,0.);

				for (int mz = 0; mz < Lz; mz++)
					((complex<float> *) m)[idx] += ((complex<float> *) m2)[idx + (mz+1)*n2];
			}

			MPI_Reduce(m, mDs, 2*n2, MPI_FLOAT, MPI_SUM, rk, MPI_COMM_WORLD);
		}

		break;
	}
}
*/

// ESTO YA NO SE USA, LA FFT ES SIEMPRE EN LA CPU
void	Scalar::fftGpu	(int sign)
{
#ifdef	USE_GPU
	runCudaFFT(m2_d, sign);
#endif
}

void	Scalar::setField (FieldType fType)
{
	switch (fType)
	{
		case FIELD_AXION:
			if (fieldType == FIELD_SAXION)
			{
         printf("| free v ");fflush(stdout);
         trackFree(&v, ALLOC_ALIGN);
        printf("| s_cast v ");fflush(stdout);
				switch (precision)
				{
					case FIELD_SINGLE:
					v = static_cast<void*>(static_cast<float*>(m) + 2*n2 + n3);
					break;

					case FIELD_DOUBLE:
					v = static_cast<void*>(static_cast<double*>(m) + 2*n2 + n3);
					break;
				}
        printf("| resize ");fflush(stdout);
				fSize /= 2;
				shift *= 2;

				const size_t	mBytes = v3*fSize;
        printf("| alloc m2 ");
				//IF low mem was used before, it creates m2 COMPLEX
				if (lowmem)
				{
					#ifdef	USE_XEON
					alignAlloc ((void**) &m2X, mAlign, 2*mBytes);
					m2  = m2X;
					#else
					alignAlloc ((void**) &m2, mAlign, 2*mBytes);
                    //m2  = v;
					#endif
					//initFFTSpectrum(m2, n1, Tz, precision, lowmem);
					initFFTSpectrum(m2, n1, Tz, precision, 0);
				}
				//IF no lowmem was used, we kill m2 complex and create m2 real ... not used
				// #ifdef	USE_XEON
				// if (!lowmem)
				// {
				// 	trackFree(&m2X, ALLOC_ALIGN);
				// 	m2 = m2X = NULL;
				// }
				// alignAlloc ((void**) &m2X, mAlign, mBytes);
				// m2  = m2X;
				// #else
				// if (!lowmem)
				// {
				// 	trackFree(&m2, ALLOC_ALIGN);
				// 	m2 = NULL;
				// }
				// alignAlloc ((void**) &m2, mAlign, mBytes);
				// #endif
			}
			break;

		case	FIELD_SAXION:
		break;
	}
    printf("| fType ");fflush(stdout);
	fieldType = fType;
    printf("| ");fflush(stdout);
}

void	Scalar::setFolded (bool foli)
{
	folded = foli ;
}


//void	Scalar::writeENERGY (double zzz, FILE *enwrite)
void	Scalar::writeENERGY (double zzz, FILE *enwrite, double &Gfr, double &Gft, double &Vfr, double &Vft, double &Kfr, double &Kft) // TEST
{
	switch	(precision)
	{
		case	FIELD_DOUBLE:
		{
//			ENERGY (zzz, enwrite);
			ENERGY (zzz, enwrite, Gfr, Gft, Vfr, Vft, Kfr, Kft); // TEST
		}
		break;

		case	FIELD_SINGLE:
		{
			//float Gr, Gt, Vr, Vt, Kr, Kt;  // TEST
			//
			// ENERGY (static_cast<float>(zzz), enwrite, Gr, Gt, Vr, Vt, Kr, Kt); // TEST
			// Gfr = static_cast<double>(Gr); // TEST
			// Gft = static_cast<double>(Gt); // TEST;
			// Vfr = static_cast<double>(Vr); // TEST;
			// Vft = static_cast<double>(Vt); // TEST;
			// Kfr = static_cast<double>(Kr); // TEST;
			// Kft = static_cast<double>(Kt); // TEST;
			//Alternative
			ENERGY2 (static_cast<float>(zzz), enwrite, Gfr, Gft, Vfr, Vft, Kfr, Kft); // TEST

		}
		break;

		default:
		printf("Unrecognized precision\n");
		exit(1);
		break;
	}
}


//JAVIER ENERGY
template<typename Float>
//void	Scalar::ENERGY(const Float zz, FILE *enWrite)
void	Scalar::ENERGY(const Float zz, FILE *enWrite, Float &Grho, Float &Gtheta, Float &Vrho, Float &Vtheta, Float &Krho, Float &Ktheta) // TEST
{
	const Float deltaa2 = pow(sizeL/sizeN,2) ;

	exchangeGhosts(FIELD_M);

	complex<Float> *mCp = static_cast<complex<Float>*> (m);
	complex<Float> *vCp = static_cast<complex<Float>*> (v);

	//printf("ENERGY CALCULATOR\n");
	fflush (stdout);
	//LEAVES BOUNDARIES OUT OF THE LOOP FOR SIMPLICITY

	//SUM variables
	//Float Vrho1 = 0, Vtheta1 = 0, Krho1 = 0, Ktheta1 = 0, Grho1 = 0, Gtheta1=0;
	double Vrho1 = 0, Vtheta1 = 0, Krho1 = 0, Ktheta1 = 0, Grho1 = 0, Gtheta1=0; // TEST

	const Float invz	= 1.0/zz;
	const Float LLzz2 = LL*zz*zz/4.0 ;
	const Float z9QCD = 9.0*pow(zz,nQcd+2) ;



		#pragma omp parallel for default(shared) schedule(static) reduction(+:Vrho1,Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1)
		for (size_t iz=0; iz < Lz; iz++)
		{
			Float modul, modfac	;
			size_t idd	;

			for (size_t iy=0; iy < n1/shift; iy++)
				for (size_t ix=0; ix < n1; ix++)
					for (size_t sy=0; sy<shift; sy++)
					{

//unfolded coordinates	//size_t oIdx = (iy+sy*(n1/shift))*n1 + ix;
//folded coordinates		//size_t dIdx = iz*n2 + ((size_t) (iy*n1*shift + ix*shift + sy));

					size_t dIdx = iz*n2 + ((size_t) (iy*n1*shift + ix*shift + sy));


					modul = abs(mCp[dIdx+n2]);
					modfac = modul*modul*invz*invz;
					//Vrho misses LLzz2 factor
					Vrho1 	+= pow(modfac-1.0,2)	;
					//Vtheta misses z9QCD factor
					Vtheta1 += 1-real(mCp[dIdx+n2])*invz	;
					//Krho1 misses 0.5 factor
					Krho1 += modfac*pow(real(vCp[dIdx]/mCp[dIdx+n2])-invz,2);
					//Krho1 misses 0.5 factor
					Ktheta1 += modfac*pow(imag(vCp[dIdx]/mCp[dIdx+n2]),2);

					//Grho1 misses a factor 3*0.5/4 delta^2
					//only computed in the z direction ... easy!
					Grho1 += modfac*pow(real((mCp[dIdx+2*n2]-mCp[dIdx])/mCp[dIdx+n2]),2);
					Gtheta1 += modfac*pow(imag((mCp[dIdx+2*n2]-mCp[dIdx])/mCp[dIdx+n2]),2);

					}
		}
	//RENORMALISE
	//Vrho misses LLzz2 factor
	Vrho1 	*= LLzz2/n3	;
	//Vtheta misses z9QCD factor
	Vtheta1 *= z9QCD/n3	;
	//Krho1 misses 0.5 factor
	Krho1 *= 0.5/n3;
	//Krho1 misses 0.5 factor
	Ktheta1 *= 0.5/n3;

	//Grho1 misses a factor 3*0.5/delta^2
	Grho1 *= 3.0*0.125/(deltaa2*n3);
	Gtheta1 *= 3.0*0.125/(deltaa2*n3);

	Vrho = Vrho1; Vtheta = Vtheta1; Krho = Krho1; Ktheta = Ktheta1; Grho = Grho1; Gtheta = Gtheta1;

/*
	fprintf(enWrite,  "%f %f %f %f %f %f %f \n", zz, Vrho1, Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1);
	printf("ENERGY & PRINTED - - - Vr=%f Va=%f Kr=%f Ka=%f Gr=%f Ga=%f \n", Vrho1, Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1);
	fflush (stdout);
*/
}

//JAVIER ENERGY
template<typename Float>
//void	Scalar::ENERGY(const Float zz, FILE *enWrite)
void	Scalar::ENERGY2(const Float zz, FILE *enWrite, double &Grho, double &Gtheta, double &Vrho, double &Vtheta, double &Krho, double &Ktheta) // TEST
{
	const Float deltaa2 = pow(sizeL/sizeN,2) ;

	exchangeGhosts(FIELD_M);

	complex<Float> *mCp = static_cast<complex<Float>*> (m);
	complex<Float> *vCp = static_cast<complex<Float>*> (v);

	//printf("ENERGY CALCULATOR\n");
	fflush (stdout);
	//LEAVES BOUNDARIES OUT OF THE LOOP FOR SIMPLICITY

	//SUM variables
	//Float Vrho1 = 0, Vtheta1 = 0, Krho1 = 0, Ktheta1 = 0, Grho1 = 0, Gtheta1=0;
	Float Vrho1 = 0, Vtheta1 = 0, Krho1 = 0, Ktheta1 = 0, Grho1 = 0, Gtheta1=0; // TEST

	const Float invz	= 1.0/zz;
	const Float LLzz2 = LL*zz*zz/4.0 ;
	const Float z9QCD = 9.0*pow(zz,nQcd+2) ;



		#pragma omp parallel for default(shared) schedule(static) reduction(+:Vrho1,Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1)
		for (size_t iz=0; iz < Lz; iz++)
		{
			Float modul, modfac;
			size_t idd;

			for (size_t iy=0; iy < n1/shift; iy++)
				for (size_t ix=0; ix < n1; ix++)
					for (size_t sy=0; sy<shift; sy++)
					{

//unfolded coordinates	//size_t oIdx = (iy+sy*(n1/shift))*n1 + ix;
//folded coordinates		//size_t dIdx = iz*n2 + ((size_t) (iy*n1*shift + ix*shift + sy));

					size_t dIdx = iz*n2 + ((size_t) (iy*n1*shift + ix*shift + sy));


					modul = abs(mCp[dIdx+n2]);
					modfac = modul*modul*invz*invz;
					//Vrho misses LLzz2 factor
					Vrho1 	+= pow(modfac-1.0,2)	;
					//Vtheta misses z9QCD factor
					Vtheta1 += 1-real(mCp[dIdx+n2])*invz	;
					//Krho1 misses 0.5 factor
					Krho1 += modfac*pow(real(vCp[dIdx]/mCp[dIdx+n2])-invz,2);
					//Krho1 misses 0.5 factor
					Ktheta1 += modfac*pow(imag(vCp[dIdx]/mCp[dIdx+n2]),2);

					//Grho1 misses a factor 3*0.5/4 delta^2
					//only computed in the z direction ... easy!
					Grho1 += modfac*pow(real((mCp[dIdx+2*n2]-mCp[dIdx])/mCp[dIdx+n2]),2);
					Gtheta1 += modfac*pow(imag((mCp[dIdx+2*n2]-mCp[dIdx])/mCp[dIdx+n2]),2);

					}
		}
	//RENORMALISE
	//Vrho misses LLzz2 factor
	Vrho1 	*= LLzz2/n3	;
	//Vtheta misses z9QCD factor
	Vtheta1 *= z9QCD/n3	;
	//Krho1 misses 0.5 factor
	Krho1 *= 0.5/n3;
	//Krho1 misses 0.5 factor
	Ktheta1 *= 0.5/n3;

	//Grho1 misses a factor 3*0.5/delta^2
	Grho1 *= 3.0*0.125/(deltaa2*n3);
	Gtheta1 *= 3.0*0.125/(deltaa2*n3);

	Vrho = Vrho1; Vtheta = Vtheta1; Krho = Krho1; Ktheta = Ktheta1; Grho = Grho1; Gtheta = Gtheta1;
/*
	fprintf(enWrite,  "%f %f %f %f %f %f %f \n", zz, Vrho1, Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1);
	printf("ENERGY & PRINTED - - - Vr=%f Va=%f Kr=%f Ka=%f Gr=%f Ga=%f \n", Vrho1, Vtheta1, Krho1, Ktheta1, Grho1, Gtheta1);
	fflush (stdout);
*/
}

// ----------------------------------------------------------------------
// 		FUNCTION FOR AXION ENERGY ; MAKES AN ENERGY MAP IN M2
// ----------------------------------------------------------------------
void	Scalar::writeMAPTHETA (double zzz, const int index, void *contbin, int numbins)//, FILE *enwrite, double &Gfr, double &Gft, double &Vfr, double &Vft, double &Kfr, double &Kft) // TEST
{
	double maxcontrast ;
	switch	(precision)
	{
		case	FIELD_DOUBLE:
		{
		}
		break;

		case	FIELD_SINGLE:
		{
			energymapTheta (static_cast<float>(zzz), index, contbin, numbins); // TEST

		}
		break;

		default:
		printf("Unrecognized precision\n");
		exit(1);
		break;
	}
	return ;
}

template<typename Float>
//void	Scalar::ENERGY(const Float zz, FILE *enWrite)
void	Scalar::energymapTheta(const Float zz, const int index, void *contbin, int numbins)
{
	// THIS TEMPLATE IS TO BE CALLED UNFOLDED
	// COPIES THE CONTRAST INTO THE REAL PART OF M2 (WHICH IS COMPLEX)
	// TO USE THE POWER SPECTRUM AFTER
	// 	FILES DENSITY CONTRAST

	// WITH NO MPI THIS WORKS FOR OUTPUT
	// char stoCON[256];
	// sprintf(stoCON, "out/con/con-%05d.txt", index);
	// FILE *file_con ;
	// file_con = NULL;
	// file_con = fopen(stoCON,"w+");
	// fprintf(file_con,  "# %d %f %f %f \n", sizeN, sizeL, sizeL/sizeN, zz );

	// 	CONSTANTS
	const Float deltaa2 = pow(sizeL/sizeN,2.)*2. ;
	const Float invz	= 1.0/(*z);
	const Float z9QCD4 = 9.0*pow((*z),nQcd+4.) ;

	//	AUX VARIABLES
	Float maxi = 0.;
	Float maxibin = 0.;
	double toti = 0.;

	exchangeGhosts(FIELD_M);

	//SUM variables
	double contbin_local[numbins] ;
	double toti_global;
	double maxi_global;
	//printf("\n q1-%d",commRank());fflush(stdout);
	if(fieldType == FIELD_AXION)
	{
		Float *mTheta = static_cast<Float*> (m);
		Float *mVeloc = static_cast<Float*> (v);
		// REAL VERSION
		//Float *mCONT = static_cast<Float*> (m2);
		// COMPLEX VERSION
		complex<Float> *mCONT = static_cast<complex<Float>*> (m2);
		//printf("ENERGY map theta \n");

		#pragma omp parallel for default(shared) schedule(static) reduction(max:maxi), reduction(+:toti)
		for (int iz=0; iz < Lz; iz++)
		{
			Float acu , grad ;
			size_t idx, idaux ;
			for (int iy=0; iy < n1; iy++)
			{
				int iyP = (iy+1)%n1;
				int iyM = (iy-1+n1)%n1;
				for (int ix=0; ix < n1; ix++)
				{
					int ixP = (ix+1)%n1;
					int ixM = (ix-1+n1)%n1;

					idx = ix + iy*n1+(iz+1)*n2 ;
					//KINETIC + POTENTIAL
					acu = mVeloc[idx-n2]*mVeloc[idx-n2]/2. + z9QCD4*(1.0-cos(mTheta[idx]*invz)) ;
					//GRADIENTS
					idaux = ixP + iy*n1+(iz+1)*n2 ;
					grad = pow(mTheta[idaux]-mTheta[idx],2);
					idaux = ixM + iy*n1+(iz+1)*n2 ;
					grad += pow(mTheta[idaux]-mTheta[idx],2);
					idaux = ix + iyP*n1+(iz+1)*n2 ;
					grad += pow(mTheta[idaux]-mTheta[idx],2);
					idaux = ix + iyM*n1+(iz+1)*n2 ;
					grad += pow(mTheta[idaux]-mTheta[idx],2);
					grad += pow(mTheta[idx+n2]-mTheta[idx],2);
					grad += pow(mTheta[idx-n2]-mTheta[idx],2);
					mCONT[idx] = acu + grad/deltaa2 ;
					//mCONT[idx] = acu ;

					toti += (double) mCONT[idx].real() ;
					if (mCONT[idx].real() > maxi)
					{
						maxi = mCONT[idx].real() ;
					}
				} //END X LOOP
			} //END Y LOOP
		} //END Z LOOP

		//printf("\n q2-%d",commRank());fflush(stdout);
		double maxid = (double) maxi;

		MPI_Allreduce(&toti, &toti_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		MPI_Allreduce(&maxid, &maxi_global, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
		//printf("\n q3-%d",commRank());fflush(stdout);
		toti_global = toti_global/(n3*nSplit) ;

		#pragma omp parallel for default(shared) schedule(static)
		for (size_t idx=n2; idx < n3+n2; idx++)
		{
			mCONT[idx] = mCONT[idx].real()/toti_global	;
		}
		//printf("\n q4-%d",commRank());fflush(stdout);
		maxi = ((Float) (maxi_global/toti_global)) ;

		if (maxi >100.)
		{
			maxibin = 100.;
		}
		else
		{
			maxibin = maxi ;
		}
		//BIN delta from 0 to maxi+1
		size_t auxintarray[numbins] ;

		for(size_t bin = 0; bin < numbins ; bin++)
		{
		(static_cast<double *> (contbin_local))[bin] = 0.;
		auxintarray[bin] = 0;
		}
		// 	SAVE AVERAGE
		//	MAXIMUM VALUE OF ENERGY CONTRAST
		//	MAXIMUM VALUE TO BE BINNED
		(static_cast<double *> (contbin_local))[0] = toti_global;
		(static_cast<double *> (contbin_local))[1] = maxi_global;
		(static_cast<double *> (contbin_local))[2] = (double) maxibin;

		//printf("\n q5-%d",commRank());fflush(stdout);


		Float norma = (maxi_global)/(numbins-3) ;
		for(size_t i=n2; i < n3+n2; i++)
		{
			int bin;
			bin = (mCONT[i].real()/norma)	;
			//(static_cast<double *> (contbin))[bin+2] += 1. ;
			if (bin<numbins)
			{
				auxintarray[bin] +=1;
			}
		}

		//printf("\n q7-%d",commRank());fflush(stdout);

		#pragma omp parallel for default(shared) schedule(static)
		for(size_t bin = 0; bin < numbins-3 ; bin++)
		{
			(static_cast<double *> (contbin_local))[bin+3] = (double) auxintarray[bin];
		}

		// NO BORRAR!
		// //PRINT 3D maps
		// #pragma omp parallel for default(shared) schedule(static)
		// for (size_t idx = 0; idx < n3; idx++)
		// {
		// 	size_t ix, iy, iz;
		// 		if (mCONT[n2+idx].real() > 5.)
		// 		{
		// 			iz = idx/n2 ;
		// 			iy = (idx%n2)/n1 ;
		// 			ix = (idx%n2)%n1 ;
		// 			#pragma omp critical
		// 			{
		// 				fprintf(file_con,   "%d %d %d %f \n", ix, iy, iz, mCONT[n2+idx].real() ) ;
		// 			}
		// 		}
		// }

		//printf("\n q8-%d",commRank());fflush(stdout);

	}
	else // FIELD_SAXION
	{
			// DO NOTHING
	}

	MPI_Reduce(contbin_local, contbin, numbins, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	//printf("\n q9-%d",commRank());fflush(stdout);

	if (commRank() ==0)
	printf("%d/?? - - - ENERGYdens = %f(%f) Max contrast = %f(%f)\n ", index, toti_global, toti, maxi_global, maxi);

	fflush (stdout);
	return ;
}


// ----------------------------------------------------------------------
// 		FUNCTIONS FOR MAX THETA [works but integrated with next]
// ----------------------------------------------------------------------

double	Scalar::maxtheta()//int *window)
{
	double mymaxd = 0.;
	double mymaxd_global = 0.;
	if (precision == FIELD_DOUBLE)
	{

		double tauxd;
		#pragma omp parallel for reduction(max:mymaxd)
		for(size_t i=0; i < n3; i++)
		{
			if(fieldType == FIELD_SAXION)
			{
				tauxd = abs(arg(((complex<double> *) m)[i]));
			}
			else // FILD_AXION
			{
				tauxd = abs(((double *) m)[i]/(*z));
			}
			if( tauxd > mymaxd )
			{
				mymaxd = tauxd ;
			}
		}
	}
	else // PRECISION SINGLE
	{
		float mymax = 0.f, taux;
		#pragma omp parallel for reduction(max:mymax)
		for(size_t i=0; i < n3; i++)
		{
			if(fieldType == FIELD_SAXION)
			{
				taux = abs(arg(((complex<float> *) m)[i]));
			}
			else // FILD_AXION
			{
				taux = abs(((float *) m)[i]/(*z));
			}
			if( taux > mymax )
			{
				mymax = taux ;
			}
		}
		mymaxd = (double) mymax;
	}
	MPI_Allreduce(&mymaxd, &mymaxd_global, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
	return mymaxd_global ;
}

//----------------------------------------------------------------------
//		FUNCTION FOR THETA DISTRIBUTION [and max]
//----------------------------------------------------------------------
// 		THIS VERSION GIVES MAX IN EACH SLICE (used to fix the theta transition problem)
//
// double	Scalar::thetaDIST(void * thetabin)//int *window)
// {
//
// 	for(size_t iz = 0; iz < Lz+2; iz++)
// 	{
// 	(static_cast<double *> (thetabin))[iz] = 0.;
// 	}
//
// 	if (precision == FIELD_DOUBLE)
// 	{
// 			//double tauxd;
// 			if(fieldType == FIELD_SAXION)
// 			{
// 				#pragma omp parallel for //reduction(max:mymaxd)
// 				for(size_t iz=0; iz < Lz+2; iz++)
// 				{
// 					double tauxd ;
// 					size_t n2shift = iz*n2;
// 					for(size_t i=0; i < n2; i++)
// 					{
// 						tauxd = abs(arg(((complex<double> *) m)[i+n2shift]));
// 						if( tauxd > (static_cast<double *> (thetabin))[iz] )
// 						{
// 							(static_cast<double *> (thetabin))[iz] = tauxd ;
// 						}
// 					}
// 				}
// 			}
// 			else // FIELD_AXION // recall c_theta is saved
// 			{
// 				#pragma omp parallel for //reduction(max:mymaxd)
// 				for(size_t iz=0; iz < Lz+2; iz++)
// 				{
// 					double tauxd ;
// 					size_t n2shift = iz*n2;
// 					for(size_t i=0; i < n2; i++)
// 					{
// 						tauxd = abs(((double *) m)[i + n2shift]/(*z));
// 						if( tauxd > (static_cast<double *> (thetabin))[iz] )
// 						{
// 							(static_cast<double *> (thetabin))[iz] = tauxd ;
// 						}
// 					}
// 				}
// 			}
// 	}
// 	else // PRECISION SINGLE
// 	{
// 		if(fieldType == FIELD_SAXION)
// 		{
// 			#pragma omp parallel for //reduction(max:mymax)
// 			for(size_t iz=0; iz < Lz+2; iz++)
// 			{
// 				float taux;
// 				size_t n2shift = iz*n2;
// 				for(size_t i=0; i < n2; i++)
// 				{
// 					taux = abs(arg(((complex<float> *) m)[i+n2shift]));
// 					if( (double) taux > (static_cast<double *> (thetabin))[iz] )
// 					{
// 						(static_cast<double *> (thetabin))[iz] = (double) taux ;
// 					}
// 				}
// 			}
// 		}
// 		else	//FIELD_AXION
// 		{
// 			#pragma omp parallel for //reduction(max:mymax)
// 			for(size_t iz=0; iz < Lz+2; iz++)
// 			{
// 				double taux;
// 				size_t n2shift = iz*n2;
// 				for(size_t i=0; i < n2; i++)
// 				{
// 					taux = abs(((float *) m)[i+n2shift]/(*z));
// 					if( (double) taux > (static_cast<double *> (thetabin))[iz] )
// 					{
// 						(static_cast<double *> (thetabin))[iz] = (double) taux ;
// 					}
// 				}
// 			}
// 		}
// 	}
// 	double mymaxd = 0.;
// 	for(size_t iz = 1; iz < Lz+1; iz++)
// 	{
// 		if((static_cast<double *> (thetabin))[iz]>mymaxd)
// 		{
// 			mymaxd = (static_cast<double *> (thetabin))[iz];
// 		}
// 	}
//
// 	return mymaxd ;
// }

//----------------------------------------------------------------------
//		FUNCTION FOR THETA DISTRIBUTION [and max]
//----------------------------------------------------------------------
//		BINS THE DISTRIBUTION OF THETA

double	Scalar::thetaDIST(int numbins, void * thetabin)//int *window)
{
	double thetamaxi = maxtheta();
	printf("\n qq10-%d (%f)",commRank(), thetamaxi);fflush(stdout);
//	printf("hallo von inside %f\n", thetamaxi);

	double n2p = numbins/thetamaxi;
	double thetabin_local[numbins];

	for(size_t i = 0; i < numbins ; i++)
	{
	(static_cast<double *> (thetabin_local))[i] = 0.;
	}

	if (precision == FIELD_DOUBLE)
	{
			if(fieldType == FIELD_SAXION)
			{
	//			#pragma omp parallel for default(shared)
				for(size_t i=0; i < n3 ; i++)
				{
					int bin;
					bin =  n2p*abs(arg(((complex<double> *) m)[i+n2]));
					(static_cast<double *> (thetabin_local))[bin] += 1.;
				}
			}
			else	//FIELD_AXION
			{
	//			#pragma omp parallel for default(shared)
				for(size_t i=0; i < n3; i++)
				{
					int bin;
					bin = n2p*abs(((double *) m)[i+n2]/(*z));
					(static_cast<double *> (thetabin_local))[bin] += 1. ;
				}
			}
	}
	else // PRECISION SINGLE
	{
		float n2pf = numbins/thetamaxi;

		if(fieldType == FIELD_SAXION)
		{
//			#pragma omp parallel for default(shared)
			for(size_t i=0; i < n3 ; i++)
			{
				int bin;
				bin =  n2pf*abs(arg(((complex<float> *) m)[i+n2]));
				(static_cast<double *> (thetabin_local))[bin] += 1.;
			}
		}
		else	//FIELD_AXION
		{
//			#pragma omp parallel for default(shared)
			for(size_t i=0; i < n3; i++)
			{
				int bin;
				bin = n2pf*abs(((float *) m)[i+n2]/(*z));
				(static_cast<double *> (thetabin_local))[bin] += 1. ;
			}
		}
	}

	// #pragma omp critical
	// {
	// 	for(int n=0; n<numbins; n++)
	// 	{
	// 		static_cast<double*>(thetabin)[n] += thetabin_private[n];
	// 	}
	// }

//}//END PARALLEL
	MPI_Reduce(thetabin_local, thetabin, numbins, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	//if (commRank()==0)
	//printf("out = %f\n",thetamaxi);
	return thetamaxi ;
}

//--------------------------------------------------------------------
//		DENS M2 -> M
//--------------------------------------------------------------------

//	FOR FINAL OUTPUT, COPIES M2 INTO M

void	Scalar::denstom()//int *window)
{
	double thetamaxi = maxtheta();

//	printf("hallo von inside %f\n", thetamaxi);

if(fieldType == FIELD_AXION)
{
	if (precision == FIELD_SINGLE)
	{
	float *mTheta = static_cast<float*> (m);
	complex<float> *mCONT = static_cast<complex<float>*> (m2);

	#pragma omp parallel for default(shared)
		for(size_t idx=0; idx < n3; idx++)
			{
				mTheta[n2+idx] = mCONT[n2+idx].real();
			}

	}
	else //	FIELD_DOUBLE
	{
	double *mThetad = static_cast<double*> (m);
	complex<double> *mCONTd = static_cast<complex<double>*> (m2);

	#pragma omp parallel for default(shared)
		for(size_t idx=0; idx < n3; idx++)
			{
				mThetad[n2+idx] = mCONTd[n2+idx].real();
			}
	}
	printf("dens to m ... done\n");

}
else
{
	printf("dens to m not available for SAXION\n");
}

}
