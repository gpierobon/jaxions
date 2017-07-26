#include "kernelParms.cuh"
#include "complexGpu.cuh"
#include "utils/index.cuh"

#include "enum-field.h"

#include "scalar/varNQCD.h"
#include "utils/parse.h"

using namespace gpuCu;
using namespace indexHelper;

template<typename Float>
static __device__ __forceinline__ void	propagateThetaCoreGpu(const uint idx, const Float * __restrict__ m, Float * __restrict__ v, Float * __restrict__ m2, const Float zQ,
							      const Float iz, const Float dzc, const Float dzd, const Float ood2, const uint Lx, const uint Sf)
{
	uint X[3], idxPx, idxPy, idxMx, idxMy, idxPz, idxMz;

	Float mel, a, tmp;

	idx2Vec(idx, X, Lx);

	if (X[0] == Lx-1)
		idxPx = idx - Lx+1;
	else
		idxPx = idx+1;

	if (X[0] == 0)
		idxMx = idx + Lx-1;
	else
		idxMx = idx-1;

	if (X[1] == Lx-1)
		idxPy = idx - Sf + Lx;
	else
		idxPy = idx + Lx;

	if (X[1] == 0)
		idxMy = idx + Sf - Lx;
	else
		idxMy = idx - Lx;

	idxPz = idx + Sf;
	idxMz = idx - Sf;

	tmp = m[idx];
	mel = m[idxPx] + m[idxMx] + m[idxPy] + m[idxMy] + m[idxPz] + m[idxMz] - ((Float) 6.)*m[idx];

	a = mel*ood2 - zQ*sin(tmp*iz);

	mel = v[idxMz];
	mel += a*dzc;
	v[idxMz] = mel;
	mel *= dzd;
	tmp += mel;
	m2[idx] = tmp;
}

template<typename Float>
__global__ void	propagateThetaKernel(const Float * __restrict__ m, Float * __restrict__ v, Float * __restrict__ m2, const Float zQ, const Float dzc, const Float dzd,
				     const Float ood2, const Float iz, const uint Lx, const uint Sf, const uint Vo, const uint Vf)
{
	uint idx = Vo + (threadIdx.x + blockDim.x*(blockIdx.x + gridDim.x*blockIdx.y));

	if	(idx >= Vf)
		return;

	propagateThetaCoreGpu<Float>(idx, m, v, m2, zQ, iz, dzc, dzd, ood2, Lx, Sf);
}

void	propThetaGpu(const void * __restrict__ m, void * __restrict__ v, void * __restrict__ m2, double *z, const double dz, const double c, const double d, const double delta2,
		     const double nQcd, const uint Lx, const uint Lz, const uint Vo, const uint Vf, FieldPrecision precision, cudaStream_t &stream)
{
	#define	BLSIZE 256
	const uint Lz2 = (Vf-Vo)/(Lx*Lx);
	dim3 gridSize((Lx*Lx+BLSIZE-1)/BLSIZE,Lz2,1);
	dim3 blockSize(BLSIZE,1,1);

	if (precision == FIELD_DOUBLE)
	{
		const double dzc  = dz*c;
		const double dzd  = dz*d;
		const double zR   = *z;
		const double zQ   = axionmass2((double) zR, nQcd, zthres, zrestore)*zR*zR*zR;
		const double iZ   = 1./zR;
		const double ood2 = 1./delta2;
		propagateThetaKernel<<<gridSize,blockSize,0,stream>>>((const double *) m, (double *) v, (double *) m2, zQ, dzc, dzd, ood2, iZ, Lx, Lx*Lx, Vo, Vf);
	}
	else if (precision == FIELD_SINGLE)
	{
		const float dzc = dz*c;
		const float dzd = dz*d;
		const float zR = *z;
		const float zQ = (float) axionmass2((double) zR, nQcd, zthres, zrestore)*zR*zR*zR;
		const float iZ   = 1./zR;
		const float ood2 = 1./delta2;
		propagateThetaKernel<<<gridSize,blockSize,0,stream>>>((const float *) m, (float *) v, (float *) m2, zQ, dzc, dzd, ood2, iZ, Lx, Lx*Lx, Vo, Vf);
	}
}

