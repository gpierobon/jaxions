#ifndef	_ENERGY_GPU_BASE_
	#define	_ENERGY_GPU_BASE_


	int	energyGpu	(const void * __restrict__ m, const void * __restrict__ v, double *z, const double delta2, const double LL, const double nQcd,
				 const uint Lx, const uint Lz, const uint V, const uint Vt, const uint S, FieldPrecision precision, double *eR, cudaStream_t &stream);
#endif